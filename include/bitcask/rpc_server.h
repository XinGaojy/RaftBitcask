#pragma once

#include "db.h"
#include "common.h"
#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <map>
#include <vector>
#include <sstream>

namespace bitcask {
namespace rpc {

// RPC请求结构
struct RpcRequest {
    std::string method;
    std::vector<std::string> params;
    std::string id;
};

// RPC响应结构
struct RpcResponse {
    std::string result;
    std::string error;
    std::string id;
    
    std::string serialize() const;
};

// RPC方法处理器类型
using RpcMethodHandler = std::function<RpcResponse(const RpcRequest&)>;

// 简单的RPC服务器类
class RpcServer {
public:
    RpcServer(const std::string& host, int port, std::shared_ptr<DB> db);
    ~RpcServer();

    // 启动服务器
    void start();

    // 停止服务器
    void stop();

    // 注册RPC方法
    void register_method(const std::string& method, RpcMethodHandler handler);

private:
    // 处理客户端连接
    void handle_connection(int client_socket);

    // 解析RPC请求
    RpcRequest parse_request(const std::string& data);

    // 设置默认的RPC方法处理器
    void setup_default_handlers();

    // RPC方法实现
    RpcResponse handle_put(const RpcRequest& request);
    RpcResponse handle_get(const RpcRequest& request);
    RpcResponse handle_delete(const RpcRequest& request);
    RpcResponse handle_list_keys(const RpcRequest& request);
    RpcResponse handle_stat(const RpcRequest& request);
    RpcResponse handle_backup(const RpcRequest& request);
    RpcResponse handle_merge(const RpcRequest& request);
    RpcResponse handle_ping(const RpcRequest& request);

    // JSON工具函数
    std::string json_escape(const std::string& str);

private:
    std::string host_;
    int port_;
    std::shared_ptr<DB> db_;
    int server_socket_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    std::map<std::string, RpcMethodHandler> method_handlers_;
};

}  // namespace rpc
}  // namespace bitcask
