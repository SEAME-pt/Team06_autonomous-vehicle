import smbus2
import time
from inputs import get_gamepad
import math
import threading
import Jetson.GPIO as GPIO
from canvas import Canvas



SENSOR_PIN = 17  # GPIO pin connected to the LM393 output 
pulsos = 0
ultimo_tempo = time.time()
RODA_DIAMETRO = 0.065  # Diâmetro da roda em metros
FUROS = 36


 
def pulso_detectado(channel):
   global pulsos
   pulsos += 1
def calcular_velocidade(pulsos, tempo):
   voltas = pulsos / FUROS
   distancia = voltas * (RODA_DIAMETRO * 3.14159)  # Distância em metros
   velocidade_ms = distancia / tempo
   velocidade_kmh = velocidade_ms * 3.6
   return velocidade_kmh

class Transmission:
    def __init__(self):
        self.is_automatic = False
        self.current_gear = 1
        self.speed = 0
        
        # Ranges para transmissão manual
        self.manual_ranges = {
            1: (0, 25),
            2: (26, 50),
            3: (51, 75),
            4: (76, 100)
        }
        
        # Pontos de mudança automática (velocidade onde muda de gear)
        self.auto_shift_points = {
            'up': {
                1: 20,    # Muda para 2ª aos 20 km/h
                2: 45,    # Muda para 3ª aos 45 km/h
                3: 70     # Muda para 4ª aos 70 km/h
            },
            'down': {
                4: 65,    # Volta para 3ª abaixo de 65 km/h
                3: 40,    # Volta para 2ª abaixo de 40 km/h
                2: 15     # Volta para 1ª abaixo de 15 km/h
            }
        }
        
        # Taxas de aceleração por andamento
        self.acceleration_rates = {
            1: 2.0,
            2: 1.5,
            3: 1.0,
            4: 0.5
        }

    def toggle_transmission_mode(self):
        """Alterna entre modo automático e manual"""
        self.is_automatic = not self.is_automatic
        return "Automatic" if self.is_automatic else "Manual"

    def update_automatic(self, speed):
        """Atualiza a gear no modo automático"""
        if speed > self.auto_shift_points['up'].get(self.current_gear, float('inf')):
            if self.current_gear < 4:
                self.current_gear += 1
        elif speed < self.auto_shift_points['down'].get(self.current_gear + 1, 0):
            if self.current_gear > 1:
                self.current_gear -= 1

    def get_acceleration_rate(self):
        """Retorna taxa de aceleração atual"""
        return self.acceleration_rates[self.current_gear]

    def get_current_range(self):
        """Retorna range de velocidade da gear atual"""
        return self.manual_ranges[self.current_gear]

class Motor:
    def __init__(self):
        self.transmission = Transmission()
        self.speed = 0
        self.throttle = 0

    def ger_up(self):
        self.transmission.current_gear = min(4, self.transmission.current_gear + 1)
    def ger_down(self): 
        self.transmission.current_gear = max(1, self.transmission.current_gear - 1)
        
    def update(self, throttle):
        self.throttle = throttle
        acc = self.transmission.get_acceleration_rate() * throttle
        if self.transmission.is_automatic:
            # Modo automático - pode usar toda a faixa de velocidade
            self.speed = min(max(0, self.speed + acc), 100)
            self.transmission.update_automatic(self.speed)
        else:
            min_speed, max_speed = self.transmission.get_current_range()
            self.speed = min(max(min_speed, self.speed + acc), max_speed)
    
    def brake(self, brake_force):
        self.speed = max(0, self.speed - (brake_force * 2))
        if self.transmission.is_automatic:
            self.transmission.update_automatic(self.speed)

