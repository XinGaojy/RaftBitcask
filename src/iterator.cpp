#include "bitcask/db.h"

namespace bitcask {

// DBIterator实现
DBIterator::DBIterator(DB* db, const IteratorOptions& options)
    : db_(db), options_(options) {
    index_iter_ = db_->index_->iterator(options_.reverse);
}

DBIterator::~DBIterator() {
    close();
}

void DBIterator::rewind() {
    if (!index_iter_) {
        return;
    }
    
    index_iter_->rewind();
    
    // 如果有前缀过滤，移动到第一个匹配的位置
    if (!options_.prefix.empty()) {
        index_iter_->seek(options_.prefix);
    }
}

void DBIterator::seek(const Bytes& key) {
    if (!index_iter_) {
        return;
    }
    index_iter_->seek(key);
}

void DBIterator::next() {
    if (!index_iter_) {
        return;
    }
    index_iter_->next();
}

bool DBIterator::valid() const {
    if (!index_iter_ || !index_iter_->valid()) {
        return false;
    }
    
    // 检查前缀过滤
    if (!options_.prefix.empty()) {
        Bytes current_key = index_iter_->key();
        if (current_key.size() < options_.prefix.size()) {
            return false;
        }
        
        // 检查是否以指定前缀开头
        for (size_t i = 0; i < options_.prefix.size(); ++i) {
            if (current_key[i] != options_.prefix[i]) {
                return false;
            }
        }
    }
    
    return true;
}

Bytes DBIterator::key() const {
    if (!valid()) {
        throw BitcaskException("Iterator is not valid");
    }
    return index_iter_->key();
}

Bytes DBIterator::value() const {
    if (!valid()) {
        throw BitcaskException("Iterator is not valid");
    }
    
    LogRecordPos pos = index_iter_->value();
    return db_->get_value_by_position(pos);
}

void DBIterator::close() {
    if (index_iter_) {
        index_iter_->close();
        index_iter_.reset();
    }
}

std::unique_ptr<DBIterator> DB::iterator(const IteratorOptions& options) {
    return std::make_unique<DBIterator>(this, options);
}

}  // namespace bitcask
