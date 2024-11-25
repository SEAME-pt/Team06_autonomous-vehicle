import time
import smbus
import math

class PCA9685:
    def __init__(self, address=0x60):
        self.bus = smbus.SMBus(1)  # Use bus 1
        self.address = address
        print(f"Iniciando PCA9685 no endereço {hex(address)}")
        
        # Reset do PCA9685
        try:
            # Mode 1, auto increment
            self.bus.write_byte_data(self.address, 0x00, 0x20)  
            # Set frequency to 60Hz
            self.set_pwm_freq(60)
            print("PCA9685 inicializado com sucesso")
        except Exception as e:
            print(f"Erro ao inicializar PCA9685: {e}")
    
    def set_pwm_freq(self, freq):
        """Set PWM frequency"""
        prescale = int(math.floor(25000000.0 / 4096.0 / freq - 1))
        oldmode = self.bus.read_byte_data(self.address, 0x00)
        newmode = (oldmode & 0x7F) | 0x10
        self.bus.write_byte_data(self.address, 0x00, newmode)
        self.bus.write_byte_data(self.address, 0xFE, prescale)
        self.bus.write_byte_data(self.address, 0x00, oldmode)
        time.sleep(0.005)
        self.bus.write_byte_data(self.address, 0x00, oldmode | 0xa1)
    
    def set_pwm(self, channel, value):
        """Set PWM value (0-4095) for a channel"""
        value = min(value, 4095)
        value = max(value, 0)
        
        # LED start at 0
        self.bus.write_byte_data(self.address, 0x06 + 4 * channel, 0)
        self.bus.write_byte_data(self.address, 0x07 + 4 * channel, 0)
        # LED end at value
        self.bus.write_byte_data(self.address, 0x08 + 4 * channel, value & 0xFF)
        self.bus.write_byte_data(self.address, 0x09 + 4 * channel, value >> 8)
        print(f"Canal {channel} definido para {value}")

def test_motors():
    try:
        # Inicializar PCA9685
        print("Iniciando controlador PCA9685...")
        pwm = PCA9685(address=0x60)
        time.sleep(0.1)  # Pequena pausa para estabilizar
        
        # print("\nTestando motor esquerdo...")
        
        pwm.set_pwm(0, 4095)  
        pwm.set_pwm(1, 0)     
        pwm.set_pwm(2, 4095) 
      
        pwm.set_pwm(5, 4095)  
        pwm.set_pwm(6, 0)     
        pwm.set_pwm(7, 4095)  

        time.sleep(2)
        pwm.set_pwm(0, 0)
        pwm.set_pwm(1, 0)
        pwm.set_pwm(2, 0)
        pwm.set_pwm(3, 0)
        pwm.set_pwm(4, 0)
        pwm.set_pwm(5, 0)
        pwm.set_pwm(6, 0)
        pwm.set_pwm(7, 0)
        pwm.set_pwm(8, 0)
          
        time.sleep(2)
        
        #tras
        pwm.set_pwm(0, 4095)  
        pwm.set_pwm(1, 4095)     
        pwm.set_pwm(2, 0) 

        pwm.set_pwm(6, 4095)  
        pwm.set_pwm(7, 4095)     
        pwm.set_pwm(8, 0)  
       
        time.sleep(2)
        pwm.set_pwm(0, 0)
        pwm.set_pwm(1, 0)
        pwm.set_pwm(2, 0)
        pwm.set_pwm(3, 0)
        pwm.set_pwm(4, 0)
        pwm.set_pwm(5, 0)
        pwm.set_pwm(6, 0)
        pwm.set_pwm(7, 0)
        pwm.set_pwm(8, 0)


        # pwm.set_pwm(0, 0)  # Canal 0 - PWM meio
        # pwm.set_pwm(1, 0)     # Canal 1 - Desligado
        # pwm.set_pwm(2, 0)  # Canal 2 - Totalmente ligado
        # time.sleep(2)
        
        # pwm.set_pwm(0, 4095)  
        # pwm.set_pwm(1, 0)     
        # pwm.set_pwm(2, 4095)  
        # time.sleep(2)


        
        # Motor Esquerdo - Parar
        # print("Motor direito - parado")
        # pwm.set_pwm(6, 4095)
        # pwm.set_pwm(7, 4095)
        
        # time.sleep(1)
        
        # # Motor Esquerdo - Para trás
        # print("Motor esquerdo - reverso")
        # pwm.set_pwm(0, 0)     # Canal 0 - Desligado
        # pwm.set_pwm(2, 0)     # Canal 2 - Desligado
        # pwm.set_pwm(1, 4095)  # Canal 1 - Totalmente ligado
        # time.sleep(2)
        
        # # Motor Esquerdo - Parar
        # print("Motor esquerdo - parado")
        # pwm.set_pwm(0, 0)
        # pwm.set_pwm(1, 0)
        # pwm.set_pwm(2, 0)
        # time.sleep(1)
        
        # print("\nTestando motor direito...")
        # # Motor Direito - Para frente
        # print("Motor direito - frente")
        # pwm.set_pwm(3, 2048)  # Canal 3 - PWM meio
        # pwm.set_pwm(4, 0)     # Canal 4 - Desligado
        # pwm.set_pwm(5, 4095)  # Canal 5 - Totalmente ligado
        # time.sleep(2)
        
        # # Motor Direito - Parar
        # print("Motor direito - parado")
        # pwm.set_pwm(3, 0)
        # pwm.set_pwm(4, 0)
        # pwm.set_pwm(5, 0)
        # time.sleep(1)
        
        # # Motor Direito - Para trás
        # print("Motor direito - reverso")
        # pwm.set_pwm(3, 0)     # Canal 3 - Desligado
        # pwm.set_pwm(5, 0)     # Canal 5 - Desligado
        # pwm.set_pwm(4, 4095)  # Canal 4 - Totalmente ligado
        # time.sleep(2)
        
        # # Motor Direito - Parar
        # print("Motor direito - parado")
        # pwm.set_pwm(3, 0)
        # pwm.set_pwm(4, 0)
        # pwm.set_pwm(5, 0)
        
    except Exception as e:
        print(f"Erro durante o teste: {e}")
    finally:
        if 'pwm' in locals():
            print("\nParando todos os motores...")
            for channel in range(6):
                pwm.set_pwm(channel, 0)
        print("Teste finalizado")

if __name__ == "__main__":
    print("Iniciando teste dos motores...")
    test_motors()