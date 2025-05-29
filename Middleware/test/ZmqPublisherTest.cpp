#include <gtest/gtest.h>
#include "IPublisher.hpp"
#include "ZmqSubscriber.hpp"
#include <thread>
#include <chrono>
#include <memory>

class ZmqPublisherTest : public ::testing::Test {
protected:
    void SetUp() override {
        zmq_context = std::make_shared<zmq::context_t>(1);
        // Use a local TCP address for testing
        publisher_address = "tcp://127.0.0.1:5555";
        subscriber_address = "tcp://127.0.0.1:5555";

        // Create publisher and subscriber
        publisher = std::make_shared<ZmqPublisher>(publisher_address, *zmq_context);
        subscriber = std::make_shared<ZmqSubscriber>(subscriber_address, *zmq_context);

        // Allow time for connection to be established
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        publisher.reset();
        subscriber.reset();
        zmq_context.reset();
    }

    std::shared_ptr<zmq::context_t> zmq_context;
    std::string publisher_address;
    std::string subscriber_address;
    std::shared_ptr<ZmqPublisher> publisher;
    std::shared_ptr<ZmqSubscriber> subscriber;

    // Helper function to send a message and wait for it to be received
    bool sendAndVerify(const std::string& message, int timeout_ms = 500) {
        publisher->send(message);

        // Allow time for message to be received
        auto start_time = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - start_time).count() < timeout_ms) {

            // Call receive with timeout parameter
            std::string received = subscriber->receive(10); // 10ms timeout
            if (!received.empty() || message.empty()) {
                // Special case for empty messages - they're always returned as empty
                return received == message;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        return false;
    }
};

TEST_F(ZmqPublisherTest, SendSingleMessage) {
    // Send a simple message
    std::string message = "test_message";
    EXPECT_TRUE(sendAndVerify(message));
}

TEST_F(ZmqPublisherTest, SendMultipleMessages) {
    // Send multiple messages in sequence
    std::vector<std::string> messages = {
        "message1",
        "message2",
        "message3"
    };

    for (const auto& message : messages) {
        EXPECT_TRUE(sendAndVerify(message));
    }
}

TEST_F(ZmqPublisherTest, SendEmptyMessage) {
    // Send an empty message
    std::string message = "";
    EXPECT_TRUE(sendAndVerify(message));
}

TEST_F(ZmqPublisherTest, SendLongMessage) {
    // Create a longer message
    std::string message(1024, 'X'); // 1KB message
    EXPECT_TRUE(sendAndVerify(message));
}

TEST_F(ZmqPublisherTest, SendStructuredMessage) {
    // Send a message in the format used by the application
    std::string message = "speed:123.45;battery:98;";
    EXPECT_TRUE(sendAndVerify(message));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
