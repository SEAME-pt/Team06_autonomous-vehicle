#ifndef MOCKCANREADER_HPP
#define MOCKCANREADER_HPP

#include <cstdint>
#include <vector>

// Forward declaration
class CanReader;

// Interface for CAN reader
class ICanReader {
public:
    virtual ~ICanReader() = default;
    virtual bool Init() = 0;
    virtual bool Send(uint16_t canId, uint8_t* data, uint8_t length) = 0;
    virtual bool Receive(uint8_t* buffer, uint8_t& length) = 0;
    virtual uint16_t getId() = 0;
};

class MockCanReader : public ICanReader {
public:
    MockCanReader() :
        _receiveData{0},
        _receiveLength(0),
        _canId(0x100),
        _shouldReceive(false) {}

    ~MockCanReader() override = default;

    // Setters for mock data
    void setReceiveData(const std::vector<uint8_t>& data) {
        _receiveLength = static_cast<uint8_t>(data.size() > 8 ? 8 : data.size());
        for (uint8_t i = 0; i < _receiveLength; ++i) {
            _receiveData[i] = data[i];
        }
    }

    void setCanId(uint16_t id) {
        _canId = id;
    }

    void setShouldReceive(bool shouldReceive) {
        _shouldReceive = shouldReceive;
    }

    // ICanReader interface implementation
    bool Init() override {
        return true;
    }

    bool Send(uint16_t canId, uint8_t* data, uint8_t length) override {
        return true;
    }

    bool Receive(uint8_t* buffer, uint8_t& length) override {
        if (!_shouldReceive) {
            return false;
        }

        for (uint8_t i = 0; i < _receiveLength; ++i) {
            buffer[i] = _receiveData[i];
        }
        length = _receiveLength;
        return true;
    }

    uint16_t getId() override {
        return _canId;
    }

private:
    uint8_t _receiveData[8];
    uint8_t _receiveLength;
    uint16_t _canId;
    bool _shouldReceive;
};

#endif
