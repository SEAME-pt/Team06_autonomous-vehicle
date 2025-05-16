#include <gtest/gtest.h>
#include "BatteryReader.hpp"
#include <map>

class BatteryReaderDirectTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a battery reader in test mode
        batteryReader = std::make_unique<BatteryReader>(true);

        // Initialize with default test values
        batteryReader->setTestAdcValue(0x01, 0);     // Shunt register
        batteryReader->setTestAdcValue(0x02, 3000);  // Voltage register (12V)
        batteryReader->setTestChargeValue(0);
    }

    std::unique_ptr<BatteryReader> batteryReader;
};

TEST_F(BatteryReaderDirectTest, CalculateVoltage) {
    // Set mock ADC value for voltage register (0x02)
    // 3000 ADC units * 0.004V = 12.0V
    batteryReader->setTestAdcValue(0x02, 3000);

    // Check if voltage calculation is correct
    EXPECT_FLOAT_EQ(batteryReader->getVoltage(), 12.0f);

    // Set a different value
    // 2500 ADC units * 0.004V = 10.0V
    batteryReader->setTestAdcValue(0x02, 2500);
    EXPECT_FLOAT_EQ(batteryReader->getVoltage(), 10.0f);
}

TEST_F(BatteryReaderDirectTest, CalculateShunt) {
    // Set mock ADC value for shunt register (0x01)
    // 1000 ADC units * 0.00001V = 0.01V (10mV)
    batteryReader->setTestAdcValue(0x01, 1000);

    // Check if shunt calculation is correct
    EXPECT_FLOAT_EQ(batteryReader->getShunt(), 0.01f);

    // Set a different value
    // 2000 ADC units * 0.00001V = 0.02V (20mV)
    batteryReader->setTestAdcValue(0x01, 2000);
    EXPECT_FLOAT_EQ(batteryReader->getShunt(), 0.02f);
}

TEST_F(BatteryReaderDirectTest, CalculatePercentage) {
    // Set values for a battery at ~50% charge
    // Voltage at ~10.8V (halfway between 9.0V and 12.6V)
    batteryReader->setTestAdcValue(0x02, 2700); // 2700 * 0.004 = 10.8V
    batteryReader->setTestAdcValue(0x01, 0);    // No shunt voltage

    // Check if percentage calculation is around 50%
    unsigned int percentage = batteryReader->getPercentage();
    EXPECT_GE(percentage, 49);
    EXPECT_LE(percentage, 51);

    // Set values for a nearly full battery
    batteryReader->setTestAdcValue(0x02, 3125); // 3125 * 0.004 = 12.5V
    percentage = batteryReader->getPercentage();
    EXPECT_GE(percentage, 95);
    EXPECT_LE(percentage, 100);

    // Set values for a nearly empty battery
    batteryReader->setTestAdcValue(0x02, 2300); // 2300 * 0.004 = 9.2V
    percentage = batteryReader->getPercentage();
    EXPECT_GE(percentage, 1);
    EXPECT_LE(percentage, 10);
}

TEST_F(BatteryReaderDirectTest, DetectCharging) {
    // Set charge value to indicate not charging
    batteryReader->setTestChargeValue(0);
    EXPECT_FALSE(batteryReader->isCharging());

    // Set charge value to indicate charging
    batteryReader->setTestChargeValue(1);
    EXPECT_TRUE(batteryReader->isCharging());

    // Test a different positive value
    batteryReader->setTestChargeValue(100);
    EXPECT_TRUE(batteryReader->isCharging());
}

TEST_F(BatteryReaderDirectTest, HandleBatteryLimits) {
    // Test over maximum voltage
    batteryReader->setTestAdcValue(0x02, 3250); // 3250 * 0.004 = 13.0V (above MAX_VOLTAGE=12.6V)
    EXPECT_EQ(batteryReader->getPercentage(), 100);

    // Test under minimum voltage
    batteryReader->setTestAdcValue(0x02, 2000); // 2000 * 0.004 = 8.0V (below MIN_VOLTAGE=9.0V)
    EXPECT_EQ(batteryReader->getPercentage(), 1); // Should clamp to minimum 1%
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
