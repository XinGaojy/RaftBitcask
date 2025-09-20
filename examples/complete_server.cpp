#include "bitcask/bitcask.h"
#include "bitcask/http_server.h"
#include "bitcask/redis_server.h"
#include "bitcask/redis_data_structure.h"
#include "bitcask/rpc_server.h"
#include <iostream>
#include <signal.h>
#include <thread>
#include <chrono>

using namespace bitcask;

// 全局变量用于信号处理
static std::unique_ptr<bitcask::http::HttpServer> http_server;
static std::unique_ptr<bitcask::redis::RedisServer> redis_server;
static std::unique_ptr<bitcask::rpc::RpcServer> rpc_server;
static std::shared_ptr<DB> db;
static std::shared_ptr<bitcask::redis::RedisDataStructure> rds;

void signal_handler(int sig) {
    std::cout << "\nReceived signal " << sig << ", shutting down servers..." << std::endl;
    
    if (http_server) {
        http_server->stop();
    }
    
    if (redis_server) {
        redis_server->stop();
    }
    
    if (rpc_server) {
        rpc_server->stop();
    }
    
    if (db) {
        db->close();
    }
    
    std::cout << "Servers stopped gracefully." << std::endl;
    exit(0);
}

int main() {
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        // 配置选项
        Options options = Options::default_options();
        options.dir_path = "/tmp/bitcask_complete_server";
        options.data_file_size = 256 * 1024 * 1024;  // 256MB
        options.sync_writes = true;
        options.index_type = IndexType::BTREE;
        options.mmap_at_startup = false;
        
        std::cout << "Starting Bitcask Complete Server..." << std::endl;
        std::cout << "Data directory: " << options.dir_path << std::endl;
        
        // 打开数据库
        db = DB::open(options);
        std::cout << "Database opened successfully." << std::endl;
        
        // 创建Redis数据结构
        rds = std::make_shared<bitcask::redis::RedisDataStructure>(db);
        std::cout << "Redis data structures initialized." << std::endl;
        
        // 你指定的IP地址
        const std::string host = "192.168.197.132";
        
        // 启动HTTP服务器 (端口 8080)
        http_server = std::make_unique<bitcask::http::HttpServer>(host, 8080, db);
        std::thread http_thread([&]() {
            try {
                http_server->start();
            } catch (const std::exception& e) {
                std::cerr << "HTTP server error: " << e.what() << std::endl;
            }
        });
        
        // 启动Redis服务器 (端口 6379)
        redis_server = std::make_unique<bitcask::redis::RedisServer>(host, 6379, rds);
        std::thread redis_thread([&]() {
            try {
                redis_server->start();
            } catch (const std::exception& e) {
                std::cerr << "Redis server error: " << e.what() << std::endl;
            }
        });
        
        // 启动RPC服务器 (端口 9090)
        rpc_server = std::make_unique<bitcask::rpc::RpcServer>(host, 9090, db);
        std::thread rpc_thread([&]() {
            try {
                rpc_server->start();
            } catch (const std::exception& e) {
                std::cerr << "RPC server error: " << e.what() << std::endl;
            }
        });
        
        std::cout << "\n=== Bitcask Complete Server Started ===" << std::endl;
        std::cout << "HTTP Server: http://" << host << ":8080" << std::endl;
        std::cout << "Redis Server: redis://" << host << ":6379" << std::endl;
        std::cout << "RPC Server: " << host << ":9090" << std::endl;
        std::cout << "\nHTTP API Endpoints:" << std::endl;
        std::cout << "  POST /bitcask/put     - Put key-value" << std::endl;
        std::cout << "  GET  /bitcask/get     - Get value by key" << std::endl;
        std::cout << "  DELETE /bitcask/delete - Delete key" << std::endl;
        std::cout << "  GET  /bitcask/listkeys - List all keys" << std::endl;
        std::cout << "  GET  /bitcask/stat     - Get database statistics" << std::endl;
        std::cout << "\nRedis Commands:" << std::endl;
        std::cout << "  String: GET, SET, DEL, EXISTS, STRLEN" << std::endl;
        std::cout << "  Hash:   HGET, HSET, HDEL, HEXISTS, HLEN, HKEYS, HVALS" << std::endl;
        std::cout << "  Set:    SADD, SREM, SISMEMBER, SCARD, SMEMBERS" << std::endl;
        std::cout << "  List:   LPUSH, RPUSH, LPOP, RPOP, LLEN, LRANGE" << std::endl;
        std::cout << "  ZSet:   ZADD, ZREM, ZSCORE, ZCARD, ZRANGE" << std::endl;
        std::cout << "\nRPC Methods (JSON-RPC):" << std::endl;
        std::cout << "  put(key, value)     - Store key-value pair" << std::endl;
        std::cout << "  get(key)            - Get value by key" << std::endl;
        std::cout << "  delete(key)         - Delete key" << std::endl;
        std::cout << "  list_keys()         - List all keys" << std::endl;
        std::cout << "  stat()              - Get database statistics" << std::endl;
        std::cout << "  backup(directory)   - Backup database" << std::endl;
        std::cout << "  merge()             - Merge database files" << std::endl;
        std::cout << "  ping()              - Test server connectivity" << std::endl;
        std::cout << "\nPress Ctrl+C to stop the servers." << std::endl;
        
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
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
