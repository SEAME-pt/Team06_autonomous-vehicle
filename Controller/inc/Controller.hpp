#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include <cstring>
#include <fcntl.h> // Para open()
#include <iostream>
#include <linux/i2c-dev.h> // Interface padrão do Linux para I2C
#include <linux/joystick.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h> // Para close(), usleep()

const unsigned char MAX_AXES = 8;
const unsigned char MAX_BUTTONS = 14;
const int16_t MAX_AXIS_VALUE = 32767; // Valor máximo do eixo no Linux

const unsigned char LEFT_BUTTON_ONE = 6;
const unsigned char LEFT_BUTTON_TWO = 8;

const unsigned char RIGHT_BUTTON_ONE = 7;
const unsigned char RIGHT_BUTTON_TWO = 9;

const unsigned char X_BUTTON = 3;
const unsigned char Y_BUTTON = 4;

const unsigned char A_BUTTON = 0;
const unsigned char B_BUTTON = 1;

const unsigned char SELECT_BUTTON = 10;
const unsigned char START_BUTTON = 11;
const unsigned char HOME_BUTTON = 12;

class BackMotors;
class FServo;

class Controller {
private:
  const std::string _device = "/dev/input/js0";
  char _name[128] = "Team06";
  bool _quit = false;
  bool _doEvent = false;

  int _rawAxes[MAX_AXES];
  float _normalizedAxes[MAX_AXES];
  char _buttons[MAX_BUTTONS];

protected:
  virtual float _normalizeAxisValue(int value);

public:
  int _joyFd = -1;

  Controller();
  virtual ~Controller();

  virtual void openDevice();
  virtual bool isQuit() const;
  virtual bool isConnected() const;
  virtual bool isEvent() const;

  virtual bool readEvent();
  virtual float getAxis(int axis) const;
  virtual int getRawAxis(int axis) const;
  virtual bool getButton(int button) const;

  virtual void setButton(int button, bool value);
  virtual void setRawAxis(int axis, int value);
  virtual void setNormalizedAxis(int axis, float value);
  virtual void setQuit(bool value);
};

#endif
