#include "bitcask/redis_server.h"
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
namespace redis {

// RedisResponse 实现
RedisResponse RedisResponse::simple_string(const std::string& str) {
    RedisResponse response(RedisResponseType::SIMPLE_STRING);
    response.value = str;
    return response;
}

RedisResponse RedisResponse::error(const std::string& msg) {
    RedisResponse response(RedisResponseType::ERROR);
    response.value = msg;
    return response;
}

RedisResponse RedisResponse::integer(int64_t value) {
    RedisResponse response(RedisResponseType::INTEGER);
    response.integer_value = value;
    return response;
}

RedisResponse RedisResponse::bulk_string(const std::string& str) {
    RedisResponse response(RedisResponseType::BULK_STRING);
    response.value = str;
    return response;
}

RedisResponse RedisResponse::null_bulk_string() {
    RedisResponse response(RedisResponseType::BULK_STRING);
    response.value = "";
    response.integer_value = -1; // 标记为null
    return response;
}

RedisResponse RedisResponse::array(const std::vector<RedisResponse>& arr) {
    RedisResponse response(RedisResponseType::ARRAY);
    response.array_value = arr;
    return response;
}

std::string RedisResponse::serialize() const {
    std::ostringstream oss;
    
    switch (type) {
        case RedisResponseType::SIMPLE_STRING:
            oss << "+" << value << "\r\n";
            break;
            
        case RedisResponseType::ERROR:
            oss << "-" << value << "\r\n";
            break;
            
        case RedisResponseType::INTEGER:
            oss << ":" << integer_value << "\r\n";
            break;
            
        case RedisResponseType::BULK_STRING:
            if (integer_value == -1) {
                oss << "$-1\r\n";
            } else {
                oss << "$" << value.length() << "\r\n" << value << "\r\n";
            }
            break;
            
        case RedisResponseType::ARRAY:
            oss << "*" << array_value.size() << "\r\n";
            for (const auto& item : array_value) {
                oss << item.serialize();
            }
            break;
    }
    
    return oss.str();
}

// RedisServer 实现
RedisServer::RedisServer(const std::string& host, int port, std::shared_ptr<RedisDataStructure> rds)
    : host_(host), port_(port), rds_(rds), server_socket_(-1), running_(false) {
    setup_default_handlers();
}

RedisServer::~RedisServer() {
    stop();
}

void RedisServer::start() {
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
    std::cout << "Redis server started on " << host_ << ":" << port_ << std::endl;

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

void RedisServer::stop() {
    if (running_) {
        running_ = false;
        if (server_socket_ != -1) {
            close(server_socket_);
            server_socket_ = -1;
        }
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        std::cout << "Redis server stopped" << std::endl;
    }
}

void RedisServer::register_command(const std::string& command, RedisCommandHandler handler) {
    std::string lower_command = command;
    std::transform(lower_command.begin(), lower_command.end(), lower_command.begin(), ::tolower);
    command_handlers_[lower_command] = handler;
}

void RedisServer::handle_connection(int client_socket) {
    char buffer[8192];
    std::string accumulated_data;
    
    while (running_) {
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received <= 0) {
            break;
        }
        
        buffer[bytes_received] = '\0';
        accumulated_data += std::string(buffer);
        
        // 尝试解析命令
        try {
            size_t processed = 0;
            while (processed < accumulated_data.length()) {
                size_t old_processed = processed;
                
                if (accumulated_data[processed] == '*') {
                    // 解析数组格式的命令
                    RedisCommand command = parse_single_command(accumulated_data, processed);
                    
                    if (command.args.empty()) {
                        break; // 数据不完整，等待更多数据
                    }
                    
                    // 处理命令
                    std::string cmd = command.args[0];
                    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
                    
                    RedisResponse response;
                    auto handler_it = command_handlers_.find(cmd);
                    if (handler_it != command_handlers_.end()) {
                        response = handler_it->second(command.args);
                    } else {
                        response = RedisResponse::error("ERR unknown command '" + cmd + "'");
                    }
                    
                    // 发送响应
                    std::string response_data = response.serialize();
                    send(client_socket, response_data.c_str(), response_data.length(), 0);
                    
                    // 处理QUIT命令
                    if (cmd == "quit") {
                        return;
                    }
                } else {
                    // 跳过无效数据
                    processed++;
                }
                
                // 防止无限循环
                if (processed == old_processed) {
                    break;
                }
            }
            
            // 移除已处理的数据
            if (processed > 0) {
                accumulated_data.erase(0, processed);
            }
            
        } catch (const std::exception& e) {
            RedisResponse error_response = RedisResponse::error("ERR " + std::string(e.what()));
            std::string response_data = error_response.serialize();
            send(client_socket, response_data.c_str(), response_data.length(), 0);
            break;
        }
    }
}

std::vector<RedisCommand> RedisServer::parse_commands(const std::string& data) {
    std::vector<RedisCommand> commands;
    size_t offset = 0;
    
    while (offset < data.length()) {
        RedisCommand command = parse_single_command(data, offset);
        if (!command.args.empty()) {
            commands.push_back(command);
        } else {
            break; // 数据不完整
        }
    }
    
    return commands;
}

RedisCommand RedisServer::parse_single_command(const std::string& data, size_t& offset) {
    RedisCommand command;
    
    if (offset >= data.length() || data[offset] != '*') {
        return command; // 无效格式
    }
    
    // 跳过 '*'
    offset++;
    
    // 读取参数数量
    size_t newline_pos = data.find("\r\n", offset);
    if (newline_pos == std::string::npos) {
        return command; // 数据不完整
    }
    
    int arg_count = std::stoi(data.substr(offset, newline_pos - offset));
    offset = newline_pos + 2;
    
    // 读取每个参数
    for (int i = 0; i < arg_count; ++i) {
        if (offset >= data.length() || data[offset] != '$') {
            return RedisCommand(); // 无效格式
        }
        
        offset++; // 跳过 '$'
        
        // 读取字符串长度
        newline_pos = data.find("\r\n", offset);
        if (newline_pos == std::string::npos) {
            return RedisCommand(); // 数据不完整
        }
        
        int str_length = std::stoi(data.substr(offset, newline_pos - offset));
        offset = newline_pos + 2;
        
        // 读取字符串内容
        if (offset + str_length + 2 > data.length()) {
            return RedisCommand(); // 数据不完整
        }
        
        std::string arg = data.substr(offset, str_length);
        command.args.push_back(arg);
        offset += str_length + 2; // 跳过字符串和 "\r\n"
    }
    
    return command;
}

void RedisServer::setup_default_handlers() {
    register_command("SET", [this](const std::vector<std::string>& args) { return handle_set(args); });
    register_command("GET", [this](const std::vector<std::string>& args) { return handle_get(args); });
    register_command("DEL", [this](const std::vector<std::string>& args) { return handle_del(args); });
    register_command("EXISTS", [this](const std::vector<std::string>& args) { return handle_exists(args); });
    register_command("TYPE", [this](const std::vector<std::string>& args) { return handle_type(args); });
    register_command("HSET", [this](const std::vector<std::string>& args) { return handle_hset(args); });
    register_command("HGET", [this](const std::vector<std::string>& args) { return handle_hget(args); });
    register_command("HDEL", [this](const std::vector<std::string>& args) { return handle_hdel(args); });
    register_command("SADD", [this](const std::vector<std::string>& args) { return handle_sadd(args); });
    register_command("SISMEMBER", [this](const std::vector<std::string>& args) { return handle_sismember(args); });
    register_command("SREM", [this](const std::vector<std::string>& args) { return handle_srem(args); });
    register_command("LPUSH", [this](const std::vector<std::string>& args) { return handle_lpush(args); });
    register_command("RPUSH", [this](const std::vector<std::string>& args) { return handle_rpush(args); });
    register_command("LPOP", [this](const std::vector<std::string>& args) { return handle_lpop(args); });
    register_command("RPOP", [this](const std::vector<std::string>& args) { return handle_rpop(args); });
    register_command("ZADD", [this](const std::vector<std::string>& args) { return handle_zadd(args); });
    register_command("ZSCORE", [this](const std::vector<std::string>& args) { return handle_zscore(args); });
    register_command("PING", [this](const std::vector<std::string>& args) { return handle_ping(args); });
    register_command("QUIT", [this](const std::vector<std::string>& args) { return handle_quit(args); });
}

// 命令处理器实现
RedisResponse RedisServer::handle_set(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        return RedisResponse::error("ERR wrong number of arguments for 'set' command");
    }
    
