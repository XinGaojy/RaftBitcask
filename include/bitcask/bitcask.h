#pragma once

// Bitcask C++ 主头文件
// 包含所有公共接口

#include "common.h"
#include "options.h"
#include "log_record.h"
#include "io_manager.h"
#include "data_file.h"
#include "index.h"
#include "db.h"
#include "utils.h"

namespace bitcask {

// 版本信息
static const std::string VERSION = "1.0.0";

// 便利函数：打开数据库
inline std::unique_ptr<DB> open(const std::string& dir_path) {
    Options options = Options::default_options();
    options.dir_path = dir_path;
    return DB::open(options);
}

// 便利函数：打开数据库（带选项）
inline std::unique_ptr<DB> open(const Options& options) {
    return DB::open(options);
}

// 字符串到字节数组转换
inline Bytes string_to_bytes(const std::string& str) {
    return Bytes(str.begin(), str.end());
}

// 字节数组到字符串转换
inline std::string bytes_to_string(const Bytes& bytes) {
    return std::string(bytes.begin(), bytes.end());
}

}  // namespace bitcask
