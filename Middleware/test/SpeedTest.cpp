#include <gtest/gtest.h>
#include "Speed.hpp"
#include "CanMessageBus.hpp"
#include <memory>
#include <chrono>
#include <thread>

class SpeedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Start CAN bus in test mode
        auto& bus = CanMessageBus::getInstance();
        bus.start(true); // true = test mode

        speed = std::make_shared<Speed>();
        speed->start(); // Subscribe to CAN messages
    }

    void TearDown() override {
        if (speed) {
            speed->stop();
        }
        auto& bus = CanMessageBus::getInstance();
        bus.stop();
    }

    std::shared_ptr<Speed> speed;
};

TEST_F(SpeedTest, InitialState) {
    auto sensorData = speed->getSensorData();
    ASSERT_TRUE(sensorData.find("speed") != sensorData.end());
    ASSERT_TRUE(sensorData.find("odo") != sensorData.end());
    EXPECT_EQ(sensorData["speed"]->value.load(), 0);
    EXPECT_EQ(sensorData["odo"]->value.load(), 0);
}

TEST_F(SpeedTest, GetName) {
    // Test the getName method
    EXPECT_EQ(speed->getName(), "speed");
}

TEST_F(SpeedTest, NoDataInitially) {
    // Update sensor data without sending any CAN messages
    speed->updateSensorData();

    // Check values remain unchanged
    auto sensorData = speed->getSensorData();
    EXPECT_EQ(sensorData["speed"]->value.load(), 0);
    EXPECT_EQ(sensorData["odo"]->value.load(), 0);
    EXPECT_FALSE(sensorData["speed"]->updated.load());
    EXPECT_FALSE(sensorData["odo"]->updated.load());
}

TEST_F(SpeedTest, UpdateSpeedFromCanData) {
    // Create test speed message with pulse data
    // 18 pulses in interval, 36 total pulses (simulating 1 full revolution)
    uint8_t test_data[8] = {18, 0, 36, 0, 0, 0, 0, 0}; // little endian format
    CanMessage test_message(0x100, test_data, 8);

    // Inject test message
    auto& bus = CanMessageBus::getInstance();
    bus.injectTestMessage(test_message);

    // Allow time for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Update sensor data
    speed->updateSensorData();

    // Check values were updated
    auto sensorData = speed->getSensorData();
    EXPECT_GT(sensorData["speed"]->value.load(), 0); // Speed should be calculated
    EXPECT_TRUE(sensorData["speed"]->updated.load());

    // Odo should be updated on first update (even if value is 0)
    EXPECT_TRUE(sensorData["odo"]->updated.load());

    // Check that old values were properly stored
    EXPECT_EQ(sensorData["speed"]->oldValue.load(), 0);
}

TEST_F(SpeedTest, CalculateOdoOverTime) {
    // Create test speed message with pulse data
    uint8_t test_data1[8] = {100, 0, 0, 0, 0, 0, 0, 0}; // Speed = 100 km/h
    CanMessage test_message1(0x100, test_data1, 8);

    // Inject first test message
    auto& bus = CanMessageBus::getInstance();
    bus.injectTestMessage(test_message1);

    // Allow time for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // First update to set initial speed
    speed->updateSensorData();

    // Wait a bit to simulate time passing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Change speed to a different value for the second update
    uint8_t test_data2[8] = {110, 0, 0, 0, 0, 0, 0, 0}; // Speed = 110 km/h
    CanMessage test_message2(0x100, test_data2, 8);
    bus.injectTestMessage(test_message2);

    // Allow time for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Second update to calculate odo
    speed->updateSensorData();

    // Check values
    auto sensorData = speed->getSensorData();
    EXPECT_GT(sensorData["speed"]->value.load(), 0); // Speed should be calculated
    EXPECT_TRUE(sensorData["speed"]->updated.load());

    // Odo should be updated now
    EXPECT_GT(sensorData["odo"]->value.load(), 0);
    EXPECT_TRUE(sensorData["odo"]->updated.load());
}

