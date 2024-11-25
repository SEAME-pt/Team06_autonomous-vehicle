
#include "Display.hpp"

Display::Display()
{
	std::cout << "Opening connection I2C..." << std::endl;
	_fd = open("/dev/i2c-1", O_RDWR);
	if (fd < 0)
		throw std::system_error("Error opening connection I2C");
}
