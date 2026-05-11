#include "KVServer.h"
#include "../common/Log.h"
#include <sstream>
#include <algorithm>

KVServer::KVServer() : last_applied_index_(0) {
    store_ = new SkipList<std::string, std::string>();
    LOG_INFO << "KVServer initialized";
}

KVServer::~KVServer() {
    delete store_;
}

KvResult KVServer::get(const std::string& key) {
    AutoLock<MutexLock> lock(mutex_);
    
    std::string value;
    if (store_->search(key, value)) {
        return KvResult(true, value);
    }
    
    return KvResult(false, "", "Key not found");
}

KvResult KVServer::put(const std::string& key, const std::string& value) {
    AutoLock<MutexLock> lock(mutex_);
    
    store_->insert(key, value);
    return KvResult(true);
}

KvResult KVServer::remove(const std::string& key) {
    AutoLock<MutexLock> lock(mutex_);
    
    if (store_->remove(key)) {
        return KvResult(true);
    }
    
    return KvResult(false, "", "Key not found");
}

KvResult KVServer::execute_command(const KvCommand& cmd) {
    switch (cmd.type) {
        case KvCommand::Type::GET:
            return get(cmd.key);
        case KvCommand::Type::PUT:
            return put(cmd.key, cmd.value);
        case KvCommand::Type::DELETE:
            return remove(cmd.key);
        default:
            return KvResult(false, "", "Unknown command");
    }
}

void KVServer::apply_log(const std::string& command) {
    KvCommand cmd = KvCommand::deserialize(command);
    execute_command(cmd);
    LOG_DEBUG << "Applied command: " << command;
}

std::string KVServer::snapshot() const {
    AutoLock<MutexLock> lock(const_cast<MutexLock&>(mutex_));
    
    std::ostringstream oss;
    bool first = true;
    
    // 遍历跳表，序列化所有 key-value 对
    store_->traverse([&](const std::string& key, const std::string& value) {
        if (!first) {
            oss << ";";
        }
        first = false;
        
        // 简单的序列化格式：key=value
        oss << escape(key) << "=" << escape(value);
    });
    
    return oss.str();
}

bool KVServer::restore(const std::string& data) {
    AutoLock<MutexLock> lock(mutex_);
    
    store_->clear();
    
    if (data.empty()) {
        LOG_INFO << "KVServer restored from empty snapshot";
        return true;
    }
    
    // 解析快照数据
    std::string token;
    std::istringstream iss(data);
    
    while (std::getline(iss, token, ';')) {
        size_t eq_pos = token.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = unescape(token.substr(0, eq_pos));
            std::string value = unescape(token.substr(eq_pos + 1));
            store_->insert(key, value);
        }
    }
    
    LOG_INFO << "KVServer restored from snapshot, " << store_->size() << " items";
    return true;
}

int KVServer::size() const {
    return store_->size();
}

void KVServer::clear() {
    AutoLock<MutexLock> lock(mutex_);
    store_->clear();
}

std::string KVServer::escape(const std::string& s) const {
    std::string result;
    for (char c : s) {
        if (c == '=' || c == ';' || c == '\\') {
            result += '\\';
        }
        result += c;
    }
    return result;
}

std::string KVServer::unescape(const std::string& s) const {
    std::string result;
    bool escaped = false;
    for (char c : s) {
        if (escaped) {
            result += c;
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else {
            result += c;
        }
    }
    return result;
}