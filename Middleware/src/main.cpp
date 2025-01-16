#include "Middleware.hpp"
#include "Battery.hpp"
#include <thread>
#include <chrono>

int main() {
    std::string zmq_c_address = "tcp://127.0.0.1:5556";
    std::string zmq_nc_address = "tcp://127.0.0.1:5555";
    Middleware middleware(zmq_c_address, zmq_nc_address);
    Battery battery("battery");
    middleware.addSensor(false, &battery);
    middleware.start();
    std::this_thread::sleep_for(std::chrono::seconds(30));
    middleware.stop();
    return 0;
}