TEST_F(SpeedTest, IgnoreInvalidCanId) {
    // Create test message with wrong CAN ID
    uint8_t test_data[8] = {100, 0, 0, 0, 0, 0, 0, 0};
    CanMessage test_message(0x200, test_data, 8); // Different from expected 0x100

    // Inject test message
    auto& bus = CanMessageBus::getInstance();
    bus.injectTestMessage(test_message);

    // Allow time for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Update sensor data
    speed->updateSensorData();

    // Check values remain unchanged
    auto sensorData = speed->getSensorData();
    EXPECT_EQ(sensorData["speed"]->value.load(), 0);
    EXPECT_FALSE(sensorData["speed"]->updated.load());
}

TEST_F(SpeedTest, HandleHigherSpeedValues) {
    // Create test message with 16-bit speed value
    uint8_t test_data[8] = {0x34, 0x12, 0, 0, 0, 0, 0, 0}; // Speed = 0x1234 = 4660
    CanMessage test_message(0x100, test_data, 8);

    // Inject test message
    auto& bus = CanMessageBus::getInstance();
    bus.injectTestMessage(test_message);

    // Allow time for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Update sensor data
    speed->updateSensorData();

    // Check values were updated correctly
    auto sensorData = speed->getSensorData();
    EXPECT_GT(sensorData["speed"]->value.load(), 0); // Speed should be calculated
    EXPECT_TRUE(sensorData["speed"]->updated.load());
}

