#include <gtest/gtest.h>
#include "Distance.hpp"
#include "CanMessageBus.hpp"
#include <memory>
#include <chrono>
#include <thread>

class DistanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Start CAN bus in test mode
        auto& bus = CanMessageBus::getInstance();
        bus.start(true); // true = test mode

        distance = std::make_shared<Distance>();
        distance->start(); // Subscribe to CAN messages
    }

    void TearDown() override {
        if (distance) {
            distance->stop();
        }
        auto& bus = CanMessageBus::getInstance();
        bus.stop();
    }

    std::shared_ptr<Distance> distance;
};

TEST_F(DistanceTest, InitialState) {
    auto sensorData = distance->getSensorData();
    ASSERT_TRUE(sensorData.find("distance") != sensorData.end());
    EXPECT_EQ(sensorData["distance"]->value.load(), 0);
}

TEST_F(DistanceTest, GetName) {
    // Test the getName method
    EXPECT_EQ(distance->getName(), "distance");
}

TEST_F(DistanceTest, NoDataWhenCanReaderReturnsNoData) {
    // Set up mock to not return data
    mockCanReader->setShouldReceive(false);

    // Update sensor data
    distance->updateSensorData();

    // Check values remain unchanged
    auto sensorData = distance->getSensorData();
    EXPECT_EQ(sensorData["distance"]->value.load(), 0);
    EXPECT_FALSE(sensorData["distance"]->updated.load());
}

TEST_F(DistanceTest, UpdateDistanceFromCanData) {
    // Set up mock to return data
    std::vector<uint8_t> canData = {100, 0, 0, 0, 0, 0, 0, 0}; // Distance = 100 cm
    mockCanReader->setReceiveData(canData);
    mockCanReader->setShouldReceive(true);
    mockCanReader->setCanId(0x101); // Match the expected CAN ID for distance

    // Update sensor data
    distance->updateSensorData();

    // Check values were updated
    auto sensorData = distance->getSensorData();
    EXPECT_EQ(sensorData["distance"]->value.load(), 100);
    EXPECT_TRUE(sensorData["distance"]->updated.load());

    // Check that old values were properly stored
    EXPECT_EQ(sensorData["distance"]->oldValue.load(), 0);
}

TEST_F(DistanceTest, IgnoreInvalidCanId) {
    // Set up mock with wrong CAN ID
    std::vector<uint8_t> canData = {100, 0, 0, 0, 0, 0, 0, 0};
    mockCanReader->setReceiveData(canData);
    mockCanReader->setShouldReceive(true);
    mockCanReader->setCanId(0x200); // Different from expected 0x101

    // Update sensor data
    distance->updateSensorData();

    // Check values remain unchanged
    auto sensorData = distance->getSensorData();
    EXPECT_EQ(sensorData["distance"]->value.load(), 0);
    EXPECT_FALSE(sensorData["distance"]->updated.load());
}

TEST_F(DistanceTest, HandleHigherDistanceValues) {
    // Set up mock with 16-bit distance value
    std::vector<uint8_t> canData = {0x34, 0x12, 0, 0, 0, 0, 0, 0}; // Distance = 0x1234 = 4660 cm
    mockCanReader->setReceiveData(canData);
    mockCanReader->setShouldReceive(true);
    mockCanReader->setCanId(0x101);

    // Update sensor data
    distance->updateSensorData();

    // Check values were updated correctly
    auto sensorData = distance->getSensorData();
    EXPECT_EQ(sensorData["distance"]->value.load(), 0x1234);
    EXPECT_TRUE(sensorData["distance"]->updated.load());
}

TEST_F(DistanceTest, ZeroDistance) {
    // First set non-zero distance
    std::vector<uint8_t> canData = {100, 0, 0, 0, 0, 0, 0, 0};
    mockCanReader->setReceiveData(canData);
    mockCanReader->setShouldReceive(true);
    mockCanReader->setCanId(0x101);
    distance->updateSensorData();

    // Now set zero distance
    std::vector<uint8_t> zeroData = {0, 0, 0, 0, 0, 0, 0, 0};
    mockCanReader->setReceiveData(zeroData);
    distance->updateSensorData();

    // Check values
    auto sensorData = distance->getSensorData();
    EXPECT_EQ(sensorData["distance"]->value.load(), 0);
    EXPECT_EQ(sensorData["distance"]->oldValue.load(), 100);
}
