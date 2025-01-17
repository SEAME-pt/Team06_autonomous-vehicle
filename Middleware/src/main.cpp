#include "Middleware.hpp"
#include "Battery.hpp"
#include <thread>
#include <chrono>

int main() {
    std::string zmq_c_address = "tcp://127.0.0.1:5555";
    std::string zmq_nc_address = "tcp://127.0.0.1:5556";
    Middleware middleware(zmq_c_address, zmq_nc_address);
    Battery battery("battery");
    middleware.addSensor(false, &battery);
    middleware.start();
    middleware.stop();
    return 0;
}
