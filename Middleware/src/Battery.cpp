#include "Battery.hpp"

Battery::Battery(std::shared_ptr<IBatteryReader> reader)
    : batteryReader(reader) {
  _name = "battery";
  _sensorData["battery"] = std::make_shared<SensorData>("battery", false);
  _sensorData["battery"]->value.store(0);

  _sensorData["charging"] = std::make_shared<SensorData>("charging", false);
  _sensorData["charging"]->value.store(0);

  // _sensorData["power"] = std::make_shared<SensorData>("power", true);
  // _sensorData["power"]->value.store(0);
}

Battery::~Battery() {}

const std::string &Battery::getName() const { return _name; }

void Battery::updateSensorData() {
  readSensor();
  checkUpdated();
}

void Battery::checkUpdated() {
  for (const auto &[name, sensor] : _sensorData) {
    if (sensor->oldValue.load() != sensor->value.load()) {
      sensor->updated.store(true);
    } else {
      sensor->updated.store(false);
    }
  }
}

void Battery::readSensor() {
  auto oldBattery = _sensorData["battery"]->value.load();
  auto oldCharging = _sensorData["charging"]->value.load();
  auto battery = batteryReader->getPercentage();
  auto charging = batteryReader->isCharging();

  _sensorData["battery"]->oldValue.store(oldBattery);
  _sensorData["charging"]->oldValue.store(oldCharging);
  _sensorData["battery"]->value.store(battery);
  _sensorData["charging"]->value.store(charging);
  // _sensorData["power"]->value.store(20);

  // Battery charging cheat. only goes up while charging, only goes down while
  // not charging
  if (!oldBattery) {
    return;
  } else if (charging) {
    auto new_value = (battery > oldBattery ? battery : oldBattery);
    _sensorData["battery"]->value.store(new_value);
  } else {
    auto new_value = (battery < oldBattery ? battery : oldBattery);
    _sensorData["battery"]->value.store(new_value);
  }

  // Update timestamps using steady_clock
  auto now = std::chrono::steady_clock::now();
  _sensorData["battery"]->timestamp = now;
  _sensorData["charging"]->timestamp = now;
  // _sensorData["power"]->timestamp = now;
}

std::unordered_map<std::string, std::shared_ptr<SensorData>>
Battery::getSensorData() const {
  return _sensorData;
}

bool Battery::getCharging() const {
  return _sensorData.at("charging")->value.load() > 0;
}
