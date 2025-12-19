set(LOGGER_VERSION "logger-decode")

include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/mmap
    ${PROJECT_SOURCE_DIR}/src/context
    ${PROJECT_SOURCE_DIR}/src/compress
    ${PROJECT_SOURCE_DIR}/src/crypt
    ${PROJECT_SOURCE_DIR}/src/utils
    ${PROJECT_SOURCE_DIR}/src/formatter
    ${PROJECT_SOURCE_DIR}/src/sinks
    ${PROJECT_SOURCE_DIR}/decode
    ${CMAKE_BINARY_DIR}/src
)

set(DECODE 

    decode.cpp
    decode_formatter.cpp

)

add_executable(${LOGGER_VERSION} ${DECODE})
target_link_libraries(${LOGGER_VERSION} ${CONANDEPS_LEGACY} logger stdc++fs)

install(TARGETS ${LOGGER_VERSION} RUNTIME DESTINATION bin)

