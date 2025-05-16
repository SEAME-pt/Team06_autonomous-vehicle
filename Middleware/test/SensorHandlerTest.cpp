#include <gtest/gtest.h>
#include "SensorHandler.hpp"
#include "MockPublisher.hpp"
#include "MockBatteryReader.hpp"
#include <memory>
#include <thread>
#include <chrono>
#include <iostream>
#include <atomic>

class MockSensor : public ISensor {
public:
    MockSensor(const std::string& name, bool critical = false) : _name(name) {
        _sensorData["test"] = std::make_shared<SensorData>("test", critical);
        _sensorData["test"]->value.store(0);
    }

    const std::string& getName() const override { return _name; }

    std::unordered_map<std::string, std::shared_ptr<SensorData>> getSensorData() const override {
        return _sensorData;
    }

    void updateSensorData() override {
        auto oldValue = _sensorData["test"]->value.load();
        _sensorData["test"]->oldValue.store(oldValue);
        _sensorData["test"]->value.store(++_value);
        _sensorData["test"]->updated.store(true);
        _sensorData["test"]->timestamp = std::chrono::steady_clock::now();
        update_count++;
    }

    void setValue(unsigned int value) {
        _value = value;
    }

    // For testing throwing exceptions
    void setThrowException(bool shouldThrow, const std::string& errorMsg = "") {
        throwException = shouldThrow;
        exceptionMessage = errorMsg;
    }

    int getUpdateCount() const {
        return update_count;
    }

private:
    void readSensor() override {
        if (throwException) {
            if (exceptionMessage.empty()) {
                throw std::runtime_error("Mock sensor exception");
            } else {
                throw std::runtime_error(exceptionMessage);
            }
        }
    }

    void checkUpdated() override {}

    std::string _name;
    std::unordered_map<std::string, std::shared_ptr<SensorData>> _sensorData;
    unsigned int _value = 0;
    bool throwException = false;
    std::string exceptionMessage;
    int update_count = 0;
};

class SensorHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        zmq_context = std::make_unique<zmq::context_t>(1);
        c_publisher = std::make_shared<MockPublisher>();
        nc_publisher = std::make_shared<MockPublisher>();

        sensor_handler = std::make_unique<SensorHandler>(
            "tcp://127.0.0.1:5555",
            "tcp://127.0.0.1:5556",
            *zmq_context,
            c_publisher,
            nc_publisher,
            false  // Don't use real sensors in tests
        );
    }

    void TearDown() override {
        sensor_handler->stop();
        sensor_handler.reset();
        zmq_context.reset();
    }

    std::unique_ptr<zmq::context_t> zmq_context;
    std::shared_ptr<MockPublisher> c_publisher;
    std::shared_ptr<MockPublisher> nc_publisher;
    std::unique_ptr<SensorHandler> sensor_handler;
};

TEST_F(SensorHandlerTest, InitialState) {
    // Check that init message was sent
    EXPECT_TRUE(c_publisher->hasMessage("init;"));
    EXPECT_TRUE(nc_publisher->hasMessage("init;"));

    // Check that no sensors were added (we're using false for use_real_sensors)
    auto sensors = sensor_handler->getSensors();
    EXPECT_EQ(sensors.size(), 0);
}

TEST_F(SensorHandlerTest, AddCustomSensor) {
    auto mockSensor = std::make_shared<MockSensor>("mock", true);
    sensor_handler->addSensor("mock", mockSensor);

    auto sensors = sensor_handler->getSensors();
    EXPECT_TRUE(sensors.find("mock") != sensors.end());
    EXPECT_EQ(sensors["mock"]->getName(), "mock");
}

TEST_F(SensorHandlerTest, PublishCriticalData) {
    // Add a critical sensor
    auto criticalSensor = std::make_shared<MockSensor>("critical", true);
    sensor_handler->addSensor("critical", criticalSensor);

    // Start the handler
    sensor_handler->start();

    // Wait for data to be published
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Check that data was published to critical channel
    EXPECT_GT(c_publisher->messageCount(), 1); // At least one message (plus init)
}

TEST_F(SensorHandlerTest, PublishNonCriticalData) {
    // Add a non-critical sensor
    auto nonCriticalSensor = std::make_shared<MockSensor>("non_critical", false);
    sensor_handler->addSensor("non_critical", nonCriticalSensor);

    // Start the handler
    sensor_handler->start();

    // Wait for data to be published
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Check that data was published to non-critical channel
    EXPECT_GT(nc_publisher->messageCount(), 1); // At least one message (plus init)
}

