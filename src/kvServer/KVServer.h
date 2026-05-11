#ifndef KV_SERVER_H
#define KV_SERVER_H

#include "../skipList/SkipList.h"
#include "../common/Lock.h"
#include "../common/ErrorCode.h"
#include <string>
#include <memory>
#include <functional>
#include <vector>

struct KvCommand {
    enum class Type : int {
        GET = 0,
        PUT = 1,
        DELETE = 2
    };
    
    Type type;
    std::string key;
    std::string value;
    
    std::string serialize() const {
        std::string result;
        switch (type) {
            case Type::GET: result = "GET"; break;
            case Type::PUT: result = "PUT"; break;
            case Type::DELETE: result = "DELETE"; break;
        }
        result += " " + key;
        if (type == Type::PUT) {
            result += " " + value;
        }
        return result;
    }
    
    static KvCommand deserialize(const std::string& str) {
        KvCommand cmd;
        size_t space1 = str.find(' ');
        std::string type_str = str.substr(0, space1);
        
        if (type_str == "GET") {
            cmd.type = Type::GET;
            cmd.key = str.substr(space1 + 1);
        } else if (type_str == "PUT") {
            size_t space2 = str.find(' ', space1 + 1);
            cmd.type = Type::PUT;
            cmd.key = str.substr(space1 + 1, space2 - space1 - 1);
            cmd.value = str.substr(space2 + 1);
        } else if (type_str == "DELETE") {
            cmd.type = Type::DELETE;
            cmd.key = str.substr(space1 + 1);
        }
        
        return cmd;
    }
};

struct KvResult {
    bool success;
    std::string value;
    std::string error;
    
    KvResult() : success(false) {}
    KvResult(bool s, const std::string& v = "", const std::string& e = "")
        : success(s), value(v), error(e) {}
};

class KVServer {
public:
    KVServer();
    ~KVServer();
    
    KvResult get(const std::string& key);
    KvResult put(const std::string& key, const std::string& value);
    KvResult remove(const std::string& key);
    
    KvResult execute_command(const KvCommand& cmd);
    
    void apply_log(const std::string& command);
    
    std::string snapshot() const;
    bool restore(const std::string& data);
    
    int size() const;
    void clear();

private:
    SkipList<std::string, std::string>* store_;
    MutexLock mutex_;
    int64_t last_applied_index_;
    
    // 辅助函数：转义和反转义特殊字符
    std::string escape(const std::string& s) const;
    std::string unescape(const std::string& s) const;
};

#endif // KV_SERVER_H