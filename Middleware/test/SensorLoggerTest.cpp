#include <gtest/gtest.h>
#include "SensorLogger.hpp"
#include "ISensor.hpp"
#include <fstream>
#include <filesystem>
#include <string>
#include <regex>
#include <memory>

class SensorLoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a temporary file for testing
        test_log_file = "test_sensor_log.log";
        // Make sure any existing file is removed
        std::filesystem::remove(test_log_file);

        // Create a test sensor data instance
        testSensorData = std::make_shared<SensorData>("test_sensor", true);
        testSensorData->value.store(100);
        testSensorData->oldValue.store(50);
        testSensorData->updated.store(true);
    }

    void TearDown() override {
        // Clean up the test log file
        std::filesystem::remove(test_log_file);
    }

    std::string test_log_file;
    std::shared_ptr<SensorData> testSensorData;

    // Helper function to read log file contents
    std::string readLogFile() {
        std::ifstream file(test_log_file);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
};

TEST_F(SensorLoggerTest, InitializationCreatesLogFile) {
    {
        SensorLogger logger(test_log_file);
        // The file should be created and have a header
    }

    // File should exist
    EXPECT_TRUE(std::filesystem::exists(test_log_file));

    // Content should include session start and end markers
    std::string content = readLogFile();
    EXPECT_TRUE(content.find("Sensor Logging Session Started") != std::string::npos);
    EXPECT_TRUE(content.find("Sensor Logging Session Ended") != std::string::npos);
}

TEST_F(SensorLoggerTest, LogSensorUpdate) {
    {
        SensorLogger logger(test_log_file);
        logger.logSensorUpdate(testSensorData);
    }

    // Content should include the logged sensor update
    std::string content = readLogFile();
    EXPECT_TRUE(content.find("Sensor: test_sensor") != std::string::npos);
    EXPECT_TRUE(content.find("Value: 100") != std::string::npos);
    EXPECT_TRUE(content.find("Old Value: 50") != std::string::npos);
    EXPECT_TRUE(content.find("Critical: Yes") != std::string::npos);
}

TEST_F(SensorLoggerTest, LogError) {
    {
        SensorLogger logger(test_log_file);
        logger.logError("test_sensor", "Sensor reading out of range");
    }

    // Content should include the error message
    std::string content = readLogFile();
    EXPECT_TRUE(content.find("ERROR") != std::string::npos);
    EXPECT_TRUE(content.find("Sensor: test_sensor") != std::string::npos);
    EXPECT_TRUE(content.find("Error: Sensor reading out of range") != std::string::npos);
}

TEST_F(SensorLoggerTest, SkipLoggingIfValueUnchanged) {
    {
        SensorLogger logger(test_log_file);

        // Create sensor data with same value and old value
        auto unchangedSensorData = std::make_shared<SensorData>("unchanged_sensor", false);
        unchangedSensorData->value.store(75);
        unchangedSensorData->oldValue.store(75);

        logger.logSensorUpdate(unchangedSensorData);
    }

    // Content should not include the unchanged sensor's data
    std::string content = readLogFile();
    EXPECT_FALSE(content.find("Sensor: unchanged_sensor") != std::string::npos);
}

TEST_F(SensorLoggerTest, NullSensorDataHandling) {
    {
        SensorLogger logger(test_log_file);

        // Log a null sensor data pointer
        std::shared_ptr<SensorData> nullSensorData;
        logger.logSensorUpdate(nullSensorData);

        // This should not crash and should not log anything
    }

    // File should still exist with session markers but no data entry
    EXPECT_TRUE(std::filesystem::exists(test_log_file));
    std::string content = readLogFile();

    // Count entries that have "Sensor:"
    size_t sensorEntryCount = 0;
    size_t pos = 0;
    while ((pos = content.find("Sensor:", pos)) != std::string::npos) {
        sensorEntryCount++;
        pos += 7; // length of "Sensor:"
    }

    // Only session markers, no data entries
    EXPECT_EQ(sensorEntryCount, 0);
}

TEST_F(SensorLoggerTest, TimestampFormatIsCorrect) {
    {
        SensorLogger logger(test_log_file);
        logger.logSensorUpdate(testSensorData);
    }

    // Check for timestamp in the expected format (YYYY-MM-DD HH:MM:SS.mmm)
    std::string content = readLogFile();
    std::regex timestamp_regex(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3})");
    EXPECT_TRUE(std::regex_search(content, timestamp_regex));
}

TEST_F(SensorLoggerTest, MultipleLogEntries) {
    {
        SensorLogger logger(test_log_file);

        // Log first entry
        logger.logSensorUpdate(testSensorData);

        // Create and log a second sensor
        auto secondSensorData = std::make_shared<SensorData>("second_sensor", false);
        secondSensorData->value.store(200);
        secondSensorData->oldValue.store(150);
        logger.logSensorUpdate(secondSensorData);

        // Log an error
        logger.logError("test_sensor", "Test error");
    }

    // Check if all entries are present
    std::string content = readLogFile();
    EXPECT_TRUE(content.find("Sensor: test_sensor") != std::string::npos);
    EXPECT_TRUE(content.find("Value: 100") != std::string::npos);
    EXPECT_TRUE(content.find("Sensor: second_sensor") != std::string::npos);
    EXPECT_TRUE(content.find("Value: 200") != std::string::npos);
    EXPECT_TRUE(content.find("Old Value: 150") != std::string::npos);
    EXPECT_TRUE(content.find("Critical: No") != std::string::npos);
    EXPECT_TRUE(content.find("Error: Test error") != std::string::npos);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
