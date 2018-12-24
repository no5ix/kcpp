set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

set (LIB_NAME kcpp)

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

add_executable(SrvTestKcp TestKcpSrv.cpp)
IF(WIN32)
    target_link_libraries(SrvTestKcp ${LIB_NAME} ws2_32.lib)
else()
    target_link_libraries(SrvTestKcp ${LIB_NAME})
ENDIF()

add_executable(CliTestKcp TestKcpCli.cpp)
IF(WIN32)
    target_link_libraries(CliTestKcp ${LIB_NAME} ws2_32.lib)
else()
    target_link_libraries(CliTestKcp ${LIB_NAME})
ENDIF()

# message(STATUS  "TestKcpp build finished")
    
