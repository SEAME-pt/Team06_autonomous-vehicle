#ifndef DISPLAY_HPP
# define DISPLAY_HPP
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <fcntl.h>
# include <linux/i2c-dev.ih>
# include <sys/ioctl.h> //TODO: Must check if any of these libraries can be converted to c++

# define DISPLAY_ADDR 0x3C
# define WIDTH 128
# define HEIGHT 32
# define PAGE_AM 4

// Addresses to write data in
# define COMM_ADD 0x00
# define PXL_ADD 0x40

// Commands
# define DISPLAY_OFF 0xAE
# define DISPLAY_ON 0xAF
# define SET_CONTRAST 0x81
# define SET_MULTIPLEX_RATIO 0xA8
# define SET_MEM_ADDR_MODE 0x20
# define DISPLAY_START_LINE 0x40
# define SEG_REMAP_LEFT 0xA0
# define SEG_REMAP_RIGHT 0xA1
# define COM_SCAN_UP 0xC0
# define COM_SCAN_DOWN 0xC8
# define COM_PINS_CONFIG 0xDA
# define SET_PAGE_NO 0xB0
# define NORMAL_DISPLAY 0xA6
# define INVERSE_DISPLAY 0xA7

class	Display
{
	private:
		int		_fd;
		unsigned char	_buffer[HEIGHT][WIDTH];
		Display(const Display& other);
		Display&	operator=(const Display& other);
	public:
		Display();
		~Display();
}

#endif
