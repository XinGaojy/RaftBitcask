#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "bitcask/bitcask.h"
#include "bitcask/http_server.h"
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>

namespace bitcask {
namespace http {
namespace test {

class HttpServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时目录
        temp_dir_ = "/tmp/bitcask_http_test";
        system(("rm -rf " + temp_dir_).c_str());
        system(("mkdir -p " + temp_dir_).c_str());
        
        // 创建数据库实例
        options_.dir_path = temp_dir_;
        options_.data_file_size = 64 * 1024;
        options_.sync_writes = false;
        
        db_ = std::shared_ptr<DB>(DB::open(options_).release());
        
        // 创建HTTP服务器
        server_ = std::make_unique<HttpServer>("127.0.0.1", 8081, db_);
        server_->start();
        
        // 等待服务器启动
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        if (server_) {
            server_->stop();
        }
        system(("rm -rf " + temp_dir_).c_str());
    }

    // 发送HTTP请求的辅助函数
    std::string send_http_request(const std::string& request) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            return "";
        }
        
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(8081);
        inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
        
        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sock);
            return "";
        }
        
        send(sock, request.c_str(), request.length(), 0);
        
        char buffer[8192];
        ssize_t bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        close(sock);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            return std::string(buffer);
        }
        
        return "";
    }

    // 解析HTTP响应
    std::pair<int, std::string> parse_response(const std::string& response) {
        std::istringstream iss(response);
        std::string line;
        
        int status_code = 0;
        if (std::getline(iss, line)) {
            std::istringstream status_line(line);
            std::string http_version, status_str;
            status_line >> http_version >> status_str;
            status_code = std::stoi(status_str);
        }
        
        // 跳过头部
        while (std::getline(iss, line) && line != "\r" && !line.empty()) {
            // Skip headers
        }
        
        // 读取body
        std::string body;
        while (std::getline(iss, line)) {
            if (!body.empty()) body += "\n";
            body += line;
        }
        
        return {status_code, body};
    }

    std::string temp_dir_;
    Options options_;
    std::shared_ptr<DB> db_;
    std::unique_ptr<HttpServer> server_;
};

// 测试PUT请求
TEST_F(HttpServerTest, TestPutRequest) {
    std::string request = 
        "POST /bitcask/put HTTP/1.1\r\n"
        "Host: localhost:8081\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 27\r\n"
        "\r\n"
        "{\"key1\":\"value1\",\"key2\":\"value2\"}";
    
    std::string response = send_http_request(request);
    auto [status_code, body] = parse_response(response);
    
    EXPECT_EQ(status_code, 200);
    EXPECT_TRUE(body.find("OK") != std::string::npos);
    
    // 验证数据是否写入成功
    Bytes key1_bytes{'k', 'e', 'y', '1'};
    Bytes value1 = db_->get(key1_bytes);
    std::string value1_str(value1.begin(), value1.end());
    EXPECT_EQ(value1_str, "value1");
}

// 测试GET请求
TEST_F(HttpServerTest, TestGetRequest) {
    // 先插入数据
    Bytes key_bytes{'t', 'e', 's', 't', '_', 'k', 'e', 'y'};
    Bytes value_bytes{'t', 'e', 's', 't', '_', 'v', 'a', 'l', 'u', 'e'};
    db_->put(key_bytes, value_bytes);
    
    std::string request = 
        "GET /bitcask/get?key=test_key HTTP/1.1\r\n"
        "Host: localhost:8081\r\n"
        "\r\n";
    
    std::string response = send_http_request(request);
    auto [status_code, body] = parse_response(response);
    
    EXPECT_EQ(status_code, 200);
    EXPECT_TRUE(body.find("test_value") != std::string::npos);
}

// 测试GET不存在的key
TEST_F(HttpServerTest, TestGetNonExistentKey) {
    std::string request = 
        "GET /bitcask/get?key=non_existent_key HTTP/1.1\r\n"
        "Host: localhost:8081\r\n"
        "\r\n";
    
    std::string response = send_http_request(request);
    auto [status_code, body] = parse_response(response);
    
    EXPECT_EQ(status_code, 404);
    EXPECT_TRUE(body.find("not found") != std::string::npos);
}

