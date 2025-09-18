#include <bitcask/bitcask.h>
#include "bitcask/utils.h"
#include <iostream>
#include <cassert>
#include <string>
#include <random>

using namespace bitcask;

class TestRunner {
public:
    void run_all_tests() {
        std::cout << "Running Bitcask C++ Tests" << std::endl;
        std::cout << "=========================" << std::endl;
        
        test_basic_operations();
        test_batch_operations();
        test_iterator();
        test_delete_operations();
        test_error_handling();
        test_persistence();
        
        std::cout << "\\nAll tests passed!" << std::endl;
    }

private:
    std::string get_test_dir(const std::string& test_name) {
        std::string dir = "/tmp/bitcask_test_" + test_name;
        utils::remove_directory(dir);
        return dir;
    }
    
    void test_basic_operations() {
        std::cout << "\\nTest 1: Basic Operations" << std::endl;
        
        auto temp_dir = get_test_dir("basic");
        auto db = bitcask::open(temp_dir);
        
        // Test put and get
        Bytes key = string_to_bytes("test_key");
        Bytes value = string_to_bytes("test_value");
        
        db->put(key, value);
        Bytes retrieved = db->get(key);
        assert(retrieved == value);
        std::cout << "  ✓ Put/Get operations" << std::endl;
        
        // Test overwrite
        Bytes new_value = string_to_bytes("new_test_value");
        db->put(key, new_value);
        retrieved = db->get(key);
        assert(retrieved == new_value);
        std::cout << "  ✓ Overwrite operation" << std::endl;
        
        // Test multiple keys
        for (int i = 0; i < 100; ++i) {
            Bytes k = string_to_bytes("key_" + std::to_string(i));
            Bytes v = string_to_bytes("value_" + std::to_string(i));
            db->put(k, v);
        }
        
        // Verify all keys
        for (int i = 0; i < 100; ++i) {
            Bytes k = string_to_bytes("key_" + std::to_string(i));
            Bytes expected_v = string_to_bytes("value_" + std::to_string(i));
            Bytes v = db->get(k);
            assert(v == expected_v);
        }
        std::cout << "  ✓ Multiple key operations" << std::endl;
        
        db->close();
        utils::remove_directory(temp_dir);
    }
    
    void test_batch_operations() {
        std::cout << "\\nTest 2: Batch Operations" << std::endl;
        
        auto temp_dir = get_test_dir("batch");
        auto db = bitcask::open(temp_dir);
        
        WriteBatchOptions opts = WriteBatchOptions::default_options();
        auto batch = db->new_write_batch(opts);
        
        // Add multiple operations to batch
        for (int i = 0; i < 50; ++i) {
            Bytes k = string_to_bytes("batch_key_" + std::to_string(i));
            Bytes v = string_to_bytes("batch_value_" + std::to_string(i));
            batch->put(k, v);
        }
        
        // Commit batch
        batch->commit();
        std::cout << "  ✓ Batch commit" << std::endl;
        
        // Verify all batched operations
        for (int i = 0; i < 50; ++i) {
            Bytes k = string_to_bytes("batch_key_" + std::to_string(i));
            Bytes expected_v = string_to_bytes("batch_value_" + std::to_string(i));
            Bytes v = db->get(k);
            assert(v == expected_v);
        }
        std::cout << "  ✓ Batch verification" << std::endl;
        
        db->close();
        utils::remove_directory(temp_dir);
    }
    
