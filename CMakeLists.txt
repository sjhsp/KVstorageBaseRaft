cmake_minimum_required(VERSION 3.10)

project(KVstorageBaseRaft)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

# 添加头文件路径
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src)

# 收集源代码文件
file(GLOB_RECURSE SRC_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")

# 创建库
add_library(raft STATIC ${SRC_FILES})

# 创建测试可执行文件
add_executable(test_raft ${PROJECT_SOURCE_DIR}/test/test_raft.cpp)
target_link_libraries(test_raft raft)

# 创建示例可执行文件
add_executable(example ${PROJECT_SOURCE_DIR}/example/main.cpp)
target_link_libraries(example raft)

# 安装目标
install(TARGETS raft DESTINATION lib)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION include)