#include <gtest/gtest.h>
#include "CanReader.hpp"
#include <memory>
#include <cstring>

class CanReaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create CanReader in test mode
        canReader = std::make_unique<CanReader>(true);
        canReader->Init(); // Initialize the reader in test mode
    }

    void TearDown() override {
        canReader.reset();
    }

    std::unique_ptr<CanReader> canReader;
};

TEST_F(CanReaderTest, TestModeInitialization) {
    // Verify the reader is in test mode
    EXPECT_TRUE(canReader->isInTestMode());
}

TEST_F(CanReaderTest, RegisterAccess) {
    // Test register write and read in test mode
    uint8_t testAddr = 0x0F; // CANCTRL register
    uint8_t testValue = 0x87;

    // Set a test register value
    canReader->setTestRegister(testAddr, testValue);

    // Read it back and verify
    uint8_t readValue = canReader->getTestRegister(testAddr);
    EXPECT_EQ(readValue, testValue);
}

TEST_F(CanReaderTest, ReceiveData) {
    // Set test data to be received
    uint8_t testData[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    uint16_t testId = 0x123;
    uint8_t testLength = 8;

    // Configure the reader to provide this test data
    canReader->setTestReceiveData(testData, testLength, testId);
    canReader->setTestShouldReceive(true);

    // Try to receive data
    uint8_t buffer[8] = {0};
    uint8_t length = 0;
    bool result = canReader->Receive(buffer, length);

    // Check the result
    EXPECT_TRUE(result);
    EXPECT_EQ(length, testLength);
    EXPECT_EQ(memcmp(buffer, testData, testLength), 0); // Data should match
    EXPECT_EQ(canReader->getId(), testId);
}

TEST_F(CanReaderTest, ReceiveNoData) {
    // Configure the reader to not provide any data
    canReader->setTestShouldReceive(false);

    // Try to receive data
    uint8_t buffer[8] = {0};
    uint8_t length = 0;
    bool result = canReader->Receive(buffer, length);

    // Check that no data was received
    EXPECT_FALSE(result);
    EXPECT_EQ(length, 0);
}

TEST_F(CanReaderTest, SendData) {
    // Test sending data
    uint8_t testData[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};
    uint16_t testId = 0x456;
    uint8_t testLength = 8;

    // Send the data
    bool result = canReader->Send(testId, testData, testLength);

    // In test mode, Send should always return true
    EXPECT_TRUE(result);
}

TEST_F(CanReaderTest, MultipleRegisterOperations) {
    // Test multiple register operations
    for (uint8_t addr = 0; addr < 10; addr++) {
        canReader->setTestRegister(addr, addr * 10);
    }

    // Read and verify all registers
    for (uint8_t addr = 0; addr < 10; addr++) {
        EXPECT_EQ(canReader->getTestRegister(addr), addr * 10);
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