// 测试DELETE请求
TEST_F(HttpServerTest, TestDeleteRequest) {
    // 先插入数据
    Bytes key_bytes{'d', 'e', 'l', 'e', 't', 'e', '_', 'k', 'e', 'y'};
    Bytes value_bytes{'d', 'e', 'l', 'e', 't', 'e', '_', 'v', 'a', 'l', 'u', 'e'};
    db_->put(key_bytes, value_bytes);
    
    std::string request = 
        "DELETE /bitcask/delete?key=delete_key HTTP/1.1\r\n"
        "Host: localhost:8081\r\n"
        "\r\n";
    
    std::string response = send_http_request(request);
    auto [status_code, body] = parse_response(response);
    
    EXPECT_EQ(status_code, 200);
    EXPECT_TRUE(body.find("OK") != std::string::npos);
    
    // 验证数据是否被删除
    EXPECT_THROW(db_->get(key_bytes), KeyNotFoundError);
}

// 测试LIST KEYS请求
TEST_F(HttpServerTest, TestListKeysRequest) {
    // 插入一些数据
    db_->put(Bytes{'k', '1'}, Bytes{'v', '1'});
    db_->put(Bytes{'k', '2'}, Bytes{'v', '2'});
    db_->put(Bytes{'k', '3'}, Bytes{'v', '3'});
    
    std::string request = 
        "GET /bitcask/listkeys HTTP/1.1\r\n"
        "Host: localhost:8081\r\n"
        "\r\n";
    
    std::string response = send_http_request(request);
    auto [status_code, body] = parse_response(response);
    
    EXPECT_EQ(status_code, 200);
    EXPECT_TRUE(body.find("k1") != std::string::npos);
    EXPECT_TRUE(body.find("k2") != std::string::npos);
    EXPECT_TRUE(body.find("k3") != std::string::npos);
}

// 测试STAT请求
TEST_F(HttpServerTest, TestStatRequest) {
    // 插入一些数据
    db_->put(Bytes{'s', 't', 'a', 't', '1'}, Bytes{'v', 'a', 'l', 'u', 'e', '1'});
    db_->put(Bytes{'s', 't', 'a', 't', '2'}, Bytes{'v', 'a', 'l', 'u', 'e', '2'});
    
    std::string request = 
        "GET /bitcask/stat HTTP/1.1\r\n"
        "Host: localhost:8081\r\n"
        "\r\n";
    
    std::string response = send_http_request(request);
    auto [status_code, body] = parse_response(response);
    
    EXPECT_EQ(status_code, 200);
    EXPECT_TRUE(body.find("key_num") != std::string::npos);
    EXPECT_TRUE(body.find("data_file_num") != std::string::npos);
    EXPECT_TRUE(body.find("reclaimable_size") != std::string::npos);
    EXPECT_TRUE(body.find("disk_size") != std::string::npos);
}

// 测试不支持的HTTP方法
TEST_F(HttpServerTest, TestUnsupportedMethod) {
    std::string request = 
        "PATCH /bitcask/put HTTP/1.1\r\n"
        "Host: localhost:8081\r\n"
        "\r\n";
    
    std::string response = send_http_request(request);
    auto [status_code, body] = parse_response(response);
    
    EXPECT_EQ(status_code, 405);
    EXPECT_TRUE(body.find("not allowed") != std::string::npos);
}

// 测试不存在的路径
TEST_F(HttpServerTest, TestNonExistentPath) {
    std::string request = 
        "GET /invalid/path HTTP/1.1\r\n"
        "Host: localhost:8081\r\n"
        "\r\n";
    
    std::string response = send_http_request(request);
    auto [status_code, body] = parse_response(response);
    
    EXPECT_EQ(status_code, 404);
    EXPECT_TRUE(body.find("Not found") != std::string::npos);
}

// 测试无效的PUT请求（空body）
TEST_F(HttpServerTest, TestInvalidPutRequest) {
    std::string request = 
        "POST /bitcask/put HTTP/1.1\r\n"
        "Host: localhost:8081\r\n"
        "Content-Length: 0\r\n"
        "\r\n";
    
    std::string response = send_http_request(request);
    auto [status_code, body] = parse_response(response);
    
    EXPECT_EQ(status_code, 400);
    EXPECT_TRUE(body.find("Empty request body") != std::string::npos);
}

// 测试无效的GET请求（缺少key参数）
TEST_F(HttpServerTest, TestInvalidGetRequest) {
    std::string request = 
        "GET /bitcask/get HTTP/1.1\r\n"
        "Host: localhost:8081\r\n"
        "\r\n";
    
    std::string response = send_http_request(request);
    auto [status_code, body] = parse_response(response);
    
    EXPECT_EQ(status_code, 400);
    EXPECT_TRUE(body.find("Missing key parameter") != std::string::npos);
}

}  // namespace test
}  // namespace http
}  // namespace bitcask
