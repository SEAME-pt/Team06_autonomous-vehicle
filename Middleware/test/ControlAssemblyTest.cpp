#include <gtest/gtest.h>
#include "ControlAssembly.hpp"
#include "MockBackMotors.hpp"
#include "MockFServo.hpp"
#include <memory>
#include <thread>
#include <chrono>

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
    // Create a ControlAssembly with our mocks
    ControlAssembly assembly("tcp://localhost:5555", *context, mockBackMotors, mockFServo);

    // Start the assembly thread
    assembly.start();

    // Wait for thread to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Send a throttle message via ZMQ
    zmq::socket_t sender(*context, ZMQ_PUB);
    sender.bind("tcp://*:5555");

    // Give the subscriber time to connect
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Send message
    std::string message = "throttle:50;";
    sender.send(zmq::buffer(message), zmq::send_flags::none);

    // Wait for message to be received and processed
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Check that the throttle was set
    EXPECT_EQ(mockBackMotors->getCurrentSpeed(), 50);

    // Stop the assembly
    assembly.stop();
}

TEST_F(ControlAssemblyTest, HandleSteeringMessage) {
    // Create a ControlAssembly with our mocks
    ControlAssembly assembly("tcp://localhost:5557", *context, mockBackMotors, mockFServo);

    // Start the assembly thread
    assembly.start();

    // Wait for thread to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Send a steering message via ZMQ
    zmq::socket_t sender(*context, ZMQ_PUB);
    sender.bind("tcp://*:5557");

    // Give the subscriber time to connect
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Send message
    std::string message = "steering:30;";
    sender.send(zmq::buffer(message), zmq::send_flags::none);

    // Wait for message to be received and processed
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Check that the steering was set
    EXPECT_EQ(mockFServo->getSteeringAngle(), 30);

    // Stop the assembly
    assembly.stop();
}

TEST_F(ControlAssemblyTest, HandleInitMessage) {
    // Set up mock components with some initial values
    mockBackMotors->open_i2c_bus();
    mockBackMotors->init_motors();
    mockBackMotors->setSpeed(50);

    mockFServo->open_i2c_bus();
    mockFServo->init_servo();
    mockFServo->set_steering(45);

    // Create a ControlAssembly with our mocks
    ControlAssembly assembly("tcp://localhost:5556", *context, mockBackMotors, mockFServo);

    // Start the assembly thread
    assembly.start();

    // Wait for thread to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Send an init message via ZMQ
    zmq::socket_t sender(*context, ZMQ_PUB);
    sender.bind("tcp://*:5556");

    // Give the subscriber time to connect
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Send init message
    std::string message = "init;";
    sender.send(zmq::buffer(message), zmq::send_flags::none);

    // Wait for message to be received and processed
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Check that values were reset to 0
    EXPECT_EQ(mockBackMotors->getCurrentSpeed(), 0);
    EXPECT_EQ(mockFServo->getSteeringAngle(), 0);

    // Stop the assembly
    assembly.stop();
}

TEST_F(ControlAssemblyTest, HandleMalformedMessages) {
    // Create a ControlAssembly with our mocks
    ControlAssembly assembly("tcp://localhost:5558", *context, mockBackMotors, mockFServo);

    // Start the assembly thread
    assembly.start();

    // Wait for thread to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Send a malformed message via ZMQ
    zmq::socket_t sender(*context, ZMQ_PUB);
    sender.bind("tcp://*:5558");

    // Give the subscriber time to connect
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

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
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Wait for all messages to be received and processed
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

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
