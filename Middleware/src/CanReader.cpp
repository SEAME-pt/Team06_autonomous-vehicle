#include "CanReader.hpp"


// MCP2515 Constants (matching the working modules)
namespace MCP2515Constants {
    // Instruction Commands
    static constexpr uint8_t RESET_INSTRUCTION = 0xC0;
    static constexpr uint8_t READ_INSTRUCTION = 0x03;
    static constexpr uint8_t WRITE_INSTRUCTION = 0x02;
    static constexpr uint8_t READ_STATUS_INSTRUCTION = 0xA0;
    static constexpr uint8_t BIT_MODIFY_INSTRUCTION = 0x05;
    static constexpr uint8_t RTS_TXB0_INSTRUCTION = 0x81;

    // Control Registers
    static constexpr uint8_t CANCTRL = 0x0F;
    static constexpr uint8_t CANSTAT = 0x0E;
    static constexpr uint8_t CNF1 = 0x2A;
    static constexpr uint8_t CNF2 = 0x29;
    static constexpr uint8_t CNF3 = 0x28;
    static constexpr uint8_t TXB0CTRL = 0x30;
    static constexpr uint8_t RXB0CTRL = 0x60;
    static constexpr uint8_t RXB1CTRL = 0x70;
    static constexpr uint8_t CANINTE = 0x2B;
    static constexpr uint8_t CANINTF = 0x2C;

    // Receive Buffer 0 Registers
    static constexpr uint8_t RXB0SIDH = 0x61;
    static constexpr uint8_t RXB0SIDL = 0x62;
    static constexpr uint8_t RXB0DLC = 0x65;
    static constexpr uint8_t RXB0D0 = 0x66;

    // Transmit Buffer 0 Registers
    static constexpr uint8_t TXB0SIDH = 0x31;
    static constexpr uint8_t TXB0SIDL = 0x32;
    static constexpr uint8_t TXB0EID8 = 0x33;
    static constexpr uint8_t TXB0EID0 = 0x34;
    static constexpr uint8_t TXB0DLC = 0x35;
    static constexpr uint8_t TXB0D0 = 0x36;

    // Operating Modes
    static constexpr uint8_t MODE_NORMAL = 0x00;
    static constexpr uint8_t MODE_SLEEP = 0x20;
    static constexpr uint8_t MODE_LOOPBACK = 0x40;
    static constexpr uint8_t MODE_LISTENONLY = 0x60;
    static constexpr uint8_t MODE_CONFIG = 0x80;

    // Interrupt Flags
    static constexpr uint8_t RX0IE = 0x01;
    static constexpr uint8_t RX1IE = 0x02;
    static constexpr uint8_t TX0IE = 0x04;
    static constexpr uint8_t TX1IE = 0x08;
    static constexpr uint8_t TX2IE = 0x10;
    static constexpr uint8_t ERRIE = 0x20;
    static constexpr uint8_t WAKIE = 0x40;
    static constexpr uint8_t MERRE = 0x80;

    // Receive Buffer Operating Modes
    static constexpr uint8_t RXM_ALL = 0x60;
    static constexpr uint8_t RXM_VALID_ONLY = 0x00;
}

using namespace MCP2515Constants;

CanReader::CanReader(bool test_mode) : test_mode(test_mode), debug(false) {
  if (!test_mode) {
    try {
      InitSPI();
    } catch (const std::exception &e) {
      std::cerr << "Error initializing CanReader: " << e.what() << std::endl;
    }
  } else {
    // Initialize test mode register defaults
    test_registers[CANCTRL] = MODE_NORMAL;
    test_registers[CANSTAT] = MODE_NORMAL;
    test_registers[RXB0CTRL] = RXM_ALL;
    test_registers[CANINTF] = 0x00;
  }
}

CanReader::~CanReader() {
  if (!test_mode && spi_fd >= 0) {
    close(spi_fd);
  }
}

bool CanReader::InitSPI() {
  if (test_mode) {
    return true;
  }

  spi_fd = open("/dev/spidev0.0", O_RDWR);
  if (spi_fd < 0) {
    throw std::runtime_error("Failed to open SPI device: /dev/spidev0.0");
  }

  try {
    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = 1000000; // 1MHz (matching working modules)

    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0) {
      throw std::runtime_error("Error setting SPI mode");
    }

    if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
      throw std::runtime_error("Error setting bits per word");
    }

    if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
      throw std::runtime_error("Error setting SPI speed");
    }
  } catch (...) {
    close(spi_fd);
    spi_fd = -1;
    throw;
  }

  return true;
}

