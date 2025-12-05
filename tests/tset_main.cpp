#include <gtest/gtest.h>
#include "internal_log.h"
#include <mutex>

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    int result = RUN_ALL_TESTS();
    return result;
}