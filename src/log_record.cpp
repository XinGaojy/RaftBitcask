#include "bitcask/log_record.h"
#include <cstring>
#include <algorithm>

// CRC32 实现选择
#ifdef USE_ZLIB_CRC32
#include <zlib.h>
namespace crc32c {
    uint32_t Crc32c(const void* data, size_t length) {
        return crc32(0, static_cast<const Bytef*>(data), length);
    }
    uint32_t Extend(uint32_t crc, const void* data, size_t length) {
        return crc32(crc, static_cast<const Bytef*>(data), length);
    }
}
#else
// 简单的CRC32实现，用于替代外部库
namespace crc32c {
    // CRC32表
    static const uint32_t crc32_table[256] = {
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
        0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
        0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
        0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
        0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
        0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
        0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
        0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
        0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
        0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
        0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
        0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
        0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
        0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
        0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
        0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
        0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
        0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
        0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
        0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
        0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
        0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
        0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
        0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
        0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
        0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
        0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
        0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
        0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
        0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
        0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
        0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
        0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
        0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
        0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
        0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
        0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
        0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
        0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
        0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
        0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
        0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
        0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
    };
    
    uint32_t Crc32c(const void* data, size_t length) {
        if (length == 0) return 0;
        
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        uint32_t crc = 0xFFFFFFFF;
        
        // 简化的CRC32计算，更保守的实现
        for (size_t i = 0; i < length; ++i) {
            uint8_t byte = bytes[i];
            crc = (crc >> 8) ^ crc32_table[(crc ^ byte) & 0xFF];
        }
        
        return crc ^ 0xFFFFFFFF;
    }
    
    uint32_t Extend(uint32_t crc, const void* data, size_t length) {
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        crc ^= 0xFFFFFFFF;
        
        for (size_t i = 0; i < length; ++i) {
            crc = (crc >> 8) ^ crc32_table[(crc ^ bytes[i]) & 0xFF];
        }
        
        return crc ^ 0xFFFFFFFF;
    }
}
#endif

