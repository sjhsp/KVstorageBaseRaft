#include "Raft.h"
#include <fstream>
#include <sstream>
#include <algorithm>

Raft::Raft(const RaftConfig& config) 
    : config_(config),
      current_term_(0),
      voted_for_(-1),
      state_(NodeState::FOLLOWER),
      commit_index_(0),
      last_applied_(0),
      leader_id_(-1),
      running_(false),
      last_heartbeat_time_(0),
      random_engine_(std::random_device{}()),
      election_timeout_dist_(config.election_timeout_ms, 2 * config.election_timeout_ms) {
    
    node_name_ = "Node_" + std::to_string(config.node_id);
    
    logs_.emplace_back(0, 0, "");
    
    next_index_.resize(config.peer_ids.size(), 1);
    match_index_.resize(config.peer_ids.size(), 0);
    
    LOG_INFO << node_name_ << " initialized";
}

Raft::~Raft() {
    stop();
}

bool Raft::start() {
    if (running_.exchange(true)) {
        LOG_WARN << node_name_ << " already running";
        return false;
    }

    load_state();
    
    election_thread_ = std::thread([this]() {
        while (running_) {
            int timeout_ms = election_timeout_dist_(random_engine_);
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
            
            if (!running_) break;
            
            AutoLock<MutexLock> lock(state_mutex_);
            int64_t now = Timestamp::now().to_milliseconds();
            if (now - last_heartbeat_time_ > config_.election_timeout_ms) {
                election_timeout();
            }
        }
    });

    LOG_INFO << node_name_ << " started as " << 
             (state_ == NodeState::FOLLOWER ? "Follower" : 
              state_ == NodeState::CANDIDATE ? "Candidate" : "Leader");
    
    return true;
}

void Raft::stop() {
    running_.store(false);
    
    if (election_thread_.joinable()) {
        election_thread_.join();
    }
    
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }
    
    persist_state();
    
    LOG_INFO << node_name_ << " stopped";
}

NodeState Raft::get_state() const {
    AutoLock<MutexLock> lock(const_cast<MutexLock&>(state_mutex_));
    return state_;
}

int64_t Raft::get_current_term() const {
    AutoLock<MutexLock> lock(const_cast<MutexLock&>(state_mutex_));
    return current_term_;
}

int64_t Raft::get_commit_index() const {
    AutoLock<MutexLock> lock(const_cast<MutexLock&>(state_mutex_));
    return commit_index_;
}

int64_t Raft::get_last_applied() const {
    AutoLock<MutexLock> lock(const_cast<MutexLock&>(state_mutex_));
    return last_applied_;
}

int64_t Raft::get_leader_id() const {
    AutoLock<MutexLock> lock(const_cast<MutexLock&>(state_mutex_));
    return leader_id_;
}

void Raft::become_follower(int64_t term) {
    state_ = NodeState::FOLLOWER;
    current_term_ = term;
    voted_for_ = -1;
    leader_id_ = -1;
    reset_election_timer();
    
    persist_state();
    LOG_INFO << node_name_ << " became Follower, term=" << term;
}

void Raft::become_candidate() {
    state_ = NodeState::CANDIDATE;
    current_term_++;
    voted_for_ = config_.node_id;
    leader_id_ = -1;
    reset_election_timer();
    
    persist_state();
    LOG_INFO << node_name_ << " became Candidate, term=" << current_term_;
    
    start_election();
}

void Raft::become_leader() {
    state_ = NodeState::LEADER;
    leader_id_ = config_.node_id;
    
    for (size_t i = 0; i < config_.peer_ids.size(); i++) {
        next_index_[i] = logs_.size();
        match_index_[i] = 0;
    }
    
    reset_election_timer();
    
    LOG_INFO << node_name_ << " became Leader, term=" << current_term_;
    
    start_heartbeat_thread();
}

void Raft::reset_election_timer() {
    last_heartbeat_time_ = Timestamp::now().to_milliseconds();
}

void Raft::election_timeout() {
    if (state_ == NodeState::LEADER) {
        return;
    }
    
    LOG_INFO << node_name_ << " election timeout, term=" << current_term_;
    become_candidate();
}

