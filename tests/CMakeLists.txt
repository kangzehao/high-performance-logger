set(LOGGER_VERSION "logger-test")
include_directories(
    ${PROJECT_SOURCE_DIR}/src
)

set(TEST 
    test_mmap.cpp
)

add_executable(${LOGGER_VERSION} ${TEST})
target_link_libraries(${LOGGER_VERSION} ${CONANDEPS_LEGACY} logger stdc++fs)

install(TARGETS ${LOGGER_VERSION} RUNTIME DESTINATION bin)

