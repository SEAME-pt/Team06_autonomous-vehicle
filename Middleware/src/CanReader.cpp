#include "CanReader.hpp"

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
    test_registers[RXB0CTRL] = RXM_FILTER_ANY;
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
    // Use 10MHz SPI speed which is compatible with most MCP2515 modules
    uint32_t speed = 10000000;

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

  uint8_t tx[3] = {CAN_READ, addr, 0};
  uint8_t rx[3] = {0};

  struct spi_ioc_transfer tr;
  memset(&tr, 0, sizeof(tr));
  tr.tx_buf = (unsigned long)tx;
  tr.rx_buf = (unsigned long)rx;
  tr.len = 3;
  tr.speed_hz = 10000000;
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

  uint8_t tx[3] = {CAN_WRITE, addr, data};

  struct spi_ioc_transfer tr;
  memset(&tr, 0, sizeof(tr));
  tr.tx_buf = (unsigned long)tx;
  tr.rx_buf = 0;
  tr.len = 3;
  tr.speed_hz = 10000000;
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
    test_registers[RXB0CTRL] = RXM_FILTER_ANY;
    test_registers[CANINTF] = 0x00;
    return;
  }

  uint8_t tx = CAN_RESET;

  struct spi_ioc_transfer tr;
  memset(&tr, 0, sizeof(tr));
  tr.tx_buf = (unsigned long)&tx;
  tr.rx_buf = 0;
  tr.len = 1;
  tr.speed_hz = 10000000;
  tr.bits_per_word = 8;
  tr.delay_usecs = 0;

  if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
    std::cerr << "Reset failed" << std::endl;
  }

  // Wait for the chip to reset
  usleep(10000);
}

bool CanReader::Send(uint16_t canId, uint8_t *data, uint8_t length) {
  if (length > 8)
    return false;

  if (test_mode) {
    return true;
  }

  // Check if TX buffer is available
  uint8_t status = ReadByte(TXB0CTRL);
  if (status & TX0IF) { // TX0IF bit set
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
  uint8_t tx = CAN_RTS_TXB0;
  struct spi_ioc_transfer tr;
  memset(&tr, 0, sizeof(tr));
  tr.tx_buf = (unsigned long)&tx;
  tr.rx_buf = 0;
  tr.len = 1;
  tr.speed_hz = 10000000;
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
  usleep(10000); // 10ms delay

  // Verify we're in config mode
  uint8_t mode = ReadByte(CANSTAT) & 0xE0;
  if (mode != MODE_CONFIG) {
    std::cerr << "Failed to enter config mode. CANSTAT = 0x" << std::hex
              << (int)mode << std::endl;
    return false;
  }

  // Configure baud rate (500Kbps with 8MHz oscillator - same as Arduino)
  WriteByte(CNF1, CAN_500Kbps);  // 0x00 for 500Kbps with 8MHz
  WriteByte(CNF2, 0x80 | 0x10 | 0x08);  // BTLMODE=1, SAM=0, PHSEG1=3, PRSEG=1
  WriteByte(CNF3, 0x05);  // PHSEG2=6 (must be >= PHSEG1)

  // Configure RX buffer 0 to receive all messages
  WriteByte(RXB0CTRL, RXM_FILTER_ANY);

  // Clear filters and masks (accept all messages)
  WriteByte(RXF0SIDH, 0x00);
  WriteByte(RXF0SIDL, 0x00);
  WriteByte(RXM0SIDH, 0x00); // Mask that accepts any ID
  WriteByte(RXM0SIDL, 0x00);

  // Configure interrupts
  WriteByte(CANINTF, 0x00); // Clear all interrupt flags
  WriteByte(CANINTE, RX0IF); // Enable RX0 interrupt only

  // Set normal mode
  WriteByte(CANCTRL, MODE_NORMAL);
  usleep(10000); // 10ms delay

  // Verify we're in normal mode
  mode = ReadByte(CANSTAT) & 0xE0;
  if (mode != MODE_NORMAL) {
    std::cerr << "Failed to enter normal mode. CANSTAT = 0x" << std::hex
              << (int)mode << std::endl;
    return false;
  }

  if (debug) {
    std::cout << "MCP2515 initialized in normal mode" << std::endl;
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

  // Check if there's data to receive by reading interrupt flags
  uint8_t intf = ReadByte(CANINTF);
  if (!(intf & RX0IF)) {
    return false;  // No message available
  }

  // Read message length
  length = ReadByte(RXB0DLC) & 0x0F;  // Lower 4 bits contain data length
  if (length > 8) {
    length = 8;  // Limit to 8 bytes
  }

  // Read data bytes one by one
  for (uint8_t i = 0; i < length; i++) {
    buffer[i] = ReadByte(RXB0D0 + i);
  }

  // Clear the receive flag to allow receiving next message
  // Use bit modify instruction to clear only RX0IF bit
  uint8_t tx[4] = {CAN_BIT_MODIFY, CANINTF, RX0IF, 0x00};
  struct spi_ioc_transfer tr;
  memset(&tr, 0, sizeof(tr));
  tr.tx_buf = (unsigned long)tx;
  tr.rx_buf = 0;
  tr.len = 4;
  tr.speed_hz = 10000000;
  tr.bits_per_word = 8;
  tr.delay_usecs = 0;

  if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
    std::cerr << "SPI transfer failed during interrupt clear" << std::endl;
  }

  if (debug) {
    std::cout << "Received CAN message with ID: 0x" << std::hex << getId()
              << ", length: " << std::dec << (int)length << std::endl;
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

  // Standard ID format: SIDH (8 bits) + SIDL (3 bits)
  return ((uint16_t)sidh << 3) | ((sidl >> 5) & 0x07);
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
      test_registers[CANINTF] = RX0IF;
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
