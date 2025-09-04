#include <gtest/gtest.h>
#include "FServo.hpp"
#include "MockFServo.hpp"
#include <memory>
#include <stdexcept>

class FServoAdvancedTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockServo = std::make_shared<MockFServo>();
    }

    void TearDown() override {
        mockServo.reset();
    }

    std::shared_ptr<MockFServo> mockServo;
};

TEST_F(FServoAdvancedTest, SteeringAngleClamping) {
    // Initialize the servo
    mockServo->open_i2c_bus();
    mockServo->init_servo();

    // Test angle clamping to -90 to +90 range
    // The real implementation should clamp angles to _maxAngle (90)

    // Test extreme negative angle
    mockServo->set_steering(-1000);
    // The real implementation would clamp this to -90
    // Our mock stores the actual value, so we test the PWM calculation
    auto pwm = mockServo->getServoPwm(0);
    // For -1000, the PWM should be calculated as if it were -90
    EXPECT_EQ(pwm.second, 170); // Minimum PWM for maximum left

    // Test extreme positive angle
    mockServo->set_steering(1000);
    // The real implementation would clamp this to 90
    auto pwm2 = mockServo->getServoPwm(0);
    // For 1000, the PWM should be calculated as if it were 90
    EXPECT_EQ(pwm2.second, 470); // Maximum PWM for maximum right
}

TEST_F(FServoAdvancedTest, PWMCalculationAccuracy) {
    // Initialize the servo
    mockServo->open_i2c_bus();
    mockServo->init_servo();

    // Test PWM calculation for various angles
    // Using the servo configuration: center=320, left=170, right=470

    struct TestCase {
        int angle;
        int expectedPwm;
    };

    TestCase testCases[] = {
        {-90, 170},   // Maximum left
        {-45, 245},   // Half left (320 - 75)
        {0, 320},     // Center
        {45, 395},    // Half right (320 + 75)
        {90, 470}     // Maximum right
    };

    for (const auto& tc : testCases) {
        mockServo->set_steering(tc.angle);
        auto pwm = mockServo->getServoPwm(0);
        EXPECT_EQ(pwm.second, tc.expectedPwm) << "Angle: " << tc.angle;
    }
}

TEST_F(FServoAdvancedTest, ServoPWMOnOffValues) {
    // Initialize the servo
    mockServo->open_i2c_bus();
    mockServo->init_servo();

    // Test setServoPwm with various on/off values
    const int channel = 0;

    struct TestCase {
        int onValue;
        int offValue;
    };

    TestCase testCases[] = {
        {0, 100},
        {50, 200},
        {100, 300},
        {200, 400},
        {300, 500}
    };

    for (const auto& tc : testCases) {
        bool result = mockServo->setServoPwm(channel, tc.onValue, tc.offValue);
        EXPECT_TRUE(result);

        auto pwm = mockServo->getServoPwm(channel);
        EXPECT_EQ(pwm.first, tc.onValue);
        EXPECT_EQ(pwm.second, tc.offValue);
    }
}

TEST_F(FServoAdvancedTest, ErrorHandlingInSetServoPwm) {
    // Initialize the servo
    mockServo->open_i2c_bus();
    mockServo->init_servo();

    // Test error handling in setServoPwm
    mockServo->setSimulateI2cFailure(true);

    bool result = mockServo->setServoPwm(0, 0, 300);
    EXPECT_FALSE(result);

    // Reset failure simulation
    mockServo->setSimulateI2cFailure(false);

    result = mockServo->setServoPwm(0, 0, 300);
    EXPECT_TRUE(result);
}

TEST_F(FServoAdvancedTest, ErrorHandlingInInitServo) {
    // Test error handling in init_servo
    mockServo->open_i2c_bus();

    // Simulate I2C failure during initialization
    mockServo->setSimulateI2cFailure(true);

    bool result = mockServo->init_servo();
    EXPECT_FALSE(result);

    // Reset failure simulation
    mockServo->setSimulateI2cFailure(false);

    result = mockServo->init_servo();
    EXPECT_TRUE(result);
}

TEST_F(FServoAdvancedTest, RegisterReadWriteOperations) {
    // Initialize the servo
    mockServo->open_i2c_bus();
    mockServo->init_servo();

    int fd = mockServo->getFd();

    // Test register write operations
    uint8_t testReg = 0x20;
    uint8_t testValue = 0xCD;

    mockServo->writeByteData(fd, testReg, testValue);

    // Test register read operations
    uint8_t readValue = mockServo->readByteData(fd, testReg);
    EXPECT_EQ(readValue, testValue);

    // Test multiple register operations
    for (uint8_t reg = 0x00; reg < 0x20; reg++) {
        uint8_t value = reg * 3;
        mockServo->writeByteData(fd, reg, value);
        uint8_t readBack = mockServo->readByteData(fd, reg);
        EXPECT_EQ(readBack, value);
    }
}

