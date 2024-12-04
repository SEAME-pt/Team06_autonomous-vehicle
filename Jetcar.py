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
    pulsos += 1
def calcular_velocidade(pulsos, tempo):
   voltas = pulsos / FUROS
   distancia = voltas * (RODA_DIAMETRO * 3.14159)  # Distância em metros
   velocidade_ms = distancia / tempo
   velocidade_kmh = velocidade_ms * 3.6
   return velocidade_kmh



class JetCar:
    def __init__(self, servo_addr=0x40, motor_addr=0x60):
        # Servo setup
        self.servo_bus = smbus2.SMBus(1)
        self.SERVO_ADDR = servo_addr
        self.STEERING_CHANNEL = 0
        self.gears = [25,50,75,100]
        self.back_speed=-50
        self.gear=0
        self.is_back=False

        # Servo config
        self.MAX_ANGLE = 180
        self.SERVO_CENTER_PWM = 320 + 0
        self.SERVO_LEFT_PWM = 320 - 250
        self.SERVO_RIGHT_PWM = 320 + 250
        
        # Motor controller setup
        self.motor_bus = smbus2.SMBus(1)
        self.MOTOR_ADDR = motor_addr
        
        # Control flags
        self.running = False
        self.current_speed = 0
        self.current_angle = 0

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


    def set_steering(self, angle):
        """Set steering angle (-90 to +90 degrees)"""
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

    def shift_up(self):
        """Increase gear if not at maximum"""
        if self.current_gear < len(self.gears) - 1:
            self.current_gear += 1
            # Adjust current speed to new gear's limit if necessary
            if self.current_speed > self.gears[self.current_gear]:
                self.set_speed(self.gears[self.current_gear])

    def shift_down(self):
        """Decrease gear if not at minimum"""
        if self.current_gear > 0:
            self.current_gear -= 1
            # Adjust current speed to new gear's limit if necessary
            if self.current_speed > self.gears[self.current_gear]:
                self.set_speed(self.gears[self.current_gear])
    
    def set_speed(self, speed):
        speed = max(-100, min(100, speed))

        if speed < 0:
            self.is_back = True
            speed = self.back_speed 
        else:
            self.is_back = False 

        max_speed = self.gears[self.current_gear]
        
        # Handle reverse
        if speed < 0:
            self.is_back = True
            speed = max(self.back_speed, speed)  # Limit reverse speed
        else:
            self.is_back = False
            speed = min(speed, max_speed)  # Limit forward speed based on gear
        
   
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

    def process_canvas(self):
        pass

    def process_joystick(self):
        print("Gamepad control started")
        print("Left stick: steering")
        print("RT: forward, LT: reverse")
        print("START/SELECT: exit")
        
        while self.running:
            try:
                events = get_gamepad()
                
                for event in events:
                    #print(event.code)
                    if event.code == 'BTN_TL':  # Left bumper
                        if event.state == 1:
                            self.shift_down()
                            print(f"Gear down: {self.current_gear + 1}, Max speed: {self.gears[self.current_gear]}%")
                    elif event.code == 'BTN_TR':  # Right bumper
                        if event.state == 1:
                            self.shift_up()
                            print(f"Gear up: {self.current_gear + 1}, Max speed: {self.gears[self.current_gear]}%")
              
                    elif event.code == 'ABS_X':
                        current_angle = ((event.state - 127) / 127) * 100
                        self.set_steering(current_angle)
                    elif event.code == 'ABS_RZ':
                        # Map 0-255 to -100 to +100
                        # 0-127 = reverse
                        # 128 = stop
                        # 129-255 = forward
                        if event.state == 128:
                            self.current_speed = 0
                        elif event.state < 128:
                            self.current_speed = (128 - event.state) / 128.0 * 100
                        else:
                            self.current_speed = -(event.state - 128) / 127.0 * 100
                        self.set_speed(self.current_speed)
                    elif event.code == 'ABS_BRAKE':
                        self.current_speed = -event.state / 255.0 * 100
                        self.current_gear =0
                        self.set_speed(self.current_speed)
                    elif event.code in ['BTN_START', 'BTN_SELECT'] and event.state == 1:
                        self.running = False
                        break
            except KeyboardInterrupt:
                self.running = False
                break
            except Exception as e:
                print(f"Gamepad error: {e}")
                self.running = False
                break
    def process(self):
        global ultimo_tempo
        global pulsos 
        if time.time() - ultimo_tempo >= 1:
            kmh = calcular_velocidade(pulsos, 1)
            print(f"Velocidade {kmh:.2f} km/h")
            pulsos = 0
            ultimo_tempo = time.time()
            self.canvas.clear()
            self.canvas.draw_text(1,2,f" {kmh:.2f} km/h",1)
            
        return self.running
    def start(self):
        """Start the car control"""
        self.running = True
        self.joystick_thread = threading.Thread(target=self.process_joystick)
        self.joystick_thread.start()
        self.canvas_thread = threading.Thread(target=self.process_canvas)
        self.canvas_thread.start()

    def stop(self):
        """Stop the car and cleanup"""
        self.running = False
        if hasattr(self, 'joystick_thread'):
            self.joystick_thread.join()
        if hasattr(self, 'canvas_thread'):
            self.canvas_thread.join()
        
        self.set_speed(0)
        self.set_steering(0)
        self.servo_bus.close()
        self.motor_bus.close()



car = JetCar()

try:
    canvas = Canvas()
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(17, GPIO.IN)  # DO pin
    GPIO.add_event_detect(17, GPIO.RISING, callback=pulso_detectado)

    #car.set_speed(100)
    # velocidade_maxima=1
    # kmh =0
    # while True:
    #     canvas.clear()
    #     if time.time() - ultimo_tempo >= 1:
    #        kmh = calcular_velocidade(pulsos, 1)
    #        if kmh > velocidade_maxima:
    #            velocidade_maxima = kmh
    #        print(f"Velocidade {kmh:.2f} km/h")
    #        pulsos = 0
    #        ultimo_tempo = time.time()

    #     canvas.draw_text(1,1,f" {kmh:.2f} km/h",1)
    #     canvas.draw_text(1,10,f" {velocidade_maxima:.2f} km/h Maximo",1)
        
    #     filled = int((kmh / velocidade_maxima) * 100)
    #     canvas.draw_rect(2,25,filled,8,True)
    #     canvas.update()
    #     time.sleep(0.1)

    car.start()
    while car.running:
        time.sleep(0.1)
except Exception as e:
    print(f"Error: {e}")

except KeyboardInterrupt as e:
    print(f"Keyboard Error: {e}")

finally:
    car.stop()
    canvas.clear()
    canvas.update()
    GPIO.cleanup()

