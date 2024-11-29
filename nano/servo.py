import smbus2
import time

class JetRacerControl:
    def __init__(self):
        self.bus = smbus2.SMBus(1)
        self.SERVO_ADDR = 0x40
        self.STEERING_CHANNEL = 0
        self.MOTOR_A1 = 0x01
        self.MOTOR_A2 = 0x02
        self.MOTOR_B1 = 0x03
        self.MOTOR_B2 = 0x04
        self.init_servo()

    def reset_pca9685(self):
        """Reset completo do PCA9685"""
        try:
            # Soft reset
            self.bus.write_byte_data(self.SERVO_ADDR, 0x00, 0x06)  # Reset
            time.sleep(0.1)
            return True
        except Exception as e:
            print(f"Erro no reset: {e}")
            return False

    def init_servo(self):
        """Inicialização completa do servo"""
        print("Inicializando servo...")
        
        # Reset completo
        self.reset_pca9685()
        time.sleep(0.1)
        
        try:
            # Modo sleep para configuração
            self.bus.write_byte_data(self.SERVO_ADDR, 0x00, 0x10)  # MODE1: SLEEP
            time.sleep(0.1)
            
            # Configura frequência PWM (~50Hz)
            prescale = 0x79  # Para ~50Hz
            self.bus.write_byte_data(self.SERVO_ADDR, 0xFE, prescale)
            time.sleep(0.1)
            
            # Configura MODE2
            self.bus.write_byte_data(self.SERVO_ADDR, 0x01, 0x04)  # MODE2: OUTDRV
            time.sleep(0.1)
            
            # Ativa e configura auto-increment
            self.bus.write_byte_data(self.SERVO_ADDR, 0x00, 0x20)  # MODE1: Normal + Auto-Increment
            time.sleep(0.1)
            
            print("Servo inicializado")
            return True
        except Exception as e:
            print(f"Erro na inicialização: {e}")
            return False

    def set_pwm(self, channel, on_value, off_value):
        """Define PWM com verificação"""
        try:
            # Registradores para o canal
            base_reg = 0x06 + (channel * 4)
            
            # Define valores PWM
            self.bus.write_byte_data(self.SERVO_ADDR, base_reg, on_value & 0xFF)
            self.bus.write_byte_data(self.SERVO_ADDR, base_reg + 1, on_value >> 8)
            self.bus.write_byte_data(self.SERVO_ADDR, base_reg + 2, off_value & 0xFF)
            self.bus.write_byte_data(self.SERVO_ADDR, base_reg + 3, off_value >> 8)
            
            # Verifica se os valores foram escritos corretamente
            read_on_l = self.bus.read_byte_data(self.SERVO_ADDR, base_reg)
            read_on_h = self.bus.read_byte_data(self.SERVO_ADDR, base_reg + 1)
            read_off_l = self.bus.read_byte_data(self.SERVO_ADDR, base_reg + 2)
            read_off_h = self.bus.read_byte_data(self.SERVO_ADDR, base_reg + 3)
            
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

    def move_servo(self, position):
        """
        Move o servo para uma posição
        position: 'center', 'left', 'right'
        """
        positions = {
            'center': 307,
            'left': 225,
            'right': 389
        }

        
        if position not in positions:
            print("Posição inválida")
            return False
        
        print(f"Movendo servo para {position}")
        
        # Reinicializa o PCA9685 antes de cada movimento
        if not self.init_servo():
            print("Falha ao reinicializar o servo")
            return False
        
        # Define o PWM com o valor correspondente
        return self.set_pwm(self.STEERING_CHANNEL, 0, positions[position])

    def test_sequence(self):
        """Sequência de teste do servo"""
        positions = ['center', 'left', 'right', 'center']
        
        for pos in positions:
            input(f"Pressione Enter para mover para {pos}...")
            if self.move_servo(pos):
                print(f"Movido para {pos}")
            else:
                print(f"Falha ao mover para {pos}")
            time.sleep(1)

    def close(self):
        """Fecha a conexão"""
        try:
            self.move_servo('center')
            self.bus.close()
        except:
            pass

def main():
    car = None
    try:
        car = JetRacerControl()
        
        while True:
            print("\n=== Menu de Controle do Servo ===")
            print("1 - Mover para Centro")
            print("2 - Mover para Esquerda")
            print("3 - Mover para Direita")
            print("4 - Executar sequência de teste")
            print("5 - Reinicializar servo")
            print("0 - Sair")
            
            choice = input("\nEscolha: ")
            
            if choice == "1":
                car.move_servo('center')
            elif choice == "2":
                car.move_servo('left')
            elif choice == "3":
                car.move_servo('right')
            elif choice == "4":
                car.test_sequence()
            elif choice == "5":
                car.init_servo()
            elif choice == "0":
                break
            else:
                print("Opção inválida")
            
    except Exception as e:
        print(f"Erro: {e}")
    finally:
        if car:
            car.close()
        print("Programa finalizado")

if __name__ == "__main__":
    main()