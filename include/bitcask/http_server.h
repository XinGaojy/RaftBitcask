#pragma once

#include "db.h"
#include <memory>
#include <string>
#include <thread>
#include <functional>
#include <map>

namespace bitcask {
namespace http {

// HTTP请求结构
struct HttpRequest {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> query_params;
    std::string body;
};

// HTTP响应结构
struct HttpResponse {
    int status_code = 200;
    std::map<std::string, std::string> headers;
    std::string body;
    
    HttpResponse() {
        headers["Content-Type"] = "application/json";
    }
};

// HTTP处理器类型
using HttpHandler = std::function<HttpResponse(const HttpRequest&)>;

// HTTP服务器类
class HttpServer {
public:
    HttpServer(const std::string& host, int port, std::shared_ptr<DB> db);
    ~HttpServer();

    // 启动服务器
    void start();

    // 停止服务器
    void stop();

    // 注册路由处理器
    void register_handler(const std::string& method, const std::string& path, HttpHandler handler);

private:
    // 处理客户端连接
    void handle_connection(int client_socket);
    
    // 解析HTTP请求
    HttpRequest parse_request(const std::string& request_data);
    
    // 构造HTTP响应
    std::string build_response(const HttpResponse& response);
    
    // 解析查询参数
    std::map<std::string, std::string> parse_query_params(const std::string& query);
    
    // URL解码
    std::string url_decode(const std::string& str);
    
    // 默认的API处理器
    void setup_default_handlers();
    
    // API处理器
    HttpResponse handle_put(const HttpRequest& request);
    HttpResponse handle_get(const HttpRequest& request);
    HttpResponse handle_delete(const HttpRequest& request);
    HttpResponse handle_list_keys(const HttpRequest& request);
    HttpResponse handle_stat(const HttpRequest& request);

private:
    std::string host_;
    int port_;
    std::shared_ptr<DB> db_;
    int server_socket_;
    bool running_;
    std::thread server_thread_;
    
    // 路由表
    std::map<std::string, std::map<std::string, HttpHandler>> routes_;
};

// 工具函数
std::string json_escape(const std::string& str);
std::string bytes_to_json_string(const Bytes& bytes);
Bytes json_string_to_bytes(const std::string& json_str);

}  // namespace http
}  // namespace bitcask
