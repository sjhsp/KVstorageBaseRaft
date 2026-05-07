#include "../src/common/Log.h"
#include "../src/common/Timestamp.h"
#include "../src/raftCore/RaftTypes.h"
#include <iostream>

int main() {
    // 初始化日志系统
    Log::get_instance()->init("raft_example.log", LogLevel::INFO);
    
    LOG_INFO("KVstorageBaseRaft Example Started");
    
    // 创建一些测试数据
    LogEntry entry(1, 1, "set name Raft");
    LOG_INFO("Created log entry: term=" + std::to_string(entry.term) + 
             ", index=" + std::to_string(entry.index) +
             ", command=" + entry.command);
    
    // 创建请求投票参数
    RequestVoteArgs args;
    args.term = 1;
    args.candidate_id = 1;
    args.last_log_index = 1;
    args.last_log_term = 1;
    
    LOG_INFO("RequestVoteArgs created: term=" + std::to_string(args.term) +
             ", candidate_id=" + std::to_string(args.candidate_id));
    
    LOG_INFO("KVstorageBaseRaft Example Completed");
    
    return 0;
}