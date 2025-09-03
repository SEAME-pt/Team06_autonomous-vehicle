#include "CanReader.hpp"

CanReader::CanReader(bool test_mode) : test_mode(test_mode), debug(false) {
  if (!test_mode) {
    try {
      InitSPI(); // LCOV_EXCL_LINE - Hardware initialization, not testable in
                 // unit tests
    } catch (
        const std::exception &e) { // LCOV_EXCL_LINE - Hardware error handling
      std::cerr
          << "Error initializing CanReader: " << e.what()
          << std::endl; // LCOV_EXCL_LINE - Error handling for hardware failures
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
    close(spi_fd); // LCOV_EXCL_LINE - Hardware cleanup, not testable in unit
                   // tests
  }
}

// LCOV_EXCL_START - Hardware SPI initialization, not testable in unit tests
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
  } catch (...) {  // LCOV_EXCL_LINE - Hardware error handling
    close(spi_fd); // LCOV_EXCL_LINE - Hardware cleanup in error path
    spi_fd = -1;   // LCOV_EXCL_LINE - Hardware error handling
    throw;         // LCOV_EXCL_LINE - Hardware error handling
  }

  return true;
  // LCOV_EXCL_STOP
}
// LCOV_EXCL_START - Hardware SPI read, not testable in unit tests
uint8_t CanReader::ReadByte(uint8_t addr) {
  if (test_mode) {
    auto it = test_registers.find(addr);
    if (it != test_registers.end()) {
      return it->second;
    }
    return 0;
  }
  // LCOV_EXCL_STOP

  // LCOV_EXCL_START - Hardware SPI transfer, not testable in unit tests
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
    std::cerr << "SPI transfer failed"
              << std::endl; // LCOV_EXCL_LINE - Hardware error handling
    return 0;
  }

  return rx[2];
  // LCOV_EXCL_STOP
}

void CanReader::WriteByte(uint8_t addr, uint8_t data) {
  if (test_mode) {
    test_registers[addr] = data;
    return;
  }

  // LCOV_EXCL_START - Hardware SPI transfer, not testable in unit tests
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
    std::cerr << "SPI transfer failed"
              << std::endl; // LCOV_EXCL_LINE - Hardware error handling
  }
  // LCOV_EXCL_STOP
}

void CanReader::Reset() {
  if (test_mode) { // LCOV_EXCL_LINE - Test mode branch
    // Reset test registers to default values
    test_registers.clear();                // LCOV_EXCL_LINE - Test mode setup
    test_registers[CANCTRL] = MODE_NORMAL; // LCOV_EXCL_LINE - Test mode setup
    test_registers[CANSTAT] = MODE_NORMAL; // LCOV_EXCL_LINE - Test mode setup
    test_registers[RXB0CTRL] =
        RXM_FILTER_ANY;             // LCOV_EXCL_LINE - Test mode setup
    test_registers[CANINTF] = 0x00; // LCOV_EXCL_LINE - Test mode setup
    return;                         // LCOV_EXCL_LINE - Test mode early return
  }

  // LCOV_EXCL_START - Hardware SPI reset, not testable in unit tests
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
    std::cerr << "Reset failed"
              << std::endl; // LCOV_EXCL_LINE - Hardware error handling
  }

  // Wait for the chip to reset
  usleep(10000);
  // LCOV_EXCL_STOP
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
    return false;       // Buffer not available
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

  // LCOV_EXCL_START - Hardware CAN initialization, not testable in unit tests
  // Reset the chip
  std::cout << "Resetting MCP2515..." << std::endl;
  Reset();
  usleep(100000); // 100ms delay

  // Set configuration mode
  WriteByte(CANCTRL, MODE_CONFIG);
  usleep(10000); // 10ms delay

  // Configure baud rate (500Kbps) for 8MHz crystal
  // For 8MHz crystal at 500kbps: TQ = 8MHz / (2 * (BRP+1)) = 8MHz / 2 = 4MHz
  // Bit time = 8 TQ, so bit rate = 4MHz / 8 = 500kbps
  WriteByte(CNF1, 0x00); // SJW=1, BRP=0 (divide by 1)
  WriteByte(CNF2, 0x91); // BTLMODE=1, SAM=0, PHSEG1=2, PRSEG=2
  WriteByte(CNF3, 0x01); // WAKFIL=0, PHSEG2=2

  // Configure RX buffer 0 to receive all messages
  WriteByte(RXB0CTRL, RXM_FILTER_ANY);

  // Clear filters and masks (accept all messages)
  WriteByte(RXF0SIDH, 0x00);
  WriteByte(RXF0SIDL, 0x00);
  WriteByte(RXM0SIDH, 0x00); // Mask that accepts any ID
  WriteByte(RXM0SIDL, 0x00);

  // Configure interrupts
  WriteByte(CANINTF, 0x00);  // Clear all interrupt flags
  WriteByte(CANINTE, RX0IF); // Enable RX0 interrupt only

  // Set normal mode
  WriteByte(CANCTRL, MODE_NORMAL);
  usleep(100);

  // Verify we're in normal mode
  uint8_t mode = ReadByte(CANSTAT) & 0xE0;
  std::cout << "MCP2515 CANSTAT = 0x" << std::hex << (int)mode << std::dec
            << std::endl;
  if (mode != MODE_NORMAL) {
    std::cerr << "Failed to enter normal mode. CANSTAT = 0x" << std::hex
              << (int)mode
              << std::endl; // LCOV_EXCL_LINE - Hardware error handling
    return false;
  }

  std::cout << "MCP2515 initialized successfully in normal mode" << std::endl;
  // LCOV_EXCL_STOP
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

  // LCOV_EXCL_START - Hardware CAN receive, not testable in unit tests
  // Check if there's data to receive
  uint8_t status = ReadByte(CANINTF);
  if (!(status & RX0IF)) {
    return false;
  }

  // Read message using READ RX instruction
  uint8_t tx[13] = {CAN_READ_RX, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // READ RX
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
    std::cerr << "SPI transfer failed during receive"
              << std::endl; // LCOV_EXCL_LINE - Hardware error handling
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
  // LCOV_EXCL_STOP

  return true;
}

uint16_t CanReader::getId() {
  if (test_mode) {
    return test_can_id;
  }

  // LCOV_EXCL_START - Hardware CAN ID read, not testable in unit tests
  // Read the CAN ID from RX buffer
  uint8_t sidh = ReadByte(RXB0SIDH);
  uint8_t sidl = ReadByte(RXB0SIDL);
  return (static_cast<uint16_t>(sidh) << 3) | (sidl >> 5);
  // LCOV_EXCL_STOP
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
    if (!Init()) { // LCOV_EXCL_LINE - Hardware initialization check
      std::cerr << "Initialization failed!"
                << std::endl; // LCOV_EXCL_LINE - Hardware error handling
      return false;           // LCOV_EXCL_LINE - Hardware error handling
    }
  }
  return true;
}