uint8_t CanReader::ReadByte(uint8_t addr) {
  if (test_mode) {
    auto it = test_registers.find(addr);
    if (it != test_registers.end()) {
      return it->second;
    }
    return 0;
  }

  uint8_t tx[3] = {READ_INSTRUCTION, addr, 0};
  uint8_t rx[3] = {0};

  struct spi_ioc_transfer tr;
  memset(&tr, 0, sizeof(tr));
  tr.tx_buf = (unsigned long)tx;
  tr.rx_buf = (unsigned long)rx;
  tr.len = 3;
  tr.speed_hz = 1000000;
  tr.bits_per_word = 8;
  tr.delay_usecs = 0;

  if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
    std::cerr << "SPI transfer failed" << std::endl;
    return 0;
  }

  return rx[2];
}

void CanReader::WriteByte(uint8_t addr, uint8_t data) {
  if (test_mode) {
    test_registers[addr] = data;
    return;
  }

  uint8_t tx[3] = {WRITE_INSTRUCTION, addr, data};

  struct spi_ioc_transfer tr;
  memset(&tr, 0, sizeof(tr));
  tr.tx_buf = (unsigned long)tx;
  tr.rx_buf = 0;
  tr.len = 3;
  tr.speed_hz = 1000000;
  tr.bits_per_word = 8;
  tr.delay_usecs = 0;

  if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
    std::cerr << "SPI transfer failed" << std::endl;
  }
}

void CanReader::Reset() {
  if (test_mode) {
    // Reset test registers to default values
    test_registers.clear();
    test_registers[CANCTRL] = MODE_NORMAL;
    test_registers[CANSTAT] = MODE_NORMAL;
    test_registers[RXB0CTRL] = RXM_ALL;
    test_registers[CANINTF] = 0x00;
    return;
  }

  uint8_t tx = RESET_INSTRUCTION;

  struct spi_ioc_transfer tr;
  memset(&tr, 0, sizeof(tr));
  tr.tx_buf = (unsigned long)&tx;
  tr.rx_buf = 0;
  tr.len = 1;
  tr.speed_hz = 1000000;
  tr.bits_per_word = 8;
  tr.delay_usecs = 0;

  if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
    std::cerr << "Reset failed" << std::endl;
  }
}

bool CanReader::Send(uint16_t canId, uint8_t *data, uint8_t length) {
  if (length > 8)
    return false;

  if (test_mode) {
    return true;
  }

  // Check if TX buffer is available
  uint8_t status = ReadByte(CANSTAT);
  if (status & 0x04) { // TXREQ bit set
    return false; // Buffer not available
  }

  // Write CAN ID
  WriteByte(TXB0SIDH, (canId >> 3) & 0xFF);
  WriteByte(TXB0SIDL, (canId & 0x07) << 5);
  WriteByte(TXB0EID8, 0);
  WriteByte(TXB0EID0, 0);

  // Write data length
  WriteByte(TXB0DLC, length);

  // Write data
  for (int i = 0; i < length; i++) {
    WriteByte(TXB0D0 + i, data[i]);
  }

  // Request to send
  uint8_t tx = RTS_TXB0_INSTRUCTION;
  struct spi_ioc_transfer tr;
  memset(&tr, 0, sizeof(tr));
  tr.tx_buf = (unsigned long)&tx;
  tr.rx_buf = 0;
  tr.len = 1;
  tr.speed_hz = 1000000;
  tr.bits_per_word = 8;
  tr.delay_usecs = 0;

  if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
    return false;
  }

  return true;
}

