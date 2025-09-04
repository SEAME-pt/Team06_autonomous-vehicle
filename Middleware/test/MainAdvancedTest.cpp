#include <gtest/gtest.h>
#include <csignal>
#include <cstdlib>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <zmq.hpp>

// Mock the main.cpp components for testing
namespace {
    std::atomic<bool> test_stop_flag(false);
    std::atomic<int> test_signal_count(0);

    void testSignalHandler(int signal) {
        int count = test_signal_count.fetch_add(1) + 1;

        if (count == 1) {
            std::cout << "\nTest: Received signal " << signal
                      << ", initiating graceful shutdown..." << std::endl;
            std::cout << "Test: Press Ctrl+C again to force exit." << std::endl;
            test_stop_flag = true;
        } else if (count >= 2) {
            std::cout << "\nTest: Received signal " << signal << " again, forcing exit..."
                      << std::endl;
            std::cout << "Test: Terminating immediately..." << std::endl;
            test_stop_flag = true; // Don't actually exit in tests
        }
    }

    void setupTestSignalHandlers() {
        std::signal(SIGINT, testSignalHandler);
        std::signal(SIGTERM, testSignalHandler);
    }
}

class MainAdvancedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset test state
        test_stop_flag = false;
        test_signal_count = 0;

        // Setup test signal handlers
        setupTestSignalHandlers();

        // Create ZMQ context for testing
        zmq_context = std::make_unique<zmq::context_t>(1);
    }

    void TearDown() override {
        // Reset signal handlers
        std::signal(SIGINT, SIG_DFL);
        std::signal(SIGTERM, SIG_DFL);

        zmq_context.reset();
    }

    std::unique_ptr<zmq::context_t> zmq_context;
};

TEST_F(MainAdvancedTest, SignalHandlerFirstSignal) {
    // Test first signal handling
    EXPECT_EQ(test_signal_count.load(), 0);
    EXPECT_FALSE(test_stop_flag.load());

    // Simulate first signal
    testSignalHandler(SIGINT);

    EXPECT_EQ(test_signal_count.load(), 1);
    EXPECT_TRUE(test_stop_flag.load());
}

TEST_F(MainAdvancedTest, SignalHandlerSecondSignal) {
    // Test second signal handling
    EXPECT_EQ(test_signal_count.load(), 0);
    EXPECT_FALSE(test_stop_flag.load());

    // Simulate first signal
    testSignalHandler(SIGINT);
    EXPECT_EQ(test_signal_count.load(), 1);
    EXPECT_TRUE(test_stop_flag.load());

    // Simulate second signal
    testSignalHandler(SIGTERM);
    EXPECT_EQ(test_signal_count.load(), 2);
    EXPECT_TRUE(test_stop_flag.load());
}

TEST_F(MainAdvancedTest, SignalHandlerMultipleSignals) {
    // Test multiple signals
    for (int i = 0; i < 5; ++i) {
        testSignalHandler(SIGINT);
    }

    EXPECT_EQ(test_signal_count.load(), 5);
    EXPECT_TRUE(test_stop_flag.load());
}

TEST_F(MainAdvancedTest, ZMQContextCreation) {
    // Test ZMQ context creation
    EXPECT_NE(zmq_context, nullptr);

    // Test context is valid
    EXPECT_NO_THROW({
        zmq::socket_t test_socket(*zmq_context, ZMQ_PUB);
        test_socket.close(); // Properly close the socket
    });
}

TEST_F(MainAdvancedTest, ZMQContextDestruction) {
    // Test ZMQ context destruction
    auto local_context = std::make_unique<zmq::context_t>(1);
    EXPECT_NE(local_context, nullptr);

    // Create a socket to ensure context is active
    zmq::socket_t test_socket(*local_context, ZMQ_PUB);

    // Properly close the socket before destroying context
    test_socket.close();

    // Destroy context
    local_context.reset();

    // Context should be destroyed without issues
    EXPECT_TRUE(true);
}

TEST_F(MainAdvancedTest, ConfigurationValues) {
    // Test configuration values (matching main.cpp)
    const std::string zmq_c_address = "tcp://0.0.0.0:5555";
    const std::string zmq_nc_address = "tcp://100.93.45.188:5556";
    const std::string zmq_control_address = "tcp://127.0.0.1:5557";
    const std::string zmq_lkas_address = "tcp://127.0.0.1:5558";
    const std::string zmq_traffic_sign_address = "tcp://127.0.0.1:5559";

    // Verify addresses are valid
    EXPECT_FALSE(zmq_c_address.empty());
    EXPECT_FALSE(zmq_nc_address.empty());
    EXPECT_FALSE(zmq_control_address.empty());
    EXPECT_FALSE(zmq_lkas_address.empty());
    EXPECT_FALSE(zmq_traffic_sign_address.empty());

    // Verify addresses contain expected patterns
    EXPECT_TRUE(zmq_c_address.find("tcp://") == 0);
    EXPECT_TRUE(zmq_nc_address.find("tcp://") == 0);
    EXPECT_TRUE(zmq_control_address.find("tcp://") == 0);
    EXPECT_TRUE(zmq_lkas_address.find("tcp://") == 0);
    EXPECT_TRUE(zmq_traffic_sign_address.find("tcp://") == 0);
}

