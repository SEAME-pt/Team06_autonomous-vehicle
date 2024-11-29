import smbus
import time


FONT_8X8 = {
    '0': [0x7C, 0xC6, 0xCE, 0xDE, 0xF6, 0xE6, 0x7C, 0x00],
    '1': [0x30, 0x70, 0x30, 0x30, 0x30, 0x30, 0xFC, 0x00],
    '2': [0x78, 0xCC, 0x0C, 0x38, 0x60, 0xCC, 0xFC, 0x00],
    '3': [0x78, 0xCC, 0x0C, 0x38, 0x0C, 0xCC, 0x78, 0x00],
    '4': [0x1C, 0x3C, 0x6C, 0xCC, 0xFE, 0x0C, 0x1E, 0x00],
    '5': [0xFC, 0xC0, 0xF8, 0x0C, 0x0C, 0xCC, 0x78, 0x00],
    '6': [0x38, 0x60, 0xC0, 0xF8, 0xCC, 0xCC, 0x78, 0x00],
    '7': [0xFC, 0xCC, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00],
    '8': [0x78, 0xCC, 0xCC, 0x78, 0xCC, 0xCC, 0x78, 0x00],
    '9': [0x78, 0xCC, 0xCC, 0x7C, 0x0C, 0x18, 0x70, 0x00],
    'A': [0x30, 0x78, 0xCC, 0xCC, 0xFC, 0xCC, 0xCC, 0x00],
    'B': [0xFC, 0x66, 0x66, 0x7C, 0x66, 0x66, 0xFC, 0x00],
    'C': [0x3C, 0x66, 0xC0, 0xC0, 0xC0, 0x66, 0x3C, 0x00],
    'D': [0xF8, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0xF8, 0x00],
    'E': [0xFE, 0x62, 0x68, 0x78, 0x68, 0x62, 0xFE, 0x00],
    'F': [0xFE, 0x62, 0x68, 0x78, 0x68, 0x60, 0xF0, 0x00],
    'G': [0x3C, 0x66, 0xC0, 0xC0, 0xCE, 0x66, 0x3E, 0x00],
    'H': [0xCC, 0xCC, 0xCC, 0xFC, 0xCC, 0xCC, 0xCC, 0x00],
    'I': [0x78, 0x30, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00],
    'J': [0x1E, 0x0C, 0x0C, 0x0C, 0xCC, 0xCC, 0x78, 0x00],
    'K': [0xE6, 0x66, 0x6C, 0x78, 0x6C, 0x66, 0xE6, 0x00],
    'L': [0xF0, 0x60, 0x60, 0x60, 0x62, 0x66, 0xFE, 0x00],
    'M': [0xC6, 0xEE, 0xFE, 0xD6, 0xC6, 0xC6, 0xC6, 0x00],
    'N': [0xC6, 0xE6, 0xF6, 0xDE, 0xCE, 0xC6, 0xC6, 0x00],
    'O': [0x38, 0x6C, 0xC6, 0xC6, 0xC6, 0x6C, 0x38, 0x00],
    'P': [0xFC, 0x66, 0x66, 0x7C, 0x60, 0x60, 0xF0, 0x00],
    'Q': [0x78, 0xCC, 0xCC, 0xCC, 0xDC, 0x78, 0x1C, 0x00],
    'R': [0xFC, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0xE6, 0x00],
    'S': [0x78, 0xCC, 0x60, 0x30, 0x18, 0xCC, 0x78, 0x00],
    'T': [0xFC, 0xB4, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00],
    'U': [0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x78, 0x00],
    'V': [0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x78, 0x30, 0x00],
    'W': [0xC6, 0xC6, 0xC6, 0xD6, 0xFE, 0xEE, 0xC6, 0x00],
    'X': [0xC6, 0xC6, 0x6C, 0x38, 0x6C, 0xC6, 0xC6, 0x00],
    'Y': [0xCC, 0xCC, 0xCC, 0x78, 0x30, 0x30, 0x78, 0x00],
    'Z': [0xFC, 0xCC, 0x98, 0x30, 0x64, 0xCC, 0xFC, 0x00],
    ' ': [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
    '.': [0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x00],
    ',': [0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x20],
    '!': [0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x30, 0x00],
    '?': [0x78, 0xCC, 0x0C, 0x18, 0x30, 0x00, 0x30, 0x00],
    ':': [0x00, 0x30, 0x30, 0x00, 0x00, 0x30, 0x30, 0x00],
    '-': [0x00, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x00],
}
class Canvas:
    def __init__(self, i2c_bus=1, i2c_address=0x3C):
        self.i2c_bus = smbus.SMBus(i2c_bus)
        self.addr = i2c_address
        
        self.width = 128
        self.height = 32
        self.pages = self.height // 8
        
        # Buffer de frame: 128 x 32 pixels = 512 bytes (4 páginas de 128 bytes)
        self.buffer = [0] * (self.width * self.pages)
        self._init_display()
        
    def _command(self, *cmd):
        for c in cmd:
            self.i2c_bus.write_byte_data(self.addr, 0x00, c)
            
    def _data(self, data):
        self.i2c_bus.write_byte_data(self.addr, 0x40, data)
    
    def _init_display(self):
        # inicialização  Canvas
        init_sequence = [
            0xAE,        # display off
            0xD5, 0x80,  # set display clock
            0xA8, 0x1F,  # set multiplex ratio
            0xD3, 0x00,  # set display offset
            0x40,        # set display start line
            0x8D, 0x14,  # set charge pump
            0x20, 0x00,  # set memory mode - horizontal
            0xA1,        # set segment remap
            0xC8,        # set COM output scan direction
            0xDA, 0x02,  # set COM pins hardware configuration
            0x81, 0x8F,  # set contrast control
            0xD9, 0xF1,  # set pre-charge period
            0xDB, 0x40,  # set vcomh deselect level
            0xA4,        # display all on resume
            0xA6,        # set normal display
            0xAF         # display on
        ]
        
        for cmd in init_sequence:
            self._command(cmd)
    
    def clear(self):
        self.buffer = [0] * (self.width * self.pages)
    
    def set_pixel(self, x, y, color=1):
        if 0 <= x < self.width and 0 <= y < self.height:
            page = y // 8
            bit = y % 8
            index = x + (page * self.width)
            if color:
                self.buffer[index] |= (1 << bit)
            else:
                self.buffer[index] &= ~(1 << bit)
    
    def draw_char(self, x, y, char, scale=1):
        if char in FONT_8X8:
            char_data = FONT_8X8[char]
            for row in range(8):
                for col in range(8):
                    if char_data[row] & (1 << (7-col)):
                        for sx in range(scale):
                            for sy in range(scale):
                                self.set_pixel(x + col * scale + sx, y + row * scale + sy)

    def draw_text(self, x, y, text, scale=1):
        cursor_x = x
        char_spacing = 6 * scale
        
        for char in text.upper():
            if cursor_x + 8 * scale > self.width:
                cursor_x = x
                y += 8 * scale
            if y + 8 * scale > self.height:
                break
            self.draw_char(cursor_x, y, char, scale)
            cursor_x += char_spacing
    
    def draw_line(self, x0, y0, x1, y1):
        dx = abs(x1 - x0)
        dy = abs(y1 - y0)
        steep = dy > dx
        
        if steep:
            x0, y0 = y0, x0
            x1, y1 = y1, x1
        
        if x0 > x1:
            x0, x1 = x1, x0
            y0, y1 = y1, y0
        
        dx = x1 - x0
        dy = abs(y1 - y0)
        error = dx // 2
        ystep = 1 if y0 < y1 else -1
        
        y = y0
        for x in range(x0, x1 + 1):
            if steep:
                self.set_pixel(y, x)
            else:
                self.set_pixel(x, y)
            
            error -= dy
            if error < 0:
                y += ystep
                error += dx
    
    def draw_rect(self, x, y, width, height, fill=False):
        if width < 0:
            x += width
            width = abs(width)
        if height < 0:
            y += height
            height = abs(height)
            
        if fill:
            for i in range(x, x + width):
                for j in range(y, y + height):
                    self.set_pixel(i, j)
        else:
            for i in range(x, x + width):
                self.set_pixel(i, y)
                self.set_pixel(i, y + height - 1)
            for i in range(y, y + height):
                self.set_pixel(x, i)
                self.set_pixel(x + width - 1, i)
    
    def draw_circle(self, x_center, y_center, radius, fill=False):
        x = radius
        y = 0
        error = -radius
        
        while x >= y:
            if fill:
                for i in range(-x, x + 1):
                    self.set_pixel(x_center + i, y_center + y)
                    self.set_pixel(x_center + i, y_center - y)
                for i in range(-y, y + 1):
                    self.set_pixel(x_center + i, y_center + x)
                    self.set_pixel(x_center + i, y_center - x)
            else:
                self.set_pixel(x_center + x, y_center + y)
                self.set_pixel(x_center + y, y_center + x)
                self.set_pixel(x_center - y, y_center + x)
                self.set_pixel(x_center - x, y_center + y)
                self.set_pixel(x_center - x, y_center - y)
                self.set_pixel(x_center - y, y_center - x)
                self.set_pixel(x_center + y, y_center - x)
                self.set_pixel(x_center + x, y_center - y)
            
            error += 2 * y + 1
            y += 1
            if error >= 0:
                error += 2 * (1 - x)
                x -= 1

    def blit(self, buffer, x=0, y=0):
            buffer_height = len(buffer)
            buffer_width = len(buffer[0])
            
            for by in range(buffer_height):
                for bx in range(buffer_width):
                    dx = x + bx
                    dy = y + by
                    if 0 <= dx < self.width and 0 <= dy < self.height:
                        self.set_pixel(dx,dy,buffer[by][bx])

    def update(self):
        self._command(
            0x20, 0x00,  # Modo de endereçamento horizontal
            0x21, 0, self.width-1,  # Coluna
            0x22, 0, self.pages-1   # Página
        )
        for i in range(len(self.buffer)):
            self._data(self.buffer[i])


def test_real_image():
    display = Canvas()
    
    buffer = [[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], [0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0], [0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0], [0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0], [0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0], [0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0], [0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0], [0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0], [0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0], [0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0], [0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0], [0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0], [0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0], [0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1], [0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1], [0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1], [0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1], [0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1], [0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1], [0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1], [0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0], [0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0], [0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0], [0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0], [0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0], [0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0], [0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0], [0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0], [0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0]]
    import socket
    hostname = socket.gethostname()
    local_ip = socket.gethostbyname(hostname)


    
    display.blit(buffer,0,1)
    display.draw_text(50,5,f"IP {local_ip}")

    running = True
    #while running:
    display.update()
    time.sleep(0.1)




if __name__ == "__main__":
    #teste_text()
    #teste_faces()
    #teste_buffer()
    #teste_doom_face()
    test_real_image()