#ifndef CANREADER_HPP
#define CANREADER_HPP

#include "MockCanReader.hpp" // Include the interface definition
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/spi/spidev.h>
#include <map>
#include <stdexcept>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

// MCP2515 Register definitions
// Control Registers
#define CANSTAT 0x0E
#define CANCTRL 0x0F
#define BFPCTRL 0x0C
#define TEC 0x1C
#define REC 0x1D
#define CNF3 0x28
#define CNF2 0x29
#define CNF1 0x2A
#define CANINTE 0x2B
#define CANINTF 0x2C
#define EFLG 0x2D
#define TXRTSCTRL 0x0D

// Receive Registers
#define RXB0CTRL 0x60
#define RXB0SIDH 0x61
#define RXB0SIDL 0x62
#define RXB0DLC 0x65
#define RXB0D0 0x66

// Transmit Registers
#define TXB0CTRL 0x30
#define TXB0SIDH 0x31
#define TXB0SIDL 0x32
#define TXB0EID8 0x33
#define TXB0EID0 0x34
#define TXB0DLC 0x35
#define TXB0D0 0x36

// Receive Filters
#define RXF0SIDH 0x00
#define RXF0SIDL 0x01
#define RXF1SIDH 0x04
#define RXF1SIDL 0x05

// Receive Masks
#define RXM0SIDH 0x20
#define RXM0SIDL 0x21
#define RXM1SIDH 0x24
#define RXM1SIDL 0x25

// CAN Speed configurations
#define CAN_10Kbps 0x31
#define CAN_25Kbps 0x13
#define CAN_50Kbps 0x09
#define CAN_100Kbps 0x04
#define CAN_125Kbps 0x03
#define CAN_250Kbps 0x01
#define CAN_500Kbps 0x00

// SPI Commands
#define CAN_RESET 0xC0
#define CAN_READ 0x03
#define CAN_WRITE 0x02
#define CAN_RTS 0x80
#define CAN_RTS_TXB0 0x81
#define CAN_RD_STATUS 0xA0
#define CAN_BIT_MODIFY 0x05
#define CAN_READ_RX 0x90

// Operating Modes
#define MODE_NORMAL 0x00
#define MODE_SLEEP 0x20
#define MODE_LOOPBACK 0x40
#define MODE_LISTENONLY 0x60
#define MODE_CONFIG 0x80

// Interrupt Flags
#define RX0IF 0x01
#define RX1IF 0x02
#define TX0IF 0x04
#define TX1IF 0x08
#define TX2IF 0x10
#define ERRIF 0x20
#define WAKIF 0x40
#define MERRF 0x80

// Receive Buffer Operating Modes
#define RXM_FILTER_ANY 0x60
#define RXM_FILTER_STD 0x20
#define RXM_FILTER_EXT 0x40

class CanReader : public ICanReader {
public:
  // Constructor with test_mode parameter
  CanReader(bool test_mode = false);
  ~CanReader() override;

  bool Init() override;
  bool Send(uint16_t canId, uint8_t *data, uint8_t length) override;
  bool Receive(uint8_t *buffer, uint8_t &length) override;
  uint16_t getId() override;

  // Test mode methods
  bool isInTestMode() const { return test_mode; }
  uint8_t setTestRegister(uint8_t addr, uint8_t value);
  uint8_t getTestRegister(uint8_t addr) const;
  void setTestReceiveData(const uint8_t *data, uint8_t length,
                          uint16_t id = 0x100);
  void setTestShouldReceive(bool shouldReceive);

  bool initialize();

private:
  int spi_fd = -1;
  bool debug = false;
  bool test_mode = false;

  // Test mode storage
  std::map<uint8_t, uint8_t> test_registers;
  uint8_t test_receive_data[8] = {0};
  uint8_t test_receive_length = 0;
  uint16_t test_can_id = 0x100;
  bool test_should_receive = false;

  // Hardware access methods
  uint8_t ReadByte(uint8_t addr);
  void WriteByte(uint8_t addr, uint8_t data);
  void Reset();
  bool InitSPI();
};

#endif
