#pragma once

#include "redis_data_structure.h"
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <functional>
#include <map>

namespace bitcask {
namespace redis {

// Redis命令结构
struct RedisCommand {
    std::vector<std::string> args;
};

// Redis响应类型
enum class RedisResponseType {
    SIMPLE_STRING,
    ERROR,
    INTEGER,
    BULK_STRING,
    ARRAY
};

// Redis响应结构
struct RedisResponse {
    RedisResponseType type;
    std::string value;
    int64_t integer_value;
    std::vector<RedisResponse> array_value;
    
    RedisResponse(RedisResponseType t = RedisResponseType::SIMPLE_STRING) 
        : type(t), integer_value(0) {}
    
    // 创建简单字符串响应
    static RedisResponse simple_string(const std::string& str);
    
    // 创建错误响应
    static RedisResponse error(const std::string& msg);
    
    // 创建整数响应
    static RedisResponse integer(int64_t value);
    
    // 创建批量字符串响应
    static RedisResponse bulk_string(const std::string& str);
    
    // 创建空响应
    static RedisResponse null_bulk_string();
    
    // 创建数组响应
    static RedisResponse array(const std::vector<RedisResponse>& arr);
    
    // 序列化为Redis协议格式
    std::string serialize() const;
};

// Redis命令处理器类型
using RedisCommandHandler = std::function<RedisResponse(const std::vector<std::string>&)>;

// Redis服务器类
class RedisServer {
public:
    RedisServer(const std::string& host, int port, std::shared_ptr<RedisDataStructure> rds);
    ~RedisServer();

    // 启动服务器
    void start();

    // 停止服务器
    void stop();

    // 注册命令处理器
    void register_command(const std::string& command, RedisCommandHandler handler);

private:
    // 处理客户端连接
    void handle_connection(int client_socket);
    
    // 解析Redis协议
    std::vector<RedisCommand> parse_commands(const std::string& data);
    
    // 解析单个命令
    RedisCommand parse_single_command(const std::string& data, size_t& offset);
    
    // 读取批量字符串
    std::string read_bulk_string(const std::string& data, size_t& offset, int length);
    
    // 设置默认命令处理器
    void setup_default_handlers();
    
    // 命令处理器
    RedisResponse handle_set(const std::vector<std::string>& args);
    RedisResponse handle_get(const std::vector<std::string>& args);
    RedisResponse handle_del(const std::vector<std::string>& args);
    RedisResponse handle_exists(const std::vector<std::string>& args);
    RedisResponse handle_type(const std::vector<std::string>& args);
    RedisResponse handle_hset(const std::vector<std::string>& args);
    RedisResponse handle_hget(const std::vector<std::string>& args);
    RedisResponse handle_hdel(const std::vector<std::string>& args);
    RedisResponse handle_sadd(const std::vector<std::string>& args);
    RedisResponse handle_sismember(const std::vector<std::string>& args);
    RedisResponse handle_srem(const std::vector<std::string>& args);
    RedisResponse handle_lpush(const std::vector<std::string>& args);
    RedisResponse handle_rpush(const std::vector<std::string>& args);
    RedisResponse handle_lpop(const std::vector<std::string>& args);
    RedisResponse handle_rpop(const std::vector<std::string>& args);
    RedisResponse handle_zadd(const std::vector<std::string>& args);
    RedisResponse handle_zscore(const std::vector<std::string>& args);
    RedisResponse handle_ping(const std::vector<std::string>& args);
    RedisResponse handle_quit(const std::vector<std::string>& args);

private:
    std::string host_;
    int port_;
    std::shared_ptr<RedisDataStructure> rds_;
    int server_socket_;
    bool running_;
    std::thread server_thread_;
    
    // 命令处理器映射
    std::map<std::string, RedisCommandHandler> command_handlers_;
};

}  // namespace redis
}  // namespace bitcask