bool CanReader::Init() {
  if (test_mode) {
    WriteByte(CANCTRL, MODE_NORMAL);
    return true;
  }

  // Reset the chip
  Reset();
  usleep(100000); // 100ms delay

  // Set configuration mode
  WriteByte(CANCTRL, MODE_CONFIG);
  usleep(100);

  // Configure baud rate (500Kbps)
  WriteByte(CNF1, 0x00); // 8MHz oscillator, 500Kbps
  WriteByte(CNF2, 0x90); // PHSEG1_3TQ | PRSEG_1TQ
  WriteByte(CNF3, 0x02); // PHSEG2_3TQ

  // Configure RX buffer 0 to receive all messages
  WriteByte(RXB0CTRL, RXM_ALL);

  // Clear filters and masks (accept all messages)
  WriteByte(RXB0SIDH, 0x00);
  WriteByte(RXB0SIDL, 0x00);

  // Configure interrupts
  WriteByte(CANINTE, RX0IE); // Enable RX0 interrupt only

  // Set normal mode
  WriteByte(CANCTRL, MODE_NORMAL);
  usleep(100);

  // Verify we're in normal mode
  uint8_t mode = ReadByte(CANSTAT) & 0xE0;
  if (mode != MODE_NORMAL) {
    std::cerr << "Failed to enter normal mode. CANSTAT = 0x" << std::hex
              << (int)mode << std::endl;
    return false;
  }

  return true;
}

bool CanReader::Receive(uint8_t *buffer, uint8_t &length) {
  if (test_mode) {
    if (!test_should_receive) {
      length = 0;
      return false;
    }

    memcpy(buffer, test_receive_data, test_receive_length);
    length = test_receive_length;
    return true;
  }

  // Check if there's data to receive
  uint8_t status = ReadByte(CANINTF);
  if (!(status & RX0IE)) {
    return false;
  }

  // Read message using READ RX instruction
  uint8_t tx[13] = {0x90, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // READ RXB0
  uint8_t rx[13] = {0};

  struct spi_ioc_transfer tr;
  memset(&tr, 0, sizeof(tr));
  tr.tx_buf = (unsigned long)tx;
  tr.rx_buf = (unsigned long)rx;
  tr.len = 13;
  tr.speed_hz = 1000000;
  tr.bits_per_word = 8;
  tr.delay_usecs = 0;

  if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
    std::cerr << "SPI transfer failed during receive" << std::endl;
    return false;
  }

  // Extract CAN ID
  uint16_t canId = (static_cast<uint16_t>(rx[1]) << 3) | (rx[2] >> 5);

  // Extract data length
  length = rx[5] & 0x0F;
  if (length > 8) {
    length = 8;
  }

  // Copy data
  memcpy(buffer, &rx[6], length);

  // Clear interrupt flag
  WriteByte(CANINTF, 0x00);

  if (debug) {
    std::cout << "Received CAN ID: 0x" << std::hex << canId << std::endl;
    std::cout << "Data length: " << std::dec << (int)length << std::endl;
  }

  return true;
}

uint16_t CanReader::getId() {
  if (test_mode) {
    return test_can_id;
  }

  // Read the CAN ID from RX buffer
  uint8_t sidh = ReadByte(RXB0SIDH);
  uint8_t sidl = ReadByte(RXB0SIDL);
  return (static_cast<uint16_t>(sidh) << 3) | (sidl >> 5);
}

// Test mode methods
uint8_t CanReader::setTestRegister(uint8_t addr, uint8_t value) {
  if (test_mode) {
    test_registers[addr] = value;
  }
  return value;
}

uint8_t CanReader::getTestRegister(uint8_t addr) const {
  if (test_mode) {
    auto it = test_registers.find(addr);
    if (it != test_registers.end()) {
      return it->second;
    }
  }
  return 0;
}

void CanReader::setTestReceiveData(const uint8_t *data, uint8_t length,
                                   uint16_t id) {
  if (test_mode) {
    test_should_receive = true;
    test_can_id = id;
    test_receive_length = length > 8 ? 8 : length;
    memcpy(test_receive_data, data, test_receive_length);

    // Update registers to match
    test_registers[RXB0SIDH] = (id >> 3) & 0xFF;
    test_registers[RXB0SIDL] = (id & 0x07) << 5;
    test_registers[RXB0DLC] = test_receive_length;

    for (uint8_t i = 0; i < test_receive_length; i++) {
      test_registers[RXB0D0 + i] = test_receive_data[i];
    }
  }
}

void CanReader::setTestShouldReceive(bool shouldReceive) {
  if (test_mode) {
    test_should_receive = shouldReceive;
    if (shouldReceive) {
      test_registers[CANINTF] = RX0IE;
    } else {
      test_registers[CANINTF] = 0x00;
    }
  }
}

bool CanReader::initialize() {
  if (!test_mode) {
    if (!Init()) {
      std::cerr << "Initialization failed!" << std::endl;
      return false;
    }
  }
  return true;
}