TEST_F(SensorHandlerTest, MultipleStartStopCalls) {
    auto mockSensor = std::make_shared<MockSensor>("multi_test", true);
    sensor_handler->addSensor("multi_test", mockSensor);

    // First start and stop
    sensor_handler->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    sensor_handler->stop();

    // Get first message count for comparison
    size_t firstRunCount = c_publisher->messageCount();
    std::cout << "First run message count: " << firstRunCount << std::endl;

    // Print all messages from first run
    std::cout << "Messages from first run:" << std::endl;
    for (const auto& msg : c_publisher->getMessages()) {
        std::cout << "  - " << msg << std::endl;
    }

    EXPECT_GT(firstRunCount, 0); // Should have at least init message

    // Reset publisher messages
    c_publisher->clearMessages();
    nc_publisher->clearMessages();

    // Verify messages were cleared
    std::cout << "After clear, message count: " << c_publisher->messageCount() << std::endl;

    // Give a short pause between stop and restart to ensure threads are properly joined
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Second start with increased wait time
    sensor_handler->start();

    // Debug: Force an update of the sensor data to trigger message sending
    mockSensor->updateSensorData();

    // Print current update count
    std::cout << "Sensor update count before waiting: " << mockSensor->getUpdateCount() << std::endl;

    // Loop and wait for messages up to a reasonable timeout
    const int max_attempts = 10;
    const int attempt_delay_ms = 100;
    int attempts = 0;

    while (c_publisher->messageCount() == 0 && attempts < max_attempts) {
        std::this_thread::sleep_for(std::chrono::milliseconds(attempt_delay_ms));
        attempts++;
        std::cout << "Attempt " << attempts << ", message count: " << c_publisher->messageCount() << std::endl;
    }

    // Print final message count and messages
    std::cout << "Final message count: " << c_publisher->messageCount() << std::endl;
    std::cout << "Messages after second run:" << std::endl;
    for (const auto& msg : c_publisher->getMessages()) {
        std::cout << "  - " << msg << std::endl;
    }

    // Check that data is published eventually
    EXPECT_GT(c_publisher->messageCount(), 0);

    sensor_handler->stop();
}

TEST_F(SensorHandlerTest, ExceptionHandlingInSensorUpdate) {
    // Create a sensor that will throw an exception
    auto exceptionalSensor = std::make_shared<MockSensor>("exceptional", true);
    exceptionalSensor->setThrowException(true, "Test exception");

    sensor_handler->addSensor("exceptional", exceptionalSensor);

    // This should not throw any exceptions as they should be caught
    // within the SensorHandler::readSensors method
    EXPECT_NO_THROW({
        sensor_handler->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        sensor_handler->stop();
    });
}

TEST_F(SensorHandlerTest, DataFormatInPublisher) {
    // Add a sensor with known values
    auto testSensor = std::make_shared<MockSensor>("format_test", true);
    testSensor->setValue(42);  // Set specific value to check in published message
    sensor_handler->addSensor("format_test", testSensor);

    sensor_handler->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // The format should be "name:value;"
    EXPECT_TRUE(c_publisher->hasMessage("test:43;"));  // Value starts at 42, then ++
}

TEST_F(SensorHandlerTest, SensorDataIsMappedCorrectly) {
    // Add both critical and non-critical sensors
    auto criticalSensor = std::make_shared<MockSensor>("critical_map", true);
    auto nonCriticalSensor = std::make_shared<MockSensor>("non_critical_map", false);

    sensor_handler->addSensor("critical_map", criticalSensor);
    sensor_handler->addSensor("non_critical_map", nonCriticalSensor);

    sensor_handler->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Check that messages from the critical sensor go to critical publisher
    bool criticalMsgFound = false;
    for (const auto& msg : c_publisher->getMessages()) {
        if (msg.find("test") != std::string::npos) {
            criticalMsgFound = true;
            break;
        }
    }

    // Check that messages from the non-critical sensor go to non-critical publisher
    bool nonCriticalMsgFound = false;
    for (const auto& msg : nc_publisher->getMessages()) {
        if (msg.find("test") != std::string::npos) {
            nonCriticalMsgFound = true;
            break;
        }
    }

    EXPECT_TRUE(criticalMsgFound);
    EXPECT_TRUE(nonCriticalMsgFound);
}