    try {
        rds_->set(args[1], args[2]);
        return RedisResponse::simple_string("OK");
    } catch (const std::exception& e) {
        return RedisResponse::error("ERR " + std::string(e.what()));
    }
}

RedisResponse RedisServer::handle_get(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return RedisResponse::error("ERR wrong number of arguments for 'get' command");
    }
    
    try {
        std::string value = rds_->get(args[1]);
        return RedisResponse::bulk_string(value);
    } catch (const KeyNotFoundError&) {
        return RedisResponse::null_bulk_string();
    } catch (const std::exception& e) {
        return RedisResponse::error("ERR " + std::string(e.what()));
    }
}

RedisResponse RedisServer::handle_del(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        return RedisResponse::error("ERR wrong number of arguments for 'del' command");
    }
    
    int64_t deleted_count = 0;
    for (size_t i = 1; i < args.size(); ++i) {
        if (rds_->del(args[i])) {
            deleted_count++;
        }
    }
    
    return RedisResponse::integer(deleted_count);
}

RedisResponse RedisServer::handle_exists(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return RedisResponse::error("ERR wrong number of arguments for 'exists' command");
    }
    
    bool exists = rds_->exists(args[1]);
    return RedisResponse::integer(exists ? 1 : 0);
}

RedisResponse RedisServer::handle_type(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return RedisResponse::error("ERR wrong number of arguments for 'type' command");
    }
    
    try {
        RedisDataType data_type = rds_->type(args[1]);
        std::string type_str;
        switch (data_type) {
            case RedisDataType::STRING: type_str = "string"; break;
            case RedisDataType::HASH: type_str = "hash"; break;
            case RedisDataType::SET: type_str = "set"; break;
            case RedisDataType::LIST: type_str = "list"; break;
            case RedisDataType::ZSET: type_str = "zset"; break;
        }
        return RedisResponse::simple_string(type_str);
    } catch (const KeyNotFoundError&) {
        return RedisResponse::simple_string("none");
    } catch (const std::exception& e) {
        return RedisResponse::error("ERR " + std::string(e.what()));
    }
}