    void test_iterator() {
        std::cout << "\\nTest 3: Iterator" << std::endl;
        
        auto temp_dir = get_test_dir("iterator");
        auto db = bitcask::open(temp_dir);
        
        // Insert test data
        std::vector<std::string> keys = {"apple", "banana", "cherry", "date", "elderberry"};
        for (const auto& key_str : keys) {
            Bytes k = string_to_bytes(key_str);
            Bytes v = string_to_bytes(key_str + "_value");
            db->put(k, v);
        }
        
        // Test forward iteration
        IteratorOptions iter_opts;
        auto iter = db->iterator(iter_opts);
        
        std::vector<std::string> retrieved_keys;
        for (iter->rewind(); iter->valid(); iter->next()) {
            retrieved_keys.push_back(bytes_to_string(iter->key()));
        }
        
        assert(retrieved_keys.size() == keys.size());
        std::cout << "  ✓ Forward iteration" << std::endl;
        
        // Test reverse iteration
        iter_opts.reverse = true;
        iter = db->iterator(iter_opts);
        
        retrieved_keys.clear();
        for (iter->rewind(); iter->valid(); iter->next()) {
            retrieved_keys.push_back(bytes_to_string(iter->key()));
        }
        
        assert(retrieved_keys.size() == keys.size());
        std::cout << "  ✓ Reverse iteration" << std::endl;
        
        // Test prefix iteration
        db->put(string_to_bytes("test_1"), string_to_bytes("value_1"));
        db->put(string_to_bytes("test_2"), string_to_bytes("value_2"));
        db->put(string_to_bytes("other"), string_to_bytes("other_value"));
        
        iter_opts.reverse = false;
        iter_opts.prefix = string_to_bytes("test_");
        iter = db->iterator(iter_opts);
        
        int prefix_count = 0;
        for (iter->rewind(); iter->valid(); iter->next()) {
            std::string key = bytes_to_string(iter->key());
            assert(key.starts_with("test_"));
            prefix_count++;
        }
        assert(prefix_count == 2);
        std::cout << "  ✓ Prefix iteration" << std::endl;
        
        db->close();
        utils::remove_directory(temp_dir);
    }
    
    void test_delete_operations() {
        std::cout << "\\nTest 4: Delete Operations" << std::endl;
        
        auto temp_dir = get_test_dir("delete");
        auto db = bitcask::open(temp_dir);
        
        // Insert and then delete
        Bytes key = string_to_bytes("delete_me");
        Bytes value = string_to_bytes("delete_value");
        
        db->put(key, value);
        Bytes retrieved = db->get(key);
        assert(retrieved == value);
        
        db->remove(key);
        
        try {
            db->get(key);
            assert(false); // Should not reach here
        } catch (const KeyNotFoundError&) {
            std::cout << "  ✓ Delete operation" << std::endl;
        }
        
        // Test delete non-existent key (should not throw)
        db->remove(string_to_bytes("non_existent"));
        std::cout << "  ✓ Delete non-existent key" << std::endl;
        
        db->close();
        utils::remove_directory(temp_dir);
    }
    
    void test_error_handling() {
        std::cout << "\\nTest 5: Error Handling" << std::endl;
        
        auto temp_dir = get_test_dir("error");
        auto db = bitcask::open(temp_dir);
        
        // Test empty key
        try {
            db->put(Bytes(), string_to_bytes("value"));
            assert(false); // Should not reach here
        } catch (const KeyEmptyError&) {
            std::cout << "  ✓ Empty key error" << std::endl;
        }
        
        // Test key not found
        try {
            db->get(string_to_bytes("non_existent"));
            assert(false); // Should not reach here
        } catch (const KeyNotFoundError&) {
            std::cout << "  ✓ Key not found error" << std::endl;
        }
        
        db->close();
        utils::remove_directory(temp_dir);
    }
    
    void test_persistence() {
        std::cout << "\\nTest 6: Persistence" << std::endl;
        
        auto temp_dir = get_test_dir("persistence");
        
        // Write data and close
        {
            auto db = bitcask::open(temp_dir);
            
            for (int i = 0; i < 10; ++i) {
                Bytes k = string_to_bytes("persist_key_" + std::to_string(i));
                Bytes v = string_to_bytes("persist_value_" + std::to_string(i));
                db->put(k, v);
            }
            
            db->sync();
            db->close();
        }
        
        // Reopen and verify data
        {
            auto db = bitcask::open(temp_dir);
            
            for (int i = 0; i < 10; ++i) {
                Bytes k = string_to_bytes("persist_key_" + std::to_string(i));
                Bytes expected_v = string_to_bytes("persist_value_" + std::to_string(i));
                Bytes v = db->get(k);
                assert(v == expected_v);
            }
            
            std::cout << "  ✓ Data persistence" << std::endl;
            
            db->close();
        }
        
        utils::remove_directory(temp_dir);
    }
};

int main() {
    try {
        TestRunner runner;
        runner.run_all_tests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
