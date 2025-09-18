#include "bitcask/bitcask.h"
#include "bitcask/redis_data_structure.h"
#include "bitcask/redis_server.h"
#include <iostream>
#include <signal.h>
#include <memory>
#include <thread>
#include <chrono>

using namespace bitcask;
using namespace bitcask::redis;

std::unique_ptr<RedisServer> server;

void signal_handler(int /*signal*/) {
    std::cout << "\nShutting down Redis server..." << std::endl;
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
        options.dir_path = "/tmp/bitcask-redis-example";
        auto db = std::shared_ptr<DB>(DB::open(options).release());
        
        // 创建Redis数据结构
        auto rds = std::make_shared<RedisDataStructure>(db);
        
        // 创建Redis服务器
        server = std::make_unique<RedisServer>("127.0.0.1", 6380, rds);
        
        // 启动服务器
        server->start();
        
        std::cout << "Bitcask Redis Server is running..." << std::endl;
        std::cout << "Compatible with Redis protocol on port 6380" << std::endl;
        std::cout << "\nSupported commands:" << std::endl;
        std::cout << "  String: SET, GET, DEL" << std::endl;
        std::cout << "  Hash:   HSET, HGET, HDEL" << std::endl;
        std::cout << "  Set:    SADD, SISMEMBER, SREM" << std::endl;
        std::cout << "  List:   LPUSH, RPUSH, LPOP, RPOP" << std::endl;
        std::cout << "  ZSet:   ZADD, ZSCORE" << std::endl;
        std::cout << "  Other:  EXISTS, TYPE, PING, QUIT" << std::endl;
        std::cout << "\nExamples with redis-cli:" << std::endl;
        std::cout << "  redis-cli -p 6380 SET mykey myvalue" << std::endl;
        std::cout << "  redis-cli -p 6380 GET mykey" << std::endl;
        std::cout << "  redis-cli -p 6380 HSET myhash field1 value1" << std::endl;
        std::cout << "  redis-cli -p 6380 HGET myhash field1" << std::endl;
        std::cout << "  redis-cli -p 6380 SADD myset member1" << std::endl;
        std::cout << "  redis-cli -p 6380 LPUSH mylist element1" << std::endl;
        std::cout << "  redis-cli -p 6380 ZADD myzset 1.0 member1" << std::endl;
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
