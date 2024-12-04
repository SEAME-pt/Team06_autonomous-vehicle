import smbus2
import time
import math
import threading
import Jetson.GPIO as GPIO
from canvas import Canvas

try:
    from inputs import get_gamepad
    HAS_GAMEPAD = True
except ImportError:
    HAS_GAMEPAD = False

class JetCar:
    def __init__(self, servo_addr=0x40, motor_addr=0x60):
        self.servo_bus = smbus2.SMBus(1)
        self.motor_bus = smbus2.SMBus(1)
        self.canvas = Canvas()
        
        # Control flags
        self.running = False
        self.current_speed = 0
        self.target_speed = 0
        self.current_angle = 0
        self.gear = 0
        self.is_back = False
        
        # Config
        self.gears = [25,50,75,100]
        self.back_speed = -50
        self.MAX_ANGLE = 180
        self.SERVO_CENTER_PWM = 330
        self.SERVO_LEFT_PWM = 80
        self.SERVO_RIGHT_PWM = 580
        
        # Addresses
        self.SERVO_ADDR = servo_addr
        self.MOTOR_ADDR = motor_addr
        self.STEERING_CHANNEL = 0
        
        self.init_servo()
        self.init_motors()

    def init_servo(self):
        try:
            self.servo_bus.write_byte_data(self.SERVO_ADDR, 0x00, 0x06)
            time.sleep(0.1)
            self.servo_bus.write_byte_data(self.SERVO_ADDR, 0x00, 0x10)
            time.sleep(0.1)
            self.servo_bus.write_byte_data(self.SERVO_ADDR, 0xFE, 0x79)
            time.sleep(0.1)
            self.servo_bus.write_byte_data(self.SERVO_ADDR, 0x01, 0x04)
            time.sleep(0.1)
            return True
        except Exception as e:
            print(f"Servo init error: {e}")
            return False

    def init_motors(self):
        try:
            self.motor_bus.write_byte_data(self.MOTOR_ADDR, 0x00, 0x20)
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

    def movement_thread(self):
        acceleration = 5
        while self.running:
            try:
                if self.current_speed < self.target_speed:
                    self.current_speed = min(self.current_speed + acceleration, self.target_speed)
                elif self.current_speed > self.target_speed:
                    self.current_speed = max(self.current_speed - acceleration, self.target_speed)
                
                self._apply_speed(self.current_speed)
                self._update_display()
                time.sleep(0.05)
            except Exception as e:
                print(f"Movement error: {e}")
                time.sleep(0.1)

    def _apply_speed(self, speed):
        pwm_value = int(abs(speed) / 100.0 * 4095)
        if speed > 0:
            self.set_motor_pwm(0, pwm_value)
            self.set_motor_pwm(1, 0)
            self.set_motor_pwm(2, pwm_value)
            self.set_motor_pwm(5, pwm_value)
            self.set_motor_pwm(6, 0)
            self.set_motor_pwm(7, pwm_value)
        elif speed < 0:
            self.set_motor_pwm(0, pwm_value)
            self.set_motor_pwm(1, pwm_value)
            self.set_motor_pwm(2, 0)
            self.set_motor_pwm(6, pwm_value)
            self.set_motor_pwm(7, pwm_value)
            self.set_motor_pwm(8, 0)
        else:
            for channel in range(9):
                self.set_motor_pwm(channel, 0)

    def _update_display(self):
        try:
            self.canvas.clear()
            self.canvas.draw_text(1, 1, f"Speed: {abs(self.current_speed):.1f} km/h", 1)
            self.canvas.draw_text(1, 10, f"Gear: {self.gear + 1}", 1)
            self.canvas.draw_text(1, 20, "REVERSE" if self.is_back else "FORWARD", 1)
            filled = int((abs(self.current_speed) / 100) * 100)
            self.canvas.draw_rect(2, 30, filled, 8, True)
            self.canvas.update()
        except Exception as e:
            print(f"Display error: {e}")

    def set_servo_pwm(self, channel, on_value, off_value):
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
        try:
            value = min(max(value, 0), 4095)
            self.motor_bus.write_byte_data(self.MOTOR_ADDR, 0x06 + 4 * channel, 0)
            self.motor_bus.write_byte_data(self.MOTOR_ADDR, 0x07 + 4 * channel, 0)
            self.motor_bus.write_byte_data(self.MOTOR_ADDR, 0x08 + 4 * channel, value & 0xFF)
            self.motor_bus.write_byte_data(self.MOTOR_ADDR, 0x09 + 4 * channel, value >> 8)
        except Exception as e:
            print(f"Motor PWM error: {e}")

    def set_speed(self, speed):
        speed = max(-100, min(100, speed))
        if speed < 0:
            self.is_back = True
            self.target_speed = self.back_speed
        else:
            self.is_back = False
            self.target_speed = self.gears[self.gear]

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

    def process_joystick(self):
        if not HAS_GAMEPAD:
            print("No gamepad detected")
            return

        print("Gamepad control started")
        while self.running:
            try:
                events = get_gamepad()
                for event in events:
                    if event.code == 'BTN_TL':
                        self.gear = max(0, self.gear - 1)
                    elif event.code == 'BTN_TR':
                        self.gear = min(3, self.gear + 1)
                    elif event.code == 'ABS_X':
                        angle = ((event.state - 127) / 127) * 100
                        self.set_steering(angle)
                    elif event.code == 'ABS_RZ':
                        if event.state == 128:
                            speed = 0
                        else:
                            speed = -(event.state - 128) / 127.0 * 100
                        self.set_speed(speed)
                    elif event.code == 'ABS_Z':
                        speed = -event.state / 255.0 * 100
                        self.set_speed(speed)
                    elif event.code in ['BTN_START', 'BTN_SELECT'] and event.state == 1:
                        self.running = False
                        break
            except Exception as e:
                print(f"Gamepad error: {e}")
                time.sleep(0.1)

    def start(self):
        self.running = True
        self.movement_thread = threading.Thread(target=self.movement_thread)
        self.movement_thread.start()
        
        if HAS_GAMEPAD:
            self.joystick_thread = threading.Thread(target=self.process_joystick)
            self.joystick_thread.start()

    def stop(self):
        self.running = False
        if hasattr(self, 'movement_thread'):
            self.movement_thread.join()
        if hasattr(self, 'joystick_thread'):
            self.joystick_thread.join()
        self.set_speed(0)
        self.set_steering(0)
        self.canvas.clear()
        self.canvas.update()
        self.servo_bus.close()
        self.motor_bus.close()

def main():
    car = JetCar()
    try:
        car.start()
        while car.running:
            if not HAS_GAMEPAD:
                # Alternative control method if no gamepad
                if input("Press Enter to stop..."):
                    break
            time.sleep(0.1)
    except KeyboardInterrupt:
        print("\nStopping...")
    finally:
        car.stop()
        GPIO.cleanup()

if __name__ == "__main__":
    main()