namespace bitcask {

// 声明编码和解码函数
size_t encode_varint(uint64_t value, uint8_t* buffer);
std::pair<uint64_t, size_t> decode_varint(const uint8_t* buffer, size_t buffer_size);

// 编码变长整数(类似protobuf的varint编码)
size_t encode_varint(uint64_t value, uint8_t* buffer) {
    size_t pos = 0;
    while (value >= 0x80) {
        buffer[pos++] = (value & 0xFF) | 0x80;
        value >>= 7;
    }
    buffer[pos++] = value & 0xFF;
    return pos;
}

// 解码变长整数
std::pair<uint64_t, size_t> decode_varint(const uint8_t* buffer, size_t buffer_size) {
    if (buffer_size == 0) {
        throw BitcaskException("Empty buffer for varint");
    }
    
    uint64_t result = 0;
    size_t pos = 0;
    int shift = 0;
    
    while (pos < buffer_size) {
        uint8_t byte = buffer[pos++];
        result |= (uint64_t(byte & 0x7F) << shift);
        if ((byte & 0x80) == 0) {
            return {result, pos};
        }
        shift += 7;
        if (shift >= 64) {
            // varint太长，这是明显错误
            throw BitcaskException("Varint too long");
        }
    }
    
    // 如果到达缓冲区末尾但没有找到结束标记，这是不完整的varint
    throw BitcaskException("Incomplete varint");
}

// LogRecordPos实现
Bytes LogRecordPos::encode() const {
    Bytes result;
    result.reserve(15); // 最大可能大小
    
    uint8_t buffer[10];
    size_t len;
    
    // 编码文件ID
    len = encode_varint(fid, buffer);
    result.insert(result.end(), buffer, buffer + len);
    
    // 编码偏移量
    len = encode_varint(offset, buffer);
    result.insert(result.end(), buffer, buffer + len);
    
    // 编码大小
    len = encode_varint(size, buffer);
    result.insert(result.end(), buffer, buffer + len);
    
    return result;
}

LogRecordPos LogRecordPos::decode(const Bytes& data) {
    const uint8_t* ptr = data.data();
    size_t remaining = data.size();
    LogRecordPos pos;
    
    // 解码文件ID
    auto [fid_val, fid_len] = decode_varint(ptr, remaining);
    pos.fid = static_cast<uint32_t>(fid_val);
    ptr += fid_len;
    remaining -= fid_len;
    
    // 解码偏移量
    auto [offset_val, offset_len] = decode_varint(ptr, remaining);
    pos.offset = offset_val;
    ptr += offset_len;
    remaining -= offset_len;
    
    // 解码大小
    auto [size_val, size_len] = decode_varint(ptr, remaining);
    pos.size = static_cast<uint32_t>(size_val);
    
    return pos;
}

// LogRecord实现
std::pair<Bytes, size_t> LogRecord::encode() const {
    size_t total_size = encoded_size();
    Bytes result;
    result.reserve(total_size);
    
    // 预留CRC位置
    result.resize(4);
    
    // 写入类型
    result.push_back(static_cast<uint8_t>(type));
    
    // 写入key长度
    uint8_t buffer[10];
    size_t len = encode_varint(key.size(), buffer);
    result.insert(result.end(), buffer, buffer + len);
    
    // 写入value长度
    len = encode_varint(value.size(), buffer);
    result.insert(result.end(), buffer, buffer + len);
    
    // 写入key和value
    result.insert(result.end(), key.begin(), key.end());
    result.insert(result.end(), value.begin(), value.end());
    
    // 计算CRC（跳过前4个字节的CRC字段）
    uint32_t crc = crc32c::Crc32c(result.data() + 4, result.size() - 4);
    
    // 写入CRC (小端序)
    result[0] = crc & 0xFF;
    result[1] = (crc >> 8) & 0xFF;
    result[2] = (crc >> 16) & 0xFF;
    result[3] = (crc >> 24) & 0xFF;
    
    return {result, result.size()};
}

uint32_t LogRecord::get_crc() const {
    // 创建临时缓冲区来计算CRC
    Bytes temp;
    temp.reserve(encoded_size() - 4); // 不包含CRC字段
    
    // 写入类型
    temp.push_back(static_cast<uint8_t>(type));
    
    // 写入key长度
    uint8_t buffer[10];
    size_t len = encode_varint(key.size(), buffer);
    temp.insert(temp.end(), buffer, buffer + len);
    
    // 写入value长度
    len = encode_varint(value.size(), buffer);
    temp.insert(temp.end(), buffer, buffer + len);
    
    // 写入key和value
    temp.insert(temp.end(), key.begin(), key.end());
    temp.insert(temp.end(), value.begin(), value.end());
    
    return crc32c::Crc32c(temp.data(), temp.size());
}

size_t LogRecord::encoded_size() const {
    size_t size = 4; // CRC
    size += 1; // type
    
    // key长度的变长编码大小
    uint64_t key_len = key.size();
    while (key_len >= 0x80) {
        size++;
        key_len >>= 7;
    }
    size++; // 最后一个字节
    
    // value长度的变长编码大小
    uint64_t value_len = value.size();
    while (value_len >= 0x80) {
        size++;
        value_len >>= 7;
    }
    size++; // 最后一个字节
    
    size += key.size() + value.size();
    return size;
}

// 解码日志记录头部
std::pair<LogRecordHeader, size_t> decode_log_record_header(const Bytes& data) {
    if (data.size() < 5) { // 至少需要4字节CRC + 1字节type
        throw BitcaskException("Invalid header data");
    }
    
    LogRecordHeader header;
    size_t pos = 0;
    
    // 读取CRC（小端序）
    header.crc = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    pos += 4;
    
    // 读取类型
    header.type = static_cast<LogRecordType>(data[pos++]);
    
    // 读取key长度
    auto [key_size, key_len] = decode_varint(data.data() + pos, data.size() - pos);
    header.key_size = static_cast<uint32_t>(key_size);
    pos += key_len;
    
    // 读取value长度
    auto [value_size, value_len] = decode_varint(data.data() + pos, data.size() - pos);
    header.value_size = static_cast<uint32_t>(value_size);
    pos += value_len;
    
    return {header, pos};
}

// 计算日志记录CRC值
uint32_t get_log_record_crc(const LogRecord& record, const Bytes& header) {
    // 计算不包含CRC的头部 + key + value的CRC
    uint32_t crc = crc32c::Crc32c(header.data(), header.size());
    crc = crc32c::Extend(crc, record.key.data(), record.key.size());
    crc = crc32c::Extend(crc, record.value.data(), record.value.size());
    return crc;
}

}  // namespace bitcask

