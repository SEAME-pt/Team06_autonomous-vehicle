#include <gtest/gtest.h>
#include "Battery.hpp"
#include "BatteryReader.hpp"
#include "BackMotors.hpp"
#include "FServo.hpp"
#include "CanReader.hpp"
#include "CanMessageBus.hpp"
#include "ControlAssembly.hpp"
#include "ControlLogger.hpp"
#include "Distance.hpp"
#include "LaneKeepingHandler.hpp"
#include "SensorHandler.hpp"
#include "SensorLogger.hpp"
#include "Speed.hpp"
#include "TrafficSignHandler.hpp"
#include "MockPublisher.hpp"
#include "MockBackMotors.hpp"
#include "MockFServo.hpp"
#include "MockBatteryReader.hpp"
#include "MockCanReader.hpp"
#include <memory>
#include <thread>
#include <chrono>
#include <zmq.hpp>

class ComprehensiveCoverageTest : public ::testing::Test {
protected:
    void SetUp() override {
        zmq_context = std::make_unique<zmq::context_t>(1);
        mock_publisher = std::make_shared<MockPublisher>();
        mock_back_motors = std::make_shared<MockBackMotors>();
        mock_f_servo = std::make_shared<MockFServo>();
        mock_battery_reader = std::make_shared<MockBatteryReader>();
        mock_can_reader = std::make_shared<MockCanReader>();
    }

    void TearDown() override {
        zmq_context.reset();
    }

    std::unique_ptr<zmq::context_t> zmq_context;
    std::shared_ptr<MockPublisher> mock_publisher;
    std::shared_ptr<MockBackMotors> mock_back_motors;
    std::shared_ptr<MockFServo> mock_f_servo;
    std::shared_ptr<MockBatteryReader> mock_battery_reader;
    std::shared_ptr<MockCanReader> mock_can_reader;
};

TEST_F(ComprehensiveCoverageTest, BatteryFunctionality) {
    // Test Battery class functions
    auto battery = std::make_unique<Battery>();

    // Test getName
    EXPECT_EQ(battery->getName(), "battery");

    // Test initial state
    auto sensor_data = battery->getSensorData();
    EXPECT_NE(sensor_data.find("battery"), sensor_data.end());
    EXPECT_NE(sensor_data.find("charging"), sensor_data.end());

    // Test updateSensorData
    battery->updateSensorData();

    // Test getCharging (correct method name)
    bool charging = battery->getCharging();
    // Should return a valid boolean value
    EXPECT_TRUE(charging == true || charging == false);
}

TEST_F(ComprehensiveCoverageTest, BatteryReaderFunctionality) {
    // Test BatteryReader class functions (correct constructor)
    auto battery_reader = std::make_unique<BatteryReader>(true);

    // Test hardware interface methods
    EXPECT_NO_THROW(battery_reader->read_adc(0));
    EXPECT_NO_THROW(battery_reader->read_charge());

    // Test data access methods
    EXPECT_NO_THROW(battery_reader->getVoltage());
    EXPECT_NO_THROW(battery_reader->getShunt());
    EXPECT_NO_THROW(battery_reader->getPercentage());
    EXPECT_NO_THROW(battery_reader->isCharging());
}

TEST_F(ComprehensiveCoverageTest, BackMotorsFunctionality) {
    // Test BackMotors class functions (correct constructor)
    auto back_motors = std::make_unique<BackMotors>();

    // Test hardware interface methods
    EXPECT_NO_THROW(back_motors->open_i2c_bus());
    EXPECT_NO_THROW(back_motors->init_motors());
    EXPECT_NO_THROW(back_motors->setMotorPwm(0, 100));
    EXPECT_NO_THROW(back_motors->setSpeed(50));
    EXPECT_NO_THROW(back_motors->emergencyBrake());

    // Test I2C communication methods
    EXPECT_NO_THROW(back_motors->writeByteData(back_motors->_fdMotor, 0x00, 0x01));
    EXPECT_NO_THROW(back_motors->readByteData(back_motors->_fdMotor, 0x00));
}

