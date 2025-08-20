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

TEST_F(SpeedTest, NoDataWhenCanReaderReturnsNoData) {
    // Set up mock to not return data
    mockCanReader->setShouldReceive(false);

    // Update sensor data
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

    // Odo should be updated but will be 0 on first update (no time elapsed)
    EXPECT_FALSE(sensorData["odo"]->updated.load());

    // Check that old values were properly stored
    EXPECT_EQ(sensorData["speed"]->oldValue.load(), 0);
}

TEST_F(SpeedTest, CalculateOdoOverTime) {
    // Set up mock to return data
    std::vector<uint8_t> canData = {100, 0, 0, 0, 0, 0, 0, 0}; // Speed = 100 km/h
    mockCanReader->setReceiveData(canData);
    mockCanReader->setShouldReceive(true);
    mockCanReader->setCanId(0x100);

    // First update to set initial speed
    speed->updateSensorData();

    // Wait a bit to simulate time passing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Change speed to a different value for the second update
    std::vector<uint8_t> newCanData = {110, 0, 0, 0, 0, 0, 0, 0}; // Speed = 110 km/h
    mockCanReader->setReceiveData(newCanData);

    // Second update to calculate odo
    speed->updateSensorData();

    // Check values
    auto sensorData = speed->getSensorData();
    EXPECT_EQ(sensorData["speed"]->value.load(), 110); // Now expecting 110 instead of 100
    EXPECT_TRUE(sensorData["speed"]->updated.load());

    // Odo should be updated now
    EXPECT_GT(sensorData["odo"]->value.load(), 0);
    EXPECT_TRUE(sensorData["odo"]->updated.load());

    // Check that old values were properly stored
    EXPECT_EQ(sensorData["speed"]->oldValue.load(), 100);
}

TEST_F(SpeedTest, IgnoreInvalidCanId) {
    // Set up mock with wrong CAN ID
    std::vector<uint8_t> canData = {100, 0, 0, 0, 0, 0, 0, 0};
    mockCanReader->setReceiveData(canData);
    mockCanReader->setShouldReceive(true);
    mockCanReader->setCanId(0x200); // Different from expected 0x100

    // Update sensor data
    speed->updateSensorData();

    // Check values remain unchanged
    auto sensorData = speed->getSensorData();
    EXPECT_EQ(sensorData["speed"]->value.load(), 0);
    EXPECT_FALSE(sensorData["speed"]->updated.load());
}

TEST_F(SpeedTest, HandleHigherSpeedValues) {
    // Set up mock with 16-bit speed value
    std::vector<uint8_t> canData = {0x34, 0x12, 0, 0, 0, 0, 0, 0}; // Speed = 0x1234 = 4660
    mockCanReader->setReceiveData(canData);
    mockCanReader->setShouldReceive(true);
    mockCanReader->setCanId(0x100);

    // Update sensor data
    speed->updateSensorData();

    // Check values were updated correctly
    auto sensorData = speed->getSensorData();
    EXPECT_EQ(sensorData["speed"]->value.load(), 0x1234);
    EXPECT_TRUE(sensorData["speed"]->updated.load());
}

TEST_F(SpeedTest, ZeroSpeed) {
    // First set non-zero speed
    std::vector<uint8_t> canData = {100, 0, 0, 0, 0, 0, 0, 0};
    mockCanReader->setReceiveData(canData);
    mockCanReader->setShouldReceive(true);
    mockCanReader->setCanId(0x100);
    speed->updateSensorData();

    // Now set zero speed
    std::vector<uint8_t> zeroData = {0, 0, 0, 0, 0, 0, 0, 0};
    mockCanReader->setReceiveData(zeroData);
    speed->updateSensorData();

    // Check values
    auto sensorData = speed->getSensorData();
    EXPECT_EQ(sensorData["speed"]->value.load(), 0);
    EXPECT_EQ(sensorData["speed"]->oldValue.load(), 100);
}

TEST_F(SpeedTest, LongDistanceOdo) {
    // Set a high speed to accumulate distance quickly
    std::vector<uint8_t> canData = {200, 0, 0, 0, 0, 0, 0, 0}; // 200 km/h
    mockCanReader->setReceiveData(canData);
    mockCanReader->setShouldReceive(true);
    mockCanReader->setCanId(0x100);

    // First update
    speed->updateSensorData();

    // Wait longer to accumulate more distance
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Second update
    speed->updateSensorData();

    // Check odo increased significantly
    auto sensorData = speed->getSensorData();
    EXPECT_GT(sensorData["odo"]->value.load(), 0);
    EXPECT_TRUE(sensorData["odo"]->updated.load());

    // Save current odo
    unsigned int firstOdo = sensorData["odo"]->value.load();

    // Wait again and update
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    speed->updateSensorData();

    // Check odo increased further
    sensorData = speed->getSensorData();
    EXPECT_GT(sensorData["odo"]->value.load(), firstOdo);
}

TEST_F(SpeedTest, ConstructorWithNullCanReader) {
    // Test that the constructor handles a null CanReader by creating a default one
    auto speedWithNullReader = std::make_shared<Speed>(nullptr);

    // Should still have valid sensor data
    auto sensorData = speedWithNullReader->getSensorData();
    ASSERT_TRUE(sensorData.find("speed") != sensorData.end());
    ASSERT_TRUE(sensorData.find("odo") != sensorData.end());
}

TEST_F(SpeedTest, MultipleSequentialUpdates) {
    // Set up mock
    mockCanReader->setShouldReceive(true);
    mockCanReader->setCanId(0x100);

    // First update to initialize timestamp
    std::vector<uint8_t> initialData = {10, 0, 0, 0, 0, 0, 0, 0};
    mockCanReader->setReceiveData(initialData);
    speed->updateSensorData();

    // Wait longer between updates to ensure odo changes
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Perform multiple updates with increasing speeds
    for (int i = 20; i <= 100; i += 20) {
        // Higher speeds and longer delays increase chance of odo change
        std::vector<uint8_t> canData = {static_cast<uint8_t>(i), 0, 0, 0, 0, 0, 0, 0};
        mockCanReader->setReceiveData(canData);
        speed->updateSensorData();

        auto sensorData = speed->getSensorData();
        EXPECT_EQ(sensorData["speed"]->value.load(), i);
        EXPECT_TRUE(sensorData["speed"]->updated.load());

        // Longer delay to ensure odo calculation has enough time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // After all updates, odo should be non-zero
    auto sensorData = speed->getSensorData();
    EXPECT_GT(sensorData["odo"]->value.load(), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
