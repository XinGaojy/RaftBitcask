#include "bitcask/bitcask.h"
#include "bitcask/rpc_server.h"
#include <iostream>
#include <signal.h>
#include <thread>
#include <chrono>

using namespace bitcask;

// 全局变量用于信号处理
static std::unique_ptr<bitcask::rpc::RpcServer> rpc_server;
static std::shared_ptr<DB> db;

void signal_handler(int sig) {
    std::cout << "\nReceived signal " << sig << ", shutting down RPC server..." << std::endl;
    
    if (rpc_server) {
        rpc_server->stop();
    }
    
    if (db) {
        db->close();
    }
    
    std::cout << "RPC server stopped gracefully." << std::endl;
    exit(0);
}

int main() {
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        // 配置选项
        Options options = Options::default_options();
        options.dir_path = "/tmp/bitcask_rpc_server";
        options.data_file_size = 256 * 1024 * 1024;  // 256MB
        options.sync_writes = true;
        options.index_type = IndexType::BTREE;
        options.mmap_at_startup = false;
        
        std::cout << "Starting Bitcask RPC Server..." << std::endl;
        std::cout << "Data directory: " << options.dir_path << std::endl;
        
        // 打开数据库
        db = DB::open(options);
        std::cout << "Database opened successfully." << std::endl;
        
        // 你指定的IP地址和RPC端口
        const std::string host = "192.168.197.132";
        const int rpc_port = 9090;  // RPC服务端口
        
        // 启动RPC服务器
        rpc_server = std::make_unique<bitcask::rpc::RpcServer>(host, rpc_port, db);
        
        std::thread rpc_thread([&]() {
            try {
                rpc_server->start();
            } catch (const std::exception& e) {
                std::cerr << "RPC server error: " << e.what() << std::endl;
            }
        });
        
        std::cout << "\n=== Bitcask RPC Server Started ===" << std::endl;
        std::cout << "RPC Server: " << host << ":" << rpc_port << std::endl;
        std::cout << "\nSupported RPC Methods:" << std::endl;
        std::cout << "  put(key, value)     - Store key-value pair" << std::endl;
        std::cout << "  get(key)            - Get value by key" << std::endl;
        std::cout << "  delete(key)         - Delete key" << std::endl;
        std::cout << "  list_keys()         - List all keys" << std::endl;
        std::cout << "  stat()              - Get database statistics" << std::endl;
        std::cout << "  backup(directory)   - Backup database" << std::endl;
        std::cout << "  merge()             - Merge database files" << std::endl;
        std::cout << "  ping()              - Test server connectivity" << std::endl;
        std::cout << "\nRPC Request Format (JSON-RPC):" << std::endl;
        std::cout << R"(  {"method": "put", "params": ["key1", "value1"], "id": "1"})" << std::endl;
        std::cout << R"(  {"method": "get", "params": ["key1"], "id": "2"})" << std::endl;
        std::cout << R"(  {"method": "ping", "params": [], "id": "3"})" << std::endl;
        std::cout << "\nPress Ctrl+C to stop the server." << std::endl;
        
        // 保持主线程运行
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // 定期输出统计信息
            static int counter = 0;
            if (++counter % 60 == 0) {  // 每分钟输出一次
                try {
                    auto stat = db->stat();
                    std::cout << "\n[Stats] Keys: " << stat.key_num 
                              << ", Disk Size: " << stat.disk_size 
                              << ", Reclaimable: " << stat.reclaimable_size << std::endl;
                } catch (const std::exception& e) {
                    // 忽略统计错误
                }
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "RPC server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
