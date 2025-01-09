#include <iostream>
#include <zmq.hpp>
#include <thread>
#include <fstream>
#include "CanReader.h"
#include "BatteryReader.h"

// Include VSS tree structure (manually parse or use a library for YAML)
#include "vss_tree.hpp"

CanReader canReader;
BatteryReader batteryReader;

void updateVSSData(VSSNode& rootNode) {
    // Update VSS signals with sensor values
    rootNode["Vehicle.Speed"].setValue(canReader.getSpeed());
    rootNode["Vehicle.Battery"].setValue(batteryReader.getBattery());
}

std::string serializeVSSData(const VSSNode& rootNode) {
    // Serialize the VSS tree to a string (e.g., JSON)
    return rootNode.toJSON();
}

int main() {
    zmq::context_t context(1);
    zmq::socket_t publisher(context, ZMQ_PUB);
    publisher.bind("tcp://*:5555");

    // Load VSS tree
    VSSNode rootNode = VSSNode::loadFromFile("vss_tree/vehicle.vspec");

    while (true) {
        // Update VSS data with real sensor values
        updateVSSData(rootNode);

        // Serialize and send VSS data
        std::string message = serializeVSSData(rootNode);
        zmq::message_t zmq_message(message.size());
        memcpy(zmq_message.data(), message.c_str(), message.size());
        publisher.send(zmq_message, zmq::send_flags::none);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    return 0;
}
