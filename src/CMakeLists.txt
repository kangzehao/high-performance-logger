
include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/mmap
    ${PROJECT_SOURCE_DIR}/src/context
    ${PROJECT_SOURCE_DIR}/src/compress
    ${PROJECT_SOURCE_DIR}/src/utils
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

set(SRCS
    ${MMAP_SRCS}
    ${UTIL_SRCS}
    ${CONTEXT_SRCS}
    ${COMPRESS_SRCS}
    
)

message(STATUS "libs ${LIBS}")

add_library(logger STATIC ${SRCS})

target_link_libraries(logger ${CONANDEPS_LEGACY} stdc++fs)
