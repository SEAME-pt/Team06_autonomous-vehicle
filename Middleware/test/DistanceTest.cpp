#include <gtest/gtest.h>
#include "Distance.hpp"
#include "CanMessageBus.hpp"
#include "ISensor.hpp"
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>

class DistanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Start CAN bus in test mode
        auto& bus = CanMessageBus::getInstance();
        bus.start(true); // true = test mode

        distance = std::make_shared<Distance>();
        distance->start(); // Subscribe to CAN messages

        // Reset emergency brake flag
        emergency_brake_called = false;
    }

    void TearDown() override {
        if (distance) {
            distance->stop();
        }
        auto& bus = CanMessageBus::getInstance();
        bus.stop();
    }

    std::shared_ptr<Distance> distance;
    std::atomic<bool> emergency_brake_called{false};

    // Emergency brake callback for testing
    void emergencyBrakeCallback(bool active) {
        emergency_brake_called = active;
    }

    // Mock speed data for testing speed-based thresholds
    class MockSpeedData : public SensorData {
    public:
        MockSpeedData(uint32_t speed_value) : SensorData("speed", true) {
            value.store(speed_value);
        }
    };
};

TEST_F(DistanceTest, InitialState) {
    auto sensorData = distance->getSensorData();
    ASSERT_TRUE(sensorData.find("obs") != sensorData.end());
    EXPECT_EQ(sensorData["obs"]->value.load(), 0);
}

TEST_F(DistanceTest, GetName) {
    // Test the getName method
    EXPECT_EQ(distance->getName(), "distance");
}

TEST_F(DistanceTest, NoDataInitially) {
    // Update sensor data without sending any CAN messages
    distance->updateSensorData();

    // Check values remain unchanged
    auto sensorData = distance->getSensorData();
    EXPECT_EQ(sensorData["obs"]->value.load(), 0);
    EXPECT_FALSE(sensorData["obs"]->updated.load());
}

TEST_F(DistanceTest, UpdateDistanceFromCanData) {
    // Create test distance message (100 cm)
    uint8_t test_data[8] = {100, 0, 0, 0, 0, 0, 0, 0}; // 100 cm in little endian
    CanMessage test_message(0x101, test_data, 8);

    // Inject test message
    auto& bus = CanMessageBus::getInstance();
    bus.injectTestMessage(test_message);

    // Allow time for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Update sensor data
    distance->updateSensorData();

    // Verify sensor received the message
    auto sensor_data = distance->getSensorData();
    ASSERT_TRUE(sensor_data.find("obs") != sensor_data.end());
    EXPECT_EQ(sensor_data["obs"]->value.load(), 0); // 100 cm is safe (risk level 0)
    EXPECT_TRUE(sensor_data["obs"]->updated.load());

    // Check that old values were properly stored
    EXPECT_EQ(sensor_data["obs"]->oldValue.load(), 0);
}

TEST_F(DistanceTest, HandleHigherDistanceValues) {
    // Create test message with 16-bit distance value
    uint8_t test_data[8] = {0x34, 0x12, 0, 0, 0, 0, 0, 0}; // Distance = 0x1234 = 4660 cm
    CanMessage test_message(0x101, test_data, 8);

    // Inject test message
    auto& bus = CanMessageBus::getInstance();
    bus.injectTestMessage(test_message);

    // Allow time for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Update sensor data
    distance->updateSensorData();

    // Check values were updated correctly
    auto sensorData = distance->getSensorData();
    EXPECT_EQ(sensorData["obs"]->value.load(), 0); // 4660 cm is safe (risk level 0)
    EXPECT_TRUE(sensorData["obs"]->updated.load());
}

