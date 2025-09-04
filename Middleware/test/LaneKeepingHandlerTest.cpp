#include <gtest/gtest.h>
#include "LaneKeepingHandler.hpp"
#include "MockPublisher.hpp"
#include <memory>
#include <thread>
#include <chrono>

class LaneKeepingHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        zmq_context = std::make_unique<zmq::context_t>(1);
        mock_publisher = std::make_shared<MockPublisher>();

        // Create handler in test mode
        handler = std::make_unique<LaneKeepingHandler>(
            "tcp://127.0.0.1:5559",  // Test address
            *zmq_context,
            mock_publisher,
            true  // test_mode = true
        );
    }

    void TearDown() override {
        if (handler) {
            handler->stop();
        }
        handler.reset();
        zmq_context.reset();
    }

    std::unique_ptr<zmq::context_t> zmq_context;
    std::shared_ptr<MockPublisher> mock_publisher;
    std::unique_ptr<LaneKeepingHandler> handler;
};

TEST_F(LaneKeepingHandlerTest, Initialization) {
    // Handler should be created successfully
    EXPECT_NE(handler, nullptr);
}

TEST_F(LaneKeepingHandlerTest, TestModeDataInjection) {
    // Test data injection in test mode
    LaneKeepingData test_data;
    test_data.lane_status = 1; // Left deviation

    handler->setTestLaneKeepingData(test_data);

    // Start the handler
    handler->start();

    // Allow time for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop the handler
    handler->stop();

    // In test mode, we can't easily verify message publishing without
    // more complex setup, but we can verify no crashes occurred
    EXPECT_TRUE(true); // Test passes if no exceptions thrown
}

TEST_F(LaneKeepingHandlerTest, LaneStatusValues) {
    // Test different lane status values
    std::vector<int> test_statuses = {0, 1, 2}; // no deviation, left, right

    for (int status : test_statuses) {
        LaneKeepingData test_data;
        test_data.lane_status = status;

        handler->setTestLaneKeepingData(test_data);

        // Test toString conversion
        std::string data_string = test_data.toString();
        EXPECT_FALSE(data_string.empty());

        // Test fromString conversion
        LaneKeepingData parsed_data = LaneKeepingData::fromString(data_string);
        EXPECT_EQ(parsed_data.lane_status, status);
    }
}

TEST_F(LaneKeepingHandlerTest, DataStringConversion) {
    // Test LaneKeepingData string conversion
    LaneKeepingData original_data;
    original_data.lane_status = 2; // Right deviation

    std::string data_string = original_data.toString();
    EXPECT_TRUE(data_string.find("lane:2") != std::string::npos);

    // Test parsing back
    LaneKeepingData parsed_data = LaneKeepingData::fromString(data_string);
    EXPECT_EQ(parsed_data.lane_status, 2);
}

TEST_F(LaneKeepingHandlerTest, InvalidDataString) {
    // Test handling of invalid data strings
    std::string invalid_string = "invalid_data";

    // Should handle gracefully without crashing
    EXPECT_NO_THROW({
        LaneKeepingData parsed_data = LaneKeepingData::fromString(invalid_string);
        // Default values should be used
        EXPECT_EQ(parsed_data.lane_status, 0);
    });
}

TEST_F(LaneKeepingHandlerTest, StartStopCycles) {
    // Test multiple start/stop cycles
    for (int i = 0; i < 3; i++) {
        handler->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        handler->stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Test should complete without issues
    EXPECT_TRUE(true);
}

TEST_F(LaneKeepingHandlerTest, RapidDataUpdates) {
    // Test rapid data updates
    handler->start();

    for (int i = 0; i < 10; i++) {
        LaneKeepingData test_data;
        test_data.lane_status = i % 3; // Cycle through 0, 1, 2

        handler->setTestLaneKeepingData(test_data);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    handler->stop();

    // Test should complete without issues
    EXPECT_TRUE(true);
}

TEST_F(LaneKeepingHandlerTest, EdgeCaseLaneStatus) {
    // Test edge case lane status values
    std::vector<int> edge_cases = {-1, 3, 100, -100};

    for (int status : edge_cases) {
        LaneKeepingData test_data;
        test_data.lane_status = status;

        // Should handle gracefully
        EXPECT_NO_THROW({
            std::string data_string = test_data.toString();
            LaneKeepingData parsed_data = LaneKeepingData::fromString(data_string);
            EXPECT_EQ(parsed_data.lane_status, status);
        });
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
