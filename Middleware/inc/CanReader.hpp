#ifndef CANREADER_HPP
#define CANREADER_HPP

#include "MockCanReader.hpp" // Include the interface definition
#include <fcntl.h>
#include <iostream>
#include <linux/spi/spidev.h>
#include <map>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <cstdint>

// MCP2515 Register definitions - these are now defined as static constexpr in the source file
// within the MCP2515Constants namespace to avoid macro conflicts

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
