#include <gtest/gtest.h>
#include "CanMessageBus.hpp"
#include "Distance.hpp"
#include "Speed.hpp"
#include <thread>
#include <chrono>

class CanMessageBusTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Start CAN bus in test mode
        auto& bus = CanMessageBus::getInstance();
        ASSERT_TRUE(bus.start(true)); // true = test mode
    }

    void TearDown() override {
        auto& bus = CanMessageBus::getInstance();
        bus.stop();
    }
};

TEST_F(CanMessageBusTest, DistanceSensorReceivesMessages) {
    auto distance_sensor = std::make_shared<Distance>();
    distance_sensor->start();

    // Create test distance message (100 cm)
    uint8_t test_data[8] = {100, 0, 0, 0, 0, 0, 0, 0}; // 100 cm in little endian
    CanMessage test_message(0x101, test_data, 8);

    // Inject test message
    auto& bus = CanMessageBus::getInstance();
    bus.injectTestMessage(test_message);

    // Allow time for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Update sensor data
    distance_sensor->updateSensorData();

    // Verify sensor received the message
    auto sensor_data = distance_sensor->getSensorData();
    ASSERT_TRUE(sensor_data.find("distance") != sensor_data.end());
    EXPECT_EQ(sensor_data["distance"]->value.load(), 100); // 100 cm
    EXPECT_TRUE(sensor_data["distance"]->updated.load());

    distance_sensor->stop();
}

TEST_F(CanMessageBusTest, SpeedSensorCalculatesSpeedFromPulses) {
    auto speed_sensor = std::make_shared<Speed>();
    speed_sensor->start();

    // Create test speed message (18 pulses in interval, 36 total pulses)
    uint8_t test_data[8] = {18, 0, 36, 0, 0, 0, 0, 0}; // little endian format
    CanMessage test_message(0x100, test_data, 8);

    // Inject test message
    auto& bus = CanMessageBus::getInstance();
    bus.injectTestMessage(test_message);

    // Allow time for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Update sensor data
    speed_sensor->updateSensorData();

    // Verify sensor received and processed the message
    auto sensor_data = speed_sensor->getSensorData();
    ASSERT_TRUE(sensor_data.find("speed") != sensor_data.end());
    ASSERT_TRUE(sensor_data.find("odo") != sensor_data.end());

    // Speed should be calculated (exact value depends on timing)
    EXPECT_TRUE(sensor_data["speed"]->updated.load());
    EXPECT_TRUE(sensor_data["odo"]->updated.load());

    speed_sensor->stop();
}

TEST_F(CanMessageBusTest, MultipleSensorsReceiveDifferentMessages) {
    auto distance_sensor = std::make_shared<Distance>();
    auto speed_sensor = std::make_shared<Speed>();

    distance_sensor->start();
    speed_sensor->start();

    // Create test messages for both sensors
    uint8_t distance_data[8] = {150, 0, 0, 0, 0, 0, 0, 0}; // 150 cm
    uint8_t speed_data[8] = {9, 0, 18, 0, 0, 0, 0, 0}; // 9 pulses, 18 total

    CanMessage distance_message(0x101, distance_data, 8);
    CanMessage speed_message(0x100, speed_data, 8);

    // Inject both messages
    auto& bus = CanMessageBus::getInstance();
    bus.injectTestMessage(distance_message);
    bus.injectTestMessage(speed_message);

    // Allow time for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Update both sensors
    distance_sensor->updateSensorData();
    speed_sensor->updateSensorData();

    // Verify both sensors received their respective messages
    auto distance_data_result = distance_sensor->getSensorData();
    auto speed_data_result = speed_sensor->getSensorData();

    EXPECT_EQ(distance_data_result["distance"]->value.load(), 150);
    EXPECT_TRUE(distance_data_result["distance"]->updated.load());

    EXPECT_TRUE(speed_data_result["speed"]->updated.load());
    EXPECT_TRUE(speed_data_result["odo"]->updated.load());

    distance_sensor->stop();
    speed_sensor->stop();
}

TEST_F(CanMessageBusTest, SensorUnsubscriptionWorks) {
    auto distance_sensor = std::make_shared<Distance>();
    distance_sensor->start();

    // Send a message
    uint8_t test_data[8] = {200, 0, 0, 0, 0, 0, 0, 0};
    CanMessage test_message(0x101, test_data, 8);

    auto& bus = CanMessageBus::getInstance();
    bus.injectTestMessage(test_message);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    distance_sensor->updateSensorData();

    // Verify first message received
    auto sensor_data = distance_sensor->getSensorData();
    EXPECT_EQ(sensor_data["distance"]->value.load(), 200);

    // Stop the sensor (unsubscribe)
    distance_sensor->stop();

    // Send another message
    uint8_t test_data2[8] = {300, 1, 0, 0, 0, 0, 0, 0}; // 300 + 256 = 556 cm
    CanMessage test_message2(0x101, test_data2, 8);
    bus.injectTestMessage(test_message2);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    distance_sensor->updateSensorData();

    // Verify second message was NOT received (still 200)
    sensor_data = distance_sensor->getSensorData();
    EXPECT_EQ(sensor_data["distance"]->value.load(), 200); // Should still be 200
}
