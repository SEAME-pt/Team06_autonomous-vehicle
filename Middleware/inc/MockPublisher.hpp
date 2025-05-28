#ifndef MOCKPUBLISHER_HPP
#define MOCKPUBLISHER_HPP

#include "IPublisher.hpp"
#include <mutex>
#include <vector>

class MockPublisher : public IPublisher {
public:
  MockPublisher() = default;
  ~MockPublisher() override = default;

  void send(const std::string &message) override {
    std::lock_guard<std::mutex> lock(mutex);
    messages.push_back(message);
  }

  // Methods for test verification
  std::vector<std::string> getMessages() const {
    std::lock_guard<std::mutex> lock(mutex);
    return messages;
  }

  void clearMessages() {
    std::lock_guard<std::mutex> lock(mutex);
    messages.clear();
  }

  bool hasMessage(const std::string &message) const {
    std::lock_guard<std::mutex> lock(mutex);
    for (const auto &m : messages) {
      if (m == message) {
        return true;
      }
    }
    return false;
  }

  size_t messageCount() const {
    std::lock_guard<std::mutex> lock(mutex);
    return messages.size();
  }

private:
  std::vector<std::string> messages;
  mutable std::mutex mutex;
};

#endif
