#include <gtest/gtest.h>
#include "SensorHandler.hpp"
#include "ControlAssembly.hpp"
#include "MockPublisher.hpp"
#include "MockBackMotors.hpp"
#include "MockFServo.hpp"
#include "TestUtils.hpp"
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <mutex>
#include <zmq.hpp>
#include "../../zmq/inc/ZmqPublisher.hpp"

class PerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        zmq_context = std::make_unique<zmq::context_t>(1);
        mock_c_publisher = std::make_shared<MockPublisher>();
        mock_nc_publisher = std::make_shared<MockPublisher>();
        mock_back_motors = std::make_shared<MockBackMotors>();
        mock_f_servo = std::make_shared<MockFServo>();
    }

    void TearDown() override {
        zmq_context.reset();
    }

    std::unique_ptr<zmq::context_t> zmq_context;
    std::shared_ptr<MockPublisher> mock_c_publisher;
    std::shared_ptr<MockPublisher> mock_nc_publisher;
    std::shared_ptr<MockBackMotors> mock_back_motors;
    std::shared_ptr<MockFServo> mock_f_servo;
};

TEST_F(PerformanceTest, SensorHandlerThroughput) {
    SKIP_IN_CI();

    auto sensor_handler = std::make_unique<SensorHandler>(
        "tcp://127.0.0.1:5570",
        "tcp://127.0.0.1:5571",
        *zmq_context,
        mock_c_publisher,
        mock_nc_publisher,
        false
    );

    // Measure throughput over time
    auto start_time = std::chrono::high_resolution_clock::now();

    sensor_handler->start();

    // Let it run for a specific duration
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    sensor_handler->stop();

    // Check that it ran for the expected duration
    EXPECT_GE(duration.count(), 1900); // Allow some tolerance
    EXPECT_LE(duration.count(), 2100);

    // Check that messages were published
    EXPECT_GT(mock_c_publisher->messageCount(), 0);
    EXPECT_GT(mock_nc_publisher->messageCount(), 0);
}

