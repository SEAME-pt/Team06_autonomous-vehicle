#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <iostream>

int main() {
    zmq::context_t context(1);
    zmq::socket_t subscriber(context, zmq::socket_type::sub);

    subscriber.connect("tcp://localhost:5555");
    subscriber.set(zmq::sockopt::subscribe, ""); // Subscribe to all messages

    while (true) {
        zmq::message_t message;
        zmq::recv_result_t result = subscriber.recv(message, zmq::recv_flags::none);

        if (!result) {
            std::cerr << "Failed to receive message\n";
            break; // Exit the loop if receiving the message failed
        }

        std::string json_str(static_cast<char*>(message.data()), message.size());
        nlohmann::json json_data = nlohmann::json::parse(json_str);

        std::cout << "Received Sensor Data:\n"
                  << "Name: " << json_data["name"] << "\n"
                  << "Unit: " << json_data["unit"] << "\n"
                  << "Value: " << json_data["value"] << "\n"
                  << "Type: " << json_data["type"] << "\n"
                  << "Timestamp: " << json_data["timestamp"] << "\n\n";
    }

    return 0;
}
