set(LOGGER_VERSION "logger-test")

include_directories(
    ${PROJECT_SOURCE_DIR}/src
)

set(TEST 
    test_mmap.cpp
    test_thread_pool.cpp
    test_context.cpp
    test_compress.cpp
    test_crypt.cpp
    tset_main.cpp
)

add_executable(${LOGGER_VERSION} ${TEST})
target_link_libraries(${LOGGER_VERSION} ${CONANDEPS_LEGACY} logger stdc++fs)

install(TARGETS ${LOGGER_VERSION} RUNTIME DESTINATION bin)

