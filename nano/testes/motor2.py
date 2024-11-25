import smbus2
import time

bus = smbus2.SMBus(1)
SERVO_ADDR = 0x40
STEERING_CHANNEL = 0



def reset():
    """Reset completo do PCA9685"""
    try:
        bus.write_byte_data(SERVO_ADDR, 0x00, 0x06)  # Reset
        time.sleep(0.1)
        return True
    except Exception as e:
        print(f"Erro no reset: {e}")
        return False

def init_servo():
    """Inicialização completa do servo"""
    print("Inicializando servo...")
    

    reset()
    time.sleep(0.1)
    
    try:
        # Modo sleep para configuração
        bus.write_byte_data(SERVO_ADDR, 0x00, 0x10)  # MODE1: SLEEP
        time.sleep(0.1)
        
        # Configura frequência PWM (~50Hz)
        prescale = 0x79  # Para ~50Hz
        bus.write_byte_data(SERVO_ADDR, 0xFE, prescale)
        time.sleep(0.1)
        
        # Configura MODE2
        bus.write_byte_data(SERVO_ADDR, 0x01, 0x04)  # MODE2: OUTDRV
        time.sleep(0.1)
        
        # Ativa e configura auto-increment
        bus.write_byte_data(SERVO_ADDR, 0x00, 0x20)  # MODE1: Normal + Auto-Increment
        time.sleep(0.1)
        
        print("Servo inicializado")
        return True
    except Exception as e:
        print(f"Erro na inicialização: {e}")
        return False

def set_pwm( channel, on_value, off_value):
    """Define PWM com verificação"""
    try:
        # Registradores para o canal
        base_reg = 0x06 + (channel * 4)
        
        # Define valores PWM
        bus.write_byte_data(SERVO_ADDR, base_reg, on_value & 0xFF)
        bus.write_byte_data(SERVO_ADDR, base_reg + 1, on_value >> 8)
        bus.write_byte_data(SERVO_ADDR, base_reg + 2, off_value & 0xFF)
        bus.write_byte_data(SERVO_ADDR, base_reg + 3, off_value >> 8)
        time.sleep(1)
        # Verifica se os valores foram escritos corretamente
        read_on_l = bus.read_byte_data(SERVO_ADDR, base_reg)
        read_on_h = bus.read_byte_data(SERVO_ADDR, base_reg + 1)
        read_off_l = bus.read_byte_data(SERVO_ADDR, base_reg + 2)
        read_off_h = bus.read_byte_data(SERVO_ADDR, base_reg + 3)

        
        if (read_on_l != (on_value & 0xFF) or 
            read_on_h != (on_value >> 8) or 
            read_off_l != (off_value & 0xFF) or 
            read_off_h != (off_value >> 8)):
            print("Valores PWM não foram escritos corretamente")
            return False
            
        return True
    except Exception as e:
        print(f"Erro ao definir PWM: {e}")
        return False


init_servo()

set_pwm(STEERING_CHANNEL, 0, 307)#centro
set_pwm(STEERING_CHANNEL, 0, 225)#left
# set_pwm(STEERING_CHANNEL, 0, 307)#centro
# set_pwm(STEERING_CHANNEL, 0, 389)#right


# set_pwm(STEERING_CHANNEL, 0, 307)#centro
# set_pwm(STEERING_CHANNEL, 0, 225)#left
# set_pwm(STEERING_CHANNEL, 0, 307)#centro
# set_pwm(STEERING_CHANNEL, 0, 389)#right