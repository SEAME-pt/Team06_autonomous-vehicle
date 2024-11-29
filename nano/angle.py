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
        
        # Configurações do servo
        self.MAX_ANGLE = 180  # Ângulo máximo para cada lado
        self.SERVO_CENTER_PWM = 307  # Valor PWM para centro (0 graus)
        self.SERVO_LEFT_PWM = 225    # Valor PWM para -90 graus
        self.SERVO_RIGHT_PWM = 389   # Valor PWM para +90 graus
        
        self.init_servo()

    def reset_pca9685(self):
        """Reset completo do PCA9685"""
        try:
            self.bus.write_byte_data(self.SERVO_ADDR, 0x00, 0x06)
            time.sleep(0.1)
            return True
        except Exception as e:
            print(f"Erro no reset: {e}")
            return False

    def init_servo(self):
        """Inicialização completa do servo"""
        print("Inicializando servo...")
        
        self.reset_pca9685()
        time.sleep(0.1)
        
        try:
            # Modo sleep para configuração
            self.bus.write_byte_data(self.SERVO_ADDR, 0x00, 0x10)
            time.sleep(0.1)
            
            # Configura frequência PWM (~50Hz)
            prescale = 0x79
            self.bus.write_byte_data(self.SERVO_ADDR, 0xFE, prescale)
            time.sleep(0.1)
            
            # Configura MODE2
            self.bus.write_byte_data(self.SERVO_ADDR, 0x01, 0x04)
            time.sleep(0.1)
            
            # Ativa e configura auto-increment
            self.bus.write_byte_data(self.SERVO_ADDR, 0x00, 0x20)
            time.sleep(0.1)
            
            print("Servo inicializado")
            return True
        except Exception as e:
            print(f"Erro na inicialização: {e}")
            return False

    def set_pwm(self, channel, on_value, off_value):
        """Define PWM com verificação"""
        try:
            base_reg = 0x06 + (channel * 4)
            
            self.bus.write_byte_data(self.SERVO_ADDR, base_reg, on_value & 0xFF)
            self.bus.write_byte_data(self.SERVO_ADDR, base_reg + 1, on_value >> 8)
            self.bus.write_byte_data(self.SERVO_ADDR, base_reg + 2, off_value & 0xFF)
            self.bus.write_byte_data(self.SERVO_ADDR, base_reg + 3, off_value >> 8)
            
            # Verificação da escrita
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

    def angle_to_pwm(self, angle):
        """
        Converte ângulo (-90 a +90) para valor PWM
        Ângulo negativo = esquerda
        Ângulo positivo = direita
        Ângulo zero = centro
        """
        # Limita o ângulo entre -90 e +90
        angle = max(-self.MAX_ANGLE, min(self.MAX_ANGLE, angle))
        
        if angle < 0:  # Esquerda
            # Interpola entre centro e esquerda
            return int(self.SERVO_CENTER_PWM + 
                      (angle / self.MAX_ANGLE) * (self.SERVO_CENTER_PWM - self.SERVO_LEFT_PWM))
        elif angle > 0:  # Direita
            # Interpola entre centro e direita
            return int(self.SERVO_CENTER_PWM + 
                      (angle / self.MAX_ANGLE) * (self.SERVO_RIGHT_PWM - self.SERVO_CENTER_PWM))
        else:  # Centro
            return self.SERVO_CENTER_PWM

    def set_angle(self, angle):
        """
        Define o ângulo do servo (-180 a +180 graus)
        Negativo = Esquerda
        Positivo = Direita
        Zero = Centro
        """
        if not isinstance(angle, (int, float)):
            print("Ângulo deve ser um número")
            return False
            
        if abs(angle) > self.MAX_ANGLE:
            print(f"Ângulo deve estar entre -{self.MAX_ANGLE} e +{self.MAX_ANGLE} graus")
            return False
        
        direction = "centro" if angle == 0 else "esquerda" if angle < 0 else "direita"
        print(f"Movendo servo para {abs(angle)}° ({direction})")
        
        pwm_value = self.angle_to_pwm(angle)
        return self.set_pwm(self.STEERING_CHANNEL, 0, pwm_value)

    def test_angle_sequence(self):
        """Sequência de teste com diferentes ângulos"""
        test_angles = [0, -45, 45, -90, 90, 0]  # Centro, esquerda, direita, extremos, centro
        
        for angle in test_angles:
            direction = "centro" if angle == 0 else "esquerda" if angle < 0 else "direita"
            input(f"Pressione Enter para mover {abs(angle)}° para {direction}...")
            if self.set_angle(angle):
                print(f"Movido para {angle}°")
            else:
                print(f"Falha ao mover para {angle}°")
            time.sleep(1)

    def close(self):
        """Fecha a conexão"""
        try:
            self.set_angle(0)  # Retorna ao centro
            self.bus.close()
        except:
            pass

def main():
    car = None
    try:
        car = JetRacerControl()
        
        while True:
            print("\n=== Menu de Controle do Servo ===")
            print("1 - Mover para Centro (0°)")
            print("2 - Mover para Esquerda (-45°)")
            print("3 - Mover para Direita (+45°)")
            print("4 - Mover para Extrema Esquerda (-90°)")
            print("5 - Mover para Extrema Direita (+90°)")
            print("6 - Definir ângulo personalizado")
            print("7 - Executar sequência de teste")
            print("8 - Reinicializar servo")
            print("0 - Sair")
            
            choice = input("\nEscolha: ")
            
            if choice == "1":
                car.set_angle(0)
            elif choice == "2":
                car.set_angle(-45)
            elif choice == "3":
                car.set_angle(45)
            elif choice == "4":
                car.set_angle(-90)
            elif choice == "5":
                car.set_angle(90)
            elif choice == "6":
                try:
                    angle = float(input("Digite o ângulo desejado (-90 a +90): "))
                    car.set_angle(angle)
                except ValueError:
                    print("Ângulo inválido")
            elif choice == "7":
                car.test_angle_sequence()
            elif choice == "8":
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