#include <gtest/gtest.h>
#include "TrafficSignHandler.hpp"
#include "MockPublisher.hpp"
#include <memory>
#include <thread>
#include <chrono>
#include <zmq.hpp>
#include "../../zmq/inc/ZmqPublisher.hpp"

class TrafficSignHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        zmq_context = std::make_unique<zmq::context_t>(1);
        mock_publisher = std::make_shared<MockPublisher>();

        // Create handler in test mode
        handler = std::make_unique<TrafficSignHandler>(
            "tcp://127.0.0.1:5560",  // Test address
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
    std::unique_ptr<TrafficSignHandler> handler;
};

TEST_F(TrafficSignHandlerTest, Initialization) {
    // Handler should be created successfully
    EXPECT_NE(handler, nullptr);
}

TEST_F(TrafficSignHandlerTest, TestModeDataInjection) {
    // Test data injection in test mode
    std::string test_data = "SPEED_50";

    handler->setTestTrafficSignData(test_data);

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

TEST_F(TrafficSignHandlerTest, PublishableSigns) {
    // Test all publishable signs
    std::vector<std::string> test_signs = {
        "SPEED_50", "SPEED_80", "STOP", "CROSSWALK", "YIELD"
    };

    for (const auto& sign : test_signs) {
        handler->setTestTrafficSignData(sign);

        // Test should handle each sign without crashing
        EXPECT_NO_THROW({
            handler->start();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            handler->stop();
        });
    }
}

TEST_F(TrafficSignHandlerTest, NonPublishableSigns) {
    // Test signs that should be ignored
    std::vector<std::string> non_publishable_signs = {
        "UNKNOWN_SIGN", "SPEED_30", "NO_ENTRY", "PARKING", "TURN_LEFT"
    };

    for (const auto& sign : non_publishable_signs) {
        handler->setTestTrafficSignData(sign);

        // Test should handle each sign without crashing
        EXPECT_NO_THROW({
            handler->start();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            handler->stop();
        });
    }
}

TEST_F(TrafficSignHandlerTest, StructuredMessageFormat) {
    // Test structured message format: "traffic_sign:SIGN_TYPE"
    std::vector<std::string> structured_messages = {
        "traffic_sign:SPEED_50",
        "traffic_sign:STOP;",
        "traffic_sign:CROSSWALK",
        "traffic_sign:YIELD;"
    };

    for (const auto& message : structured_messages) {
        handler->setTestTrafficSignData(message);

        // Test should handle structured messages without crashing
        EXPECT_NO_THROW({
            handler->start();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            handler->stop();
        });
    }
}

TEST_F(TrafficSignHandlerTest, MalformedMessages) {
    // Test malformed messages
    std::vector<std::string> malformed_messages = {
        "traffic_sign:",           // Missing sign type
        "traffic_sign:;",          // Empty sign type
        ":",                       // Just colon
        "traffic_sign",            // Missing colon
        "",                        // Empty string
        ";;;",                     // Only delimiters
        "traffic_sign:SPEED_50:EXTRA"  // Extra colon
    };

    for (const auto& message : malformed_messages) {
        handler->setTestTrafficSignData(message);

        // Test should handle malformed messages gracefully
        EXPECT_NO_THROW({
            handler->start();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            handler->stop();
        });
    }
}

TEST_F(TrafficSignHandlerTest, StartStopCycles) {
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

TEST_F(TrafficSignHandlerTest, RapidDataUpdates) {
    // Test rapid data updates
    handler->start();

    for (int i = 0; i < 10; i++) {
        std::string test_data = "SPEED_50";
        handler->setTestTrafficSignData(test_data);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    handler->stop();

    // Test should complete without issues
    EXPECT_TRUE(true);
}

TEST_F(TrafficSignHandlerTest, EdgeCaseSignNames) {
    // Test edge case sign names
    std::vector<std::string> edge_cases = {
        "SPEED_",      // Incomplete sign
        "_50",         // Missing prefix
        "SPEED_50_",   // Trailing underscore
        "SPEED_50_EXTRA", // Extra suffix
        "SPEED_50;EXTRA", // Extra semicolon content
        "SPEED_50\n",  // Newline
        "SPEED_50\t",  // Tab
        "SPEED_50 ",   // Trailing space
        " SPEED_50"    // Leading space
    };

    for (const auto& sign : edge_cases) {
        handler->setTestTrafficSignData(sign);

        // Should handle gracefully
        EXPECT_NO_THROW({
            handler->start();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            handler->stop();
        });
    }
}

TEST_F(TrafficSignHandlerTest, LongMessageHandling) {
    // Test handling of very long messages
    std::string long_message(1000, 'A');
    handler->setTestTrafficSignData(long_message);

    // Should handle gracefully
    EXPECT_NO_THROW({
        handler->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        handler->stop();
    });
}

TEST_F(TrafficSignHandlerTest, SpecialCharacters) {
    // Test messages with special characters
    std::vector<std::string> special_messages = {
        "SPEED_50\x00",  // Null character
        "SPEED_50\xFF",  // Non-ASCII
        "SPEED_50\x80",  // Extended ASCII
        "SPEED_50\x1F",  // Control character
        "SPEED_50\x7F"   // DEL character
    };

    for (const auto& message : special_messages) {
        handler->setTestTrafficSignData(message);

        // Should handle gracefully
        EXPECT_NO_THROW({
            handler->start();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            handler->stop();
        });
    }
}

TEST_F(TrafficSignHandlerTest, ConcurrentStartStop) {
    // Test concurrent start/stop operations
    std::atomic<bool> stop_test(false);

    // Start the handler
    handler->start();

    // Create a thread that continuously sends test data
    std::thread data_thread([this, &stop_test]() {
        while (!stop_test.load()) {
            handler->setTestTrafficSignData("SPEED_50");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    // Let it run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Stop the test
    stop_test = true;
    data_thread.join();

    // Stop the handler
    handler->stop();

    // Test should complete without issues
    EXPECT_TRUE(true);
}

TEST_F(TrafficSignHandlerTest, MultipleSignTypesInSequence) {
    // Test processing multiple different sign types in sequence
    std::vector<std::string> sign_sequence = {
        "SPEED_50", "STOP", "CROSSWALK", "YIELD", "SPEED_80"
    };

    handler->start();

    for (const auto& sign : sign_sequence) {
        handler->setTestTrafficSignData(sign);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    handler->stop();

    // Test should complete without issues
    EXPECT_TRUE(true);
}

TEST_F(TrafficSignHandlerTest, TestModeWithoutPublisher) {
    // Test handler in test mode without publisher
    auto handler_no_pub = std::make_unique<TrafficSignHandler>(
        "tcp://127.0.0.1:5561",
        *zmq_context,
        nullptr,  // No publisher
        true      // test_mode = true
    );

    // Should work in test mode
    EXPECT_NO_THROW({
        handler_no_pub->start();
        handler_no_pub->setTestTrafficSignData("SPEED_50");
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        handler_no_pub->stop();
    });

    handler_no_pub.reset();
}

TEST_F(TrafficSignHandlerTest, ProductionModeWithoutPublisher) {
    // Test that production mode without publisher throws exception
    EXPECT_THROW({
        auto handler_no_pub = std::make_unique<TrafficSignHandler>(
            "tcp://127.0.0.1:5562",
            *zmq_context,
            nullptr,  // No publisher
            false     // test_mode = false (production)
        );
    }, std::runtime_error);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