void Raft::start_election() {
    LOG_INFO << node_name_ << " starting election, term=" << current_term_;
    
    int64_t votes = 1;
    
    for (size_t i = 0; i < config_.peer_ids.size(); i++) {
        int64_t peer_id = config_.peer_ids[i];
        
        RequestVoteArgs args;
        args.term = current_term_;
        args.candidate_id = config_.node_id;
        args.last_log_index = logs_.size() - 1;
        args.last_log_term = logs_.back().term;
        
        LOG_DEBUG << node_name_ << " sending RequestVote to " << peer_id;
    }
}

void Raft::start_heartbeat_thread() {
    heartbeat_thread_ = std::thread([this]() {
        while (running_ && state_ == NodeState::LEADER) {
            send_heartbeat();
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config_.heartbeat_interval_ms));
        }
    });
}

void Raft::stop_heartbeat_thread() {
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }
}

void Raft::send_heartbeat() {
    if (state_ != NodeState::LEADER) {
        return;
    }
    
    for (size_t i = 0; i < config_.peer_ids.size(); i++) {
        int64_t peer_id = config_.peer_ids[i];
        
        AppendEntriesArgs args;
        args.term = current_term_;
        args.leader_id = config_.node_id;
        args.prev_log_index = next_index_[i] - 1;
        args.prev_log_term = (args.prev_log_index > 0) ? logs_[args.prev_log_index].term : 0;
        args.leader_commit = commit_index_;
        
        LOG_DEBUG << node_name_ << " sending heartbeat to " << peer_id;
    }
}

RequestVoteReply Raft::request_vote(const RequestVoteArgs& args) {
    AutoLock<MutexLock> lock(state_mutex_);
    
    RequestVoteReply reply;
    reply.term = current_term_;
    reply.vote_granted = false;
    
    if (args.term > current_term_) {
        become_follower(args.term);
        reply.term = args.term;
    }
    
    if (args.term < current_term_) {
        LOG_DEBUG << node_name_ << " reject vote: term too old";
        return reply;
    }
    
    if (voted_for_ != -1 && voted_for_ != args.candidate_id) {
        LOG_DEBUG << node_name_ << " reject vote: already voted for " << voted_for_;
        return reply;
    }
    
    int64_t last_log_index = logs_.size() - 1;
    int64_t last_log_term = logs_.back().term;
    
    if (args.last_log_term < last_log_term ||
        (args.last_log_term == last_log_term && args.last_log_index < last_log_index)) {
        LOG_DEBUG << node_name_ << " reject vote: log not up-to-date";
        return reply;
    }
    
    voted_for_ = args.candidate_id;
    reply.vote_granted = true;
    reset_election_timer();
    persist_state();
    
    LOG_INFO << node_name_ << " voted for " << args.candidate_id << ", term=" << args.term;
    
    return reply;
}

AppendEntriesReply Raft::append_entries(const AppendEntriesArgs& args) {
    AutoLock<MutexLock> lock(state_mutex_);
    
    AppendEntriesReply reply;
    reply.term = current_term_;
    reply.success = false;
    reply.next_index = 1;
    
    if (args.term > current_term_) {
        become_follower(args.term);
        reply.term = args.term;
    }
    
    if (args.term < current_term_) {
        LOG_DEBUG << node_name_ << " reject AppendEntries: term too old";
        return reply;
    }
    
    leader_id_ = args.leader_id;
    reset_election_timer();
    
    if (args.prev_log_index > 0) {
        if (args.prev_log_index >= static_cast<int64_t>(logs_.size())) {
            LOG_DEBUG << node_name_ << " reject AppendEntries: prev index out of range";
            reply.next_index = logs_.size();
            return reply;
        }
        
        if (logs_[args.prev_log_index].term != args.prev_log_term) {
            LOG_DEBUG << node_name_ << " reject AppendEntries: term mismatch";
            int64_t conflict_term = logs_[args.prev_log_index].term;
            int64_t conflict_index = args.prev_log_index;
            while (conflict_index > 0 && logs_[conflict_index].term == conflict_term) {
                conflict_index--;
            }
            reply.next_index = conflict_index + 1;
            return reply;
        }
    }
    
    for (const auto& entry : args.entries) {
        if (entry.index < static_cast<int64_t>(logs_.size()) && 
            logs_[entry.index].term == entry.term) {
            continue;
        }
        
        if (entry.index < static_cast<int64_t>(logs_.size())) {
            logs_.resize(entry.index);
        }
        
        logs_.push_back(entry);
        LOG_DEBUG << node_name_ << " appended log entry: index=" << entry.index 
                  << ", term=" << entry.term;
    }
    
    if (args.leader_commit > commit_index_) {
        commit_index_ = std::min(args.leader_commit, static_cast<int64_t>(logs_.size() - 1));
        LOG_DEBUG << node_name_ << " commit_index updated to " << commit_index_;
    }
    
    reply.success = true;
    reply.next_index = logs_.size();
    persist_state();
    
    return reply;
}

