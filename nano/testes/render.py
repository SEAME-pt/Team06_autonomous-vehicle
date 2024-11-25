import smbus2
import numpy as np
import time

class OLED:
    # Constantes do display
    ADDR = 0x3C
    WIDTH = 128
    HEIGHT = 64
    PAGES = 8

    # Comandos do SSD1306
    CMD_DISPLAY_OFF = 0xAE
    CMD_DISPLAY_ON = 0xAF
    CMD_SET_CONTRAST = 0x81
    CMD_NORMAL_DISPLAY = 0xA6

    # Fonte 8X8
    FONT = {
        'A': [0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11, 0x00],
        'B': [0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E, 0x00],
        'C': [0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E, 0x00],
        'D': [0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E, 0x00],
        'E': [0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F, 0x00],
        'F': [0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10, 0x00],
        'G': [0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E, 0x00],
        'H': [0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11, 0x00],
        'I': [0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F, 0x00],
        'J': [0x1F, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C, 0x00],
        'K': [0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11, 0x00],
        'L': [0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F, 0x00],
        'M': [0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11, 0x00],
        'N': [0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x00],
        'O': [0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E, 0x00],
        'P': [0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10, 0x00],
        'Q': [0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D, 0x00],
        'R': [0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11, 0x00],
        'S': [0x0E, 0x11, 0x10, 0x0E, 0x01, 0x11, 0x0E, 0x00],
        'T': [0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00],
        'U': [0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E, 0x00],
        'V': [0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04, 0x00],
        'W': [0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11, 0x00],
        'X': [0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11, 0x00],
        'Y': [0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04, 0x00],
        'Z': [0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F, 0x00],
        '0': [0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E, 0x00],
        '1': [0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E, 0x00],
        '2': [0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F, 0x00],
        '3': [0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E, 0x00],
        '4': [0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02, 0x00],
        '5': [0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E, 0x00],
        '6': [0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E, 0x00],
        '7': [0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08, 0x00],
        '8': [0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E, 0x00],
        '9': [0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C, 0x00],
        ' ': [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
        ':': [0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00, 0x00],
        '.': [0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00],
        '!': [0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x00, 0x0C, 0x00],
        '?': [0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04, 0x00],
        '-': [0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00],
        '_': [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00],
        '(': [0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02, 0x00],
        ')': [0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08, 0x00],
    }

    def __init__(self, bus_number=1):
        """Inicializa o display OLED"""
        self.bus = smbus2.SMBus(bus_number)
        self.buffer = np.zeros((self.HEIGHT, self.WIDTH), dtype=np.uint8)

    def write_cmd(self, cmd):
        """Envia um comando para o display"""
        self.bus.write_byte_data(self.ADDR, 0x00, cmd)
        time.sleep(0.001)

    def write_data(self, data):
        """Envia dados para o display"""
        self.bus.write_byte_data(self.ADDR, 0x40, data)
        time.sleep(0.001)

    def init(self):
        """Inicializa o display com as configurações básicas"""
        commands = [
            self.CMD_DISPLAY_OFF,    # display off
            0x20, 0x00,             # memory addressing mode
            0x40,                   # set display start line
            0xA1,                   # segment remap
            0xC8,                   # COM output scan direction
            0xDA, 0x12,            # COM pins hardware configuration
            0x81, 0xFF,            # contrast control
            0xA4,                   # disable entire display on
            0xA6,                   # normal display
            0xD5, 0x80,            # set display clock divide ratio/oscillator frequency
            0x8D, 0x14,            # set DC-DC enable
            self.CMD_DISPLAY_ON     # display on
        ]
        
        for cmd in commands:
            self.write_cmd(cmd)
        
        self.clear()
        self.display()

    def set_pixel(self, x, y, color=1):
        """Define o estado de um pixel"""
        if 0 <= x < self.WIDTH and 0 <= y < self.HEIGHT:
            self.buffer[y, x] = color

    def clear(self):
        """Limpa o buffer do display"""
        self.buffer.fill(0)

    def fill(self, color=1):
        """Preenche todo o display"""
        self.buffer.fill(color)

    def display(self):
        """Atualiza o display com o conteúdo do buffer"""
        for page in range(self.PAGES):
            self.write_cmd(0xB0 + page)
            self.write_cmd(0x00)
            self.write_cmd(0x10)
            
            for col in range(self.WIDTH):
                byte = 0
                for bit in range(8):
                    y = page * 8 + bit
                    if self.buffer[y, col]:
                        byte |= (1 << bit)
                self.write_data(byte)

    def text(self, x, y, string, color=1, scale=1):
        """Escreve texto na posição especificada com o devido espaçamento e escala"""
        for char in string.upper():
            if char in self.FONT:
                char_data = self.FONT[char]
                
                # Itera sobre as linhas do caractere (8x8)
                for cy in range(8):
                    row = char_data[cy]
                    
                    # Itera sobre os bits de cada linha
                    for cx in range(8):
                        if row & (0x80 >> cx):  # Verifica se o bit correspondente está ativado
                            if scale == 1:
                                self.set_pixel(x + cx, y + cy, color)  # Escala 1: pixel único
                            else:
                                # Desenha o caractere com escala
                                for sy in range(scale):
                                    for sx in range(scale):
                                        self.set_pixel(x + cx * scale + sx, 
                                                        y + cy * scale + sy, 
                                                        color)
            x += 8 * scale  # Avança para a próxima posição horizontal após o caractere



    def line(self, x0, y0, x1, y1, color=1):
        """Desenha uma linha"""
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
        y = y0
        y_step = 1 if y0 < y1 else -1

        for x in range(x0, x1 + 1):
            if steep:
                self.set_pixel(y, x, color)
            else:
                self.set_pixel(x, y, color)
            error -= dy
            if error < 0:
                y += y_step
                error += dx

    def rect(self, x, y, w, h, color=1, fill=False):
        """Desenha um retângulo"""
        if fill:
            for px in range(x, x + w):
                for py in range(y, y + h):
                    self.set_pixel(px, py, color)
        else:
            for px in range(x, x + w):
                self.set_pixel(px, y, color)
                self.set_pixel(px, y + h - 1, color)
            for py in range(y, y + h):
                self.set_pixel(x, py, color)
                self.set_pixel(x + w - 1, py, color)

    def circle(self, xc, yc, r, color=1, fill=False):
        """Desenha um círculo com centro (xc, yc) e raio r"""
        x = r
        y = 0
        p = 1 - r  # Ponto inicial de decisão

        # Desenha o círculo em 8 pontos simétricos
        while x >= y:
            # Desenha os 8 pontos simétricos
            self.set_pixel(xc + x, yc + y, color)
            self.set_pixel(xc - x, yc + y, color)
            self.set_pixel(xc + x, yc - y, color)
            self.set_pixel(xc - x, yc - y, color)
            self.set_pixel(xc + y, yc + x, color)
            self.set_pixel(xc - y, yc + x, color)
            self.set_pixel(xc + y, yc - x, color)
            self.set_pixel(xc - y, yc - x, color)

            # Se o círculo for preenchido, desenha os pontos internos
            if fill:
                for i in range(xc - x, xc + x + 1):
                    self.set_pixel(i, yc + y, color)
                    self.set_pixel(i, yc - y, color)
                for i in range(xc - y, xc + y + 1):
                    self.set_pixel(i, yc + x, color)
                    self.set_pixel(i, yc - x, color)

            # Atualiza o ponto de decisão de acordo com o algoritmo de Bresenham
            y += 1
            if p <= 0:
                p += 2 * y + 1  # Aumento na direção Y
            else:
                x -= 1
                p += 2 * (y - x) + 1  # Aumento nas direções X e Y

import time

def test_display():
    """Função de teste que demonstra várias funcionalidades do display"""
    try:
        # Inicializa o display
        oled = OLED()
        oled.init()
        oled.clear()
        # oled.text(0, 28, "ola OLA!", 28)
        # oled.display()
        # time.sleep(2)        
        # Teste 1: Texto Básico
        print("Teste 1: Texto básico")
        oled.clear()
        oled.text(0, 0, "OLED TEST", 1)
        oled.text(0, 16, "Hello!", 1)
        oled.display()
        time.sleep(2)
        
        # Teste 2: Texto em diferentes tamanhos
        print("Teste 2: Texto em diferentes tamanhos")
        oled.clear()
        oled.text(0, 0, "SMALL", 1)
        oled.text(0, 16, "BIG", 2)
        oled.display()
        time.sleep(2)
        
        # Teste 3: Desenho de formas básicas
        print("Teste 3: Formas básicas")
        oled.clear()
        # Retângulo
        oled.rect(0, 0, 30, 20, 1)
        # Retângulo preenchido
        oled.rect(35, 0, 30, 20, 1, True)
        # Linha
        oled.line(0, 30, 64, 30, 1)
        # Círculo

        oled.display()
        time.sleep(2)
        
        # Teste 4: Animação simples
        print("Teste 4: Animação simples")
        for i in range(0, 128, 4):
            oled.clear()
            oled.circle(i, 32, 10, 1)
            oled.display()
            time.sleep(0.1)
        
        # Teste 5: Rosto sorridente
        print("Teste 5: Rosto sorridente")
        oled.clear()
        # Cabeça
        oled.circle(64, 32, 20, 1)
        # Olhos
        oled.circle(56, 25, 4, 1, True)
        oled.circle(72, 25, 4, 1, True)
        # Boca
        for i in range(50, 78):
            y = 35 + int((i - 64) * (i - 64) / 30)
            oled.set_pixel(i, y, 1)
        oled.display()
        time.sleep(2)
        
        # Teste 6: Contador
        print("Teste 6: Contador")
        for i in range(10):
            oled.clear()
            oled.text(0, 0, "COUNTER:", 1)
            oled.text(64, 0, str(i), 2)
            oled.display()
            time.sleep(0.5)
            
        # Teste 7: Padrão de xadrez
        print("Teste 7: Padrão de xadrez")
        oled.clear()
        for y in range(0, 64, 8):
            for x in range(0, 128, 8):
                if (x + y) % 16 == 0:
                    oled.rect(x, y, 8, 8, 1, True)
        oled.display()
        time.sleep(2)
        
        # Teste 8: Texto rolando
        print("Teste 8: Texto rolando")
        message = "OLED DISPLAY TEST - ROLLING TEXT..."
        for i in range(len(message) * 8):
            oled.clear()
            oled.text(-i, 28, message, 1)
            oled.display()
            time.sleep(0.1)
        
        # Teste final: Menu de demonstração
        print("Teste final: Menu de demonstração")
        oled.clear()
        oled.rect(0, 0, 128, 16, 1)
        oled.text(5, 4, "DEMO MENU", 1)
        oled.line(0, 16, 128, 16, 1)
        oled.text(5, 20, "> Option 1", 1)
        oled.text(5, 30, "  Option 2", 1)
        oled.text(5, 40, "  Option 3", 1)
        oled.display()
        time.sleep(3)
        
        # Limpa o display no final
        oled.clear()
        oled.text(0, 28, "TEST COMPLETE!", 1)
        oled.display()
        time.sleep(2)
        
        print("Teste concluído!")
        
    except KeyboardInterrupt:
        print("\nTeste interrompido pelo usuário")
    except Exception as e:
        print(f"Erro durante o teste: {e}")
    finally:
        # Limpa o display antes de sair
        oled.clear()
        oled.display()

if __name__ == "__main__":
    test_display()