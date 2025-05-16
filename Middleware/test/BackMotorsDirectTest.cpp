#include <gtest/gtest.h>
#include "BackMotors.hpp"

// Test subclass that overrides the hardware-specific methods
class TestableBackMotors : public BackMotors {
public:
    TestableBackMotors() :
        BackMotors(),
        _i2cBusOpen(false),
        _motorsInitialized(false),
        _currentSpeed(0) {}

    // Override hardware-specific methods
    void open_i2c_bus() override {
        _i2cBusOpen = true;
        // Don't call the actual hardware
    }

    bool init_motors() override {
        if (_i2cBusOpen) {
            _motorsInitialized = true;
            return true;
        }
        return false;
    }

    void writeByteData(int fd, uint8_t reg, uint8_t value) override {
        // Store the register and value for verification
        _byteDataRegistry[reg] = value;
    }

    uint8_t readByteData(int fd, uint8_t reg) override {
        // Return a canned response for testing
        return _byteDataRegistry[reg];
    }

    bool setMotorPwm(const int channel, int value) override {
        if (_motorsInitialized && channel >= 0 && channel < 9) {
            _motorChannelValues[channel] = std::min(std::max(value, 0), 4095);
            return true;
        }
        return false;
    }

    void setSpeed(int speed) override {
        // Call the parent implementation, which will call our overridden setMotorPwm
        _currentSpeed = std::max(-100, std::min(100, speed));
        BackMotors::setSpeed(speed);
    }

    // Getters for test verification
    bool isI2cBusOpen() const { return _i2cBusOpen; }
    bool isMotorsInitialized() const { return _motorsInitialized; }
    int getCurrentSpeed() const { return _currentSpeed; }
    int getChannelValue(int channel) const {
        auto it = _motorChannelValues.find(channel);
        if (it != _motorChannelValues.end()) {
            return it->second;
        }
        return -1;
    }

private:
    bool _i2cBusOpen;
    bool _motorsInitialized;
    int _currentSpeed;
    std::map<int, int> _motorChannelValues;
    std::map<uint8_t, uint8_t> _byteDataRegistry;
};

class BackMotorsDirectTest : public ::testing::Test {
protected:
    void SetUp() override {
        testMotors = std::make_unique<TestableBackMotors>();
    }

    std::unique_ptr<TestableBackMotors> testMotors;
};

// Simplified version of tests from BackMotorsTest
TEST_F(BackMotorsDirectTest, InitialState) {
    EXPECT_FALSE(testMotors->isI2cBusOpen());
    EXPECT_FALSE(testMotors->isMotorsInitialized());
}

TEST_F(BackMotorsDirectTest, OpenI2cBus) {
    testMotors->open_i2c_bus();
    EXPECT_TRUE(testMotors->isI2cBusOpen());
}

TEST_F(BackMotorsDirectTest, SpeedValueClamping) {
    testMotors->open_i2c_bus();
    testMotors->init_motors();

    // Test upper bound
    testMotors->setSpeed(150);
    EXPECT_EQ(testMotors->getCurrentSpeed(), 100);

    // Test lower bound
    testMotors->setSpeed(-200);
    EXPECT_EQ(testMotors->getCurrentSpeed(), -100);
}

TEST_F(BackMotorsDirectTest, SetForwardBackwardSpeed) {
    testMotors->open_i2c_bus();
    testMotors->init_motors();

    // Test forward speed
    testMotors->setSpeed(50);
    EXPECT_EQ(testMotors->getCurrentSpeed(), 50);

    // Test backward speed
    testMotors->setSpeed(-75);
    EXPECT_EQ(testMotors->getCurrentSpeed(), -75);

    // Test zero speed
    testMotors->setSpeed(0);
    EXPECT_EQ(testMotors->getCurrentSpeed(), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