RedisResponse RedisServer::handle_hset(const std::vector<std::string>& args) {
    if (args.size() != 4) {
        return RedisResponse::error("ERR wrong number of arguments for 'hset' command");
    }
    
    try {
        bool is_new = rds_->hset(args[1], args[2], args[3]);
        return RedisResponse::integer(is_new ? 1 : 0);
    } catch (const std::exception& e) {
        return RedisResponse::error("ERR " + std::string(e.what()));
    }
}

RedisResponse RedisServer::handle_hget(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        return RedisResponse::error("ERR wrong number of arguments for 'hget' command");
    }
    
    try {
        std::string value = rds_->hget(args[1], args[2]);
        return RedisResponse::bulk_string(value);
    } catch (const KeyNotFoundError&) {
        return RedisResponse::null_bulk_string();
    } catch (const std::exception& e) {
        return RedisResponse::error("ERR " + std::string(e.what()));
    }
}

RedisResponse RedisServer::handle_hdel(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        return RedisResponse::error("ERR wrong number of arguments for 'hdel' command");
    }
    
    try {
        bool deleted = rds_->hdel(args[1], args[2]);
        return RedisResponse::integer(deleted ? 1 : 0);
    } catch (const std::exception& e) {
        return RedisResponse::error("ERR " + std::string(e.what()));
    }
}

RedisResponse RedisServer::handle_sadd(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        return RedisResponse::error("ERR wrong number of arguments for 'sadd' command");
    }
    
    try {
        bool is_new = rds_->sadd(args[1], args[2]);
        return RedisResponse::integer(is_new ? 1 : 0);
    } catch (const std::exception& e) {
        return RedisResponse::error("ERR " + std::string(e.what()));
    }
}