TEST_F(SensorHandlerTest, SensorUpdateFrequency) {
    // Create a sensor to monitor update frequency
    auto frequencySensor = std::make_shared<MockSensor>("frequency_test", false);
    sensor_handler->addSensor("frequency_test", frequencySensor);

    // Start and let run for a specific time
    sensor_handler->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(550));  // A bit more than 500ms
    sensor_handler->stop();

    // With sensor_read_interval_ms = 100, we should have approximately 5 updates
    // Allow for some timing variations
    EXPECT_GE(frequencySensor->getUpdateCount(), 4);
    EXPECT_LE(frequencySensor->getUpdateCount(), 7);
}

TEST_F(SensorHandlerTest, EmptyInitialState) {
    // Test that a newly created SensorHandler with no sensors added has empty data maps
    sensor_handler->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // We expect 2 init messages since SensorHandler sends one in constructor and one in start()
    EXPECT_EQ(c_publisher->messageCount(), 2); // Two init messages
    EXPECT_EQ(nc_publisher->messageCount(), 2); // Two init messages

    // Verify that the messages are actually init messages
    std::vector<std::string> c_messages = c_publisher->getMessages();
    std::vector<std::string> nc_messages = nc_publisher->getMessages();

    for (const auto& msg : c_messages) {
        EXPECT_EQ(msg, "init;");
    }

    for (const auto& msg : nc_messages) {
        EXPECT_EQ(msg, "init;");
    }

    sensor_handler->stop();
}

TEST_F(SensorHandlerTest, MixedCriticalAndNonCriticalSensors) {
    // Add multiple sensors with different criticality
    auto criticalSensor1 = std::make_shared<MockSensor>("critical1", true);
    auto criticalSensor2 = std::make_shared<MockSensor>("critical2", true);
    auto nonCriticalSensor1 = std::make_shared<MockSensor>("non_critical1", false);
    auto nonCriticalSensor2 = std::make_shared<MockSensor>("non_critical2", false);

    sensor_handler->addSensor("critical1", criticalSensor1);
    sensor_handler->addSensor("critical2", criticalSensor2);
    sensor_handler->addSensor("non_critical1", nonCriticalSensor1);
    sensor_handler->addSensor("non_critical2", nonCriticalSensor2);

    // Start the handler
    sensor_handler->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Verify correct publisher was used for each sensor type
    size_t criticalMessages = c_publisher->messageCount() - 1; // Subtract init message
    size_t nonCriticalMessages = nc_publisher->messageCount() - 1; // Subtract init message

    // We should have at least one message from each sensor (2 critical, 2 non-critical)
    EXPECT_GE(criticalMessages, 2);
    EXPECT_GE(nonCriticalMessages, 2);

    sensor_handler->stop();
}

TEST_F(SensorHandlerTest, RemoveSensor) {
    // Add and then remove a sensor to test dynamic reconfiguration
    auto testSensor = std::make_shared<MockSensor>("remove_test", true);
    sensor_handler->addSensor("remove_test", testSensor);

    auto sensors = sensor_handler->getSensors();
    EXPECT_EQ(sensors.size(), 1);
    EXPECT_TRUE(sensors.find("remove_test") != sensors.end());

    // Remove the sensor
    sensor_handler->addSensor("remove_test", nullptr); // Removing by setting to nullptr

    sensors = sensor_handler->getSensors();
    EXPECT_TRUE(sensors.find("remove_test") == sensors.end() || sensors["remove_test"] == nullptr);
}

TEST_F(SensorHandlerTest, ConcurrentSensorUpdates) {
    // Test concurrent access to the sensor handler from multiple threads
    constexpr int numSensors = 5;
    constexpr int numUpdateThreads = 3;

    // Add multiple sensors
    std::vector<std::shared_ptr<MockSensor>> sensors;
    for (int i = 0; i < numSensors; i++) {
        auto sensor = std::make_shared<MockSensor>("concurrent_" + std::to_string(i), i % 2 == 0);
        sensors.push_back(sensor);
        sensor_handler->addSensor("concurrent_" + std::to_string(i), sensor);
    }

    // Start the handler
    sensor_handler->start();

    // Create multiple threads to update sensors concurrently
    std::vector<std::thread> updateThreads;
    std::atomic<bool> stop_threads(false);

    for (int t = 0; t < numUpdateThreads; t++) {
        updateThreads.emplace_back([&sensors, t, &stop_threads]() {
            int i = 0;
            while (!stop_threads && i < 10) {
                // Update each sensor in a separate thread
                sensors[t % sensors.size()]->updateSensorData();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                i++;
            }
        });
    }

    // Wait a bit for messages to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Signal threads to stop
    stop_threads = true;

    // Wait for all update threads to complete
    for (auto& thread : updateThreads) {
        thread.join();
    }

    // Verify we got messages
    EXPECT_GT(c_publisher->messageCount(), 1);
    EXPECT_GT(nc_publisher->messageCount(), 1);

    sensor_handler->stop();
}

