#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>

// Base class for tests that need to suppress output
class OutputSuppressor {
protected:
    std::streambuf* oldCout;
    std::streambuf* oldCerr;
    std::stringstream outputStream;
    std::stringstream errorStream;

    void suppressOutput() {
        oldCout = std::cout.rdbuf();
        oldCerr = std::cerr.rdbuf();
        std::cout.rdbuf(outputStream.rdbuf());
        std::cerr.rdbuf(errorStream.rdbuf());
    }

    void restoreOutput() {
        std::cout.rdbuf(oldCout);
        std::cerr.rdbuf(oldCerr);
    }

public:
    virtual ~OutputSuppressor() {
        restoreOutput();
    }
};

// Helper function to wait for a condition with timeout
template <typename Func>
bool waitForCondition(Func condition, int timeoutMs = 3000, int checkIntervalMs = 100) {
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
           std::chrono::steady_clock::now() - start).count() < timeoutMs) {
        if (condition()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(checkIntervalMs));
    }
    return false;
}

// Helper to check if running in CI environment
inline bool isRunningInCI() {
    return std::getenv("CI") != nullptr;
}

// Helper to skip test in CI environment
#define SKIP_IN_CI() \
    if (isRunningInCI()) { \
        GTEST_SKIP() << "Skipping in CI environment"; \
        return; \
    }

#endif // TEST_UTILS_HPP
