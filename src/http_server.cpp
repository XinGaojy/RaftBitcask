#include "bitcask/http_server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace bitcask {
namespace http {

HttpServer::HttpServer(const std::string& host, int port, std::shared_ptr<DB> db)
    : host_(host), port_(port), db_(db), server_socket_(-1), running_(false) {
    setup_default_handlers();
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    // 创建socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ == -1) {
        throw BitcaskException("Failed to create socket");
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
        throw BitcaskException("Failed to bind socket");
    }

    // 监听连接
    if (listen(server_socket_, 10) == -1) {
        close(server_socket_);
        throw BitcaskException("Failed to listen on socket");
    }

    running_ = true;
    std::cout << "HTTP server started on " << host_ << ":" << port_ << std::endl;

    // 在单独线程中运行服务器
    server_thread_ = std::thread([this]() {
        while (running_) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
            if (client_socket == -1) {
                if (running_) {
                    std::cerr << "Failed to accept client connection" << std::endl;
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

void HttpServer::stop() {
    if (running_) {
        running_ = false;
        if (server_socket_ != -1) {
            close(server_socket_);
            server_socket_ = -1;
        }
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        std::cout << "HTTP server stopped" << std::endl;
    }
}

void HttpServer::register_handler(const std::string& method, const std::string& path, HttpHandler handler) {
    routes_[method][path] = handler;
}

void HttpServer::handle_connection(int client_socket) {
    char buffer[8192];
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_received <= 0) {
        return;
    }
    
    buffer[bytes_received] = '\0';
    std::string request_data(buffer);
    
    try {
        HttpRequest request = parse_request(request_data);
        HttpResponse response;
        
        // 查找路由处理器
        auto method_it = routes_.find(request.method);
        if (method_it != routes_.end()) {
            auto path_it = method_it->second.find(request.path);
            if (path_it != method_it->second.end()) {
                response = path_it->second(request);
            } else {
                response.status_code = 404;
                response.body = R"({"error": "Not found"})";
            }
        } else {
            response.status_code = 405;
            response.body = R"({"error": "Method not allowed"})";
        }
        
        std::string response_data = build_response(response);
        send(client_socket, response_data.c_str(), response_data.length(), 0);
        
    } catch (const std::exception& e) {
        HttpResponse error_response;
        error_response.status_code = 500;
        error_response.body = R"({"error": "Internal server error"})";
        
        std::string response_data = build_response(error_response);
        send(client_socket, response_data.c_str(), response_data.length(), 0);
    }
}

HttpRequest HttpServer::parse_request(const std::string& request_data) {
    HttpRequest request;
    std::istringstream iss(request_data);
    std::string line;
    
    // 解析请求行
    if (std::getline(iss, line)) {
        std::istringstream line_stream(line);
        std::string url;
        line_stream >> request.method >> url;
        
        // 解析URL和查询参数
        size_t query_pos = url.find('?');
        if (query_pos != std::string::npos) {
            request.path = url.substr(0, query_pos);
            std::string query = url.substr(query_pos + 1);
            request.query_params = parse_query_params(query);
        } else {
            request.path = url;
        }
    }
    
    // 解析头部
    while (std::getline(iss, line) && line != "\r" && !line.empty()) {
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            
            // 去除空白字符
            key.erase(key.find_last_not_of(" \t\r") + 1);
            value.erase(0, value.find_first_not_of(" \t\r"));
            value.erase(value.find_last_not_of(" \t\r") + 1);
            
            request.headers[key] = value;
        }
    }
    
    // 读取body
    std::string remaining_data;
    while (std::getline(iss, line)) {
        if (!remaining_data.empty()) {
            remaining_data += "\n";
        }
        remaining_data += line;
    }
    request.body = remaining_data;
    
    return request;
}

std::string HttpServer::build_response(const HttpResponse& response) {
    std::ostringstream oss;
    
    // 状态行
    oss << "HTTP/1.1 " << response.status_code << " ";
    switch (response.status_code) {
        case 200: oss << "OK"; break;
        case 400: oss << "Bad Request"; break;
        case 404: oss << "Not Found"; break;
        case 405: oss << "Method Not Allowed"; break;
        case 500: oss << "Internal Server Error"; break;
        default: oss << "Unknown"; break;
    }
    oss << "\r\n";
    
    // 头部
    for (const auto& [key, value] : response.headers) {
        oss << key << ": " << value << "\r\n";
    }
    
    // Content-Length
    oss << "Content-Length: " << response.body.length() << "\r\n";
    oss << "\r\n";
    
    // Body
    oss << response.body;
    
    return oss.str();
}

std::map<std::string, std::string> HttpServer::parse_query_params(const std::string& query) {
    std::map<std::string, std::string> params;
    std::istringstream iss(query);
    std::string pair;
    
    while (std::getline(iss, pair, '&')) {
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = url_decode(pair.substr(0, eq_pos));
            std::string value = url_decode(pair.substr(eq_pos + 1));
            params[key] = value;
        }
    }
    
    return params;
}

std::string HttpServer::url_decode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int hex_value;
            std::istringstream hex_stream(str.substr(i + 1, 2));
            hex_stream >> std::hex >> hex_value;
            result += static_cast<char>(hex_value);
            i += 2;
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

void HttpServer::setup_default_handlers() {
    // PUT /bitcask/put
    register_handler("POST", "/bitcask/put", [this](const HttpRequest& request) {
        return handle_put(request);
    });
    
    // GET /bitcask/get
    register_handler("GET", "/bitcask/get", [this](const HttpRequest& request) {
        return handle_get(request);
    });
    
    // DELETE /bitcask/delete
    register_handler("DELETE", "/bitcask/delete", [this](const HttpRequest& request) {
        return handle_delete(request);
    });
    
    // GET /bitcask/listkeys
    register_handler("GET", "/bitcask/listkeys", [this](const HttpRequest& request) {
        return handle_list_keys(request);
    });
    
    // GET /bitcask/stat
    register_handler("GET", "/bitcask/stat", [this](const HttpRequest& request) {
        return handle_stat(request);
    });
}

HttpResponse HttpServer::handle_put(const HttpRequest& request) {
    HttpResponse response;
    
    try {
        // 解析JSON body
        if (request.body.empty()) {
            response.status_code = 400;
            response.body = R"({"error": "Empty request body"})";
            return response;
        }
        
        // 更健壮的JSON解析
        std::string key, value;
        
        // 查找 "key" 字段
        size_t key_pos = request.body.find("\"key\"");
        if (key_pos == std::string::npos) {
            response.status_code = 400;
            response.body = R"({"error": "Missing key field"})";
            return response;
        }
        
        // 查找key的值
        size_t key_colon = request.body.find(":", key_pos);
        if (key_colon == std::string::npos) {
            response.status_code = 400;
            response.body = R"({"error": "Invalid key format"})";
            return response;
        }
        
        size_t key_start = request.body.find("\"", key_colon);
        if (key_start == std::string::npos) {
            response.status_code = 400;
            response.body = R"({"error": "Invalid key value format"})";
            return response;
        }
        key_start++; // 跳过开始引号
        
        size_t key_end = request.body.find("\"", key_start);
        if (key_end == std::string::npos) {
            response.status_code = 400;
            response.body = R"({"error": "Invalid key value format"})";
            return response;
        }
        
        key = request.body.substr(key_start, key_end - key_start);
        
        // 查找 "value" 字段
        size_t value_pos = request.body.find("\"value\"");
        if (value_pos == std::string::npos) {
            response.status_code = 400;
            response.body = R"({"error": "Missing value field"})";
            return response;
        }
        
        // 查找value的值
        size_t value_colon = request.body.find(":", value_pos);
        if (value_colon == std::string::npos) {
            response.status_code = 400;
            response.body = R"({"error": "Invalid value format"})";
            return response;
        }
        
        size_t value_start = request.body.find("\"", value_colon);
        if (value_start == std::string::npos) {
            response.status_code = 400;
            response.body = R"({"error": "Invalid value format"})";
            return response;
        }
        value_start++; // 跳过开始引号
        
        size_t value_end = request.body.find("\"", value_start);
        if (value_end == std::string::npos) {
            response.status_code = 400;
            response.body = R"({"error": "Invalid value format"})";
            return response;
        }
        
        value = request.body.substr(value_start, value_end - value_start);
        
        // 存储到数据库
        Bytes key_bytes(key.begin(), key.end());
        Bytes value_bytes(value.begin(), value.end());
        db_->put(key_bytes, value_bytes);
        
        response.body = R"({"status": "OK"})";
        
    } catch (const std::exception& e) {
        response.status_code = 500;
        response.body = R"({"error": "Failed to put data"})";
    }
    
    return response;
}

HttpResponse HttpServer::handle_get(const HttpRequest& request) {
    HttpResponse response;
    
    try {
        auto key_it = request.query_params.find("key");
        if (key_it == request.query_params.end()) {
            response.status_code = 400;
            response.body = R"({"error": "Missing key parameter"})";
            return response;
        }
        
        Bytes key_bytes(key_it->second.begin(), key_it->second.end());
        Bytes value_bytes = db_->get(key_bytes);
        
        std::string value_str(value_bytes.begin(), value_bytes.end());
        response.body = R"({"key": ")" + json_escape(key_it->second) + R"(", "value": ")" + json_escape(value_str) + R"("})";
        
    } catch (const KeyNotFoundError&) {
        response.status_code = 404;
        response.body = R"({"error": "Key not found"})";
    } catch (const std::exception& e) {
        response.status_code = 500;
        response.body = R"({"error": "Failed to get data"})";
    }
    
    return response;
}

HttpResponse HttpServer::handle_delete(const HttpRequest& request) {
    HttpResponse response;
    
    try {
        auto key_it = request.query_params.find("key");
        if (key_it == request.query_params.end()) {
            response.status_code = 400;
            response.body = R"({"error": "Missing key parameter"})";
            return response;
        }
        
        Bytes key_bytes(key_it->second.begin(), key_it->second.end());
        db_->remove(key_bytes);
        
        response.body = R"({"status": "OK"})";
        
    } catch (const std::exception& e) {
        response.status_code = 500;
        response.body = R"({"error": "Failed to delete data"})";
    }
    
    return response;
}

HttpResponse HttpServer::handle_list_keys(const HttpRequest& /*request*/) {
    HttpResponse response;
    
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
        
        response.body = oss.str();
        
    } catch (const std::exception& e) {
        response.status_code = 500;
        response.body = R"({"error": "Failed to list keys"})";
    }
    
    return response;
}

HttpResponse HttpServer::handle_stat(const HttpRequest& /*request*/) {
    HttpResponse response;
    
    try {
        auto stat = db_->stat();
        
        std::ostringstream oss;
        oss << "{";
        oss << "\"key_num\":" << stat.key_num << ",";
        oss << "\"data_file_num\":" << stat.data_file_num << ",";
        oss << "\"reclaimable_size\":" << stat.reclaimable_size << ",";
        oss << "\"disk_size\":" << stat.disk_size;
        oss << "}";
        
        response.body = oss.str();
        
    } catch (const std::exception& e) {
        response.status_code = 500;
        response.body = R"({"error": "Failed to get statistics"})";
    }
    
    return response;
}

// 工具函数实现
std::string json_escape(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

std::string bytes_to_json_string(const Bytes& bytes) {
    std::string str(bytes.begin(), bytes.end());
    return json_escape(str);
}

Bytes json_string_to_bytes(const std::string& json_str) {
    return Bytes(json_str.begin(), json_str.end());
}

}  // namespace http
}  // namespace bitcask
