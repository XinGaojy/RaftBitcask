#include "bitcask/bitcask.h"
#include <iostream>
#include <sys/stat.h>

int main() {
    try {
        // 创建一个简单的测试
        std::string test_dir = "/tmp/sync_debug_test";
        
        // 清理旧目录
        system(("rm -rf " + test_dir).c_str());
        
        // 创建数据库
        bitcask::Options options = bitcask::Options::default_options();
        options.dir_path = test_dir;
        options.data_file_size = 1024 * 1024;  // 1MB
        options.sync_writes = true;
        
        std::cout << "Opening database..." << std::endl;
        auto db = bitcask::DB::open(options);
        
        std::cout << "Writing test data..." << std::endl;
        bitcask::Bytes key = {0x74, 0x65, 0x73, 0x74};  // "test"
        bitcask::Bytes value = {0x76, 0x61, 0x6c, 0x75, 0x65};  // "value"
        db->put(key, value);
        
        std::cout << "Calling sync..." << std::endl;
        db->sync();  // 这里可能会失败
        
        std::cout << "Reading data..." << std::endl;
        auto result = db->get(key);
        
        std::cout << "Closing database..." << std::endl;
        db->close();
        
        std::cout << "✓ Test completed successfully" << std::endl;
        
        // 清理
        system(("rm -rf " + test_dir).c_str());
        
    } catch (const std::exception& e) {
        std::cerr << "✗ Error: " << e.what() << std::endl;
        std::cerr << "errno: " << errno << " (" << strerror(errno) << ")" << std::endl;
        return 1;
    }
    
    return 0;
}
