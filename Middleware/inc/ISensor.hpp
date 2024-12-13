#ifndef ISENSOR_HPP
#define ISENSOR_HPP

#include <string>

class ISensor {
public:
    virtual std::string getName() const = 0;
    virtual float getValue() const = 0;
    virtual ~ISensor() = default;
};

#endif
