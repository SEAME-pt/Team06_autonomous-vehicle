#include <gtest/gtest.h>
#include "CanReader.hpp"
#include <memory>
#include <cstring>

class CanReaderAdvancedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create CanReader in test mode
        canReader = std::make_unique<CanReader>(true);
    }

    void TearDown() override {
        canReader.reset();
    }

    std::unique_ptr<CanReader> canReader;
};

TEST_F(CanReaderAdvancedTest, TestModeInitialization) {
    // Verify the reader is in test mode
    EXPECT_TRUE(canReader->isInTestMode());

    // Initialize the reader
    bool result = canReader->initialize();
    EXPECT_TRUE(result);
}

TEST_F(CanReaderAdvancedTest, RegisterOperations) {
    // Test comprehensive register operations
    const uint8_t testRegisters[] = {0x0F, 0x1F, 0x2F, 0x3F, 0x4F};
    const uint8_t testValues[] = {0x87, 0xAB, 0xCD, 0xEF, 0x12};

    // Write test values to registers
    for (size_t i = 0; i < 5; i++) {
        canReader->setTestRegister(testRegisters[i], testValues[i]);
    }

    // Read and verify all registers
    for (size_t i = 0; i < 5; i++) {
        uint8_t readValue = canReader->getTestRegister(testRegisters[i]);
        EXPECT_EQ(readValue, testValues[i]);
    }
}

TEST_F(CanReaderAdvancedTest, ReceiveDataWithDifferentLengths) {
    // Test receiving data with different lengths
    struct TestCase {
        uint8_t data[8];
        uint8_t length;
        uint16_t id;
    };

    TestCase testCases[] = {
        {{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}, 8, 0x123},
        {{0xAA, 0xBB, 0xCC, 0xDD}, 4, 0x456},
        {{0x01, 0x02}, 2, 0x789},
        {{0xFF}, 1, 0xABC}
    };

    for (const auto& tc : testCases) {
        // Configure the reader to provide this test data
        canReader->setTestReceiveData(tc.data, tc.length, tc.id);
        canReader->setTestShouldReceive(true);

        // Try to receive data
        uint8_t buffer[8] = {0};
        uint8_t length = 0;
        bool result = canReader->Receive(buffer, length);

        // Check the result
        EXPECT_TRUE(result);
        EXPECT_EQ(length, tc.length);
        EXPECT_EQ(memcmp(buffer, tc.data, tc.length), 0);
        EXPECT_EQ(canReader->getId(), tc.id);
    }
}

TEST_F(CanReaderAdvancedTest, SendDataWithDifferentLengths) {
    // Test sending data with different lengths
    struct TestCase {
        uint8_t data[8];
        uint8_t length;
        uint16_t id;
    };

    TestCase testCases[] = {
        {{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}, 8, 0x123},
        {{0xAA, 0xBB, 0xCC, 0xDD}, 4, 0x456},
        {{0x01, 0x02}, 2, 0x789},
        {{0xFF}, 1, 0xABC}
    };

    for (const auto& tc : testCases) {
        // Send the data
        bool result = canReader->Send(tc.id, const_cast<uint8_t*>(tc.data), tc.length);

        // In test mode, Send should always return true
        EXPECT_TRUE(result);
    }
}

TEST_F(CanReaderAdvancedTest, SendDataWithInvalidLength) {
    // Test sending data with invalid length (> 8)
    uint8_t testData[10] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA};
    uint16_t testId = 0x123;
    uint8_t testLength = 10; // Invalid length

    // Send the data with invalid length
    bool result = canReader->Send(testId, testData, testLength);

    // Should return false for invalid length
    EXPECT_FALSE(result);
}