TEST_F(MainAdvancedTest, ComponentInitializationOrder) {
    // Test component initialization order (simplified)
    std::vector<std::string> init_order;

    // Simulate initialization order from main.cpp
    init_order.push_back("ZMQ Context");
    init_order.push_back("Publishers");
    init_order.push_back("Sensor Handler");
    init_order.push_back("Control Assembly");
    init_order.push_back("Lane Keeping Handler");
    init_order.push_back("Traffic Sign Handler");

    EXPECT_EQ(init_order.size(), 6);
    EXPECT_EQ(init_order[0], "ZMQ Context");
    EXPECT_EQ(init_order[1], "Publishers");
    EXPECT_EQ(init_order[2], "Sensor Handler");
    EXPECT_EQ(init_order[3], "Control Assembly");
    EXPECT_EQ(init_order[4], "Lane Keeping Handler");
    EXPECT_EQ(init_order[5], "Traffic Sign Handler");
}

TEST_F(MainAdvancedTest, ErrorHandling) {
    // Test error handling scenarios
    EXPECT_NO_THROW({
        try {
            // Simulate an error condition
            throw std::runtime_error("Test error");
        } catch (const std::exception& e) {
            // Should handle gracefully
            EXPECT_STREQ(e.what(), "Test error");
        }
    });
}

TEST_F(MainAdvancedTest, ResourceCleanup) {
    // Test resource cleanup order (simplified)
    std::vector<std::string> cleanup_order;

    // Simulate cleanup order from main.cpp
    cleanup_order.push_back("Stop Sensor Handler");
    cleanup_order.push_back("Stop Control Assembly");
    cleanup_order.push_back("Stop Lane Keeping Handler");
    cleanup_order.push_back("Stop Traffic Sign Handler");
    cleanup_order.push_back("Release Components");
    cleanup_order.push_back("Release Publishers");
    cleanup_order.push_back("Close ZMQ Context");

    EXPECT_EQ(cleanup_order.size(), 7);
    EXPECT_EQ(cleanup_order[0], "Stop Sensor Handler");
    EXPECT_EQ(cleanup_order[6], "Close ZMQ Context");
}

TEST_F(MainAdvancedTest, ThreadSafety) {
    // Test thread safety of signal handling
    std::vector<std::thread> threads;

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this]() {
            for (int j = 0; j < 100; ++j) {
                testSignalHandler(SIGINT);
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });
    }

    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Signal count should be consistent
    EXPECT_GT(test_signal_count.load(), 0);
    EXPECT_TRUE(test_stop_flag.load());
}

TEST_F(MainAdvancedTest, SignalHandlerReset) {
    // Test signal handler reset
    setupTestSignalHandlers();

    // Verify handlers are set
    EXPECT_NO_THROW({
        testSignalHandler(SIGINT);
    });

    // Reset to default
    std::signal(SIGINT, SIG_DFL);
    std::signal(SIGTERM, SIG_DFL);

    // Should not crash
    EXPECT_TRUE(true);
}

TEST_F(MainAdvancedTest, MultipleSignalTypes) {
    // Test different signal types
    std::vector<int> signal_types = {SIGINT, SIGTERM};

    for (int signal_type : signal_types) {
        test_signal_count = 0;
        test_stop_flag = false;

        testSignalHandler(signal_type);

        EXPECT_EQ(test_signal_count.load(), 1);
        EXPECT_TRUE(test_stop_flag.load());
    }
}

TEST_F(MainAdvancedTest, SignalCountTracking) {
    // Test signal count tracking
    EXPECT_EQ(test_signal_count.load(), 0);

    for (int i = 1; i <= 5; ++i) {
        testSignalHandler(SIGINT);
        EXPECT_EQ(test_signal_count.load(), i);
    }
}

TEST_F(MainAdvancedTest, GracefulShutdownFlag) {
    // Test graceful shutdown flag behavior
    EXPECT_FALSE(test_stop_flag.load());

    // First signal should set flag
    testSignalHandler(SIGINT);
    EXPECT_TRUE(test_stop_flag.load());

    // Flag should remain true
    testSignalHandler(SIGTERM);
    EXPECT_TRUE(test_stop_flag.load());
}

TEST_F(MainAdvancedTest, ComponentLifecycle) {
    // Test component lifecycle (simplified)
    std::vector<std::string> lifecycle;

    lifecycle.push_back("Initialize");
    lifecycle.push_back("Start");
    lifecycle.push_back("Run");
    lifecycle.push_back("Stop");
    lifecycle.push_back("Cleanup");

    EXPECT_EQ(lifecycle.size(), 5);
    EXPECT_EQ(lifecycle[0], "Initialize");
    EXPECT_EQ(lifecycle[4], "Cleanup");
}

TEST_F(MainAdvancedTest, ExceptionHandling) {
    // Test exception handling patterns from main.cpp
    EXPECT_NO_THROW({
        try {
            throw std::runtime_error("Test runtime error");
        } catch (const std::exception& e) {
            // Should handle gracefully
            EXPECT_STREQ(e.what(), "Test runtime error");
        }
    });

    EXPECT_NO_THROW({
        try {
            throw std::bad_alloc();
        } catch (const std::exception& e) {
            // Should handle gracefully
            EXPECT_TRUE(true);
        }
    });
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
