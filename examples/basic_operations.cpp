#include <bitcask/bitcask.h>
#include <iostream>
#include <string>

using namespace bitcask;

int main() {
    try {
        // 创建临时目录
        std::string temp_dir = "/tmp/bitcask_example";
        if (utils::directory_exists(temp_dir)) {
            utils::remove_directory(temp_dir); // 清理可能存在的旧数据
        }
        
        std::cout << "Bitcask C++ Example" << std::endl;
        std::cout << "===================" << std::endl;
        std::cout << "Using directory: " << temp_dir << std::endl << std::endl;
        
        // 配置选项
        Options options = Options::default_options();
        options.dir_path = temp_dir;
        options.data_file_size = 64 * 1024 * 1024; // 64MB
        options.sync_writes = false;
        
        // 打开数据库
        std::cout << "Opening database..." << std::endl;
        auto db = bitcask::open(options);
        
        // 基本的Put/Get操作
        std::cout << "\\n1. Basic Put/Get operations:" << std::endl;
        
        // 写入一些数据
        db->put(string_to_bytes("name"), string_to_bytes("Bitcask"));
        db->put(string_to_bytes("version"), string_to_bytes("1.0.0"));
        db->put(string_to_bytes("language"), string_to_bytes("C++"));
        db->put(string_to_bytes("author"), string_to_bytes("Your Name"));
        
        // 读取数据
        try {
            Bytes value = db->get(string_to_bytes("name"));
            std::cout << "name = " << bytes_to_string(value) << std::endl;
            
            value = db->get(string_to_bytes("version"));
            std::cout << "version = " << bytes_to_string(value) << std::endl;
            
            value = db->get(string_to_bytes("language"));
            std::cout << "language = " << bytes_to_string(value) << std::endl;
        } catch (const KeyNotFoundError& e) {
            std::cout << "Key not found: " << e.what() << std::endl;
        }
        
        // 删除操作
        std::cout << "\\n2. Delete operation:" << std::endl;
        db->remove(string_to_bytes("author"));
        
        try {
            Bytes value = db->get(string_to_bytes("author"));
            std::cout << "author = " << bytes_to_string(value) << std::endl;
        } catch (const KeyNotFoundError& e) {
            std::cout << "author key was deleted successfully" << std::endl;
        }
        
        // 列出所有keys
        std::cout << "\\n3. List all keys:" << std::endl;
        auto keys = db->list_keys();
        std::cout << "Total keys: " << keys.size() << std::endl;
        for (const auto& key : keys) {
            std::cout << "  - " << bytes_to_string(key) << std::endl;
        }
        
        // 使用Fold遍历所有数据
        std::cout << "\\n4. Fold over all data:" << std::endl;
        db->fold([](const Bytes& key, const Bytes& value) -> bool {
            std::cout << "  " << bytes_to_string(key) << " => " << bytes_to_string(value) << std::endl;
            return true; // 继续遍历
        });
        
        // 使用迭代器
        std::cout << "\\n5. Using iterator:" << std::endl;
        IteratorOptions iter_opts;
        auto iter = db->iterator(iter_opts);
        
        std::cout << "Forward iteration:" << std::endl;
        for (iter->rewind(); iter->valid(); iter->next()) {
            std::cout << "  " << bytes_to_string(iter->key()) << " => " << bytes_to_string(iter->value()) << std::endl;
        }
        
        // 反向迭代
        iter_opts.reverse = true;
        iter = db->iterator(iter_opts);
        std::cout << "Reverse iteration:" << std::endl;
        for (iter->rewind(); iter->valid(); iter->next()) {
            std::cout << "  " << bytes_to_string(iter->key()) << " => " << bytes_to_string(iter->value()) << std::endl;
        }
        
        // 批量写入
        std::cout << "\\n6. Batch write operations:" << std::endl;
        WriteBatchOptions batch_opts = WriteBatchOptions::default_options();
        auto batch = db->new_write_batch(batch_opts);
        
        batch->put(string_to_bytes("batch_key1"), string_to_bytes("batch_value1"));
        batch->put(string_to_bytes("batch_key2"), string_to_bytes("batch_value2"));
        batch->put(string_to_bytes("batch_key3"), string_to_bytes("batch_value3"));
        
        batch->commit();
        std::cout << "Batch write committed" << std::endl;
        
        // 验证批量写入
        try {
            Bytes value = db->get(string_to_bytes("batch_key2"));
            std::cout << "batch_key2 = " << bytes_to_string(value) << std::endl;
        } catch (const KeyNotFoundError& e) {
            std::cout << "Batch write failed: " << e.what() << std::endl;
        }
        
        // 统计信息
        std::cout << "\\n7. Database statistics:" << std::endl;
        Stat stat = db->stat();
        std::cout << "Key count: " << stat.key_num << std::endl;
        std::cout << "Data file count: " << stat.data_file_num << std::endl;
        std::cout << "Reclaimable size: " << stat.reclaimable_size << " bytes" << std::endl;
        std::cout << "Disk size: " << stat.disk_size << " bytes" << std::endl;
        
        // 同步数据
        std::cout << "\\n8. Syncing data to disk..." << std::endl;
        db->sync();
        
        // 关闭数据库
        std::cout << "\\nClosing database..." << std::endl;
        db->close();
        
        std::cout << "\\nExample completed successfully!" << std::endl;
        
        // 清理
        if (utils::directory_exists(temp_dir)) {
            utils::remove_directory(temp_dir);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
