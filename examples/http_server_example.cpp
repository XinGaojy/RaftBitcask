#include "bitcask/bitcask.h"
#include "bitcask/http_server.h"
#include <iostream>
#include <signal.h>
#include <memory>
#include <thread>
#include <chrono>

using namespace bitcask;

std::unique_ptr<http::HttpServer> server;

void signal_handler(int /*signal*/) {
    std::cout << "\nShutting down server..." << std::endl;
    if (server) {
        server->stop();
    }
    exit(0);
}

int main() {
    // 设置信号处理器
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        // 创建数据库实例
        Options options = Options::default_options();
        options.dir_path = "/tmp/bitcask-http-example";
        auto db = std::shared_ptr<DB>(DB::open(options).release());
        
        // 创建HTTP服务器
        server = std::make_unique<http::HttpServer>("127.0.0.1", 8080, db);
        
        // 启动服务器
        server->start();
        
        std::cout << "Bitcask HTTP Server is running..." << std::endl;
        std::cout << "API endpoints:" << std::endl;
        std::cout << "  POST /bitcask/put    - Put key-value pairs" << std::endl;
        std::cout << "  GET  /bitcask/get    - Get value by key" << std::endl;
        std::cout << "  DELETE /bitcask/delete - Delete key" << std::endl;
        std::cout << "  GET  /bitcask/listkeys - List all keys" << std::endl;
        std::cout << "  GET  /bitcask/stat   - Get database statistics" << std::endl;
        std::cout << "\nExamples:" << std::endl;
        std::cout << "  curl -X POST -d '{\"key1\":\"value1\",\"key2\":\"value2\"}' http://localhost:8080/bitcask/put" << std::endl;
        std::cout << "  curl http://localhost:8080/bitcask/get?key=key1" << std::endl;
        std::cout << "  curl -X DELETE http://localhost:8080/bitcask/delete?key=key1" << std::endl;
        std::cout << "  curl http://localhost:8080/bitcask/listkeys" << std::endl;
        std::cout << "  curl http://localhost:8080/bitcask/stat" << std::endl;
        std::cout << "\nPress Ctrl+C to stop the server" << std::endl;
        
        // 保持主线程运行
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
