

class L298N_HBridge_DC_Motor(object):
    '''
    Motor controlled with an L298N hbridge from the gpio pins on Rpi
    '''
    def __init__(self, pin_forward, pin_backward, pwm_pin, freq = 50):
        import RPi.GPIO as GPIO
        self.pin_forward = pin_forward
        self.pin_backward = pin_backward
        self.pwm_pin = pwm_pin

        GPIO.setmode(GPIO.BOARD)
        GPIO.setup(self.pin_forward, GPIO.OUT)
        GPIO.setup(self.pin_backward, GPIO.OUT)
        GPIO.setup(self.pwm_pin, GPIO.OUT)
        
        self.pwm = GPIO.PWM(self.pwm_pin, freq)
        self.pwm.start(0)

    def run(self, speed):
        import RPi.GPIO as GPIO
        '''
        Update the speed of the motor where 1 is full forward and
        -1 is full backwards.
        '''
        if speed > 1 or speed < -1:
            raise ValueError( "Speed must be between 1(forward) and -1(reverse)")
        
        self.speed = speed
        max_duty = 90 #I've read 90 is a good max
        self.throttle = int(dk.utils.map_range(speed, -1, 1, -max_duty, max_duty))
        
        if self.throttle > 0:
            self.pwm.ChangeDutyCycle(self.throttle)
            GPIO.output(self.pin_forward, GPIO.HIGH)
            GPIO.output(self.pin_backward, GPIO.LOW)
        elif self.throttle < 0:
            self.pwm.ChangeDutyCycle(-self.throttle)
            GPIO.output(self.pin_forward, GPIO.LOW)
            GPIO.output(self.pin_backward, GPIO.HIGH)
        else:
            self.pwm.ChangeDutyCycle(self.throttle)
            GPIO.output(self.pin_forward, GPIO.LOW)
            GPIO.output(self.pin_backward, GPIO.LOW)


    def shutdown(self):
        import RPi.GPIO as GPIO
        self.pwm.stop()
        GPIO.cleanup()

motor = L298N_HBridge_DC_Motor(5, 7, 11)