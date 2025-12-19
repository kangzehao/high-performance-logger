
include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/mmap
    ${PROJECT_SOURCE_DIR}/src/context
    ${PROJECT_SOURCE_DIR}/src/compress
    ${PROJECT_SOURCE_DIR}/src/crypt
    ${PROJECT_SOURCE_DIR}/src/utils
    ${PROJECT_SOURCE_DIR}/src/formatter
    ${PROJECT_SOURCE_DIR}/src/sinks
    ${CMAKE_BINARY_DIR}/src
)


add_definitions(-DJM_XORSTR_DISABLE_AVX_INTRINSICS)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(MMAP_SRCS mmap/mmap_handle.cpp mmap/mmap_handle_linux.cpp)
    set(UTIL_SRCS utils/file_util.cpp utils/sys_util_linux.cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(MMAP_SRCS mmap/mmap_handle.cpp mmap/mmap_handle_windows.cpp)
    set(UTIL_SRCS utils/file_util.cpp utils/sys_util_windows.cpp)
else()
    message(FATAL_ERROR "unsupported.")
endif()

set(CONTEXT_SRCS context/thread_pool.cpp context/executor.cpp context/context.cpp)

set(COMPRESS_SRCS compress/zlib_compress.cpp compress/zstd_compress.cpp)

set(CRYPT_SRCS crypt/crypt.cpp crypt/aes_crypt.cpp)



# 设置.proto文件的路径 生成C++代码
set(PROTO_FILES proto/effective_msg.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

set(PROTO_SRCS ${CMAKE_BINARY_DIR}/src/proto/effective_msg.pb.cc)

message(STATUS "Generated source: ${PROTO_SRCS}")
message(STATUS "Generated header: ${PROTO_HDRS}")

set(FORMATTER_SRCS formatter/default_formatter.cpp formatter/effective_formatter.cpp)

set(SINKS_SRCS sinks/console_sink.cpp sinks/effective_sink.cpp)

set(SRCS
    ${MMAP_SRCS}
    ${UTIL_SRCS}
    ${CONTEXT_SRCS}
    ${COMPRESS_SRCS}
    ${CRYPT_SRCS}
    ${PROTO_SRCS} 
    ${FORMATTER_SRCS} 
    ${SINKS_SRCS} 
    log_handle.cpp
    log_factory.cpp
)

message(STATUS "libs ${LIBS}")

add_library(logger STATIC ${SRCS})

target_link_libraries(logger ${CONANDEPS_LEGACY} stdc++fs)
