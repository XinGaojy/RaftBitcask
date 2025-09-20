#include "bitcask/rpc_server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace bitcask {
namespace rpc {

RpcServer::RpcServer(const std::string& host, int port, std::shared_ptr<DB> db)
    : host_(host), port_(port), db_(db), server_socket_(-1), running_(false) {
    setup_default_handlers();
}

RpcServer::~RpcServer() {
    stop();
}

void RpcServer::start() {
    // 创建socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ == -1) {
        throw BitcaskException("Failed to create RPC socket");
    }

    // 设置socket选项
    int opt = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 绑定地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr);

    if (bind(server_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        close(server_socket_);
        throw BitcaskException("Failed to bind RPC socket");
    }

    // 监听连接
    if (listen(server_socket_, 10) == -1) {
        close(server_socket_);
        throw BitcaskException("Failed to listen on RPC socket");
    }

    running_ = true;
    std::cout << "RPC server started on " << host_ << ":" << port_ << std::endl;

    // 在单独线程中运行服务器
    server_thread_ = std::thread([this]() {
        while (running_) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
            if (client_socket == -1) {
                if (running_) {
                    std::cerr << "Failed to accept RPC client connection" << std::endl;
                }
                continue;
            }

            // 在单独线程中处理客户端连接
            std::thread client_thread([this, client_socket]() {
                handle_connection(client_socket);
                close(client_socket);
            });
            client_thread.detach();
        }
    });
}

void RpcServer::stop() {
    if (running_) {
        running_ = false;
        if (server_socket_ != -1) {
            close(server_socket_);
            server_socket_ = -1;
        }
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        std::cout << "RPC server stopped" << std::endl;
    }
}

void RpcServer::register_method(const std::string& method, RpcMethodHandler handler) {
    method_handlers_[method] = handler;
}

void RpcServer::handle_connection(int client_socket) {
    char buffer[8192];
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_received <= 0) {
        return;
    }
    
    buffer[bytes_received] = '\0';
    std::string request_data(buffer);
    
    try {
        RpcRequest request = parse_request(request_data);
        RpcResponse response;
        
        // 查找方法处理器
        auto method_it = method_handlers_.find(request.method);
        if (method_it != method_handlers_.end()) {
            response = method_it->second(request);
        } else {
            response.error = "Method not found: " + request.method;
        }
        
        response.id = request.id;
        std::string response_data = response.serialize();
        send(client_socket, response_data.c_str(), response_data.length(), 0);
        
    } catch (const std::exception& e) {
        RpcResponse error_response;
        error_response.error = "Internal server error: " + std::string(e.what());
        
        std::string response_data = error_response.serialize();
        send(client_socket, response_data.c_str(), response_data.length(), 0);
    }
}

RpcRequest RpcServer::parse_request(const std::string& data) {
    RpcRequest request;
    
    // 简单的JSON-RPC解析
    // 格式: {"method": "put", "params": ["key", "value"], "id": "1"}
    
    // 查找method
    size_t method_pos = data.find("\"method\"");
    if (method_pos != std::string::npos) {
        size_t method_start = data.find("\"", method_pos + 8);
        if (method_start != std::string::npos) {
            method_start++;
            size_t method_end = data.find("\"", method_start);
            if (method_end != std::string::npos) {
                request.method = data.substr(method_start, method_end - method_start);
            }
        }
    }
    
    // 查找params数组
    size_t params_pos = data.find("\"params\"");
    if (params_pos != std::string::npos) {
        size_t array_start = data.find("[", params_pos);
        size_t array_end = data.find("]", array_start);
        if (array_start != std::string::npos && array_end != std::string::npos) {
            std::string params_str = data.substr(array_start + 1, array_end - array_start - 1);
            
            // 解析参数数组
            std::istringstream iss(params_str);
            std::string param;
            while (std::getline(iss, param, ',')) {
                // 移除引号和空白字符
                param.erase(std::remove_if(param.begin(), param.end(), 
                           [](char c) { return c == '"' || c == ' ' || c == '\t'; }), param.end());
                if (!param.empty()) {
                    request.params.push_back(param);
                }
            }
        }
    }
    
    // 查找id
    size_t id_pos = data.find("\"id\"");
    if (id_pos != std::string::npos) {
        size_t id_start = data.find("\"", id_pos + 4);
        if (id_start != std::string::npos) {
            id_start++;
            size_t id_end = data.find("\"", id_start);
            if (id_end != std::string::npos) {
                request.id = data.substr(id_start, id_end - id_start);
            }
        }
    }
    
    return request;
}

std::string RpcResponse::serialize() const {
    std::ostringstream oss;
    oss << "{";
    
    if (!result.empty()) {
        oss << "\"result\": \"" << result << "\"";
    } else {
        oss << "\"result\": null";
    }
    
    if (!error.empty()) {
        oss << ", \"error\": \"" << error << "\"";
    } else {
        oss << ", \"error\": null";
    }
    
    oss << ", \"id\": \"" << id << "\"";
    oss << "}";
    
    return oss.str();
}