TEST_F(FServoAdvancedTest, ServoInitializationSequence) {
    // Test the complete initialization sequence
    mockServo->open_i2c_bus();
    EXPECT_TRUE(mockServo->isI2cOpened());

    // Clear registers to test initialization
    mockServo->clearRegisters();

    // Run initialization
    bool result = mockServo->init_servo();
    EXPECT_TRUE(result);
    EXPECT_TRUE(mockServo->isInitialized());

    // Verify specific register values set during initialization
    // These would be set by the real init_servo() function
    EXPECT_EQ(mockServo->getRegisterValue(0x00), 0x20); // Auto-increment enabled
    EXPECT_EQ(mockServo->getRegisterValue(0xFE), 0x79); // PWM frequency ~50Hz
    EXPECT_EQ(mockServo->getRegisterValue(0x01), 0x04); // MODE2 setting
}

TEST_F(FServoAdvancedTest, SteeringAnglePrecision) {
    // Initialize the servo
    mockServo->open_i2c_bus();
    mockServo->init_servo();

    // Test steering angle precision with small increments
    for (int angle = -90; angle <= 90; angle += 5) {
        mockServo->set_steering(angle);
        EXPECT_EQ(mockServo->getSteeringAngle(), angle);

        // Calculate expected PWM value
        int expectedPwm;
        if (angle < 0) {
            // For negative angle (left): PWM = center + (angle * (center - left) / 90)
            expectedPwm = 320 + (angle * (320 - 170) / 90);
        } else if (angle > 0) {
            // For positive angle (right): PWM = center + (angle * (right - center) / 90)
            expectedPwm = 320 + (angle * (470 - 320) / 90);
        } else {
            // Center
            expectedPwm = 320;
        }

        auto pwm = mockServo->getServoPwm(0);
        EXPECT_NEAR(pwm.second, expectedPwm, 2) << "Angle: " << angle;
    }
}

TEST_F(FServoAdvancedTest, MultipleChannelPWM) {
    // Initialize the servo
    mockServo->open_i2c_bus();
    mockServo->init_servo();

    // Test PWM setting on multiple channels
    const int channels[] = {0, 1, 2, 3};
    const int onValues[] = {0, 50, 100, 150};
    const int offValues[] = {200, 250, 300, 350};

    for (size_t i = 0; i < 4; i++) {
        bool result = mockServo->setServoPwm(channels[i], onValues[i], offValues[i]);
        EXPECT_TRUE(result);

        auto pwm = mockServo->getServoPwm(channels[i]);
        EXPECT_EQ(pwm.first, onValues[i]);
        EXPECT_EQ(pwm.second, offValues[i]);
    }
}

TEST_F(FServoAdvancedTest, FileDescriptorOperations) {
    // Test file descriptor operations
    int testFd = 42;
    mockServo->setFd(testFd);
    EXPECT_EQ(mockServo->getFd(), testFd);

    // Test with different file descriptor
    int anotherFd = 99;
    mockServo->setFd(anotherFd);
    EXPECT_EQ(mockServo->getFd(), anotherFd);
}

TEST_F(FServoAdvancedTest, DestructorBehavior) {
    // Test destructor behavior
    auto tempServo = std::make_shared<MockFServo>();

    // Set up the servo
    tempServo->open_i2c_bus();
    tempServo->init_servo();
    tempServo->set_steering(45);

    // Store the file descriptor
    int fd = tempServo->getFd();
    EXPECT_GT(fd, 0);

    // Explicitly reset to trigger destructor
    tempServo.reset();

    // The destructor should have been called
    // In a real implementation, this would close the file descriptor
}

TEST_F(FServoAdvancedTest, SteeringAngleBoundaryConditions) {
    // Initialize the servo
    mockServo->open_i2c_bus();
    mockServo->init_servo();

    // Test exact boundary values
    mockServo->set_steering(-90);
    EXPECT_EQ(mockServo->getSteeringAngle(), -90);
    auto pwm = mockServo->getServoPwm(0);
    EXPECT_EQ(pwm.second, 170);

    mockServo->set_steering(90);
    EXPECT_EQ(mockServo->getSteeringAngle(), 90);
    pwm = mockServo->getServoPwm(0);
    EXPECT_EQ(pwm.second, 470);

    mockServo->set_steering(0);
    EXPECT_EQ(mockServo->getSteeringAngle(), 0);
    pwm = mockServo->getServoPwm(0);
    EXPECT_EQ(pwm.second, 320);
}

TEST_F(FServoAdvancedTest, ComprehensiveAngleRange) {
    // Initialize the servo
    mockServo->open_i2c_bus();
    mockServo->init_servo();

    // Test comprehensive angle range
    std::vector<int> testAngles = {-90, -60, -30, 0, 30, 60, 90};

    for (int angle : testAngles) {
        mockServo->set_steering(angle);
        EXPECT_EQ(mockServo->getSteeringAngle(), angle);

        // Calculate expected PWM value
        int expectedPwm;
        if (angle < 0) {
            expectedPwm = 320 + (angle * (320 - 170) / 90);
        } else if (angle > 0) {
            expectedPwm = 320 + (angle * (470 - 320) / 90);
        } else {
            expectedPwm = 320;
        }

        auto pwm = mockServo->getServoPwm(0);
        EXPECT_NEAR(pwm.second, expectedPwm, 2) << "Angle: " << angle;
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
