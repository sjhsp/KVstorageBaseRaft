#include "../src/common/Log.h"
#include "../src/common/Timestamp.h"
#include "../src/common/Lock.h"
#include "../src/raftCore/RaftTypes.h"
#include "../src/raftCore/Raft.h"
#include "../src/kvServer/KVServer.h"
#include "../src/skipList/SkipList.h"
#include <iostream>
#include <thread>
#include <chrono>

void test_log() {
    std::cout << "=== Testing Log ===" << std::endl;
    Log::get_instance()->init("raft.log", LogLevel::DEBUG);
    
    LOG_DEBUG << "This is a debug message";
    LOG_INFO << "This is an info message";
    LOG_WARN << "This is a warning message";
    LOG_ERROR << "This is an error message";
    
    std::cout << "Log test passed!" << std::endl;
}

void test_timestamp() {
    std::cout << "\n=== Testing Timestamp ===" << std::endl;
    
    Timestamp now = Timestamp::now();
    std::cout << "Current time: " << now.to_string() << std::endl;
    std::cout << "Microseconds: " << now.to_microseconds() << std::endl;
    std::cout << "Milliseconds: " << now.to_milliseconds() << std::endl;
    std::cout << "Seconds: " << now.to_seconds() << std::endl;
    
    Timestamp later = Timestamp::now();
    std::cout << "Later time: " << later.to_string() << std::endl;
    std::cout << "Later > Now: " << (later > now) << std::endl;
    
    std::cout << "Timestamp test passed!" << std::endl;
}

void test_lock() {
    std::cout << "\n=== Testing Lock ===" << std::endl;
    
    MutexLock mutex;
    AutoLock<MutexLock> lock(mutex);
    std::cout << "Mutex lock acquired" << std::endl;
    
    RWMutex rw_mutex;
    rw_mutex.lock_read();
    std::cout << "Read lock acquired" << std::endl;
    rw_mutex.unlock_read();
    
    rw_mutex.lock_write();
    std::cout << "Write lock acquired" << std::endl;
    rw_mutex.unlock_write();
    
    std::cout << "Lock test passed!" << std::endl;
}

void test_raft_types() {
    std::cout << "\n=== Testing Raft Types ===" << std::endl;
    
    LogEntry entry1(1, 5, "set key1 value1");
    LogEntry entry2(1, 5, "set key1 value1");
    LogEntry entry3(2, 6, "set key2 value2");
    
    std::cout << "entry1 == entry2: " << (entry1 == entry2) << std::endl;
    std::cout << "entry1 != entry3: " << (entry1 != entry3) << std::endl;
    
    RequestVoteArgs args;
    args.term = 5;
    args.candidate_id = 1;
    args.last_log_index = 10;
    args.last_log_term = 3;
    
    std::cout << "RequestVoteArgs term: " << args.term << std::endl;
    std::cout << "RequestVoteArgs candidate_id: " << args.candidate_id << std::endl;
    
    std::cout << "Raft Types test passed!" << std::endl;
}

void test_skip_list() {
    std::cout << "\n=== Testing SkipList ===" << std::endl;
    
    SkipList<int, std::string> skiplist(16);
    
    skiplist.insert(3, "three");
    skiplist.insert(1, "one");
    skiplist.insert(4, "four");
    skiplist.insert(2, "two");
    skiplist.insert(5, "five");
    
    std::string value;
    if (skiplist.search(3, value)) {
        std::cout << "Found key 3: " << value << std::endl;
    }
    
    if (skiplist.search(10, value)) {
        std::cout << "Found key 10: " << value << std::endl;
    } else {
        std::cout << "Key 10 not found" << std::endl;
    }
    
    skiplist.remove(3);
    
    if (skiplist.search(3, value)) {
        std::cout << "Found key 3 after remove: " << value << std::endl;
    } else {
        std::cout << "Key 3 not found after remove" << std::endl;
    }
    
    std::cout << "SkipList size: " << skiplist.size() << std::endl;
    
    std::cout << "SkipList test passed!" << std::endl;
}

void test_kv_server() {
    std::cout << "\n=== Testing KVServer ===" << std::endl;
    
    KVServer kv_server;
    
    kv_server.put("name", "Alice");
    kv_server.put("age", "25");
    kv_server.put("city", "Beijing");
    
    KvResult result1 = kv_server.get("name");
    std::cout << "GET name: " << (result1.success ? result1.value : "failed") << std::endl;
    
    kv_server.put("name", "Bob");
    KvResult result2 = kv_server.get("name");
    std::cout << "GET name (updated): " << (result2.success ? result2.value : "failed") << std::endl;
    
    kv_server.remove("age");
    KvResult result3 = kv_server.get("age");
    std::cout << "GET age (after delete): " << (result3.success ? result3.value : "not found") << std::endl;
    
    std::cout << "KVServer size: " << kv_server.size() << std::endl;
    
    std::cout << "KVServer test passed!" << std::endl;
}

void test_raft() {
    std::cout << "\n=== Testing Raft ===" << std::endl;
    
    RaftConfig config;
    config.node_id = 1;
    config.peer_ids = {2, 3};
    config.storage_path = "./data";
    config.election_timeout_ms = 150;
    config.heartbeat_interval_ms = 50;
    
    system("mkdir -p ./data");
    
    std::unique_ptr<Raft> raft = std::make_unique<Raft>(config);
    
    std::cout << "Raft node created, state: " << 
        (raft->get_state() == NodeState::FOLLOWER ? "Follower" :
         raft->get_state() == NodeState::CANDIDATE ? "Candidate" : "Leader") << std::endl;
    
    std::cout << "Current term: " << raft->get_current_term() << std::endl;
    
    raft->start();
    
    std::cout << "Raft node started" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    std::cout << "After election timeout, state: " << 
        (raft->get_state() == NodeState::FOLLOWER ? "Follower" :
         raft->get_state() == NodeState::CANDIDATE ? "Candidate" : "Leader") << std::endl;
    
    raft->stop();
    
    std::cout << "Raft test passed!" << std::endl;
}

int main() {
    std::cout << "=== Starting KVstorageBaseRaft Tests ===\n" << std::endl;
    
    test_log();
    test_timestamp();
    test_lock();
    test_raft_types();
    test_skip_list();
    test_kv_server();
    test_raft();
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
    return 0;
}