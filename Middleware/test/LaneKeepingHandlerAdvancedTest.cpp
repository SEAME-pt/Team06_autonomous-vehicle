#include <gtest/gtest.h>
#include "LaneKeepingHandler.hpp"
#include "MockPublisher.hpp"
#include <memory>
#include <thread>
#include <chrono>
#include <sstream>

class LaneKeepingHandlerAdvancedTest : public ::testing::Test {
protected:
    void SetUp() override {
        zmq_context = std::make_unique<zmq::context_t>(1);
        mock_publisher = std::make_shared<MockPublisher>();

        handler = std::make_unique<LaneKeepingHandler>(
            "tcp://127.0.0.1:5559",
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

TEST_F(LaneKeepingHandlerAdvancedTest, ParseValidDataFormats) {
    // Test various valid data formats
    std::vector<std::string> valid_formats = {
        "lane:0",
        "lane:1;",
        "lane:2",
        "lane:-1;",
        "lane:100;"
    };

    std::vector<int> expected_values = {0, 1, 2, -1, 100};

    for (size_t i = 0; i < valid_formats.size(); ++i) {
        LaneKeepingData parsed = LaneKeepingData::fromString(valid_formats[i]);
        EXPECT_EQ(parsed.lane_status, expected_values[i])
            << "Failed to parse: " << valid_formats[i];
    }
}

TEST_F(LaneKeepingHandlerAdvancedTest, ParseInvalidDataFormats) {
    // Test various invalid data formats
    std::vector<std::string> invalid_formats = {
        "invalid",
        "lane:",
        "lane:abc",
        "lane:1.5",
        "not_lane:1",
        "",
        "lane:1;extra"
    };

    for (const auto& invalid_format : invalid_formats) {
        LaneKeepingData parsed = LaneKeepingData::fromString(invalid_format);
        EXPECT_EQ(parsed.lane_status, 0)
            << "Should default to 0 for invalid format: " << invalid_format;
    }
}

TEST_F(LaneKeepingHandlerAdvancedTest, ParseEdgeCaseNumbers) {
    // Test edge case numbers
    std::vector<std::string> edge_cases = {
        "lane:0",
        "lane:1",
        "lane:-1",
        "lane:2147483647",  // max int
        "lane:-2147483648"  // min int
    };

    std::vector<int> expected_values = {0, 1, -1, 2147483647, -2147483648};

    for (size_t i = 0; i < edge_cases.size(); ++i) {
        LaneKeepingData parsed = LaneKeepingData::fromString(edge_cases[i]);
        EXPECT_EQ(parsed.lane_status, expected_values[i])
            << "Failed to parse edge case: " << edge_cases[i];
    }
}

TEST_F(LaneKeepingHandlerAdvancedTest, ParseOverflowNumbers) {
    // Test number overflow scenarios
    std::vector<std::string> overflow_cases = {
        "lane:999999999999999999999",
        "lane:-999999999999999999999"
    };

    for (const auto& overflow_case : overflow_cases) {
        // Should handle gracefully without crashing
        EXPECT_NO_THROW({
            LaneKeepingData parsed = LaneKeepingData::fromString(overflow_case);
            // Should default to 0 on overflow
            EXPECT_EQ(parsed.lane_status, 0);
        });
    }
}

TEST_F(LaneKeepingHandlerAdvancedTest, ToStringFormat) {
    // Test toString format consistency
    std::vector<int> test_values = {0, 1, -1, 100, -100};

    for (int value : test_values) {
        LaneKeepingData data;
        data.lane_status = value;

        std::string result = data.toString();
        EXPECT_TRUE(result.find("lane:") == 0) << "Should start with 'lane:'";
        EXPECT_TRUE(result.find(";") != std::string::npos) << "Should end with ';'";

        // Test round-trip conversion
        LaneKeepingData parsed = LaneKeepingData::fromString(result);
        EXPECT_EQ(parsed.lane_status, value) << "Round-trip failed for value: " << value;
    }
}

TEST_F(LaneKeepingHandlerAdvancedTest, ProductionModeInitialization) {
    // Test production mode initialization
    auto prod_handler = std::make_unique<LaneKeepingHandler>(
        "tcp://127.0.0.1:5559",
        *zmq_context,
        mock_publisher,
        false  // production mode
    );

    EXPECT_NE(prod_handler, nullptr);

    // Should not accept test data in production mode
    LaneKeepingData test_data;
    test_data.lane_status = 1;
    prod_handler->setTestLaneKeepingData(test_data);

    // In production mode, setTestLaneKeepingData should be ignored
    // (no way to directly test this, but it shouldn't crash)
    EXPECT_NO_THROW(prod_handler->setTestLaneKeepingData(test_data));

    prod_handler->stop();
}

TEST_F(LaneKeepingHandlerAdvancedTest, ProductionModeWithoutPublisher) {
    // Test production mode without publisher (should throw)
    EXPECT_THROW({
        auto handler_no_pub = std::make_unique<LaneKeepingHandler>(
            "tcp://127.0.0.1:5559",
            *zmq_context,
            nullptr,  // no publisher
            false     // production mode
        );
    }, std::runtime_error);
}

TEST_F(LaneKeepingHandlerAdvancedTest, TestModeWithoutPublisher) {
    // Test mode should work without publisher
    EXPECT_NO_THROW({
        auto test_handler = std::make_unique<LaneKeepingHandler>(
            "tcp://127.0.0.1:5559",
            *zmq_context,
            nullptr,  // no publisher
            true      // test mode
        );
        test_handler->stop();
    });
}

TEST_F(LaneKeepingHandlerAdvancedTest, MultipleStartStopCycles) {
    // Test multiple start/stop cycles for resource management
    for (int i = 0; i < 5; ++i) {
        handler->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        handler->stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Should complete without issues
    EXPECT_TRUE(true);
}

TEST_F(LaneKeepingHandlerAdvancedTest, ConcurrentDataUpdates) {
    // Test concurrent data updates
    handler->start();

    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([this, i]() {
            for (int j = 0; j < 10; ++j) {
                LaneKeepingData data;
                data.lane_status = (i + j) % 3;
                handler->setTestLaneKeepingData(data);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }

    handler->stop();

    // Should complete without crashes or deadlocks
    EXPECT_TRUE(true);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