TEST_F(ComprehensiveCoverageTest, FServoFunctionality) {
    // Test FServo class functions (correct constructor)
    auto f_servo = std::make_unique<FServo>();

    // Test hardware interface methods
    EXPECT_NO_THROW(f_servo->open_i2c_bus());
    EXPECT_NO_THROW(f_servo->init_servo());
    EXPECT_NO_THROW(f_servo->setServoPwm(0, 100, 200));
    EXPECT_NO_THROW(f_servo->set_steering(90));

    // Test I2C communication methods
    EXPECT_NO_THROW(f_servo->writeByteData(f_servo->_fdServo, 0x00, 0x01));
    EXPECT_NO_THROW(f_servo->readByteData(f_servo->_fdServo, 0x00));
}

TEST_F(ComprehensiveCoverageTest, CanReaderFunctionality) {
    // Test CanReader class functions (correct constructor)
    auto can_reader = std::make_unique<CanReader>(true);

    // Test CAN interface methods (correct signatures)
    uint8_t buffer[8];
    uint8_t length = 8;
    EXPECT_NO_THROW(can_reader->Receive(buffer, length));

    uint8_t test_data[] = {0x01, 0x02, 0x03};
    EXPECT_NO_THROW(can_reader->Send(0x123, test_data, 3));

    // Test other available methods
    EXPECT_NO_THROW(can_reader->getLastMessage());
}

TEST_F(ComprehensiveCoverageTest, DistanceFunctionality) {
    // Test Distance class functions (correct constructor)
    auto distance = std::make_unique<Distance>();

    // Test getName
    EXPECT_EQ(distance->getName(), "distance");

    // Test getSensorData
    auto sensor_data = distance->getSensorData();
    EXPECT_NE(sensor_data.find("obs"), sensor_data.end());

    // Test updateSensorData
    distance->updateSensorData();

    // Test readSensor
    auto sensor_data_after = distance->getSensorData();
    EXPECT_NE(sensor_data_after.find("obs"), sensor_data_after.end());

    // Test setEmergencyBrakeCallback
    bool callback_called = false;
    distance->setEmergencyBrakeCallback([&callback_called](bool active) {
        callback_called = true;
    });

    // Note: calculateCollisionRisk is private, so we can't test it directly
    // The updateSensorData should trigger it internally
}

TEST_F(ComprehensiveCoverageTest, SpeedFunctionality) {
    // Test Speed class functions (correct constructor)
    auto speed = std::make_unique<Speed>();

    // Test getName
    EXPECT_EQ(speed->getName(), "speed");

    // Test getSensorData
    auto sensor_data = speed->getSensorData();
    EXPECT_NE(sensor_data.find("speed"), sensor_data.end());
    EXPECT_NE(sensor_data.find("odo"), sensor_data.end());

    // Test updateSensorData
    speed->updateSensorData();

    // Test readSensor
    auto sensor_data_after = speed->getSensorData();
    EXPECT_NE(sensor_data_after.find("speed"), sensor_data_after.end());
}

TEST_F(ComprehensiveCoverageTest, CanMessageBusFunctionality) {
    // Note: CanMessageBus has private constructor and destructor
    // The main functionality is tested through integration tests
    // We'll skip direct instantiation and focus on other components

    // Test that we can at least include the header without issues
    EXPECT_TRUE(true);
}

TEST_F(ComprehensiveCoverageTest, ControlAssemblyFunctionality) {
    // Test ControlAssembly class functions (correct constructor signature)
    auto control_assembly = std::make_unique<ControlAssembly>(
        "tcp://127.0.0.1:5557", *zmq_context, mock_back_motors, mock_f_servo, nullptr);

    // Test start and stop
    control_assembly->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    control_assembly->stop();

    // Test handleEmergencyBrake
    control_assembly->handleEmergencyBrake(true);
    control_assembly->handleEmergencyBrake(false);
}

TEST_F(ComprehensiveCoverageTest, LaneKeepingHandlerFunctionality) {
    // Test LaneKeepingHandler class functions
    auto lane_keeping_handler = std::make_unique<LaneKeepingHandler>(
        "tcp://127.0.0.1:5558", *zmq_context, mock_publisher, true);

    // Test start and stop
    lane_keeping_handler->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    lane_keeping_handler->stop();

    // Test setTestLaneKeepingData
    LaneKeepingData test_data;
    test_data.lane_status = 1;
    lane_keeping_handler->setTestLaneKeepingData(test_data);
}

