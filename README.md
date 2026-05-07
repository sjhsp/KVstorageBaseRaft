# KVstorageBaseRaft

基于 Raft 一致性算法的分布式键值存储系统。

## 项目架构

```
┌─────────────────────────────────────────────────────────────────┐
│                        KV Server                               │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐       │
│  │  Node 1  │  │  Node 2  │  │  Node 3  │  │  Node N  │       │
│  │ (Leader) │  │(Follower)│  │(Follower)│  │(Follower)│       │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘       │
│       │             │             │             │              │
│       └─────────────┴─────┬───────┴─────────────┘              │
│                           ▼                                    │
│                ┌───────────────────┐                            │
│                │     Raft Core     │                            │
│                │  (Consensus)      │                            │
│                └─────────┬─────────┘                            │
│                          ▼                                    │
│                ┌───────────────────┐                            │
│                │   KV Storage      │                            │
│                │   (SkipList)      │                            │
│                └───────────────────┘                            │
└─────────────────────────────────────────────────────────────────┘
```

## 核心模块

| 模块 | 功能 |
|------|------|
| **raftCore** | Raft 一致性算法核心实现 |
| **kvServer** | KV 存储服务接口 |
| **rpc** | RPC 通信框架 |
| **skipList** | 跳表存储引擎 |
| **common** | 通用工具类 |

## 技术栈

- **语言**: C++17
- **构建工具**: CMake
- **并发**: STL Threads
- **存储**: SkipList (可替换)

## 快速开始

### 编译项目

```bash
mkdir -p build && cd build
cmake ..
make
```

### 运行测试

```bash
./test_raft
```

### 运行示例

```bash
./example
```

## Raft 算法核心

### 节点状态
- **Follower**: 被动接收 Leader 的心跳和日志
- **Candidate**: 发起选举时的状态
- **Leader**: 处理所有客户端请求

### 核心功能
1. **Leader 选举**: 通过 RequestVote RPC 选举 Leader
2. **日志复制**: 通过 AppendEntries RPC 复制日志到 Followers
3. **安全性**: 保证已提交日志不丢失

### RPC 协议
- **RequestVote**: 选举 RPC
- **AppendEntries**: 日志复制和心跳 RPC
- **InstallSnapshot**: 快照安装 RPC

## 目录结构

```
KVstorageBaseRaft/
├── src/
│   ├── common/        # 通用工具类
│   ├── raftCore/      # Raft 核心算法
│   ├── kvServer/      # KV 存储服务
│   ├── rpc/           # RPC 通信
│   └── skipList/      # 跳表实现
├── include/           # 头文件
├── test/              # 测试代码
├── example/           # 示例代码
├── docs/              # 文档
├── build/             # 构建目录
└── CMakeLists.txt     # 构建配置
```

## 学习路线

1. **基础架构**: 理解项目结构和工具类
2. **Raft 核心**: 学习选举、日志复制、安全性
3. **KV 存储**: 理解状态机和存储引擎
4. **RPC 通信**: 学习分布式节点通信
5. **整合测试**: 验证集群一致性

## 参考资料

- [Raft Paper](https://raft.github.io/raft.pdf)
- [MIT 6.824 Distributed Systems](https://pdos.csail.mit.edu/6.824/)
- [Raft 可视化演示](http://thesecretlivesofdata.com/raft/)

## License

MIT License