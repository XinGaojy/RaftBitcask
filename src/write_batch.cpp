#include "bitcask/db.h"

namespace bitcask {

// WriteBatch实现
WriteBatch::WriteBatch(DB* db, const WriteBatchOptions& options)
    : db_(db), options_(options), seq_no_(db->seq_no_.load() + 1) {
    pending_writes_.reserve(options_.max_batch_num);
}

WriteBatch::~WriteBatch() {
    pending_writes_.clear();
}

void WriteBatch::put(const Bytes& key, const Bytes& value) {
    if (key.empty()) {
        throw KeyEmptyError();
    }
    
    if (pending_writes_.size() >= options_.max_batch_num) {
        throw BitcaskException("Batch size exceeds maximum limit");
    }
    
    LogRecord record;
    record.key = key;
    record.value = value;
    record.type = LogRecordType::NORMAL;
    pending_writes_.push_back(record);
}

void WriteBatch::remove(const Bytes& key) {
    if (key.empty()) {
        throw KeyEmptyError();
    }
    
    if (pending_writes_.size() >= options_.max_batch_num) {
        throw BitcaskException("Batch size exceeds maximum limit");
    }
    
    LogRecord record;
    record.key = key;
    record.type = LogRecordType::DELETED;
    pending_writes_.push_back(record);
}

void WriteBatch::commit() {
    if (pending_writes_.empty()) {
        return;
    }
    
    std::unique_lock<std::shared_mutex> lock(db_->mutex_);
    
    uint64_t current_seq_no = seq_no_.load();
    std::vector<LogRecordPos> positions;
    positions.reserve(pending_writes_.size());
    
    // 写入所有记录
    for (auto& record : pending_writes_) {
        // 创建带序列号的副本，避免修改原记录
        LogRecord record_with_seq = record;
        record_with_seq.key = DB::log_record_key_with_seq(record.key, current_seq_no);
        
        // 写入日志记录
        LogRecordPos pos = db_->append_log_record_internal(record_with_seq);
        positions.push_back(pos);
    }
    
    // 写入事务完成标记
    LogRecord txn_fin_record;
    txn_fin_record.key = DB::log_record_key_with_seq(Bytes(), current_seq_no);
    txn_fin_record.type = LogRecordType::TXN_FINISHED;
    db_->append_log_record_internal(txn_fin_record);
    
    // 同步数据（如果需要）
    if (options_.sync_writes) {
        db_->active_file_->sync();
    }
    
    // 更新内存索引
    for (size_t i = 0; i < pending_writes_.size(); ++i) {
        const auto& record = pending_writes_[i];
        const auto& pos = positions[i];
        
        // 使用原始key（pending_writes_中保存的是原始key）
        const Bytes& real_key = record.key;
        
        if (record.type == LogRecordType::NORMAL) {
            auto old_pos = db_->index_->put(real_key, pos);
            if (old_pos) {
                db_->reclaim_size_ += old_pos->size;
            }
        } else if (record.type == LogRecordType::DELETED) {
            auto [old_pos, ok] = db_->index_->remove(real_key);
            db_->reclaim_size_ += pos.size;
            if (old_pos) {
                db_->reclaim_size_ += old_pos->size;
            }
        }
    }
    
    // 更新序列号
    db_->seq_no_.store(current_seq_no + 1);
    
    // 清空待写入记录
    pending_writes_.clear();
}

std::unique_ptr<WriteBatch> DB::new_write_batch(const WriteBatchOptions& options) {
    return std::make_unique<WriteBatch>(this, options);
}

}  // namespace bitcask