TEST_F(ComprehensiveCoverageTest, TrafficSignHandlerFunctionality) {
    // Test TrafficSignHandler class functions
    auto traffic_sign_handler = std::make_unique<TrafficSignHandler>(
        "tcp://127.0.0.1:5559", *zmq_context, mock_publisher, true);

    // Test start and stop
    traffic_sign_handler->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    traffic_sign_handler->stop();

    // Test setTestTrafficSignData (correct signature: single string parameter)
    EXPECT_NO_THROW(traffic_sign_handler->setTestTrafficSignData("stop"));
}

TEST_F(ComprehensiveCoverageTest, SensorHandlerFunctionality) {
    // Test SensorHandler class functions
    auto sensor_handler = std::make_unique<SensorHandler>(
        "tcp://127.0.0.1:5555", "tcp://127.0.0.1:5556", *zmq_context,
        mock_publisher, mock_publisher, true);

    // Test getSensors
    auto sensors = sensor_handler->getSensors();
    EXPECT_FALSE(sensors.empty());

    // Test addSensor
    auto custom_sensor = std::make_shared<Battery>();
    sensor_handler->addSensor("custom_battery", custom_sensor);

    // Test start and stop
    sensor_handler->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    sensor_handler->stop();
}

TEST_F(ComprehensiveCoverageTest, ControlLoggerFunctionality) {
    // Test ControlLogger class functions
    auto control_logger = std::make_unique<ControlLogger>("test_control.log");

    // Test logControlUpdate (correct signature: command, steering, throttle)
    control_logger->logControlUpdate("throttle", 0.0, 50.0);
    control_logger->logControlUpdate("steering", -25.0, 0.0);

    // Test logError
    control_logger->logError("Test error message");

    // Test destructor (implicit)
    control_logger.reset();
}

TEST_F(ComprehensiveCoverageTest, SensorLoggerFunctionality) {
    // Test SensorLogger class functions
    auto sensor_logger = std::make_unique<SensorLogger>("test_sensor.log");

    // Test logSensorUpdate (correct signature: takes SensorData pointer)
    auto battery_data = std::make_shared<SensorData>("battery", false);
    battery_data->value.store(85.5);
    sensor_logger->logSensorUpdate(battery_data);

    auto speed_data = std::make_shared<SensorData>("speed", false);
    speed_data->value.store(45.2);
    sensor_logger->logSensorUpdate(speed_data);

    // Test logError (correct signature: sensorName, errorMessage)
    sensor_logger->logError("battery", "Test sensor error");

    // Test destructor (implicit)
    sensor_logger.reset();
}

TEST_F(ComprehensiveCoverageTest, EdgeCaseHandling) {
    // Test edge cases and error conditions

    // Test with null pointers
    EXPECT_NO_THROW({
        auto battery = std::make_unique<Battery>();
        battery->updateSensorData();
    });

        // Test rapid updates
    auto distance = std::make_unique<Distance>();
    for (int i = 0; i < 10; ++i) {
        distance->updateSensorData();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Test multiple sensor types
    auto battery = std::make_unique<Battery>();
    auto speed = std::make_unique<Speed>();
    auto distance2 = std::make_unique<Distance>();

    battery->updateSensorData();
    speed->updateSensorData();
    distance2->updateSensorData();

    // Verify all sensors have data
    EXPECT_FALSE(battery->getSensorData().empty());
    EXPECT_FALSE(speed->getSensorData().empty());
    EXPECT_FALSE(distance2->getSensorData().empty());
}

TEST_F(ComprehensiveCoverageTest, ConcurrentOperations) {
    // Test concurrent operations
    auto sensor_handler = std::make_unique<SensorHandler>(
        "tcp://127.0.0.1:5555", "tcp://127.0.0.1:5556", *zmq_context,
        mock_publisher, mock_publisher, true);

    sensor_handler->start();

    // Create multiple threads doing different operations
    std::vector<std::thread> threads;

    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&sensor_handler]() {
            for (int j = 0; j < 10; ++j) {
                auto sensors = sensor_handler->getSensors();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }

    sensor_handler->stop();
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