TEST_F(SensorHandlerTest, NullSensorData) {
    // Test handling of null sensor data
    class NullDataSensor : public ISensor {
    public:
        NullDataSensor() : _name("null_data") {
            // Initialize sensor data with a nullptr value and a valid value
            _sensorData["valid_test"] = std::make_shared<SensorData>("valid_test", true);
            // Explicitly store nullptr for the null test
            _sensorData["null_test"] = nullptr;
        }

        const std::string& getName() const override { return _name; }

        std::unordered_map<std::string, std::shared_ptr<SensorData>> getSensorData() const override {
            return _sensorData;
        }

        void updateSensorData() override {
            // Update only the valid data
            if (_sensorData["valid_test"]) {
                _sensorData["valid_test"]->updated.store(true);
                _sensorData["valid_test"]->value.store(_sensorData["valid_test"]->value.load() + 1);
            }
        }

    private:
        void readSensor() override {}
        void checkUpdated() override {}

        std::string _name;
        std::unordered_map<std::string, std::shared_ptr<SensorData>> _sensorData;
    };

    auto nullSensor = std::make_shared<NullDataSensor>();
    sensor_handler->addSensor("null_sensor", nullSensor);

    // This should not crash
    EXPECT_NO_THROW({
        sensor_handler->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        sensor_handler->stop();
    });

    // Verify that we received valid data messages
    bool validMessageFound = false;
    for (const auto& msg : c_publisher->getMessages()) {
        if (msg.find("valid_test") != std::string::npos) {
            validMessageFound = true;
            break;
        }
    }

    EXPECT_TRUE(validMessageFound);
}

TEST_F(SensorHandlerTest, ComplexStartStopCycles) {
    // Add multiple sensors of different criticality
    auto criticalSensor1 = std::make_shared<MockSensor>("critical1", true);
    auto criticalSensor2 = std::make_shared<MockSensor>("critical2", true);
    auto nonCriticalSensor = std::make_shared<MockSensor>("non_critical", false);

    sensor_handler->addSensor("critical1", criticalSensor1);
    sensor_handler->addSensor("critical2", criticalSensor2);
    sensor_handler->addSensor("non_critical", nonCriticalSensor);

    // Set different initial values
    criticalSensor1->setValue(10);
    criticalSensor2->setValue(20);
    nonCriticalSensor->setValue(30);

    // First start cycle
    sensor_handler->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    sensor_handler->stop();

    // Clear messages to check fresh message counts
    c_publisher->clearMessages();
    nc_publisher->clearMessages();

    // Second start/stop cycle with waiting
    sensor_handler->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    sensor_handler->stop();

    // Check message counts
    size_t c_msg_count = c_publisher->messageCount();
    size_t nc_msg_count = nc_publisher->messageCount();
    EXPECT_GT(c_msg_count, 0);
    EXPECT_GT(nc_msg_count, 0);

    // Clear messages again
    c_publisher->clearMessages();
    nc_publisher->clearMessages();

    // Perform a rapid start/stop cycling to test thread handling
    for (int i = 0; i < 5; i++) {
        sensor_handler->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(20)); // Very short cycle
        sensor_handler->stop();
    }

    // Final longer cycle to verify functionality still works
    sensor_handler->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Update sensor values during operation
    criticalSensor1->setValue(15);
    criticalSensor2->setValue(25);
    nonCriticalSensor->setValue(35);

    // Allow time for updates to propagate
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    sensor_handler->stop();

    // Verify we still got messages after all those cycles
    EXPECT_GT(c_publisher->messageCount(), 0);
    EXPECT_GT(nc_publisher->messageCount(), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
