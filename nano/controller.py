# Imports no topo do arquivo
from angle import JetRacerControl  # Importa a classe do servo
from inputs import get_gamepad
import threading

class JetRacerGamepad(JetRacerControl):
    def __init__(self):
        super().__init__()
        self.MOTOR_ADDR = 0x60
        self.running = True
        self.current_speed = 0
        self.init_motors()

    def init_motors(self):
        try:
            self.bus.write_byte_data(self.MOTOR_ADDR, 0x00, 0x20)
            self.set_pwm_freq(self.MOTOR_ADDR, 60)
            print("Motors initialized")
        except Exception as e:
            print(f"Motors init error: {e}")

    def set_pwm_freq(self, address, freq):
        prescale = int(25000000.0 / 4096.0 / freq - 1)
        oldmode = self.bus.read_byte_data(address, 0x00)
        newmode = (oldmode & 0x7F) | 0x10
        self.bus.write_byte_data(address, 0x00, newmode)
        self.bus.write_byte_data(address, 0xFE, prescale)
        self.bus.write_byte_data(address, 0x00, oldmode)
        time.sleep(0.005)
        self.bus.write_byte_data(address, 0x00, oldmode | 0xa1)

    def set_motors(self, speed):
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
            self.set_pwm(self.MOTOR_ADDR, 0, 0, 0)
            self.set_pwm(self.MOTOR_ADDR, 1, 0, pwm_value)
            self.set_pwm(self.MOTOR_ADDR, 2, 0, pwm_value)
            self.set_pwm(self.MOTOR_ADDR, 5, 0, 0)
            self.set_pwm(self.MOTOR_ADDR, 6, 0, pwm_value)
            self.set_pwm(self.MOTOR_ADDR, 7, 0, pwm_value)
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
                    if event.code == 'ABS_X':
                        angle = int((event.state / 32767.0) * self.MAX_ANGLE)
                        self.set_angle(angle)
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
        self.set_angle(0)
        self.running = False

def main():
    try:
        car = JetRacerGamepad()
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