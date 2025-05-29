#include <gtest/gtest.h>
#include "ControlLogger.hpp"
#include <fstream>
#include <string>
#include <regex>
#include <unistd.h>  // for unlink
#include <sys/stat.h> // for stat

class ControlLoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a temporary file for testing
        test_log_file = "test_control_log.log";
        // Make sure any existing file is removed
        removeFile(test_log_file);
    }

    void TearDown() override {
        // Clean up the test log file
        removeFile(test_log_file);
    }

    // Helper function to remove a file
    void removeFile(const std::string& path) {
        unlink(path.c_str()); // Remove file if it exists, ignore errors
    }

    // Helper function to check if a file exists
    bool fileExists(const std::string& path) {
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0);
    }

    std::string test_log_file;

    // Helper function to read log file contents
    std::string readLogFile() {
        std::ifstream file(test_log_file);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
};

TEST_F(ControlLoggerTest, InitializationCreatesLogFile) {
    {
        ControlLogger logger(test_log_file);
        // The file should be created and have a header
    }

    // File should exist
    EXPECT_TRUE(fileExists(test_log_file));

    // Content should include session start and end markers
    std::string content = readLogFile();
    EXPECT_TRUE(content.find("Control Logging Session Started") != std::string::npos);
    EXPECT_TRUE(content.find("Control Logging Session Ended") != std::string::npos);
}

TEST_F(ControlLoggerTest, LogControlUpdate) {
    {
        ControlLogger logger(test_log_file);
        logger.logControlUpdate("throttle:50;", 0.0, 50.0);
    }

    // Content should include the logged control update
    std::string content = readLogFile();
    EXPECT_TRUE(content.find("Command: throttle:50;") != std::string::npos);
    EXPECT_TRUE(content.find("Steering: 0") != std::string::npos);
    EXPECT_TRUE(content.find("Throttle: 50") != std::string::npos);
}

TEST_F(ControlLoggerTest, LogError) {
    {
        ControlLogger logger(test_log_file);
        logger.logError("Test error message");
    }

    // Content should include the error message
    std::string content = readLogFile();
    EXPECT_TRUE(content.find("ERROR - Test error message") != std::string::npos);
}

TEST_F(ControlLoggerTest, TimestampFormatIsCorrect) {
    {
        ControlLogger logger(test_log_file);
        logger.logControlUpdate("test", 0.0, 0.0);
    }

    // Check for timestamp in the expected format (YYYY-MM-DD HH:MM:SS.mmm)
    std::string content = readLogFile();
    std::regex timestamp_regex(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3})");
    EXPECT_TRUE(std::regex_search(content, timestamp_regex));
}

TEST_F(ControlLoggerTest, MultipleLogEntries) {
    {
        ControlLogger logger(test_log_file);
        logger.logControlUpdate("throttle:10;", 0.0, 10.0);
        logger.logControlUpdate("steering:20;", 20.0, 0.0);
        logger.logControlUpdate("throttle:30;steering:40;", 40.0, 30.0);
    }

    // Content should include all log entries
    std::string content = readLogFile();
    EXPECT_TRUE(content.find("Throttle: 10") != std::string::npos);
    EXPECT_TRUE(content.find("Steering: 20") != std::string::npos);
    EXPECT_TRUE(content.find("Throttle: 30") != std::string::npos);
    EXPECT_TRUE(content.find("Steering: 40") != std::string::npos);

    // There should be at least 3 lines with Command:
    size_t command_count = 0;
    size_t pos = 0;
    while ((pos = content.find("Command:", pos)) != std::string::npos) {
        command_count++;
        pos += 8; // length of "Command:"
    }
    EXPECT_GE(command_count, 3);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
