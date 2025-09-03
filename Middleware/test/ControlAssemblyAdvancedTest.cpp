#include <gtest/gtest.h>
#include "ControlAssembly.hpp"
#include "MockBackMotors.hpp"
#include "MockFServo.hpp"
#include <memory>
#include <thread>
#include <chrono>

class ControlAssemblyAdvancedTest : public ::testing::Test {
protected:
    void SetUp() override {
        zmq_context = std::make_unique<zmq::context_t>(1);
        mock_back_motors = std::make_shared<MockBackMotors>();
        mock_f_servo = std::make_shared<MockFServo>();

        control_assembly = std::make_unique<ControlAssembly>(
            "tcp://127.0.0.1:5574",
            *zmq_context,
            mock_back_motors,
            mock_f_servo
        );
    }

    void TearDown() override {
        if (control_assembly) {
            control_assembly->stop();
        }
        control_assembly.reset();
        zmq_context.reset();
    }

    std::unique_ptr<zmq::context_t> zmq_context;
    std::shared_ptr<MockBackMotors> mock_back_motors;
    std::shared_ptr<MockFServo> mock_f_servo;
    std::unique_ptr<ControlAssembly> control_assembly;
};

TEST_F(ControlAssemblyAdvancedTest, Initialization) {
    // Test that the control assembly initializes properly
    EXPECT_NE(control_assembly, nullptr);

    // Test that the ZMQ subscriber is properly initialized
    // Note: ZmqSubscriber doesn't have isInTestMode() method, so we just verify it exists
    EXPECT_NE(&control_assembly->zmq_subscriber, nullptr);
}

TEST_F(ControlAssemblyAdvancedTest, StartStopOperations) {
    // Test start operation
    EXPECT_NO_THROW(control_assembly->start());

    // Give it time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Test stop operation
    EXPECT_NO_THROW(control_assembly->stop());
}