void RpcServer::setup_default_handlers() {
    register_method("put", [this](const RpcRequest& request) { return handle_put(request); });
    register_method("get", [this](const RpcRequest& request) { return handle_get(request); });
    register_method("delete", [this](const RpcRequest& request) { return handle_delete(request); });
    register_method("list_keys", [this](const RpcRequest& request) { return handle_list_keys(request); });
    register_method("stat", [this](const RpcRequest& request) { return handle_stat(request); });
    register_method("backup", [this](const RpcRequest& request) { return handle_backup(request); });
    register_method("merge", [this](const RpcRequest& request) { return handle_merge(request); });
    register_method("ping", [this](const RpcRequest& request) { return handle_ping(request); });
}

RpcResponse RpcServer::handle_put(const RpcRequest& request) {
    RpcResponse response;
    
    if (request.params.size() < 2) {
        response.error = "PUT requires 2 parameters: key and value";
        return response;
    }
    
    try {
        Bytes key(request.params[0].begin(), request.params[0].end());
        Bytes value(request.params[1].begin(), request.params[1].end());
        db_->put(key, value);
        response.result = "OK";
    } catch (const std::exception& e) {
        response.error = "PUT failed: " + std::string(e.what());
    }
    
    return response;
}

RpcResponse RpcServer::handle_get(const RpcRequest& request) {
    RpcResponse response;
    
    if (request.params.size() < 1) {
        response.error = "GET requires 1 parameter: key";
        return response;
    }
    
    try {
        Bytes key(request.params[0].begin(), request.params[0].end());
        Bytes value = db_->get(key);
        response.result = std::string(value.begin(), value.end());
    } catch (const KeyNotFoundError&) {
        response.error = "Key not found";
    } catch (const std::exception& e) {
        response.error = "GET failed: " + std::string(e.what());
    }
    
    return response;
}

RpcResponse RpcServer::handle_delete(const RpcRequest& request) {
    RpcResponse response;
    
    if (request.params.size() < 1) {
        response.error = "DELETE requires 1 parameter: key";
        return response;
    }
    
    try {
        Bytes key(request.params[0].begin(), request.params[0].end());
        db_->remove(key);
        response.result = "OK";
    } catch (const std::exception& e) {
        response.error = "DELETE failed: " + std::string(e.what());
    }
    
    return response;
}

RpcResponse RpcServer::handle_list_keys(const RpcRequest& /*request*/) {
    RpcResponse response;
    
    try {
        auto keys = db_->list_keys();
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < keys.size(); ++i) {
            if (i > 0) oss << ",";
            std::string key_str(keys[i].begin(), keys[i].end());
            oss << "\"" << json_escape(key_str) << "\"";
        }
        oss << "]";
        response.result = oss.str();
    } catch (const std::exception& e) {
        response.error = "LIST_KEYS failed: " + std::string(e.what());
    }
    
    return response;
}

RpcResponse RpcServer::handle_stat(const RpcRequest& /*request*/) {
    RpcResponse response;
    
    try {
        auto stat = db_->stat();
        std::ostringstream oss;
        oss << "{";
        oss << "\"key_num\":" << stat.key_num << ",";
        oss << "\"data_file_num\":" << stat.data_file_num << ",";
        oss << "\"reclaimable_size\":" << stat.reclaimable_size << ",";
        oss << "\"disk_size\":" << stat.disk_size;
        oss << "}";
        response.result = oss.str();
    } catch (const std::exception& e) {
        response.error = "STAT failed: " + std::string(e.what());
    }
    
    return response;
}

RpcResponse RpcServer::handle_backup(const RpcRequest& request) {
    RpcResponse response;
    
    if (request.params.size() < 1) {
        response.error = "BACKUP requires 1 parameter: backup_directory";
        return response;
    }
    
    try {
        db_->backup(request.params[0]);
        response.result = "OK";
    } catch (const std::exception& e) {
        response.error = "BACKUP failed: " + std::string(e.what());
    }
    
    return response;
}

RpcResponse RpcServer::handle_merge(const RpcRequest& /*request*/) {
    RpcResponse response;
    
    try {
        db_->merge();
        response.result = "OK";
    } catch (const std::exception& e) {
        response.error = "MERGE failed: " + std::string(e.what());
    }
    
    return response;
}

RpcResponse RpcServer::handle_ping(const RpcRequest& /*request*/) {
    RpcResponse response;
    response.result = "PONG";
    return response;
}

std::string RpcServer::json_escape(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (c == '"') {
            result += "\\\"";
        } else if (c == '\\') {
            result += "\\\\";
        } else if (c == '\n') {
            result += "\\n";
        } else if (c == '\r') {
            result += "\\r";
        } else if (c == '\t') {
            result += "\\t";
        } else {
            result += c;
        }
    }
    return result;
}

}  // namespace rpc
}  // namespace bitcask