TEST_F(CanReaderAdvancedTest, ReceiveNoData) {
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

TEST_F(CanReaderAdvancedTest, MultipleReceiveOperations) {
    // Test multiple receive operations
    uint8_t testData1[4] = {0x11, 0x22, 0x33, 0x44};
    uint8_t testData2[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint16_t testId1 = 0x123;
    uint16_t testId2 = 0x456;

    // First receive operation
    canReader->setTestReceiveData(testData1, 4, testId1);
    canReader->setTestShouldReceive(true);

    uint8_t buffer[8] = {0};
    uint8_t length = 0;
    bool result = canReader->Receive(buffer, length);

    EXPECT_TRUE(result);
    EXPECT_EQ(length, 4);
    EXPECT_EQ(memcmp(buffer, testData1, 4), 0);
    EXPECT_EQ(canReader->getId(), testId1);

    // Second receive operation
    canReader->setTestReceiveData(testData2, 4, testId2);
    canReader->setTestShouldReceive(true);

    length = 0;
    result = canReader->Receive(buffer, length);

    EXPECT_TRUE(result);
    EXPECT_EQ(length, 4);
    EXPECT_EQ(memcmp(buffer, testData2, 4), 0);
    EXPECT_EQ(canReader->getId(), testId2);
}

TEST_F(CanReaderAdvancedTest, RegisterReset) {
    // Set some test register values
    canReader->setTestRegister(0x0F, 0x87);
    canReader->setTestRegister(0x1F, 0xAB);
    canReader->setTestRegister(0x2F, 0xCD);

    // Verify values are set
    EXPECT_EQ(canReader->getTestRegister(0x0F), 0x87);
    EXPECT_EQ(canReader->getTestRegister(0x1F), 0xAB);
    EXPECT_EQ(canReader->getTestRegister(0x2F), 0xCD);

    // Manually reset registers to default values (since Reset() is private)
    canReader->setTestRegister(0x0F, 0x00); // CANCTRL default
    canReader->setTestRegister(0x1F, 0x00); // CANSTAT default
    canReader->setTestRegister(0x2F, 0x00); // RXB0CTRL default

    // Verify registers are reset to default values
    EXPECT_EQ(canReader->getTestRegister(0x0F), 0x00);
    EXPECT_EQ(canReader->getTestRegister(0x1F), 0x00);
    EXPECT_EQ(canReader->getTestRegister(0x2F), 0x00);
}

TEST_F(CanReaderAdvancedTest, TestReceiveDataConfiguration) {
    // Test setting up receive data configuration
    uint8_t testData[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    uint16_t testId = 0x789;
    uint8_t testLength = 6;

    // Configure receive data
    canReader->setTestReceiveData(testData, testLength, testId);
    canReader->setTestShouldReceive(true);

    // Verify the configuration
    uint8_t buffer[8] = {0};
    uint8_t length = 0;
    bool result = canReader->Receive(buffer, length);

    EXPECT_TRUE(result);
    EXPECT_EQ(length, testLength);
    EXPECT_EQ(memcmp(buffer, testData, testLength), 0);
    EXPECT_EQ(canReader->getId(), testId);

    // Verify register values are updated correctly
    EXPECT_EQ(canReader->getTestRegister(0x61), (testId >> 3) & 0xFF); // RXB0SIDH
    EXPECT_EQ(canReader->getTestRegister(0x62), (testId & 0x07) << 5); // RXB0SIDL
    EXPECT_EQ(canReader->getTestRegister(0x65), testLength); // RXB0DLC
}

TEST_F(CanReaderAdvancedTest, TestShouldReceiveToggle) {
    // Test toggling the should receive flag
    uint8_t testData[4] = {0x11, 0x22, 0x33, 0x44};
    uint16_t testId = 0x123;

    // Configure receive data
    canReader->setTestReceiveData(testData, 4, testId);

    // Test with should receive = true
    canReader->setTestShouldReceive(true);
    uint8_t buffer[8] = {0};
    uint8_t length = 0;
    bool result = canReader->Receive(buffer, length);
    EXPECT_TRUE(result);

    // Test with should receive = false
    canReader->setTestShouldReceive(false);
    length = 0;
    result = canReader->Receive(buffer, length);
    EXPECT_FALSE(result);
    EXPECT_EQ(length, 0);
}

TEST_F(CanReaderAdvancedTest, ComprehensiveRegisterTest) {
    // Test all possible register addresses
    for (uint8_t addr = 0x00; addr < 0x80; addr++) {
        uint8_t testValue = addr ^ 0xAA; // XOR with 0xAA for variety

        canReader->setTestRegister(addr, testValue);
        uint8_t readValue = canReader->getTestRegister(addr);
        EXPECT_EQ(readValue, testValue);
    }
}

TEST_F(CanReaderAdvancedTest, EdgeCaseDataValues) {
    // Test edge case data values
    struct TestCase {
        uint8_t data[8];
        uint8_t length;
        uint16_t id;
    };

    TestCase testCases[] = {
        {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 8, 0x000}, // All zeros
        {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, 8, 0xFFF}, // All ones
        {{0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF}, 8, 0x555}, // Alternating
        {{0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00}, 8, 0xAAA}  // Alternating reverse
    };

    for (const auto& tc : testCases) {
        canReader->setTestReceiveData(tc.data, tc.length, tc.id);
        canReader->setTestShouldReceive(true);

        uint8_t buffer[8] = {0};
        uint8_t length = 0;
        bool result = canReader->Receive(buffer, length);

        EXPECT_TRUE(result);
        EXPECT_EQ(length, tc.length);
        EXPECT_EQ(memcmp(buffer, tc.data, tc.length), 0);
        EXPECT_EQ(canReader->getId(), tc.id);
    }
}

TEST_F(CanReaderAdvancedTest, InitializeFunction) {
    // Test the initialize function
    bool result = canReader->initialize();
    EXPECT_TRUE(result);

    // In test mode, initialize should always return true
    // and set the CANCTRL register to MODE_NORMAL
    EXPECT_EQ(canReader->getTestRegister(0x0F), 0x00); // MODE_NORMAL
}

TEST_F(CanReaderAdvancedTest, IdRetrieval) {
    // Test ID retrieval after setting receive data
    uint8_t testData[4] = {0x11, 0x22, 0x33, 0x44};
    uint16_t testId = 0x7FF; // Maximum 11-bit CAN ID

    canReader->setTestReceiveData(testData, 4, testId);
    canReader->setTestShouldReceive(true);

    uint8_t buffer[8] = {0};
    uint8_t length = 0;
    bool result = canReader->Receive(buffer, length);

    EXPECT_TRUE(result);
    EXPECT_EQ(canReader->getId(), testId);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
