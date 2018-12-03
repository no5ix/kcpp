# CMake 最低版本号要求
cmake_minimum_required (VERSION 2.8)

set (LIB_NAME kcpp)

set(kcp_SRCS
  ikcp.c
)

add_library(${LIB_NAME} ${kcp_SRCS})
target_link_libraries(${LIB_NAME})

# support C++11
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

add_executable(ServerTestKcpp TestKcppServer.cpp)
IF(WIN32)
    target_link_libraries(ServerTestKcpp ${LIB_NAME} ws2_32.lib)
else()
    target_link_libraries(ServerTestKcpp ${LIB_NAME})
ENDIF()

add_executable(ClientTestKcpp TestKcppClient.cpp)
IF(WIN32)
    target_link_libraries(ClientTestKcpp ${LIB_NAME} ws2_32.lib)
else()
    target_link_libraries(ClientTestKcpp ${LIB_NAME})
ENDIF()

message(STATUS  "TestKcpp build finished")
    
