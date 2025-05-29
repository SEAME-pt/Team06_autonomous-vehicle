#include <gtest/gtest.h>
#include "BatteryReader.hpp"
#include <memory>

class BatteryReaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create BatteryReader in test mode
        batteryReader = std::make_unique<BatteryReader>(true);
    }

    void TearDown() override {
        batteryReader.reset();
    }

    std::unique_ptr<BatteryReader> batteryReader;
};

TEST_F(BatteryReaderTest, TestModeInitialization) {
    // Verify the reader is in test mode
    EXPECT_TRUE(batteryReader->isInTestMode());
}

TEST_F(BatteryReaderTest, VoltageCalculation) {
    // Set test values for voltage calculation
    // ADC uses register 0x02 for voltage
    batteryReader->setTestAdcValue(0x02, 3000);

    // Calculate the expected voltage based on the formula in BatteryReader::getVoltage
    // voltage = adc_value * 0.004V
    float expected_voltage = 3000 * 0.004f;

    // Get the voltage and check it matches expected value
    float voltage = batteryReader->getVoltage();
    EXPECT_NEAR(voltage, expected_voltage, 0.1f);
}

TEST_F(BatteryReaderTest, PercentageCalculation) {
    // Set test values for voltage and shunt
    batteryReader->setTestAdcValue(0x02, 3000); // 12V
    batteryReader->setTestAdcValue(0x01, 0);    // No shunt voltage

    // Get the percentage and check it's in expected range
    unsigned int percentage = batteryReader->getPercentage();
    EXPECT_GE(percentage, 1u);
    EXPECT_LE(percentage, 100u);
}

TEST_F(BatteryReaderTest, ChargingDetection) {
    // Test not charging state
    batteryReader->setTestChargeValue(0); // Value 0 means not charging
    EXPECT_FALSE(batteryReader->isCharging());

    // Test charging state (value between 0 and 255)
    batteryReader->setTestChargeValue(100);
    EXPECT_TRUE(batteryReader->isCharging());

    // Test value outside of charging range
    batteryReader->setTestChargeValue(255);
    EXPECT_FALSE(batteryReader->isCharging());
}

TEST_F(BatteryReaderTest, ShuntVoltage) {
    // Shunt voltage register is 0x01
    batteryReader->setTestAdcValue(0x01, 1000);

    // Calculate expected shunt voltage based on the formula in BatteryReader::getShunt
    // shunt = adc_value * 0.00001V
    float expected_shunt = 1000 * 0.00001f;

    // Get the shunt voltage and check it matches expected value
    float shunt = batteryReader->getShunt();
    EXPECT_NEAR(shunt, expected_shunt, 0.001f);
}

TEST_F(BatteryReaderTest, BoundaryPercentages) {
    // Test minimum voltage
    float min_voltage = batteryReader->MIN_VOLTAGE;
    int min_adc = static_cast<int>(min_voltage / 0.004f);
    batteryReader->setTestAdcValue(0x02, min_adc);
    EXPECT_EQ(batteryReader->getPercentage(), 1u); // Should return minimum 1%

    // Test slightly above minimum
    batteryReader->setTestAdcValue(0x02, min_adc + 50);
    EXPECT_GT(batteryReader->getPercentage(), 1u);

    // Test maximum voltage
    float max_voltage = batteryReader->MAX_VOLTAGE;
    int max_adc = static_cast<int>(max_voltage / 0.004f);
    batteryReader->setTestAdcValue(0x02, max_adc);
    EXPECT_EQ(batteryReader->getPercentage(), 100u); // Should return maximum 100%

    // Test over maximum
    batteryReader->setTestAdcValue(0x02, max_adc + 100);
    EXPECT_EQ(batteryReader->getPercentage(), 100u); // Should be clamped to 100%
}

TEST_F(BatteryReaderTest, ErrorHandling) {
    // Test behavior when ADC gives invalid or out-of-range values

    // Set an extreme value for voltage
    batteryReader->setTestAdcValue(0x02, 10000); // Far above normal range

    // The getter should handle this gracefully, capping at max_voltage
    unsigned int percentage = batteryReader->getPercentage();
    EXPECT_LE(percentage, 100u); // Should be capped at 100%

    // Set an extremely low value for voltage
    batteryReader->setTestAdcValue(0x02, 0); // Zero voltage

    // The getter should handle this gracefully, flooring at min_voltage
    percentage = batteryReader->getPercentage();
    EXPECT_GE(percentage, 1u); // Should be at least 1%

    // Test negative shunt value
    // BatteryReader should validate and never return negative shunt values
    batteryReader->setTestAdcValue(0x01, -1000); // Negative value
    float shunt = batteryReader->getShunt();
    EXPECT_GE(shunt, 0.0f); // Should never return negative values
    EXPECT_FLOAT_EQ(shunt, 0.0f); // Should specifically return 0.0f for negative values
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
