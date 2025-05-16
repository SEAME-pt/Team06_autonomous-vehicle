#include "CanReader.hpp"

CanReader::CanReader(bool test_mode) : test_mode(test_mode), debug(false) {
    if (!test_mode) {
        try {
            InitSPI();
            if (!Init()) {
                std::cerr << "Initialization failed!" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error initializing CanReader: " << e.what() << std::endl;
        }
    } else {
        // Initialize test mode register defaults
        test_registers[CANCTRL] = 0x00;  // Normal mode
        test_registers[CANSTAT] = 0x00;  // Normal mode
        test_registers[RXB0CTRL] = 0x60; // Receive any message
        test_registers[CANINTF] = 0x00;  // No interrupts
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
        uint32_t speed = 10000000; // 10MHz

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
        throw; // Rethrow exception after cleanup
    }

    return true;
}

uint8_t CanReader::ReadByte(uint8_t addr) {
    if (test_mode) {
        auto it = test_registers.find(addr);
        if (it != test_registers.end()) {
            return it->second;
        }
        return 0; // Default for test mode
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
        test_registers[CANCTRL] = 0x00;
        test_registers[CANSTAT] = 0x00;
        test_registers[RXB0CTRL] = 0x60;
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
}

bool CanReader::Send(uint16_t canId, uint8_t* data, uint8_t length) {
    if (length > 8) return false;

    if (test_mode) {
        // In test mode, just simulate successful send
        return true;
    }

    uint8_t status = ReadByte(CAN_RD_STATUS);

    WriteByte(0x31, (canId >> 3) & 0xFF);  // TXB0SIDH
    WriteByte(0x32, (canId & 0x07) << 5);  // TXB0SIDL
    WriteByte(0x33, 0);  // TXB0EID8
    WriteByte(0x34, 0);  // TXB0EID0
    WriteByte(0x35, length);  // TXB0DLC

    for (int i = 0; i < length; i++) {
        WriteByte(0x36 + i, data[i]);  // TXB0D0 + i
    }

    if (status & 0x04) {
        usleep(10000);  // 10ms
        WriteByte(0x30, 0);  // TXB0CTRL
        while (ReadByte(CAN_RD_STATUS) & 0x04) {
            usleep(1000);
        }
    }

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
        // In test mode, just set default register values
        WriteByte(CANCTRL, 0x00);  // Normal mode
        return true;
    }

    Reset();
    usleep(100000); // 100ms delay

    // Set baud rate 500Kbps
    WriteByte(CNF1, CAN_500Kbps);
    WriteByte(CNF2, 0x80 | 0x10 | 0x00); // PHSEG1_3TQ | PRSEG_1TQ
    WriteByte(CNF3, 0x02); // PHSEG2_3TQ

    // Setup RXB0 - Recebe todas as mensagens
    WriteByte(RXB0CTRL, 0x60); // Recebe qualquer mensagem
    WriteByte(RXB0SIDH, 0x00); // Não filtra por ID
    WriteByte(RXB0SIDL, 0x00);

    // Desabilita filtros
    WriteByte(RXF0SIDH, 0x00);
    WriteByte(RXF0SIDL, 0x00);
    WriteByte(RXM0SIDH, 0x00); // Máscara que aceita qualquer ID
    WriteByte(RXM0SIDL, 0x00);

    // Limpa e configura interrupções
    WriteByte(CANINTF, 0x00); // Limpa todas as flags
    WriteByte(CANINTE, 0x01); // Habilita interrupção de recepção

    // Modo Normal
    WriteByte(CANCTRL, 0x00); // Modo normal, clock output desabilitado

    // Verifica modo
    uint8_t mode = ReadByte(CANSTAT);
    if ((mode & 0xE0) != 0x00) {
        std::cerr << "Failed to enter normal mode. CANSTAT = 0x"
                  << std::hex << (int)mode << std::endl;
        return false;
    }

    return true;
}

bool CanReader::Receive(uint8_t* buffer, uint8_t& length) {
    if (test_mode) {
        if (!test_should_receive) {
            length = 0;
            return false;
        }

        memcpy(buffer, test_receive_data, test_receive_length);
        length = test_receive_length;
        return true;
    }

    // Verifica se há dados para receber
    uint8_t status = ReadByte(CANINTF);
    if (!(status & 0x01)) {
        return false;
    }

    // Lê o comprimento dos dados
    length = ReadByte(RXB0DLC) & 0x0F;
    if (length > 8) length = 8;

    // Lê os dados
    for (int i = 0; i < length; i++) {
        buffer[i] = ReadByte(RXB0D0 + i);
    }

    // Limpa a flag de interrupção de recepção
    WriteByte(CANINTF, 0x00);  // Limpa todas as flags

    if(debug) {
        std::cout << "Status: 0x" << std::hex << (int)status << std::endl;
        std::cout << "Length: " << std::dec << (int)length << std::endl;
    }

    return true;
}

uint16_t CanReader::getId() {
    if (test_mode) {
        return test_can_id;
    }
    return (ReadByte(RXB0SIDH) << 3) | (ReadByte(RXB0SIDL) >> 5);
}

// Test mode methods
uint8_t CanReader::setTestRegister(uint8_t addr, uint8_t value) {
    if (test_mode) {
        test_registers[addr] = value;
    }
    return value; // Return the value that was set
}

uint8_t CanReader::getTestRegister(uint8_t addr) const {
    if (test_mode) {
        auto it = test_registers.find(addr);
        if (it != test_registers.end()) {
            return it->second;
        }
    }
    return 0; // Default value if not in test mode or register not found
}

void CanReader::setTestReceiveData(const uint8_t* data, uint8_t length, uint16_t id) {
    if (test_mode) {
        test_should_receive = true;
        test_can_id = id;
        test_receive_length = length > 8 ? 8 : length;
        memcpy(test_receive_data, data, test_receive_length);

        // Also update registers to match
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
            test_registers[CANINTF] = 0x01; // RX0IF flag
        } else {
            test_registers[CANINTF] = 0x00;
        }
    }
}