class JetCar:
    def __init__(self, servo_addr=0x40, motor_addr=0x60):
        self.servo_bus = smbus2.SMBus(1)
        self.SERVO_ADDR = servo_addr
        self.STEERING_CHANNEL = 0
        self.motor_bus = smbus2.SMBus(1)
        self.MOTOR_ADDR = motor_addr

        self.MAX_ANGLE = 180
        self.SERVO_CENTER_PWM = 320 + 0
        self.SERVO_LEFT_PWM = 320 - 250
        self.SERVO_RIGHT_PWM = 320 + 250
        
        # Adiciona controle do motor
        self.motor = Motor()
        self.current_gear = 1
        self.acceleration = 0
        self.brake_force = 0
        self.max_speed = 100
        self.min_speed = 20
        
        # Configurações de aceleração
        self.acceleration_rate = 2.0
        self.brake_rate = 3.0
        self.deceleration_rate = 1.0
        
        # Estado do motor
        self.is_running = False
        self.current_speed = 0
        self.target_speed = 0
        
        # Inicialização
        self.init_servo()
        self.init_motors()

    def init_servo(self):
        try:
            # Reset PCA9685
            self.servo_bus.write_byte_data(self.SERVO_ADDR, 0x00, 0x06)
            time.sleep(0.1)
            
            # Setup servo control
            self.servo_bus.write_byte_data(self.SERVO_ADDR, 0x00, 0x10)
            time.sleep(0.1)
            
            # Set frequency (~50Hz)
            self.servo_bus.write_byte_data(self.SERVO_ADDR, 0xFE, 0x79)
            time.sleep(0.1)
            
            # Configure MODE2
            self.servo_bus.write_byte_data(self.SERVO_ADDR, 0x01, 0x04)
            time.sleep(0.1)
            
            # Enable auto-increment
            self.servo_bus.write_byte_data(self.SERVO_ADDR, 0x00, 0x20)
            time.sleep(0.1)
            
            return True
        except Exception as e:
            print(f"Servo init error: {e}")
            return False

    def init_motors(self):
        try:
            # Configure motor controller
            self.motor_bus.write_byte_data(self.MOTOR_ADDR, 0x00, 0x20)
            
            # Set frequency to 60Hz
            prescale = int(math.floor(25000000.0 / 4096.0 / 100 - 1))
            oldmode = self.motor_bus.read_byte_data(self.MOTOR_ADDR, 0x00)
            newmode = (oldmode & 0x7F) | 0x10
            self.motor_bus.write_byte_data(self.MOTOR_ADDR, 0x00, newmode)
            self.motor_bus.write_byte_data(self.MOTOR_ADDR, 0xFE, prescale)
            self.motor_bus.write_byte_data(self.MOTOR_ADDR, 0x00, oldmode)
            time.sleep(0.005)
            self.motor_bus.write_byte_data(self.MOTOR_ADDR, 0x00, oldmode | 0xa1)
            
            return True
        except Exception as e:
            print(f"Motor init error: {e}")
            return False

    def update_motor_state(self):
        """Atualiza o estado do motor baseado nos controles"""
        if self.acceleration > 0:
            acceleration = self.acceleration * self.motor.transmission.get_acceleration_rate()
            self.target_speed = min(self.current_speed + acceleration, self.max_speed)
        elif self.brake_force > 0:
            self.target_speed = max(self.current_speed - (self.brake_force * self.brake_rate), 0)
        else:
            self.target_speed = max(self.current_speed - self.deceleration_rate, 0)
        
        if self.current_speed < self.target_speed:
            self.current_speed = min(self.current_speed + self.acceleration_rate, self.target_speed)
        elif self.current_speed > self.target_speed:
            self.current_speed = max(self.current_speed - self.brake_rate, self.target_speed)
        
        self.motor.update(self.current_speed / self.max_speed)
        self.set_speed(int(self.current_speed))

    def accelerate(self, amount):
        self.acceleration = max(0, min(1, amount))
        if self.motor.transmission.is_automatic:
            self.motor.transmission.update_automatic(self.current_speed)

    def brake(self, amount):
        self.brake_force = max(0, min(1, amount))
        self.motor.brake(self.brake_force)

    def shift_gear(self, gear):
        if not self.motor.transmission.is_automatic:
            if 1 <= gear <= 4:
                self.motor.transmission.current_gear = gear
                print(f"Mudou para ger {gear}")

    def toggle_transmission(self):
        mode = self.motor.transmission.toggle_transmission_mode()


    def set_steering(self, angle):
        angle = max(-self.MAX_ANGLE, min(self.MAX_ANGLE, angle))
        
        if angle < 0:
            pwm = int(self.SERVO_CENTER_PWM + 
                     (angle / self.MAX_ANGLE) * (self.SERVO_CENTER_PWM - self.SERVO_LEFT_PWM))
        elif angle > 0:
            pwm = int(self.SERVO_CENTER_PWM + 
                     (angle / self.MAX_ANGLE) * (self.SERVO_RIGHT_PWM - self.SERVO_CENTER_PWM))
        else:
            pwm = self.SERVO_CENTER_PWM
            
        self.set_servo_pwm(self.STEERING_CHANNEL, 0, pwm)
        self.current_angle = angle

    def set_angle(self, angle):
        self.set_servo_pwm(self.STEERING_CHANNEL, 0, angle)

    def angle_to_pwm(self, angle):
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

    def set_servo_pwm(self, channel, on_value, off_value):
        """Set PWM values for servo"""
        try:
            base_reg = 0x06 + (channel * 4)
            self.servo_bus.write_byte_data(self.SERVO_ADDR, base_reg, on_value & 0xFF)
            self.servo_bus.write_byte_data(self.SERVO_ADDR, base_reg + 1, on_value >> 8)
            self.servo_bus.write_byte_data(self.SERVO_ADDR, base_reg + 2, off_value & 0xFF)
            self.servo_bus.write_byte_data(self.SERVO_ADDR, base_reg + 3, off_value >> 8)
            return True
        except Exception as e:
            print(f"Servo PWM error: {e}")
            return False

    def set_motor_pwm(self, channel, value):
        """Set PWM value for motor channel"""
        value = min(max(value, 0), 4095)
        try:
            self.motor_bus.write_byte_data(self.MOTOR_ADDR, 0x06 + 4 * channel, 0)
            self.motor_bus.write_byte_data(self.MOTOR_ADDR, 0x07 + 4 * channel, 0)
            self.motor_bus.write_byte_data(self.MOTOR_ADDR, 0x08 + 4 * channel, value & 0xFF)
            self.motor_bus.write_byte_data(self.MOTOR_ADDR, 0x09 + 4 * channel, value >> 8)
        except Exception as e:
            print(f"Motor PWM error: {e}")

    def process_canvas(self):
        self.process()

    def set_speed(self, speed):
        speed = max(-100, min(100, speed))
        if speed < 0:
            self.is_back = True
            speed = self.back_speed 
        else:
            self.is_back = False 
        pwm_value = int(abs(speed) / 100.0 * 4095)
        
        if speed > 0:  # Forward
            self.set_motor_pwm(0, pwm_value)  # IN1
            self.set_motor_pwm(1, 0)          # IN2
            self.set_motor_pwm(2, pwm_value)  # ENA
            self.set_motor_pwm(5, pwm_value)  # IN3
            self.set_motor_pwm(6, 0)          # IN4
            self.set_motor_pwm(7, pwm_value)  # ENB
        elif speed < 0:  # Backward
            self.set_motor_pwm(0, pwm_value)  # IN1
            self.set_motor_pwm(1, pwm_value)  # IN2
            self.set_motor_pwm(2, 0)          # ENA
            self.set_motor_pwm(6, pwm_value)  # IN3
            self.set_motor_pwm(7, pwm_value)  # IN4
            self.set_motor_pwm(8, 0)          # ENB
        else:  # Stop
            for channel in range(9):
                self.set_motor_pwm(channel, 0)
        
        self.current_speed = speed
    def process_joystick(self):
        print("Controle iniciado")
        print("Analógico esquerdo: direção")
        print("RT: acelerador, LT: travar")
        print("RB: mudança acima, LB: mudança abaixo")
        print("Y: alternar modo de transmissão")
        
        while self.running:
            try:
                events = get_gamepad()
                
                for event in events:
                    if event.code == 'ABS_X':
                        steering = ((event.state - 127) / 127) * 90
                        self.set_steering(steering)
                    
                    elif event.code == 'ABS_RZ':
                        self.accelerate(event.state / 255.0)
                    
                    elif event.code == 'ABS_Z':
                        self.brake(event.state / 255.0)
                    
                    elif event.code == 'BTN_TR' and event.state == 1:
                        if not self.motor.transmission.is_automatic:
                            self.motor.ger_up()
                    
                    elif event.code == 'BTN_TL' and event.state == 1:
                        if not self.motor.transmission.is_automatic:
                            self.motor.ger_down()
                    
                    elif event.code == 'BTN_NORTH' and event.state == 1:
                        self.toggle_transmission()
                    elif event.code in ['BTN_START', 'BTN_SELECT'] and event.state == 1:
                        self.running = False
                self.update_motor_state()
                
            except KeyboardInterrupt:
                self.running = False
                break
            except Exception as e:
                print(f"Erro no gamepad: {e}")
                continue

    def process(self):
        global ultimo_tempo
        global pulsos
        
        if time.time() - ultimo_tempo >= 1:
            kmh = calcular_velocidade(pulsos, 1)
            print(f"Speed: {kmh:.2f} km/h")
            print(f"Gear atual: {self.motor.transmission.current_gear}")
            print(f"Modo: {'Automático' if self.motor.transmission.is_automatic else 'Manual'}")
            
            pulsos = 0
            ultimo_tempo = time.time()
            
            if hasattr(self, 'canvas'):
                self.canvas.clear()
                self.canvas.draw_text(1, 2, f"Vel: {kmh:.2f} km/h", 1)
                self.canvas.draw_text(1, 3, f"Gear: {self.motor.transmission.current_gear}", 1)
        
        return self.running

    def start(self):
        self.running = True
        self.joystick_thread = threading.Thread(target=self.process_joystick)
        self.joystick_thread.start()
        self.canvas_thread = threading.Thread(target=self.process_canvas)
        self.canvas_thread.start()

    def stop(self):
        self.running = False
        if hasattr(self, 'joystick_thread'):
            self.joystick_thread.join()
        if hasattr(self, 'canvas_thread'):
            self.canvas_thread.join()
        
        self.set_speed(0)
        self.set_steering(0)
        self.servo_bus.close()
        self.motor_bus.close()


def main():
    try:
        car = JetCar()
        car.start()
        try:
            while car.running:
                if not car.process():  
                    break
                time.sleep(0.01)  
        except KeyboardInterrupt:
            print("\nEncerrando o programa...")
        
        finally:
            car.stop()
            print("JetCar encerrado com sucesso!")
            
    except Exception as e:
        print(f"Erro ao iniciar o JetCar: {e}")


if __name__ == "__main__":
    main()