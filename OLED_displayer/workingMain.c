#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define DISPLAY_ADDR 0x3C
#define WIDTH 128
#define HEIGHT 32
#define PAGE_AM 4

// Types of data
#define COMM 0x00
#define PXL 0x40

// Commands
#define DISPLAY_OFF 0xAE
#define DISPLAY_ON 0xAF
#define SET_CONTRAST 0x81
#define SET_MULTIPLEX_RATIO 0xA8
#define SET_MEM_ADDR_MODE 0x20
#define DISPLAY_START_LINE 0x40
#define SEG_REMAP_LEFT 0xA0
#define SEG_REMAP_RIGHT 0xA1
#define COM_SCAN_UP 0xC0
#define COM_SCAN_DOWN 0xC8
#define COM_PINS_CONFIG 0xDA
#define SET_PAGE_NO 0xB0
#define NORMAL_DISPLAY 0xA6
#define INVERSE_DISPLAY 0xA7

enum
{
        HORIZONTAL,
        VERTICAL,
        PAGE,
        INVALID
};

void    i2c_write(int fd, unsigned char type, unsigned char data)
{
        unsigned char   buffer[2];
        buffer[0] = type;
        buffer[1] = data;
        write(fd, buffer, 2);
        //usleep(1000);
}

void    init_display(int fd)
{
        i2c_write(fd, COMM, DISPLAY_OFF);
        i2c_write(fd, COMM, SET_CONTRAST);
        i2c_write(fd, COMM, 0xFF);

        i2c_write(fd, COMM, SET_MULTIPLEX_RATIO);
        i2c_write(fd, COMM, 31); //0-63

        i2c_write(fd, COMM, SET_MEM_ADDR_MODE);
        i2c_write(fd, COMM, HORIZONTAL);

        i2c_write(fd, COMM, DISPLAY_START_LINE | 0); //0-63
        i2c_write(fd, COMM, SEG_REMAP_RIGHT);
        i2c_write(fd, COMM, COM_SCAN_DOWN);

        i2c_write(fd, COMM, COM_PINS_CONFIG);
        i2c_write(fd, COMM, 0x02); //TODO

        i2c_write(fd, COMM, 0xD5); //TODO
        i2c_write(fd, COMM, 0x80); //TODO

        i2c_write(fd, 0x00, 0x8D); //TODO
        i2c_write(fd, 0x00, 0x14); //TODO

        i2c_write(fd, COMM, SET_PAGE_NO | 0);
        i2c_write(fd, COMM, 0x00);
        i2c_write(fd, COMM, 0x10);

        for (int px=0; px<WIDTH*PAGE_AM; px++)
                i2c_write(fd, PXL, 0x00);
        /*for (int page=0; page<8; page++) //Probably in horizontal adressing mode is easier
        {
                i2c_write(fd, COMM, SET_PAGE_NO | page);
                i2c_write(fd, COMM, 0x00);
                i2c_write(fd, COMM, 0x10);
                for (int col = 0; col < 128; col++)
                        i2c_write(fd, PXL, 0x00);
        }*/
        i2c_write(fd, COMM, NORMAL_DISPLAY);
        i2c_write(fd, COMM, DISPLAY_ON);
}

void    render(int fd, unsigned char *buff, unsigned char pxl)
{
        i2c_write(fd, COMM, SET_PAGE_NO | 0x00);
        i2c_write(fd, COMM, 0x00);
        i2c_write(fd, COMM, 0x10);

        for (int page=0; page<PAGE_AM; page++)
        {
                for (int col=0; col<WIDTH; col++)
                {
                        unsigned char   byte = 0x00;
                        for (int bit=0; bit<8; bit++)
                        {
                                int     y = page * 8 + bit;
                                if (buff[y*WIDTH + col] == pxl)
                                {
                                        byte |= (1 << bit);
                                }
                        }
                        i2c_write(fd, PXL, byte);
                }
        }
}

void    put_pixel(unsigned char *buffer, unsigned char x, unsigned char y, unsigned char pxl)
{
        buffer[y * WIDTH + x] = pxl;
}

int     main(void)
{
        int     fd;

        printf("Opening connection I2C...\n");

        // Opens connection I2C
        fd = open("/dev/i2c-1", O_RDWR);
        if (fd < 0)
        {
                printf("Error opening connection I2C\n");
                return (1);
        }

        printf("Configuring display address...\n");
        // Configures display address
        if (ioctl(fd, I2C_SLAVE, DISPLAY_ADDR) < 0)
        {
                printf("Error configuring display address\n");
                close(fd);
                return (1);
        }
        printf("Initializing display...\n");
        init_display(fd);

        printf("Creating image...\n");
        unsigned char   buff[WIDTH*HEIGHT];
	printf("Rendering image...\n");
        render(fd, buff, 'a');
}

