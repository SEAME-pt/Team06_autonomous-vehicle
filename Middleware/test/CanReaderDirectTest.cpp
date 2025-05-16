#include <gtest/gtest.h>
#include "CanReader.hpp"
#include <map>
#include <cstring>

// CAN register definitions from CanReader.hpp
#define CANSTAT      0x0E
#define CANCTRL      0x0F
#define BFPCTRL      0x0C
#define TEC          0x1C
#define REC          0x1D
#define CNF3         0x28
#define CNF2         0x29
#define CNF1         0x2A
#define CANINTE      0x2B
#define CANINTF      0x2C
#define EFLG         0x2D
#define TXRTSCTRL    0x0D

// Receive Registers
#define RXB0CTRL     0x60
#define RXB0SIDH     0x61
#define RXB0SIDL     0x62
#define RXB0DLC      0x65
#define RXB0D0       0x66

class CanReaderDirectTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a CAN reader in test mode
        canReader = std::make_unique<CanReader>(true);

        // Initialize test registers
        canReader->setTestRegister(CANCTRL, 0);
        canReader->setTestRegister(CANSTAT, 0);
        canReader->setTestRegister(RXB0CTRL, 0);
        canReader->setTestRegister(RXB0SIDH, 0);
        canReader->setTestRegister(RXB0SIDL, 0);
        canReader->setTestRegister(RXB0DLC, 0);
        canReader->setTestRegister(CANINTF, 0);
    }

    std::unique_ptr<CanReader> canReader;
};

TEST_F(CanReaderDirectTest, Initialization) {
    EXPECT_TRUE(canReader->Init());

    // Check if CANCTRL register is set to the correct mode
    EXPECT_EQ(canReader->getTestRegister(CANCTRL), 0x00); // Normal mode
}

TEST_F(CanReaderDirectTest, ReceiveData) {
    // Initialize the CAN reader
    EXPECT_TRUE(canReader->Init());

    // Set up data to receive
    uint8_t testData[8] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    uint8_t testLength = 8;
    uint16_t testId = 0x123;

    canReader->setTestReceiveData(testData, testLength, testId);
    canReader->setTestShouldReceive(true);

    // Try to receive data
    uint8_t receivedBuffer[8];
    uint8_t receivedLength = 0;

    EXPECT_TRUE(canReader->Receive(receivedBuffer, receivedLength));
    EXPECT_EQ(receivedLength, testLength);
    EXPECT_EQ(canReader->getId(), testId);

    // Verify received data
    for (int i = 0; i < testLength; i++) {
        EXPECT_EQ(receivedBuffer[i], testData[i]);
    }
}

TEST_F(CanReaderDirectTest, NoDataToReceive) {
    // Initialize the CAN reader
    EXPECT_TRUE(canReader->Init());

    // Set no data to receive
    canReader->setTestShouldReceive(false);

    // Try to receive data
    uint8_t receivedBuffer[8];
    uint8_t receivedLength = 0;

    EXPECT_FALSE(canReader->Receive(receivedBuffer, receivedLength));
    EXPECT_EQ(receivedLength, 0);
}

TEST_F(CanReaderDirectTest, SendData) {
    // Initialize the CAN reader
    EXPECT_TRUE(canReader->Init());

    // Prepare data to send
    uint8_t testData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    uint8_t testLength = 8;
    uint16_t testId = 0x321;

    // Send data (in test mode this will just return true)
    EXPECT_TRUE(canReader->Send(testId, testData, testLength));
}

TEST_F(CanReaderDirectTest, RegisterReadWrite) {
    // Test the basic read/write operations

    // Set a value in a register
    canReader->setTestRegister(CANCTRL, 0x87);

    // Verify the register value was set
    EXPECT_EQ(canReader->getTestRegister(CANCTRL), 0x87);

    // Change the register value
    canReader->setTestRegister(CANCTRL, 0x45);

    // Verify the change
    EXPECT_EQ(canReader->getTestRegister(CANCTRL), 0x45);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
