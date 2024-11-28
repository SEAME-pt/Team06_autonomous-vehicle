
#include "Display.hpp"

std::map<char, std::array<unsigned char, 8> > Display::_charBitmaps = {
	 {'A', {0x30, 0x78, 0xCC, 0xCC, 0xFC, 0xCC, 0xCC, 0x00}}
};

Display::Display()
{
	std::cout << "Opening connection I2C..." << std::endl;
	_fd = open("/dev/i2c-1", O_RDWR);
	if (_fd < 0)
		throw (std::exception());

	std::cout << "Configuring display address.." << std::endl;
	if (ioctl(_fd, I2C_SLAVE, DISPLAY_ADDR) < 0)
		throw (std::exception());
	

	_initDisplay();
}

Display::Display(std::string _testConstructor)
{
	_fd = STDOUT_FILENO;
	_initDisplay();
}
Display::Display(const Display& other): _fd(other._fd), _buffer(other._buffer) {}
Display&	Display::operator=(const Display& other)
{
	if (this != &other)
	{
		_fd = other._fd;
		_buffer = other._buffer;
	}
	return (*this);
}

Display::~Display()
{
	if (_fd > 0)
		close(_fd);
}


void	Display::_initDisplay(void)
{
	_writeCmd(DISPLAY_OFF);
	_writeCmd(SET_CONTRAST);
	_writeCmd(0xFF);

	_writeCmd(SET_MULTIPLEX_RATIO);
	_writeCmd(31);

	_writeCmd(SET_MEM_ADDR_MODE);
	_writeCmd(HORIZONTAL);

	_writeCmd(DISPLAY_START_LINE);
	_writeCmd(SEG_REMAP_RIGHT);
	_writeCmd(COM_SCAN_DOWN);

	_writeCmd(COM_PINS_CONFIG);
	_writeCmd(0x02);

	_writeCmd(0xD5);
	_writeCmd(0x80);

	_writeCmd(0x8D);
	_writeCmd(0x14);

	_resetPrintPos();
	_initBuffer();
	updateDisplay();
	
	_writeCmd(NORMAL_DISPLAY);
	_writeCmd(DISPLAY_ON);

}

void	Display::_initBuffer(void)
{
	_buffer[0] = PXL_ADD;
	_clearBuffer();
}
void	Display::_clearBuffer(void) {std::fill(_buffer.begin() + 1, _buffer.end(), 0);}
void	Display::_fillBuffer(void) {std::fill(_buffer.begin() + 1, _buffer.end(), 0xFF);}
void	Display::_resetPrintPos(void)
{
	_writeCmd(SET_PAGE_NO | 0);
	_writeCmd(0x00);
	_writeCmd(0x10);
}

void	Display::updateDisplay(void)
{
	_resetPrintPos();
	write(_fd, _buffer.data(), _buffer.size());
	std::cout << "Display updated!!" << std::endl;
}

void	Display::_writeCmd(unsigned char data) const
{
	unsigned char	buffer[2];
	buffer[0] = CMD_ADD;
	buffer[1] = data;
	write(_fd, buffer, 2);
}

void	Display::setPixel(int x, int y) {_buffer[y/8*WIDTH+x+1] |= 1<<(y%8);}
void	Display::unsetPixel(int x, int y) {_buffer[y/8*WIDTH+x+1] &= ~(1<<(y%8));}

void	Display::putText(std::string text, int x, int y)
{
	for (int i=0; i<text.size(); i++)
		putChar(text[i], (i*8)+x, y);
}
void	Display::putChar(char c, int x, int y)
{
	std::array<unsigned char, 8>	bitmap = Display::_charBitmaps[c];
	for (int i=0; i<8; i++)
		_buffer[y/8*WIDTH+x+i+1] = bitmap[i];
}