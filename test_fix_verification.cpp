#include "bitcask/bitcask.h"
#include <iostream>
#include <vector>
#include <string>
#include <cassert>

// 简单的备份功能验证测试
int main() {
    std::cout << "=== 备份功能修复验证测试 ===" << std::endl;
    
    const std::string test_dir = "/tmp/test_backup_fix";
    const std::string backup_dir = "/tmp/test_backup_fix_backup";
    
    // 清理测试目录
    system(("rm -rf " + test_dir).c_str());
    system(("rm -rf " + backup_dir).c_str());
    
    try {
        std::cout << "1. 创建原始数据库并写入数据..." << std::endl;
        
        // 创建数据库配置
        bitcask::Options options = bitcask::Options::default_options();
        options.dir_path = test_dir;
        options.data_file_size = 1024 * 1024;  // 1MB
        options.sync_writes = true;
        options.index_type = bitcask::IndexType::BTREE;  // 使用与测试相同的索引类型
        options.mmap_at_startup = false;
        
        // 创建数据库并写入测试数据
        auto db = bitcask::DB::open(options);
        
        std::vector<std::pair<std::string, std::string>> test_data;
        for (int i = 0; i < 100; ++i) {
            std::string key = "test_key_" + std::to_string(i);
            std::string value = "test_value_" + std::to_string(i);
            test_data.push_back({key, value});
            
            bitcask::Bytes key_bytes(key.begin(), key.end());
            bitcask::Bytes value_bytes(value.begin(), value.end());
            db->put(key_bytes, value_bytes);
        }
        
        // 删除一些数据
        for (int i = 0; i < 20; ++i) {
            std::string key = "test_key_" + std::to_string(i);
            bitcask::Bytes key_bytes(key.begin(), key.end());
            db->remove(key_bytes);
        }
        
        std::cout << "2. 执行同步和备份..." << std::endl;
        
        // 同步数据
        db->sync();
        
        // 执行备份
        db->backup(backup_dir);
        
        // 验证原始数据库
        auto original_keys = db->list_keys();
        std::cout << "原始数据库键数量: " << original_keys.size() << std::endl;
        
        db->close();
        
        std::cout << "3. 从备份恢复数据库..." << std::endl;
        
        // 从备份创建新数据库
        bitcask::Options backup_options = options;
        backup_options.dir_path = backup_dir;
        auto restored_db = bitcask::DB::open(backup_options);
        
        // 验证恢复的数据
        auto restored_keys = restored_db->list_keys();
        std::cout << "恢复数据库键数量: " << restored_keys.size() << std::endl;
        
        if (restored_keys.size() != 80) {  // 100 - 20 = 80
            std::cout << "❌ 错误：恢复的键数量不正确，期望80，实际" << restored_keys.size() << std::endl;
            return 1;
        }
        
        std::cout << "4. 验证数据完整性..." << std::endl;
        
        // 验证被删除的数据不存在
        for (int i = 0; i < 20; ++i) {
            std::string key = "test_key_" + std::to_string(i);
            bitcask::Bytes key_bytes(key.begin(), key.end());
            try {
                restored_db->get(key_bytes);
                std::cout << "❌ 错误：被删除的键 " << key << " 仍然存在" << std::endl;
                return 1;
            } catch (const bitcask::KeyNotFoundError&) {
                // 正确：键应该不存在
            }
        }
        
        // 验证剩余数据存在且正确
        for (int i = 20; i < 100; ++i) {
            std::string key = "test_key_" + std::to_string(i);
            std::string expected_value = "test_value_" + std::to_string(i);
            
            bitcask::Bytes key_bytes(key.begin(), key.end());
            try {
                auto actual_value = restored_db->get(key_bytes);
                std::string actual_str(actual_value.begin(), actual_value.end());
                
                if (actual_str != expected_value) {
                    std::cout << "❌ 错误：键 " << key << " 的值不正确" << std::endl;
                    std::cout << "   期望: " << expected_value << std::endl;
                    std::cout << "   实际: " << actual_str << std::endl;
                    return 1;
                }
            } catch (const bitcask::KeyNotFoundError&) {
                std::cout << "❌ 错误：键 " << key << " 应该存在但未找到" << std::endl;
                return 1;
            }
        }
        
        restored_db->close();
        
        std::cout << "✅ 备份功能修复验证成功！" << std::endl;
        std::cout << "   - 数据正确备份" << std::endl;
        std::cout << "   - 索引正确重建" << std::endl;
        std::cout << "   - 数据完整性验证通过" << std::endl;
        
        // 清理测试目录
        system(("rm -rf " + test_dir).c_str());
        system(("rm -rf " + backup_dir).c_str());
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "❌ 测试失败: " << e.what() << std::endl;
        return 1;
    }
}
