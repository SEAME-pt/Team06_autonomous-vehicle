#include <gtest/gtest.h>
#include "Battery.hpp"
#include <memory>

class BatteryTest : public ::testing::Test {
protected:
    void SetUp() override {
        battery = std::make_unique<Battery>();
    }

    void TearDown() override {
        battery.reset();
    }

    std::unique_ptr<Battery> battery;
};

TEST_F(BatteryTest, InitialState) {
    auto sensorData = battery->getSensorData();

    // Check if all expected sensors are present
    EXPECT_NE(sensorData.find("battery"), sensorData.end());
    EXPECT_NE(sensorData.find("charging"), sensorData.end());
    EXPECT_NE(sensorData.find("power"), sensorData.end());

    // Check initial values
    EXPECT_EQ(sensorData["battery"]->value.load(), 0);
    EXPECT_EQ(sensorData["charging"]->value.load(), 0);
    EXPECT_EQ(sensorData["power"]->value.load(), 0);
}

TEST_F(BatteryTest, UpdateSensorData) {
    // Set up initial state
    auto sensorData = battery->getSensorData();
    sensorData["battery"]->value.store(50);
    sensorData["charging"]->value.store(1);

    // Update sensor data
    battery->updateSensorData();

    // Check if values were updated
    auto updatedData = battery->getSensorData();
    EXPECT_TRUE(updatedData["battery"]->updated.load());
    EXPECT_TRUE(updatedData["charging"]->updated.load());
}

TEST_F(BatteryTest, PowerValue) {
    battery->updateSensorData();
    auto sensorData = battery->getSensorData();
    EXPECT_EQ(sensorData["power"]->value.load(), 20);
}
