#ifndef RAFT_TYPES_H
#define RAFT_TYPES_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

// Raft 节点状态
enum class NodeState {
    FOLLOWER = 0,
    CANDIDATE = 1,
    LEADER = 2
};

// 日志条目
struct LogEntry {
    int64_t term;        // 该条目被创建时的任期号
    int64_t index;       // 该条目在日志中的索引
    std::string command; // 要执行的命令（序列化后的 KV 操作）
    
    LogEntry() : term(0), index(0) {}
    LogEntry(int64_t t, int64_t i, const std::string& cmd) 
        : term(t), index(i), command(cmd) {}
    
    bool operator==(const LogEntry& other) const {
        return term == other.term && index == other.index && command == other.command;
    }
    
    bool operator!=(const LogEntry& other) const {
        return !(*this == other);
    }
};

// RequestVote RPC 参数
struct RequestVoteArgs {
    int64_t term;         // Candidate 的任期号
    int64_t candidate_id; // Candidate 的 ID
    int64_t last_log_index; // Candidate 最后一条日志的索引
    int64_t last_log_term;  // Candidate 最后一条日志的任期号
};

// RequestVote RPC 响应
struct RequestVoteReply {
    int64_t term;        // 当前任期号，Candidate 可能需要更新自己的任期
    bool vote_granted;   // 是否获得了这张票
};

// AppendEntries RPC 参数
struct AppendEntriesArgs {
    int64_t term;         // Leader 的任期号
    int64_t leader_id;    // Leader 的 ID，用于 Followers 重定向
    int64_t prev_log_index; // 新日志条目之前的日志索引
    int64_t prev_log_term;  // 新日志条目之前的日志任期
    std::vector<LogEntry> entries; // 要存储的日志条目（为空表示心跳）
    int64_t leader_commit;  // Leader 的 commitIndex
};

// AppendEntries RPC 响应
struct AppendEntriesReply {
    int64_t term;       // 当前任期号，Leader 可能需要更新自己的任期
    bool success;       // 如果 Follower 包含匹配 prev_log_index 和 prev_log_term 的日志则为 true
    int64_t next_index; // Follower 期望下一次接收日志的索引（用于快速恢复）
};

// InstallSnapshot RPC 参数
struct InstallSnapshotArgs {
    int64_t term;              // Leader 的任期号
    int64_t leader_id;         // Leader 的 ID
    int64_t last_included_index; // snapshot 包含的最后一条日志的索引
    int64_t last_included_term;  // snapshot 包含的最后一条日志的任期号
    std::string data;          // snapshot 的数据
};

// InstallSnapshot RPC 响应
struct InstallSnapshotReply {
    int64_t term;        // 当前任期号
};

// Raft 节点配置
struct RaftConfig {
    int64_t node_id;           // 当前节点 ID
    std::vector<int64_t> peer_ids; // 集群中其他节点的 ID
    std::string storage_path;  // 持久化数据路径
    int election_timeout_ms;   // 选举超时时间（毫秒）
    int heartbeat_interval_ms; // 心跳间隔时间（毫秒）
    
    RaftConfig() 
        : node_id(0), election_timeout_ms(150), heartbeat_interval_ms(50) {}
};

// 客户端请求结果
struct ClientResult {
    bool success;      // 请求是否成功
    std::string value; // 返回值（用于 Get 请求）
    std::string error; // 错误信息
};

#endif // RAFT_TYPES_H