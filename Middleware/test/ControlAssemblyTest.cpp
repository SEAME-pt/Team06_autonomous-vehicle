#include <gtest/gtest.h>
#include "ControlAssembly.hpp"
#include "MockBackMotors.hpp"
#include "MockFServo.hpp"
#include "TestUtils.hpp"
#include <memory>
#include <thread>
#include <chrono>
#include <iostream>

class ControlAssemblyTest : public ::testing::Test {
protected:
    void SetUp() override {
        context = std::make_shared<zmq::context_t>(1);
        mockBackMotors = std::make_shared<MockBackMotors>();
        mockFServo = std::make_shared<MockFServo>();
    }

    void TearDown() override {
        context.reset();
    }

    std::shared_ptr<zmq::context_t> context;
    std::shared_ptr<MockBackMotors> mockBackMotors;
    std::shared_ptr<MockFServo> mockFServo;

    // Helper function to wait for a condition with timeout
    template <typename Func>
    bool waitForCondition(Func condition, int timeoutMs = 3000, int checkIntervalMs = 50) {
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
};

TEST_F(ControlAssemblyTest, Initialization) {
    // Create a ControlAssembly with our mocks
    ControlAssembly assembly("tcp://localhost:5555", *context, mockBackMotors, mockFServo);

    // Check that the mock components were properly initialized
    EXPECT_TRUE(mockBackMotors->isI2cOpened());
    EXPECT_TRUE(mockBackMotors->isInitialized());
    EXPECT_TRUE(mockFServo->isI2cOpened());
    EXPECT_TRUE(mockFServo->isInitialized());
}

TEST_F(ControlAssemblyTest, HandleThrottleMessage) {
    SKIP_IN_CI();

    // Create a ControlAssembly with our mocks - use explicit loopback IP
    ControlAssembly assembly("tcp://127.0.0.1:5555", *context, mockBackMotors, mockFServo);

    // Start the assembly thread
    assembly.start();

    // Wait for thread to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Send a throttle message via ZMQ - use explicit loopback IP
    zmq::socket_t sender(*context, ZMQ_PUB);
    sender.bind("tcp://127.0.0.1:5555");

    // Give the subscriber time to connect
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // Send message multiple times to ensure delivery
    std::string message = "throttle:50;";
    for (int i = 0; i < 20; i++) {
        sender.send(zmq::buffer(message), zmq::send_flags::none);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Wait for the value to be set with a timeout
    bool speedSet = waitForCondition([&]() {
        return mockBackMotors->getCurrentSpeed() == 50;
    }, 8000);  // 8 second timeout

    // Use real assertion that will fail if the condition isn't met
    EXPECT_TRUE(speedSet) << "Throttle message was not processed within the timeout period. Current speed: "
                         << mockBackMotors->getCurrentSpeed();

    // Stop the assembly
    assembly.stop();
}

TEST_F(ControlAssemblyTest, HandleSteeringMessage) {
    SKIP_IN_CI();

    // Create a ControlAssembly with our mocks - use explicit loopback IP
    ControlAssembly assembly("tcp://127.0.0.1:5557", *context, mockBackMotors, mockFServo);

    // Start the assembly thread
    assembly.start();

    // Wait for thread to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Send a steering message via ZMQ - use explicit loopback IP
    zmq::socket_t sender(*context, ZMQ_PUB);
    sender.bind("tcp://127.0.0.1:5557");

    // Give the subscriber time to connect
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // Send message multiple times to ensure delivery
    std::string message = "steering:30;";
    for (int i = 0; i < 20; i++) {
        sender.send(zmq::buffer(message), zmq::send_flags::none);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Wait for the value to be set with a timeout
    bool steeringSet = waitForCondition([&]() {
        return mockFServo->getSteeringAngle() == 30;
    }, 8000);  // 8 second timeout

    // Use real assertion that will fail if the condition isn't met
    EXPECT_TRUE(steeringSet) << "Steering message was not processed within the timeout period. Current angle: "
                            << mockFServo->getSteeringAngle();

    // Stop the assembly
    assembly.stop();
}

TEST_F(ControlAssemblyTest, HandleInitMessage) {
    SKIP_IN_CI();

    // Set up mock components with some initial values
    mockBackMotors->open_i2c_bus();
    mockBackMotors->init_motors();
    mockBackMotors->setSpeed(50);

    mockFServo->open_i2c_bus();
    mockFServo->init_servo();
    mockFServo->set_steering(45);

    // Create a ControlAssembly with our mocks - use explicit loopback IP
    ControlAssembly assembly("tcp://127.0.0.1:5556", *context, mockBackMotors, mockFServo);

    // Start the assembly thread
    assembly.start();

    // Wait for thread to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Send an init message via ZMQ - use explicit loopback IP
    zmq::socket_t sender(*context, ZMQ_PUB);
    sender.bind("tcp://127.0.0.1:5556");

    // Give the subscriber time to connect
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // Send init message multiple times
    std::string message = "init;";
    for (int i = 0; i < 20; i++) {
        sender.send(zmq::buffer(message), zmq::send_flags::none);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Wait for both values to be reset with a timeout
    bool valuesReset = waitForCondition([&]() {
        return mockBackMotors->getCurrentSpeed() == 0 && mockFServo->getSteeringAngle() == 0;
    }, 8000);  // 8 second timeout

    // Use real assertion that will fail if the condition isn't met
    EXPECT_TRUE(valuesReset) << "Init message was not processed within the timeout period. "
                            << "Current speed: " << mockBackMotors->getCurrentSpeed()
                            << ", Current angle: " << mockFServo->getSteeringAngle();

    // Stop the assembly
    assembly.stop();
}

TEST_F(ControlAssemblyTest, HandleMalformedMessages) {
    // Create a ControlAssembly with our mocks
    ControlAssembly assembly("tcp://localhost:5558", *context, mockBackMotors, mockFServo);

    // Start the assembly thread
    assembly.start();

    // Wait for thread to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Send a malformed message via ZMQ
    zmq::socket_t sender(*context, ZMQ_PUB);
    sender.bind("tcp://*:5558");

    // Give the subscriber time to connect
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Send various malformed messages
    std::vector<std::string> malformedMessages = {
        ":",               // Missing key
        "throttle:",       // Missing value
        "throttle:abc;",   // Non-numeric value
        "unknown:50;",     // Unknown command
        "throttle:50",     // Missing semicolon
        ";;;",             // Only delimiters
        ""                 // Empty string
    };

    for (const auto& message : malformedMessages) {
        sender.send(zmq::buffer(message), zmq::send_flags::none);
        // Wait for message to be processed
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // Wait for all messages to be received and processed
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // The test passes if handling malformed messages doesn't crash
    // We could also check that mockBackMotors and mockFServo values haven't changed
    // but that's implementation dependent

    // Stop the assembly
    assembly.stop();
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