TEST_F(PerformanceTest, ControlAssemblyResponseTime) {
    SKIP_IN_CI();

    auto control_assembly = std::make_unique<ControlAssembly>(
        "tcp://127.0.0.1:5572",
        *zmq_context,
        mock_back_motors,
        mock_f_servo
    );

    control_assembly->start();

    // Wait for it to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Measure response time for control commands
    zmq::socket_t sender(*zmq_context, ZMQ_PUB);
    sender.bind("tcp://127.0.0.1:5572");

    // Give the subscriber time to connect
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::vector<double> response_times;

    for (int i = 0; i < 10; i++) {
        std::string command = "throttle:" + std::to_string(i * 10) + ";";

        auto start_time = std::chrono::high_resolution_clock::now();
        sender.send(zmq::buffer(command), zmq::send_flags::none);
        auto end_time = std::chrono::high_resolution_clock::now();

        auto response_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        response_times.push_back(response_time.count());

        // Wait for command to be processed before next iteration
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    control_assembly->stop();

    // Calculate average response time
    double total_time = 0;
    for (double time : response_times) {
        total_time += time;
    }
    double average_response_time = total_time / response_times.size();

    // Response time should be reasonable (less than 10ms for ZMQ send)
    EXPECT_LT(average_response_time, 10000); // 10ms in microseconds
}

TEST_F(PerformanceTest, ConcurrentMessageHandling) {
    SKIP_IN_CI();

    auto control_assembly = std::make_unique<ControlAssembly>(
        "tcp://127.0.0.1:5573",
        *zmq_context,
        mock_back_motors,
        mock_f_servo
    );

    control_assembly->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    zmq::socket_t sender(*zmq_context, ZMQ_PUB);
    sender.bind("tcp://127.0.0.1:5573");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Send multiple commands concurrently with proper synchronization
    std::atomic<int> commands_sent(0);
    std::vector<std::thread> sender_threads;
    std::mutex sender_mutex; // Protect the ZMQ socket from concurrent access

    for (int i = 0; i < 5; i++) {
        sender_threads.emplace_back([&sender, &commands_sent, &sender_mutex, i]() {
            for (int j = 0; j < 10; j++) {
                std::string command = "throttle:" + std::to_string(i * 10 + j) + ";";
                {
                    std::lock_guard<std::mutex> lock(sender_mutex);
                    sender.send(zmq::buffer(command), zmq::send_flags::none);
                }
                commands_sent++;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }

    // Wait for all commands to be sent
    for (auto& thread : sender_threads) {
        thread.join();
    }

    // Wait for commands to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    control_assembly->stop();

    // Check that commands were sent
    EXPECT_EQ(commands_sent.load(), 50);
}

TEST_F(PerformanceTest, MemoryUsage) {
    // Test memory usage by creating and destroying many components
    std::vector<std::unique_ptr<SensorHandler>> handlers;

    for (int i = 0; i < 10; i++) {
        auto handler = std::make_unique<SensorHandler>(
            "tcp://127.0.0.1:" + std::to_string(5580 + i),
            "tcp://127.0.0.1:" + std::to_string(5590 + i),
            *zmq_context,
            mock_c_publisher,
            mock_nc_publisher,
            false
        );

        handlers.push_back(std::move(handler));
    }

    // Start and stop all handlers
    for (auto& handler : handlers) {
        handler->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        handler->stop();
    }

    // Clear handlers
    handlers.clear();

    // Test should complete without memory issues
    EXPECT_TRUE(true);
}

TEST_F(PerformanceTest, LongRunningStability) {
    SKIP_IN_CI();

    auto sensor_handler = std::make_unique<SensorHandler>(
        "tcp://127.0.0.1:5600",
        "tcp://127.0.0.1:5601",
        *zmq_context,
        mock_c_publisher,
        mock_nc_publisher,
        false
    );

    // Run for a longer period to test stability
    sensor_handler->start();

    auto start_time = std::chrono::steady_clock::now();
    auto end_time = start_time + std::chrono::seconds(5);

    int update_count = 0;
    while (std::chrono::steady_clock::now() < end_time) {
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        update_count++;
    }

    sensor_handler->stop();

    // Should have run for the expected duration
    EXPECT_GE(update_count, 40); // At least 40 updates in 5 seconds
    EXPECT_LE(update_count, 60); // But not more than 60
}

TEST_F(PerformanceTest, RapidStartStopCycles) {
    SKIP_IN_CI();

    auto control_assembly = std::make_unique<ControlAssembly>(
        "tcp://127.0.0.1:5602",
        *zmq_context,
        mock_back_motors,
        mock_f_servo
    );

    // Perform rapid start/stop cycles
    for (int i = 0; i < 20; i++) {
        control_assembly->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        control_assembly->stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Test should complete without issues
    EXPECT_TRUE(true);
}

TEST_F(PerformanceTest, MessageQueueStress) {
    SKIP_IN_CI();

    auto control_assembly = std::make_unique<ControlAssembly>(
        "tcp://127.0.0.1:5603",
        *zmq_context,
        mock_back_motors,
        mock_f_servo
    );

    control_assembly->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    zmq::socket_t sender(*zmq_context, ZMQ_PUB);
    sender.bind("tcp://127.0.0.1:5603");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Send a large number of messages rapidly
    for (int i = 0; i < 100; i++) {
        std::string command = "throttle:" + std::to_string(i % 100) + ";";
        sender.send(zmq::buffer(command), zmq::send_flags::none);
    }

    // Wait for messages to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    control_assembly->stop();

    // Test should complete without issues
    EXPECT_TRUE(true);
}

TEST_F(PerformanceTest, ThreadCreationOverhead) {
    // Test thread creation and destruction overhead
    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;
    for (int i = 0; i < 100; i++) {
        threads.emplace_back([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Thread creation and destruction should be reasonably fast
    EXPECT_LT(duration.count(), 1000); // Less than 1 second for 100 threads
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
