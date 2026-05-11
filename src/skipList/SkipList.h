#ifndef SKIP_LIST_H
#define SKIP_LIST_H

#include <string>
#include <memory>
#include <random>
#include <mutex>
#include <atomic>
#include <vector>

template <typename Key, typename Value>
class SkipList {
public:
    struct Node {
        Key key;
        Value value;
        int level;
        Node** forward;
        
        Node(const Key& k, const Value& v, int lvl) 
            : key(k), value(v), level(lvl) {
            forward = new Node*[level + 1]();
        }
        
        ~Node() {
            delete[] forward;
        }
    };
    
    explicit SkipList(int max_level = 16);
    ~SkipList();
    
    bool insert(const Key& key, const Value& value);
    bool search(const Key& key, Value& value);
    bool remove(const Key& key);
    
    int size() const { return item_count_.load(); }
    void clear();
    
    // 遍历所有节点，调用回调函数
    template <typename Func>
    void traverse(Func func) const {
        std::lock_guard<std::mutex> lock(mutex_);
        Node* current = header_->forward[0];
        while (current != nullptr) {
            func(current->key, current->value);
            current = current->forward[0];
        }
    }
    
private:
    int random_level();
    
    int max_level_;
    int level_;
    Node* header_;
    std::atomic<int> item_count_;
    
    std::mt19937 random_engine_;
    std::uniform_real_distribution<double> distribution_;
    
    mutable std::mutex mutex_;
};

template <typename Key, typename Value>
SkipList<Key, Value>::SkipList(int max_level) 
    : max_level_(max_level),
      level_(0),
      item_count_(0),
      distribution_(0.0, 1.0) {
    
    header_ = new Node(Key(), Value(), max_level_);
}

template <typename Key, typename Value>
SkipList<Key, Value>::~SkipList() {
    clear();
    delete header_;
}

template <typename Key, typename Value>
int SkipList<Key, Value>::random_level() {
    int lvl = 1;
    while (distribution_(random_engine_) < 0.5 && lvl < max_level_) {
        lvl++;
    }
    return lvl;
}

template <typename Key, typename Value>
bool SkipList<Key, Value>::insert(const Key& key, const Value& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Node*> update(max_level_ + 1);
    Node* current = header_;
    
    for (int i = level_; i >= 0; i--) {
        while (current->forward[i] != nullptr && 
               current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    
    current = current->forward[0];
    
    if (current != nullptr && current->key == key) {
        current->value = value;
        return true;
    }
    
    int new_level = random_level();
    
    if (new_level > level_) {
        for (int i = level_ + 1; i <= new_level; i++) {
            update[i] = header_;
        }
        level_ = new_level;
    }
    
    Node* new_node = new Node(key, value, new_level);
    
    for (int i = 0; i <= new_level; i++) {
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }
    
    item_count_++;
    return true;
}

template <typename Key, typename Value>
bool SkipList<Key, Value>::search(const Key& key, Value& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Node* current = header_;
    
    for (int i = level_; i >= 0; i--) {
        while (current->forward[i] != nullptr && 
               current->forward[i]->key < key) {
            current = current->forward[i];
        }
    }
    
    current = current->forward[0];
    
    if (current != nullptr && current->key == key) {
        value = current->value;
        return true;
    }
    
    return false;
}

template <typename Key, typename Value>
bool SkipList<Key, Value>::remove(const Key& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Node*> update(max_level_ + 1);
    Node* current = header_;
    
    for (int i = level_; i >= 0; i--) {
        while (current->forward[i] != nullptr && 
               current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    
    current = current->forward[0];
    
    if (current == nullptr || current->key != key) {
        return false;
    }
    
    for (int i = 0; i <= level_; i++) {
        if (update[i]->forward[i] == current) {
            update[i]->forward[i] = current->forward[i];
        }
    }
    
    while (level_ > 0 && header_->forward[level_] == nullptr) {
        level_--;
    }
    
    delete current;
    item_count_--;
    return true;
}

template <typename Key, typename Value>
void SkipList<Key, Value>::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Node* current = header_->forward[0];
    while (current != nullptr) {
        Node* next = current->forward[0];
        delete current;
        current = next;
    }
    
    for (int i = 0; i <= max_level_; i++) {
        header_->forward[i] = nullptr;
    }
    
    level_ = 0;
    item_count_ = 0;
}

#endif // SKIP_LIST_H