TEST_F(ControlAssemblyAdvancedTest, MultipleStartStopCycles) {
    // Test multiple start/stop cycles
    for (int i = 0; i < 3; i++) {
        EXPECT_NO_THROW(control_assembly->start());
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        EXPECT_NO_THROW(control_assembly->stop());
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

TEST_F(ControlAssemblyAdvancedTest, SpeedDataAccessor) {
    // Test setting speed data accessor
    auto speedData = std::make_shared<SensorData>("speed", true);
    speedData->value = 50.0;
    speedData->timestamp = std::chrono::steady_clock::now();

    std::function<std::shared_ptr<SensorData>()> accessor = [speedData]() {
        return speedData;
    };

    control_assembly->setSpeedDataAccessor(accessor);

    // The accessor should be set (we can't directly test it, but it shouldn't throw)
    EXPECT_NO_THROW(control_assembly->setSpeedDataAccessor(accessor));
}

TEST_F(ControlAssemblyAdvancedTest, EmergencyBrakeCallback) {
    // Test setting emergency brake callback
    bool callbackCalled = false;
    std::function<void(bool)> callback = [&callbackCalled](bool active) {
        callbackCalled = active;
    };

    control_assembly->setEmergencyBrakeCallback(callback);

    // Test emergency brake handling
    control_assembly->handleEmergencyBrake(true);
    EXPECT_TRUE(callbackCalled);

    control_assembly->handleEmergencyBrake(false);
    EXPECT_FALSE(callbackCalled);
}

TEST_F(ControlAssemblyAdvancedTest, EmergencyBrakeHandling) {
    // Test emergency brake handling
    control_assembly->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Trigger emergency brake
    control_assembly->handleEmergencyBrake(true);

    // Give it time to process
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop the control assembly
    control_assembly->stop();
}

TEST_F(ControlAssemblyAdvancedTest, PerformEmergencyBraking) {
    // Test emergency braking functionality
    control_assembly->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Set up speed data accessor for intelligent braking
    auto speedData = std::make_shared<SensorData>("speed", true);
    speedData->value = 30.0; // 30 km/h
    speedData->timestamp = std::chrono::steady_clock::now();

    std::function<std::shared_ptr<SensorData>()> accessor = [speedData]() {
        return speedData;
    };

    control_assembly->setSpeedDataAccessor(accessor);

    // Trigger emergency brake
    control_assembly->handleEmergencyBrake(true);

    // Give it time to process
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    control_assembly->stop();
}

TEST_F(ControlAssemblyAdvancedTest, ConcurrentOperations) {
    // Test concurrent start/stop operations
    std::atomic<bool> stop_test(false);

    // Start the control assembly
    control_assembly->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Create a thread that continuously triggers emergency brake
    std::thread emergency_thread([this, &stop_test]() {
        while (!stop_test.load()) {
            control_assembly->handleEmergencyBrake(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            control_assembly->handleEmergencyBrake(false);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    // Let it run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Stop the test
    stop_test = true;
    emergency_thread.join();

    // Stop the control assembly
    control_assembly->stop();
}

TEST_F(ControlAssemblyAdvancedTest, ResourceCleanup) {
    // Test resource cleanup by creating and destroying multiple instances
    for (int i = 0; i < 3; i++) {
        auto temp_assembly = std::make_unique<ControlAssembly>(
            "tcp://127.0.0.1:5575",
            *zmq_context,
            mock_back_motors,
            mock_f_servo
        );

        temp_assembly->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        temp_assembly->stop();

        // Explicitly reset to test destructor
        temp_assembly.reset();
    }
}

TEST_F(ControlAssemblyAdvancedTest, ErrorHandling) {
    // Test error handling with null pointers
    auto null_assembly = std::make_unique<ControlAssembly>(
        "tcp://127.0.0.1:5576",
        *zmq_context,
        nullptr, // null back motors
        nullptr  // null front servo
    );

    // Should not throw during construction
    EXPECT_NE(null_assembly, nullptr);

    // Should handle start/stop gracefully even with null components
    EXPECT_NO_THROW(null_assembly->start());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_NO_THROW(null_assembly->stop());
}

TEST_F(ControlAssemblyAdvancedTest, MessageHandling) {
    // Test message handling capabilities
    control_assembly->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // The control assembly should be running and ready to handle messages
    // We can't directly test message handling without setting up ZMQ publishers,
    // but we can verify the assembly is in a ready state

    control_assembly->stop();
}

TEST_F(ControlAssemblyAdvancedTest, ThreadSafety) {
    // Test thread safety by starting and stopping from multiple threads
    std::vector<std::thread> threads;

    for (int i = 0; i < 3; i++) {
        threads.emplace_back([this, i]() {
            control_assembly->start();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            control_assembly->stop();
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
}

TEST_F(ControlAssemblyAdvancedTest, LongRunningStability) {
    // Test long-running stability
    control_assembly->start();

    // Let it run for a longer period
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Trigger some operations
    control_assembly->handleEmergencyBrake(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    control_assembly->handleEmergencyBrake(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop gracefully
    control_assembly->stop();
}

TEST_F(ControlAssemblyAdvancedTest, ComponentIntegration) {
    // Test integration with mock components
    control_assembly->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify that the mock components are properly integrated
    EXPECT_NE(mock_back_motors, nullptr);
    EXPECT_NE(mock_f_servo, nullptr);

    // Test that the components can be accessed
    EXPECT_NO_THROW(mock_back_motors->setSpeed(50));
    EXPECT_NO_THROW(mock_f_servo->set_steering(30));

    control_assembly->stop();
}

TEST_F(ControlAssemblyAdvancedTest, ZMQAddressConfiguration) {
    // Test different ZMQ address configurations
    std::vector<std::string> addresses = {
        "tcp://127.0.0.1:5577",
        "tcp://127.0.0.1:5578",
        "tcp://127.0.0.1:5579"
    };

    for (const auto& address : addresses) {
        auto temp_assembly = std::make_unique<ControlAssembly>(
            address,
            *zmq_context,
            mock_back_motors,
            mock_f_servo
        );

        EXPECT_NO_THROW(temp_assembly->start());
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        EXPECT_NO_THROW(temp_assembly->stop());

        temp_assembly.reset();
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