TEST_F(DistanceTest, ZeroDistance) {
    // First set non-zero distance
    uint8_t test_data1[8] = {100, 0, 0, 0, 0, 0, 0, 0};
    CanMessage test_message1(0x101, test_data1, 8);

    auto& bus = CanMessageBus::getInstance();
    bus.injectTestMessage(test_message1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    distance->updateSensorData();

    // Now set zero distance
    uint8_t test_data2[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    CanMessage test_message2(0x101, test_data2, 8);
    bus.injectTestMessage(test_message2);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    distance->updateSensorData();

    // Check values
    auto sensorData = distance->getSensorData();
    EXPECT_EQ(sensorData["obs"]->value.load(), 0); // 0 cm is safe (risk level 0)
    EXPECT_EQ(sensorData["obs"]->oldValue.load(), 0); // Previous was also safe
}

TEST_F(DistanceTest, EmergencyBrakeCallback) {
    // Set up emergency brake callback
    distance->setEmergencyBrakeCallback([this](bool active) {
        this->emergencyBrakeCallback(active);
    });

    // Create test message with very close distance (10 cm - should trigger emergency brake)
    uint8_t test_data[8] = {10, 0, 0, 0, 0, 0, 0, 0}; // 10 cm
    CanMessage test_message(0x101, test_data, 8);

    // Inject test message
    auto& bus = CanMessageBus::getInstance();
    bus.injectTestMessage(test_message);

    // Allow time for message processing and collision detection
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Update sensor data to trigger collision detection
    distance->updateSensorData();

    // Allow additional time for emergency brake callback
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Verify emergency brake was called
    EXPECT_TRUE(emergency_brake_called.load());
}

TEST_F(DistanceTest, SafeDistanceNoEmergencyBrake) {
    // Set up emergency brake callback
    distance->setEmergencyBrakeCallback([this](bool active) {
        this->emergencyBrakeCallback(active);
    });

    // Create test message with safe distance (150 cm)
    uint8_t test_data[8] = {150, 0, 0, 0, 0, 0, 0, 0}; // 150 cm
    CanMessage test_message(0x101, test_data, 8);

    // Inject test message
    auto& bus = CanMessageBus::getInstance();
    bus.injectTestMessage(test_message);

    // Allow time for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Update sensor data
    distance->updateSensorData();

    // Allow time for potential emergency brake callback
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Verify emergency brake was NOT called
    EXPECT_FALSE(emergency_brake_called.load());
}

TEST_F(DistanceTest, MultipleCanIds) {
    // Test that Distance sensor responds to multiple CAN IDs
    // Create test messages for different CAN IDs
    uint8_t test_data1[8] = {50, 0, 0, 0, 0, 0, 0, 0};
    uint8_t test_data2[8] = {75, 0, 0, 0, 0, 0, 0, 0};
    uint8_t test_data3[8] = {90, 0, 0, 0, 0, 0, 0, 0};

    CanMessage test_message1(0x101, test_data1, 8);  // Primary ID
    CanMessage test_message2(0x181, test_data2, 8);  // Secondary ID
    CanMessage test_message3(0x581, test_data3, 8);  // Tertiary ID

    auto& bus = CanMessageBus::getInstance();

    // Inject test messages
    bus.injectTestMessage(test_message1);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    distance->updateSensorData();

    auto sensorData = distance->getSensorData();
    EXPECT_EQ(sensorData["obs"]->value.load(), 0); // 50 cm is safe (risk level 0)

    // Test second CAN ID
    bus.injectTestMessage(test_message2);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    distance->updateSensorData();

    sensorData = distance->getSensorData();
    EXPECT_EQ(sensorData["obs"]->value.load(), 0); // 75 cm is safe (risk level 0)

    // Test third CAN ID
    bus.injectTestMessage(test_message3);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    distance->updateSensorData();

    sensorData = distance->getSensorData();
    EXPECT_EQ(sensorData["obs"]->value.load(), 0); // 90 cm is safe (risk level 0)
}

// Speed-based distance threshold tests
TEST_F(DistanceTest, SpeedBasedThresholds_LowSpeed) {
    // Set up speed data accessor for low speed (400 mm/s)
    auto mock_speed_data = std::make_shared<MockSpeedData>(400);
    distance->setSpeedDataAccessor([mock_speed_data]() {
        return std::static_pointer_cast<SensorData>(mock_speed_data);
    });

    // Set up emergency brake callback
    distance->setEmergencyBrakeCallback([this](bool active) {
        this->emergencyBrakeCallback(active);
    });

    // Test with 15cm distance - should trigger emergency at low speed (threshold: 20cm)
    uint8_t test_data[8] = {15, 0, 0, 0, 0, 0, 0, 0}; // 15 cm
    CanMessage test_message(0x101, test_data, 8);

    auto& bus = CanMessageBus::getInstance();
    bus.injectTestMessage(test_message);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    distance->updateSensorData();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Should trigger emergency brake at low speed
    EXPECT_TRUE(emergency_brake_called.load());
}

TEST_F(DistanceTest, SpeedBasedThresholds_HighSpeed) {
    // Set up speed data accessor for high speed (2500 mm/s)
    auto mock_speed_data = std::make_shared<MockSpeedData>(2500);
    distance->setSpeedDataAccessor([mock_speed_data]() {
        return std::static_pointer_cast<SensorData>(mock_speed_data);
    });

    // Set up emergency brake callback
    distance->setEmergencyBrakeCallback([this](bool active) {
        this->emergencyBrakeCallback(active);
    });

    // Test with 50cm distance - should trigger emergency at high speed (threshold: 75cm)
    uint8_t test_data[8] = {50, 0, 0, 0, 0, 0, 0, 0}; // 50 cm
    CanMessage test_message(0x101, test_data, 8);

    auto& bus = CanMessageBus::getInstance();
    bus.injectTestMessage(test_message);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    distance->updateSensorData();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Should trigger emergency brake at high speed
    EXPECT_TRUE(emergency_brake_called.load());
}

TEST_F(DistanceTest, SpeedBasedThresholds_MediumSpeed) {
    // Set up speed data accessor for medium speed (1600 mm/s)
    auto mock_speed_data = std::make_shared<MockSpeedData>(1600);
    distance->setSpeedDataAccessor([mock_speed_data]() {
        return std::static_pointer_cast<SensorData>(mock_speed_data);
    });

    // Set up emergency brake callback
    distance->setEmergencyBrakeCallback([this](bool active) {
        this->emergencyBrakeCallback(active);
    });

    // Test with 35cm distance - should trigger emergency at medium speed (threshold: ~39cm)
    uint8_t test_data[8] = {35, 0, 0, 0, 0, 0, 0, 0}; // 35 cm
    CanMessage test_message(0x101, test_data, 8);

    auto& bus = CanMessageBus::getInstance();
    bus.injectTestMessage(test_message);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    distance->updateSensorData();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Should trigger emergency brake at medium speed
    EXPECT_TRUE(emergency_brake_called.load());
}

TEST_F(DistanceTest, SpeedBasedThresholds_SafeAtHighSpeed) {
    // Set up speed data accessor for high speed (2500 mm/s)
    auto mock_speed_data = std::make_shared<MockSpeedData>(2500);
    distance->setSpeedDataAccessor([mock_speed_data]() {
        return std::static_pointer_cast<SensorData>(mock_speed_data);
    });

    // Set up emergency brake callback
    distance->setEmergencyBrakeCallback([this](bool active) {
        this->emergencyBrakeCallback(active);
    });

    // Test with 80cm distance - should be safe at high speed (threshold: 75cm)
    uint8_t test_data[8] = {80, 0, 0, 0, 0, 0, 0, 0}; // 80 cm
    CanMessage test_message(0x101, test_data, 8);

    auto& bus = CanMessageBus::getInstance();
    bus.injectTestMessage(test_message);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    distance->updateSensorData();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Should NOT trigger emergency brake at high speed
    EXPECT_FALSE(emergency_brake_called.load());
}

TEST_F(DistanceTest, SpeedBasedThresholds_NoSpeedData) {
    // Don't set up speed data accessor - should use default thresholds
    distance->setEmergencyBrakeCallback([this](bool active) {
        this->emergencyBrakeCallback(active);
    });

    // Test with 15cm distance - should trigger emergency with default thresholds
    uint8_t test_data[8] = {15, 0, 0, 0, 0, 0, 0, 0}; // 15 cm
    CanMessage test_message(0x101, test_data, 8);

    auto& bus = CanMessageBus::getInstance();
    bus.injectTestMessage(test_message);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    distance->updateSensorData();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Should trigger emergency brake with default thresholds (no speed data = 0 speed = 1.0x multiplier)
    EXPECT_TRUE(emergency_brake_called.load());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
