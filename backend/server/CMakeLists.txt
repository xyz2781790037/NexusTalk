# cmake_minimum_required(VERSION 3.13)
# project(MychatroomProject)
# find_package(CURL REQUIRED)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# # 包含头文件
# include_directories(/usr/local/include)

# # 链接库路径
# link_directories(/usr/local/lib)
# # 查找库
# find_package(nlohmann_json 3.2.0 REQUIRED)
# # 添加可执行文件
# set(SERVER_HEADERS
#     handleData.h
#     redisCmd.h
#     ../base/MegType.h
#     ../base/tool.h
#     ../base/threadPool.h
#     ConnectionManager.h
# )
# set(SERVER_SOURCES
#     server.cc
#     handleData.cc
#     redisCmd.cc
#     ../base/MegType.cc
#     ../base/tool.cc
#     ConnectionManager.cc
# )
# # 定义可执行文件
# add_executable(server 
#     server.cc 
#     handleData.cc 
#     ConnectionManager.cc 
#     redisCmd.cc
#     ../base/MegType.cc
#     ../base/tool.cc
# )

# add_executable(server ${SERVER_SOURCES})
# # 链接 cpp_redis 和 tacopie
# target_link_libraries(server
#     PRIVATE
#     netlib
#     cpp_redis
#     tacopie
#     pthread
#     nlohmann_json::nlohmann_json
#     CURL::libcurl
#     profiler
# )
# target_include_directories(server PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}
#     ${CMAKE_SOURCE_DIR}/../base
# )
cmake_minimum_required(VERSION 3.13)
project(MychatroomProject)

# 必须项
find_package(CURL REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含头文件
include_directories(/usr/local/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../base)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../netlib)

# 链接库路径
link_directories(/usr/local/lib)

# 整理源文件列表
# [修改] 确保这里包含了所有必要的 C++ 逻辑文件
set(SERVER_SOURCES
    server.cc
    handleData.cc
    redisCmd.cc
    ConnectionManager.cc
    ../base/MegType.cc
    ../base/tool.cc
)

# [修改] 删掉之前重复的 add_executable(server ...)，只保留这一行
add_executable(server ${SERVER_SOURCES})

# 链接库
# [注意] 确保 netlib 路径正确 [cite: 2]
target_link_libraries(server
    PRIVATE
    netlib
    cpp_redis
    tacopie
    pthread
    nlohmann_json::nlohmann_json
    CURL::libcurl
    # 如果你不需要性能分析，可以去掉 profiler
)

# 设置包含目录优先级
target_include_directories(server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/../base
)