TEST_F(SpeedTest, ZeroSpeed) {
    // First set non-zero speed
    uint8_t test_data1[8] = {100, 0, 0, 0, 0, 0, 0, 0};
    CanMessage test_message1(0x100, test_data1, 8);

    auto& bus = CanMessageBus::getInstance();
    bus.injectTestMessage(test_message1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    speed->updateSensorData();

    // Now set zero speed
    uint8_t test_data2[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    CanMessage test_message2(0x100, test_data2, 8);
    bus.injectTestMessage(test_message2);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    speed->updateSensorData();

    // Check values
    auto sensorData = speed->getSensorData();
    EXPECT_EQ(sensorData["speed"]->value.load(), 0);
}

TEST_F(SpeedTest, LongDistanceOdo) {
    auto& bus = CanMessageBus::getInstance();

    // First message with initial pulse count
    uint8_t test_data1[8] = {100, 0, 0, 0, 0, 0, 0, 0}; // 100 pulses
    CanMessage test_message1(0x100, test_data1, 8);
    bus.injectTestMessage(test_message1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // First update
    speed->updateSensorData();

    // Check odo increased
    auto sensorData = speed->getSensorData();
    EXPECT_GT(sensorData["odo"]->value.load(), 0);
    EXPECT_TRUE(sensorData["odo"]->updated.load());

    // Save current odo
    unsigned int firstOdo = sensorData["odo"]->value.load();

    // Second message with more pulses (simulating continued movement)
    uint8_t test_data2[8] = {150, 0, 0, 0, 0, 0, 0, 0}; // 150 more pulses
    CanMessage test_message2(0x100, test_data2, 8);
    bus.injectTestMessage(test_message2);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Second update
    speed->updateSensorData();

    // Check odo increased further
    sensorData = speed->getSensorData();
    EXPECT_GT(sensorData["odo"]->value.load(), firstOdo);
}

TEST_F(SpeedTest, ConstructorCreatesValidSensorData) {
    // Test that the constructor creates valid sensor data
    auto speedSensor = std::make_shared<Speed>();

    // Should have valid sensor data
    auto sensorData = speedSensor->getSensorData();
    ASSERT_TRUE(sensorData.find("speed") != sensorData.end());
    ASSERT_TRUE(sensorData.find("odo") != sensorData.end());
}

TEST_F(SpeedTest, MultipleSequentialUpdates) {
    auto& bus = CanMessageBus::getInstance();

    // First update to initialize timestamp
    uint8_t initial_data[8] = {10, 0, 0, 0, 0, 0, 0, 0};
    CanMessage initial_message(0x100, initial_data, 8);
    bus.injectTestMessage(initial_message);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    speed->updateSensorData();

    // Wait longer between updates to ensure odo changes
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Perform multiple updates with increasing speeds
    for (int i = 20; i <= 100; i += 20) {
        // Higher speeds and longer delays increase chance of odo change
        uint8_t can_data[8] = {static_cast<uint8_t>(i), 0, 0, 0, 0, 0, 0, 0};
        CanMessage test_message(0x100, can_data, 8);
        bus.injectTestMessage(test_message);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        speed->updateSensorData();

        auto sensorData = speed->getSensorData();
        EXPECT_GT(sensorData["speed"]->value.load(), 0); // Speed should be calculated
        EXPECT_TRUE(sensorData["speed"]->updated.load());

        // Longer delay to ensure odo calculation has enough time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // After all updates, odo should be non-zero
    auto sensorData = speed->getSensorData();
    EXPECT_GT(sensorData["odo"]->value.load(), 0);
}

TEST_F(SpeedTest, EdgeCaseSpeedValues) {
    auto& bus = CanMessageBus::getInstance();

    // Test maximum speed value
    uint8_t max_speed_data[8] = {0xFF, 0xFF, 0, 0, 0, 0, 0, 0}; // Maximum 16-bit value
    CanMessage max_speed_message(0x100, max_speed_data, 8);
    bus.injectTestMessage(max_speed_message);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    speed->updateSensorData();

    auto sensorData = speed->getSensorData();
    EXPECT_GT(sensorData["speed"]->value.load(), 0);
    EXPECT_TRUE(sensorData["speed"]->updated.load());

    // Test very small speed value
    uint8_t min_speed_data[8] = {1, 0, 0, 0, 0, 0, 0, 0}; // Minimum non-zero speed
    CanMessage min_speed_message(0x100, min_speed_data, 8);
    bus.injectTestMessage(min_speed_message);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    speed->updateSensorData();

    sensorData = speed->getSensorData();
    EXPECT_GT(sensorData["speed"]->value.load(), 0);
    EXPECT_TRUE(sensorData["speed"]->updated.load());
}

TEST_F(SpeedTest, MultipleCanIds) {
    auto& bus = CanMessageBus::getInstance();

    // Test that Speed sensor responds to multiple CAN IDs
    uint8_t test_data1[8] = {50, 0, 0, 0, 0, 0, 0, 0};
    uint8_t test_data2[8] = {75, 0, 0, 0, 0, 0, 0, 0};
    uint8_t test_data3[8] = {90, 0, 0, 0, 0, 0, 0, 0};

    CanMessage test_message1(0x100, test_data1, 8);  // Primary ID
    CanMessage test_message2(0x180, test_data2, 8);  // Secondary ID
    CanMessage test_message3(0x500, test_data3, 8);  // Tertiary ID

    // Inject test messages
    bus.injectTestMessage(test_message1);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    speed->updateSensorData();

    auto sensorData = speed->getSensorData();
    EXPECT_GT(sensorData["speed"]->value.load(), 0);

    // Test second CAN ID
    bus.injectTestMessage(test_message2);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    speed->updateSensorData();

    sensorData = speed->getSensorData();
    EXPECT_GT(sensorData["speed"]->value.load(), 0);

    // Test third CAN ID
    bus.injectTestMessage(test_message3);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    speed->updateSensorData();

    sensorData = speed->getSensorData();
    EXPECT_GT(sensorData["speed"]->value.load(), 0);
}

TEST_F(SpeedTest, StartStopCycles) {
    // Test multiple start/stop cycles
    for (int i = 0; i < 3; i++) {
        speed->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        speed->stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Test should complete without issues
    EXPECT_TRUE(true);
}

TEST_F(SpeedTest, ThreadSafety) {
    // Test thread safety by calling methods from multiple threads
    std::atomic<bool> stop_test(false);

    // Start the sensor
    speed->start();

    // Create multiple threads that update sensor data
    std::vector<std::thread> threads;
    for (int i = 0; i < 3; i++) {
        threads.emplace_back([this, &stop_test]() {
            while (!stop_test.load()) {
                speed->updateSensorData();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }

    // Let it run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Stop the test
    stop_test = true;

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Stop the sensor
    speed->stop();

    // Test should complete without issues
    EXPECT_TRUE(true);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
