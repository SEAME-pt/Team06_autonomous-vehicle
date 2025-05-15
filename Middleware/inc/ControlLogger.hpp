#ifndef CONTROLLOGGER_HPP
#define CONTROLLOGGER_HPP

#include <string>
#include <fstream>
#include <chrono>
#include <mutex>
#include <memory>

class ControlLogger {
public:
    explicit ControlLogger(const std::string& log_file_path = "control_updates.log");
    ~ControlLogger();

    void logControlUpdate(const std::string& command, double steering, double throttle);
    void logError(const std::string& errorMessage);

private:
    std::ofstream log_file;
    std::mutex log_mutex;
    std::string getTimestamp() const;
};

#endif
