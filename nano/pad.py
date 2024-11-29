import smbus2
import time
from inputs import get_gamepad
import math
import threading

class JetRacer:
    def __init__(self):
        self.bus = smbus2.SMBus(1)
        self.SERVO_ADDR = 0x40
        self.MOTOR_ADDR = 0x60
        self.STEERING_CHANNEL = 0
        
        # Servo config
        self.MAX_ANGLE = 180
        self.SERVO_CENTER_PWM = 307
        self.SERVO_LEFT_PWM = 225
        self.SERVO_RIGHT_PWM = 389


        # Control flags
        self.running = True
        self.current_speed = 0
        self.current_angle = 0
        
        self.init_devices()
        
    def init_devices(self):
        self.init_servo()
        self.init_motors()
        
    def init_servo(self):
        try:
            self.bus.write_byte_data(self.SERVO_ADDR, 0x00, 0x06)
            time.sleep(0.1)
            self.bus.write_byte_data(self.SERVO_ADDR, 0x00, 0x10)
            self.bus.write_byte_data(self.SERVO_ADDR, 0xFE, 0x79)
            self.bus.write_byte_data(self.SERVO_ADDR, 0x01, 0x04)
            self.bus.write_byte_data(self.SERVO_ADDR, 0x00, 0x20)
            print("Servo initialized")
        except Exception as e:
            print(f"Servo init error: {e}")
            
    def init_motors(self):
        try:
            self.bus.write_byte_data(self.MOTOR_ADDR, 0x00, 0x20)
            self.set_pwm_freq(self.MOTOR_ADDR, 60)
            print("Motors initialized")
        except Exception as e:
            print(f"Motors init error: {e}")
            
    def set_pwm_freq(self, address, freq):
        prescale = int(math.floor(25000000.0 / 4096.0 / freq - 1))
        oldmode = self.bus.read_byte_data(address, 0x00)
        newmode = (oldmode & 0x7F) | 0x10
        self.bus.write_byte_data(address, 0x00, newmode)
        self.bus.write_byte_data(address, 0xFE, prescale)
        self.bus.write_byte_data(address, 0x00, oldmode)
        time.sleep(0.005)
        self.bus.write_byte_data(address, 0x00, oldmode | 0xa1)

    def set_pwm(self, address, channel, on_value, off_value):
        base_reg = 0x06 + (channel * 4)
        self.bus.write_byte_data(address, base_reg, on_value & 0xFF)
        self.bus.write_byte_data(address, base_reg + 1, on_value >> 8)
        self.bus.write_byte_data(address, base_reg + 2, off_value & 0xFF)
        self.bus.write_byte_data(address, base_reg + 3, off_value >> 8)

    def angle_to_pwm(self, angle):
        angle = max(-self.MAX_ANGLE, min(self.MAX_ANGLE, angle))
        if angle < 0:
            return int(self.SERVO_CENTER_PWM + 
                      (angle / self.MAX_ANGLE) * (self.SERVO_CENTER_PWM - self.SERVO_LEFT_PWM))
        elif angle > 0:
            return int(self.SERVO_CENTER_PWM + 
                      (angle / self.MAX_ANGLE) * (self.SERVO_RIGHT_PWM - self.SERVO_CENTER_PWM))
        return self.SERVO_CENTER_PWM

    def set_steering(self, angle):
        pwm_value = self.angle_to_pwm(angle)
        self.set_pwm(self.SERVO_ADDR, self.STEERING_CHANNEL, 0, pwm_value)

    def set_motors(self, speed):
        """Speed: -100 to +100"""
        speed = max(-100, min(100, speed))
        pwm_value = int(abs(speed) / 100.0 * 4095)
        
        if speed > 0:  # Forward
            self.set_pwm(self.MOTOR_ADDR, 0, 0, pwm_value)
            self.set_pwm(self.MOTOR_ADDR, 1, 0, 0)
            self.set_pwm(self.MOTOR_ADDR, 2, 0, pwm_value)
            self.set_pwm(self.MOTOR_ADDR, 5, 0, pwm_value)
            self.set_pwm(self.MOTOR_ADDR, 6, 0, 0)
            self.set_pwm(self.MOTOR_ADDR, 7, 0, pwm_value)
        elif speed < 0:  # Reverse
            self.set_pwm(self.MOTOR_ADDR, 0, 0, pwm_value)
            self.set_pwm(self.MOTOR_ADDR, 1, 0, pwm_value)
            self.set_pwm(self.MOTOR_ADDR, 2, 0, 0)
            self.set_pwm(self.MOTOR_ADDR, 6, 0, pwm_value)
            self.set_pwm(self.MOTOR_ADDR, 7, 0, pwm_value)
            self.set_pwm(self.MOTOR_ADDR, 8, 0, 0)


 
        else:  # Stop
            for channel in range(8):
                self.set_pwm(self.MOTOR_ADDR, channel, 0, 0)

    def process_gamepad(self):
        print("Gamepad control started")
        print("Left stick: steering")
        print("RT: forward, LT: reverse")
        print("START/SELECT: exit")
        
        while self.running:
            try:
                events = get_gamepad()
                for event in events:
                    #print(event.code)
                    if event.code == 'ABS_X':
                        self.current_angle = -int((event.state / 32767.0) * self.MAX_ANGLE)
                      
                        print(self.current_angle)
                        
                        #self.set_steering(self.current_angle)
                        
                    
                        
                    elif event.code == 'ABS_GAS':
                        self.current_speed = event.state / 255.0 * 100
                        self.set_motors(self.current_speed)
                    elif event.code == 'ABS_BRAKE':
                        self.current_speed = -event.state / 255.0 * 100
                        self.set_motors(self.current_speed)
                    elif event.code in ['BTN_START', 'BTN_SELECT'] and event.state == 1:
                        self.running = False
                        break
            except KeyboardInterrupt:
                self.running = False
                break
            except Exception as e:
                print(f"Gamepad error: {e}")
                continue

    def stop(self):
        self.set_motors(0)
        self.set_steering(0)
        self.running = False

def main():
    try:
        car = JetRacer()
        gamepad_thread = threading.Thread(target=car.process_gamepad)
        gamepad_thread.start()
        gamepad_thread.join()
    except Exception as e:
        print(f"Error: {e}")
    finally:
        if 'car' in locals():
            car.stop()
        print("Program ended")

if __name__ == "__main__":
    main()