#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <linux/joystick.h>
#include <sys/select.h>
#include "Logger.hpp"

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

class Joystick
{
public:
    Joystick(const std::string &device = "/dev/input/js0")
    {
        memset(raw_axes, 0, sizeof(raw_axes));
        memset(normalized_axes, 0, sizeof(normalized_axes));
        memset(buttons, 0, sizeof(buttons));

        fd = open(device.c_str(), O_RDONLY);
        if (fd == -1)
        {
            std::cerr << "Erro ao abrir o dispositivo: " << device << std::endl;
            return;
        }

        // Pega o nome do joystick
        if (ioctl(fd, JSIOCGNAME(128), name) < 0)
        {
            std::cerr << "Erro ao obter o nome do joystick" << std::endl;
            return;
        }

        std::cout << "Joystick: " << name << " aberto com sucesso!" << std::endl;
    }

    ~Joystick()
    {
        if (fd != -1)
        {
            close(fd);
            std::cout << "Joystick fechado." << std::endl;
        }
    }

    bool isQuit() const{return _quit;}
    bool isConnected() const{return fd != -1;}
    bool isEvent() const { return _doEvent; }

    bool readEvent()
    {
        struct js_event event;

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        _doEvent = false;

        struct timeval tv = {0, 0};

        int ret = select(fd + 1, &fds, nullptr, nullptr, &tv);
        if (ret == -1)
        {
            std::cerr << "Erro no select!" << std::endl;
            return false;
        }

        if (ret > 0 && FD_ISSET(fd, &fds))
        {
            ssize_t bytesRead = read(fd, &event, sizeof(event));

            if (bytesRead == -1)
            {
                std::cerr << "Erro ao ler evento do joystick!" << std::endl;
                return false;
            }

            if (bytesRead == sizeof(event))
            {
              
                switch (event.type)
                {
                    case JS_EVENT_AXIS:
                    {
                        float normalizedAxis = normalizeAxisValue(event.value);
                       // std::cout << "Eixo " << (int)event.number << ": " << normalizedAxis << std::endl;
                        if (event.number < MAX_AXES)
                        {
                            raw_axes[event.number] = event.value;
                            normalized_axes[event.number] = normalizeAxisValue(event.value);
                        }
                        _doEvent = true;
                        break;
                    }
                    case JS_EVENT_BUTTON:
                    {
                        std::cout << "Button " << (int)event.number
                                  << " " << (event.value ? "pressed" : "released") << std::endl;
    
                        if (event.number < MAX_BUTTONS)
                        {
                            buttons[event.number] = event.value;
                        }
                        if (event.number == SELECT_BUTTON && event.value == 1)
                        {
                            _quit = true;
                        }
                        _doEvent = true;
                        break;
                }
                default:
                    break;
                }
            }
        }

        return true;
    }

    float getAxis(int axis) const
    {
        if (axis < 0 || axis >= MAX_AXES)
        {
            return 0;
        }
        return normalized_axes[axis];
    }

    int getRawAxis(int axis) const
    {
        if (axis < 0 || axis >= MAX_AXES)
        {
            return 0;
        }
        return raw_axes[axis];
    }

    bool getButton(int button) const
    {
        if (button < 0 || button >= MAX_BUTTONS)
        {
            return false;
        }
        return buttons[button] == 1;
    }

private:
    int fd = -1;                     // File descriptor
    char name[128] = "Desconhecido"; // Nome do joystick
    bool _quit = false;
    bool _doEvent = false;

    int raw_axes[MAX_AXES];
    float normalized_axes[MAX_AXES];
    char buttons[MAX_BUTTONS];
    float normalizeAxisValue(int value)
    {
        return (static_cast<float>(value) / MAX_AXIS_VALUE) ;
    }
};
