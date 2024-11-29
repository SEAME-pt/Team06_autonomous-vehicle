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


class MotorController:
    def __init__(self, address=0x60):
        self.pwm = PCA9685(address)
        time.sleep(0.1)  # Pequena pausa para estabilizar
        
    def stop(self):
        """Para todos os motores"""
        for channel in range(9):
            self.pwm.set_pwm(channel, 0)
            
    def set_speed(self, speed):
        """
        Define a velocidade dos motores (0-100%)
        """
        if speed < 0:
            speed = 0
        if speed > 100:
            speed = 100
            
        # Converte porcentagem (0-100) para valor PWM (0-4095)
        pwm_value = int((speed / 100.0) * 4095)
        return pwm_value
        
    def forward(self, speed):
        """
        Movimento para frente com velocidade controlada
        """
        pwm_value = self.set_speed(speed)
        # Motor esquerdo
        self.pwm.set_pwm(0, pwm_value)  # IN1
        self.pwm.set_pwm(1, 0)          # IN2
        self.pwm.set_pwm(2, pwm_value)  # ENA
        
        # Motor direito
        self.pwm.set_pwm(5, pwm_value)  # IN3
        self.pwm.set_pwm(6, 0)          # IN4
        self.pwm.set_pwm(7, pwm_value)  # ENB
        
    def backward(self, speed):
        """
        Movimento para trás com velocidade controlada
        """
        pwm_value = self.set_speed(speed)
        # Motor esquerdo
        self.pwm.set_pwm(0, pwm_value)  # IN1
        self.pwm.set_pwm(1, pwm_value)  # IN2
        self.pwm.set_pwm(2, 0)          # ENA
        
        # Motor direito
        self.pwm.set_pwm(6, pwm_value)  # IN3
        self.pwm.set_pwm(7, pwm_value)  # IN4
        self.pwm.set_pwm(8, 0)          # ENB

def test_variable_speeds():
    try:
        print("Iniciando teste de velocidades...")
        motors = MotorController()
        
        # Teste movimento para frente com diferentes velocidades
        print("\nTestando movimento para frente...")
        speeds = [25, 50, 75, 100]  # Diferentes velocidades em porcentagem
        
        for speed in speeds:
            print(f"\nVelocidade: {speed}%")
            motors.forward(speed)
            time.sleep(2)
            motors.stop()
            time.sleep(1)
            
        # Teste movimento para trás com diferentes velocidades
        print("\nTestando movimento para trás...")
        for speed in speeds:
            print(f"\nVelocidade: {speed}%")
            motors.backward(speed)
            time.sleep(2)
            motors.stop()
            time.sleep(1)
            
        print("\nTeste concluído!")
        
    except Exception as e:
        print(f"Erro durante o teste: {e}")
        motors.stop()

if __name__ == "__main__":
    test_variable_speeds()