InstallSnapshotReply Raft::install_snapshot(const InstallSnapshotArgs& args) {
    AutoLock<MutexLock> lock(state_mutex_);
    
    InstallSnapshotReply reply;
    reply.term = current_term_;
    
    if (args.term > current_term_) {
        become_follower(args.term);
        reply.term = args.term;
    }
    
    if (args.term < current_term_) {
        return reply;
    }
    
    leader_id_ = args.leader_id;
    reset_election_timer();
    
    logs_.resize(args.last_included_index + 1);
    logs_[args.last_included_index].term = args.last_included_term;
    
    commit_index_ = args.last_included_index;
    last_applied_ = args.last_included_index;
    
    persist_state();
    
    LOG_INFO << node_name_ << " installed snapshot, last_included_index=" << args.last_included_index;
    
    return reply;
}

ErrorCode Raft::submit_log(const std::string& command) {
    AutoLock<MutexLock> lock(state_mutex_);
    
    if (state_ != NodeState::LEADER) {
        return ErrorCode::NOT_LEADER;
    }
    
    LogEntry entry(current_term_, logs_.size(), command);
    logs_.push_back(entry);
    
    persist_state();
    
    LOG_DEBUG << node_name_ << " submitted log entry: index=" << entry.index 
              << ", term=" << entry.term;
    
    return ErrorCode::SUCCESS;
}

void Raft::persist_state() {
    std::string filepath = config_.storage_path + "/raft_state_" + 
                          std::to_string(config_.node_id) + ".dat";
    
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR << node_name_ << " failed to persist state";
        return;
    }
    
    file.write(reinterpret_cast<const char*>(&current_term_), sizeof(current_term_));
    file.write(reinterpret_cast<const char*>(&voted_for_), sizeof(voted_for_));
    
    int64_t log_count = logs_.size();
    file.write(reinterpret_cast<const char*>(&log_count), sizeof(log_count));
    
    for (const auto& entry : logs_) {
        file.write(reinterpret_cast<const char*>(&entry.term), sizeof(entry.term));
        file.write(reinterpret_cast<const char*>(&entry.index), sizeof(entry.index));
        int64_t cmd_len = entry.command.size();
        file.write(reinterpret_cast<const char*>(&cmd_len), sizeof(cmd_len));
        file.write(entry.command.c_str(), cmd_len);
    }
    
    LOG_DEBUG << node_name_ << " state persisted";
}

void Raft::load_state() {
    std::string filepath = config_.storage_path + "/raft_state_" + 
                          std::to_string(config_.node_id) + ".dat";
    
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        LOG_INFO << node_name_ << " no existing state file, starting fresh";
        return;
    }
    
    file.read(reinterpret_cast<char*>(&current_term_), sizeof(current_term_));
    file.read(reinterpret_cast<char*>(&voted_for_), sizeof(voted_for_));
    
    int64_t log_count;
    file.read(reinterpret_cast<char*>(&log_count), sizeof(log_count));
    
    logs_.resize(log_count);
    for (auto& entry : logs_) {
        file.read(reinterpret_cast<char*>(&entry.term), sizeof(entry.term));
        file.read(reinterpret_cast<char*>(&entry.index), sizeof(entry.index));
        int64_t cmd_len;
        file.read(reinterpret_cast<char*>(&cmd_len), sizeof(cmd_len));
        entry.command.resize(cmd_len);
        file.read(&entry.command[0], cmd_len);
    }
    
    LOG_INFO << node_name_ << " state loaded, term=" << current_term_ 
             << ", log_count=" << logs_.size();
}