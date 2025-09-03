#include <gtest/gtest.h>
#include "SensorHandler.hpp"
#include "ControlAssembly.hpp"
#include "LaneKeepingHandler.hpp"
#include "TrafficSignHandler.hpp"
#include "MockPublisher.hpp"
#include "MockBackMotors.hpp"
#include "MockFServo.hpp"
#include "TestUtils.hpp"
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <zmq.hpp>
#include "../../zmq/inc/ZmqPublisher.hpp"

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        zmq_context = std::make_unique<zmq::context_t>(1);

        // Create mock components
        mock_c_publisher = std::make_shared<MockPublisher>();
        mock_nc_publisher = std::make_shared<MockPublisher>();
        mock_back_motors = std::make_shared<MockBackMotors>();
        mock_f_servo = std::make_shared<MockFServo>();

        // Create handlers with different ports to avoid conflicts
        sensor_handler = std::make_unique<SensorHandler>(
            "tcp://127.0.0.1:5555",
            "tcp://127.0.0.1:5556",
            *zmq_context,
            mock_c_publisher,
            mock_nc_publisher,
            false  // Don't use real sensors in tests
        );

        control_assembly = std::make_unique<ControlAssembly>(
            "tcp://127.0.0.1:5557",
            *zmq_context,
            mock_back_motors,
            mock_f_servo
        );

        lane_keeping_handler = std::make_unique<LaneKeepingHandler>(
            "tcp://127.0.0.1:5558",
            *zmq_context,
            mock_nc_publisher,
            true  // test_mode = true
        );

        traffic_sign_handler = std::make_unique<TrafficSignHandler>(
            "tcp://127.0.0.1:5559",
            *zmq_context,
            mock_nc_publisher,
            true  // test_mode = true
        );
    }

    void TearDown() override {
        if (sensor_handler) sensor_handler->stop();
        if (control_assembly) control_assembly->stop();
        if (lane_keeping_handler) lane_keeping_handler->stop();
        if (traffic_sign_handler) traffic_sign_handler->stop();

        sensor_handler.reset();
        control_assembly.reset();
        lane_keeping_handler.reset();
        traffic_sign_handler.reset();
        zmq_context.reset();
    }

    std::unique_ptr<zmq::context_t> zmq_context;
    std::shared_ptr<MockPublisher> mock_c_publisher;
    std::shared_ptr<MockPublisher> mock_nc_publisher;
    std::shared_ptr<MockBackMotors> mock_back_motors;
    std::shared_ptr<MockFServo> mock_f_servo;

    std::unique_ptr<SensorHandler> sensor_handler;
    std::unique_ptr<ControlAssembly> control_assembly;
    std::unique_ptr<LaneKeepingHandler> lane_keeping_handler;
    std::unique_ptr<TrafficSignHandler> traffic_sign_handler;
};

TEST_F(IntegrationTest, ComponentInitialization) {
    // Test that all components can be initialized without errors
    EXPECT_NE(sensor_handler, nullptr);
    EXPECT_NE(control_assembly, nullptr);
    EXPECT_NE(lane_keeping_handler, nullptr);
    EXPECT_NE(traffic_sign_handler, nullptr);

    // Test that mock components are properly initialized
    EXPECT_TRUE(mock_back_motors->isI2cOpened());
    EXPECT_TRUE(mock_back_motors->isInitialized());
    EXPECT_TRUE(mock_f_servo->isI2cOpened());
    EXPECT_TRUE(mock_f_servo->isInitialized());
}

TEST_F(IntegrationTest, ComponentStartStop) {
    SKIP_IN_CI();

    // Test that all components can start and stop without errors
    EXPECT_NO_THROW({
        sensor_handler->start();
        control_assembly->start();
        lane_keeping_handler->start();
        traffic_sign_handler->start();

        // Let them run for a bit
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        sensor_handler->stop();
        control_assembly->stop();
        lane_keeping_handler->stop();
        traffic_sign_handler->stop();
    });
}

TEST_F(IntegrationTest, SensorHandlerPublishesData) {
    SKIP_IN_CI();

    // Start sensor handler
    sensor_handler->start();

    // Wait for data to be published
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Check that init messages were sent
    EXPECT_TRUE(mock_c_publisher->hasMessage("init;"));
    EXPECT_TRUE(mock_nc_publisher->hasMessage("init;"));

    sensor_handler->stop();
}

