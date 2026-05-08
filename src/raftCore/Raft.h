#ifndef RAFT_H
#define RAFT_H

#include "../common/Log.h"
#include "../common/Timestamp.h"
#include "../common/Lock.h"
#include "../common/ErrorCode.h"
#include "RaftTypes.h"
#include <vector>
#include <string>
#include <memory>
#include <random>
#include <thread>
#include <atomic>

class Raft {
public:
    explicit Raft(const RaftConfig& config);
    ~Raft();

    // 启动 Raft 节点
    bool start();

    // 停止 Raft 节点
    void stop();

    // 获取当前节点状态
    NodeState get_state() const;

    // 获取当前任期号
    int64_t get_current_term() const;

    // 获取已提交日志的最高索引
    int64_t get_commit_index() const;

    // 获取应用到状态机的最高索引
    int64_t get_last_applied() const;

    // 获取当前 Leader ID（如果是 Follower）
    int64_t get_leader_id() const;

    // 客户端请求：提交日志条目
    ErrorCode submit_log(const std::string& command);

    // RequestVote RPC
    RequestVoteReply request_vote(const RequestVoteArgs& args);

    // AppendEntries RPC
    AppendEntriesReply append_entries(const AppendEntriesArgs& args);

    // InstallSnapshot RPC
    InstallSnapshotReply install_snapshot(const InstallSnapshotArgs& args);

private:
    // 选举相关方法
    void start_election();
    void reset_election_timer();
    void election_timeout();
    
    // 日志相关方法
    bool append_log_entry(const LogEntry& entry);
    bool log_matches(int64_t index, int64_t term);
    
    // 状态转换
    void become_follower(int64_t term);
    void become_candidate();
    void become_leader();
    
    // Leader 相关方法
    void send_heartbeat();
    void start_heartbeat_thread();
    void stop_heartbeat_thread();
    
    // 持久化方法
    void persist_state();
    void load_state();

    // 配置
    RaftConfig config_;

    // Raft 核心状态（需要持久化）
    mutable MutexLock state_mutex_;
    int64_t current_term_;
    int64_t voted_for_;
    std::vector<LogEntry> logs_;

    // 运行时状态（不需要持久化）
    NodeState state_;
    int64_t commit_index_;
    int64_t last_applied_;
    int64_t leader_id_;

    // Leader 专用状态
    std::vector<int64_t> next_index_;
    std::vector<int64_t> match_index_;

    // 选举定时器
    std::atomic<bool> running_;
    std::thread election_thread_;
    std::thread heartbeat_thread_;
    int64_t last_heartbeat_time_;

    // 随机数生成器（用于选举超时）
    std::mt19937 random_engine_;
    std::uniform_int_distribution<int> election_timeout_dist_;

    // 日志记录
    std::string node_name_;
};

#endif // RAFT_H