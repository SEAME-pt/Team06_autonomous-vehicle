#include <gtest/gtest.h>
#include "Battery.hpp"
#include "MockBatteryReader.hpp"
#include <memory>

class BatteryTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockReader = std::make_shared<MockBatteryReader>();
        battery = std::make_shared<Battery>(mockReader);
    }

    std::shared_ptr<MockBatteryReader> mockReader;
    std::shared_ptr<Battery> battery;
};

TEST_F(BatteryTest, InitialState) {
    auto sensorData = battery->getSensorData();
    ASSERT_TRUE(sensorData.find("battery") != sensorData.end());
    ASSERT_TRUE(sensorData.find("charging") != sensorData.end());
    EXPECT_EQ(sensorData["battery"]->value.load(), 0);
    EXPECT_EQ(sensorData["charging"]->value.load(), 0);
}

TEST_F(BatteryTest, UpdateSensorData) {
    // Setup mock values
    mockReader->setPercentage(75);
    mockReader->setCharging(true);

    // Update sensor data
    battery->updateSensorData();

    // Check values were updated
    auto sensorData = battery->getSensorData();
    EXPECT_EQ(sensorData["battery"]->value.load(), 75);
    EXPECT_EQ(sensorData["charging"]->value.load(), 1);
    EXPECT_TRUE(sensorData["battery"]->updated.load());
    EXPECT_TRUE(sensorData["charging"]->updated.load());

    // Check that the old values were properly stored
    EXPECT_EQ(sensorData["battery"]->oldValue.load(), 0);
    EXPECT_EQ(sensorData["charging"]->oldValue.load(), 0);
}

TEST_F(BatteryTest, ChargingStateChange) {
    // Initial state
    mockReader->setPercentage(50);
    mockReader->setCharging(false);
    battery->updateSensorData();

    // Simulate charging
    mockReader->setPercentage(55);
    mockReader->setCharging(true);
    battery->updateSensorData();

    auto sensorData = battery->getSensorData();
    EXPECT_EQ(sensorData["battery"]->value.load(), 55);
    EXPECT_EQ(sensorData["charging"]->value.load(), 1);

    // Check that the old values were updated properly
    EXPECT_EQ(sensorData["battery"]->oldValue.load(), 50);
    EXPECT_EQ(sensorData["charging"]->oldValue.load(), 0);
}

TEST_F(BatteryTest, BatteryDecreasesWhenNotCharging) {
    // Initial state with charging
    mockReader->setPercentage(80);
    mockReader->setCharging(true);
    battery->updateSensorData();

    // Now not charging and battery decreases
    mockReader->setPercentage(75);
    mockReader->setCharging(false);
    battery->updateSensorData();

    auto sensorData = battery->getSensorData();
    EXPECT_EQ(sensorData["battery"]->value.load(), 75);
    EXPECT_EQ(sensorData["charging"]->value.load(), 0);
}

TEST_F(BatteryTest, BatteryMaxValue) {
    // Test with maximum battery level
    mockReader->setPercentage(100);
    mockReader->setCharging(true);
    battery->updateSensorData();

    auto sensorData = battery->getSensorData();
    EXPECT_EQ(sensorData["battery"]->value.load(), 100);
}

TEST_F(BatteryTest, BatteryMinValue) {
    // Test with minimum battery level
    mockReader->setPercentage(0);
    mockReader->setCharging(false);
    battery->updateSensorData();

    auto sensorData = battery->getSensorData();
    EXPECT_EQ(sensorData["battery"]->value.load(), 0);
}

TEST_F(BatteryTest, GetName) {
    // Test the getName method
    EXPECT_EQ(battery->getName(), "battery");
}

TEST_F(BatteryTest, GetChargingState) {
    // Test the getCharging method
    mockReader->setCharging(false);
    battery->updateSensorData();
    EXPECT_FALSE(battery->getCharging());

    mockReader->setCharging(true);
    battery->updateSensorData();
    EXPECT_TRUE(battery->getCharging());
}

TEST_F(BatteryTest, MultipleUpdates) {
    // Test multiple sequential updates
    int expected_value = 0; // Start at 0 as per initial state

    for (int i = 10; i <= 50; i += 10) {
        bool is_charging = (i % 20 == 0); // Alternate charging state
        mockReader->setPercentage(i);
        mockReader->setCharging(is_charging);
        battery->updateSensorData();

        // On first update (when expected_value is 0), the battery value is set directly from mockReader
        if (expected_value == 0) {
            expected_value = i;
        } else {
            // Update expected value based on charging behavior
            if (is_charging) {
                // When charging, value only increases
                expected_value = (i > expected_value) ? i : expected_value;
            } else {
                // When not charging, value only decreases
                expected_value = (i < expected_value) ? i : expected_value;
            }
        }

        auto sensorData = battery->getSensorData();
        EXPECT_EQ(sensorData["battery"]->value.load(), expected_value);
        EXPECT_EQ(sensorData["charging"]->value.load(), is_charging ? 1 : 0);
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