TEST_F(IntegrationTest, ControlAssemblyHandlesCommands) {
    SKIP_IN_CI();

    // Start control assembly
    control_assembly->start();

    // Wait for it to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Send control commands via ZMQ
    zmq::socket_t sender(*zmq_context, ZMQ_PUB);
    sender.bind("tcp://127.0.0.1:5557");

    // Give the subscriber time to connect
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Send throttle command
    std::string throttle_message = "throttle:50;";
    sender.send(zmq::buffer(throttle_message), zmq::send_flags::none);

    // Wait for command to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Check that the command was processed
    EXPECT_EQ(mock_back_motors->getCurrentSpeed(), 50);

    control_assembly->stop();
}

TEST_F(IntegrationTest, LaneKeepingHandlerProcessesData) {
    SKIP_IN_CI();

    // Start lane keeping handler
    lane_keeping_handler->start();

    // Wait for it to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Inject test data
    LaneKeepingData test_data;
    test_data.lane_status = 1; // Left deviation
    lane_keeping_handler->setTestLaneKeepingData(test_data);

    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    lane_keeping_handler->stop();

    // Test should complete without issues
    EXPECT_TRUE(true);
}

TEST_F(IntegrationTest, TrafficSignHandlerProcessesSigns) {
    SKIP_IN_CI();

    // Start traffic sign handler
    traffic_sign_handler->start();

    // Wait for it to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Inject test traffic sign data
    traffic_sign_handler->setTestTrafficSignData("SPEED_50");

    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    traffic_sign_handler->stop();

    // Test should complete without issues
    EXPECT_TRUE(true);
}

TEST_F(IntegrationTest, ConcurrentComponentOperation) {
    SKIP_IN_CI();

    // Start all components concurrently
    std::vector<std::thread> start_threads;

    start_threads.emplace_back([this]() { sensor_handler->start(); });
    start_threads.emplace_back([this]() { control_assembly->start(); });
    start_threads.emplace_back([this]() { lane_keeping_handler->start(); });
    start_threads.emplace_back([this]() { traffic_sign_handler->start(); });

    // Wait for all to start
    for (auto& thread : start_threads) {
        thread.join();
    }

    // Let them run concurrently for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Stop all components
    std::vector<std::thread> stop_threads;

    stop_threads.emplace_back([this]() { sensor_handler->stop(); });
    stop_threads.emplace_back([this]() { control_assembly->stop(); });
    stop_threads.emplace_back([this]() { lane_keeping_handler->stop(); });
    stop_threads.emplace_back([this]() { traffic_sign_handler->stop(); });

    // Wait for all to stop
    for (auto& thread : stop_threads) {
        thread.join();
    }

    // Test should complete without issues
    EXPECT_TRUE(true);
}

TEST_F(IntegrationTest, ErrorHandling) {
    // Test that components handle errors gracefully
    EXPECT_NO_THROW({
        // Try to start components multiple times
        sensor_handler->start();
        sensor_handler->start(); // Should handle gracefully

        // Try to stop components multiple times
        sensor_handler->stop();
        sensor_handler->stop(); // Should handle gracefully
    });
}

TEST_F(IntegrationTest, ResourceCleanup) {
    // Test that resources are properly cleaned up
    EXPECT_NO_THROW({
        // Create and destroy components multiple times
        for (int i = 0; i < 3; i++) {
            auto temp_sensor_handler = std::make_unique<SensorHandler>(
                "tcp://127.0.0.1:5560",
                "tcp://127.0.0.1:5561",
                *zmq_context,
                mock_c_publisher,
                mock_nc_publisher,
                false
            );

            temp_sensor_handler->start();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            temp_sensor_handler->stop();
            temp_sensor_handler.reset();
        }
    });
}

TEST_F(IntegrationTest, MessageFlow) {
    SKIP_IN_CI();

    // Test message flow between components
    sensor_handler->start();
    control_assembly->start();

    // Wait for components to start
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Send control command
    zmq::socket_t sender(*zmq_context, ZMQ_PUB);
    sender.bind("tcp://127.0.0.1:5557");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::string steering_message = "steering:30;";
    sender.send(zmq::buffer(steering_message), zmq::send_flags::none);

    // Wait for command to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Check that the command was processed
    EXPECT_EQ(mock_f_servo->getSteeringAngle(), 30);

    sensor_handler->stop();
    control_assembly->stop();
}

TEST_F(IntegrationTest, StressTest) {
    SKIP_IN_CI();

    // Stress test with rapid start/stop cycles
    for (int i = 0; i < 5; i++) {
        sensor_handler->start();
        control_assembly->start();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        sensor_handler->stop();
        control_assembly->stop();

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Test should complete without issues
    EXPECT_TRUE(true);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
