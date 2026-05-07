#ifndef ERROR_CODE_H
#define ERROR_CODE_H

#include <string>

enum class ErrorCode {
    SUCCESS = 0,
    
    // 通用错误
    UNKNOWN_ERROR = 1,
    INVALID_PARAMETER = 2,
    NOT_FOUND = 3,
    TIMEOUT = 4,
    CONNECTION_ERROR = 5,
    
    // Raft 相关错误
    NO_LEADER = 100,
    NOT_LEADER = 101,
    ELECTION_IN_PROGRESS = 102,
    LOG_MISMATCH = 103,
    TERM_OUT_OF_DATE = 104,
    
    // KV 存储相关错误
    KEY_NOT_FOUND = 200,
    KEY_EXISTS = 201,
    STORAGE_ERROR = 202,
    MEMORY_ERROR = 203,
    
    // RPC 相关错误
    RPC_TIMEOUT = 300,
    RPC_CONNECTION_REFUSED = 301,
    RPC_SERIALIZATION_ERROR = 302
};

inline std::string error_code_to_string(ErrorCode code) {
    switch (code) {
        case ErrorCode::SUCCESS: return "Success";
        case ErrorCode::UNKNOWN_ERROR: return "Unknown error";
        case ErrorCode::INVALID_PARAMETER: return "Invalid parameter";
        case ErrorCode::NOT_FOUND: return "Not found";
        case ErrorCode::TIMEOUT: return "Timeout";
        case ErrorCode::CONNECTION_ERROR: return "Connection error";
        case ErrorCode::NO_LEADER: return "No leader";
        case ErrorCode::NOT_LEADER: return "Not leader";
        case ErrorCode::ELECTION_IN_PROGRESS: return "Election in progress";
        case ErrorCode::LOG_MISMATCH: return "Log mismatch";
        case ErrorCode::TERM_OUT_OF_DATE: return "Term out of date";
        case ErrorCode::KEY_NOT_FOUND: return "Key not found";
        case ErrorCode::KEY_EXISTS: return "Key exists";
        case ErrorCode::STORAGE_ERROR: return "Storage error";
        case ErrorCode::MEMORY_ERROR: return "Memory error";
        case ErrorCode::RPC_TIMEOUT: return "RPC timeout";
        case ErrorCode::RPC_CONNECTION_REFUSED: return "RPC connection refused";
        case ErrorCode::RPC_SERIALIZATION_ERROR: return "RPC serialization error";
        default: return "Unknown error code";
    }
}

#endif // ERROR_CODE_H