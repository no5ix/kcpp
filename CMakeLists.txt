# CMake 最低版本号要求
cmake_minimum_required (VERSION 2.8)

# support C++11
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set (LIB_NAME kcpp)

if(UNIX)
    if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
        # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
        # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")  # Inhibit all warning messages.
        set(CXX_FLAGS
            -g
            # -DVALGRIND
            # -DMUDUO_STD_STRING
            -DCHECK_PTHREAD_RETURN_VALUE
            -D_FILE_OFFSET_BITS=64
            -Wall
            -Wextra
            -Werror
            # -Wconversion
            -Wno-unused-parameter
            # -Wold-style-cast
            -Wno-multichar
            -Wno-unused-function
            -Wno-reorder
            -Woverloaded-virtual
            -Wpointer-arith
            # -Wshadow
            -Wwrite-strings
            -march=native
            # -MMD
            -std=c++0x
            -rdynamic
        )
        string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")
    endif()
endif()


set(kcp_SRCS ikcp.c)

add_library(${LIB_NAME} ${kcp_SRCS})
target_link_libraries(${LIB_NAME})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})



if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test)
    message(STATUS  "has test subdirectory.")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test)
else()
    message(STATUS  "has no test subdirectory.")
endif()