RedisResponse RedisServer::handle_sismember(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        return RedisResponse::error("ERR wrong number of arguments for 'sismember' command");
    }
    
    try {
        bool is_member = rds_->sismember(args[1], args[2]);
        return RedisResponse::integer(is_member ? 1 : 0);
    } catch (const std::exception& e) {
        return RedisResponse::error("ERR " + std::string(e.what()));
    }
}

RedisResponse RedisServer::handle_srem(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        return RedisResponse::error("ERR wrong number of arguments for 'srem' command");
    }
    
    try {
        bool removed = rds_->srem(args[1], args[2]);
        return RedisResponse::integer(removed ? 1 : 0);
    } catch (const std::exception& e) {
        return RedisResponse::error("ERR " + std::string(e.what()));
    }
}

RedisResponse RedisServer::handle_lpush(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        return RedisResponse::error("ERR wrong number of arguments for 'lpush' command");
    }
    
    try {
        uint32_t length = rds_->lpush(args[1], args[2]);
        return RedisResponse::integer(length);
    } catch (const std::exception& e) {
        return RedisResponse::error("ERR " + std::string(e.what()));
    }
}

RedisResponse RedisServer::handle_rpush(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        return RedisResponse::error("ERR wrong number of arguments for 'rpush' command");
    }
    
    try {
        uint32_t length = rds_->rpush(args[1], args[2]);
        return RedisResponse::integer(length);
    } catch (const std::exception& e) {
        return RedisResponse::error("ERR " + std::string(e.what()));
    }
}

RedisResponse RedisServer::handle_lpop(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return RedisResponse::error("ERR wrong number of arguments for 'lpop' command");
    }
    
    try {
        std::string value = rds_->lpop(args[1]);
        return RedisResponse::bulk_string(value);
    } catch (const KeyNotFoundError&) {
        return RedisResponse::null_bulk_string();
    } catch (const std::exception& e) {
        return RedisResponse::error("ERR " + std::string(e.what()));
    }
}

RedisResponse RedisServer::handle_rpop(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return RedisResponse::error("ERR wrong number of arguments for 'rpop' command");
    }
    
    try {
        std::string value = rds_->rpop(args[1]);
        return RedisResponse::bulk_string(value);
    } catch (const KeyNotFoundError&) {
        return RedisResponse::null_bulk_string();
    } catch (const std::exception& e) {
        return RedisResponse::error("ERR " + std::string(e.what()));
    }
}

RedisResponse RedisServer::handle_zadd(const std::vector<std::string>& args) {
    if (args.size() != 4) {
        return RedisResponse::error("ERR wrong number of arguments for 'zadd' command");
    }
    
    try {
        double score = std::stod(args[2]);
        bool is_new = rds_->zadd(args[1], score, args[3]);
        return RedisResponse::integer(is_new ? 1 : 0);
    } catch (const std::invalid_argument&) {
        return RedisResponse::error("ERR value is not a valid float");
    } catch (const std::exception& e) {
        return RedisResponse::error("ERR " + std::string(e.what()));
    }
}

RedisResponse RedisServer::handle_zscore(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        return RedisResponse::error("ERR wrong number of arguments for 'zscore' command");
    }
    
    try {
        double score = rds_->zscore(args[1], args[2]);
        return RedisResponse::bulk_string(std::to_string(score));
    } catch (const KeyNotFoundError&) {
        return RedisResponse::null_bulk_string();
    } catch (const std::exception& e) {
        return RedisResponse::error("ERR " + std::string(e.what()));
    }
}

RedisResponse RedisServer::handle_ping(const std::vector<std::string>& args) {
    if (args.size() == 1) {
        return RedisResponse::simple_string("PONG");
    } else if (args.size() == 2) {
        return RedisResponse::bulk_string(args[1]);
    } else {
        return RedisResponse::error("ERR wrong number of arguments for 'ping' command");
    }
}

RedisResponse RedisServer::handle_quit(const std::vector<std::string>& /*args*/) {
    return RedisResponse::simple_string("OK");
}

}  // namespace redis
}  // namespace bitcask
