import RPi.GPIO as GPIO
import time

# Pinos GPIO para controle de direção (L298N ou TB306A2)
IN1 = 18   # Controle de direção 1
IN2 = 19   # Controle de direção 2
ENA = 5   # Controle de velocidade (PWM)

# Configurar os pinos GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setup(IN1, GPIO.OUT)
GPIO.setup(IN2, GPIO.OUT)
GPIO.setup(ENA, GPIO.OUT)

# Configurar PWM para o pino ENA (controle de velocidade)
pwm = GPIO.PWM(ENA, 1000)  # Frequência de 1kHz
pwm.start(0)  # Começa com a velocidade 0%

def move():
    GPIO.output(IN1, GPIO.HIGH)
    GPIO.output(IN2, GPIO.LOW)


def ajustar_velocidade(duty_cycle):
    pwm.ChangeDutyCycle(duty_cycle)  # Ajusta a velocidade do motor

try:
    # Teste de direção e velocidade
    print("Motor rodando para frente com 50% de velocidade")
    move()
    time.sleep(3)
    
    print("Parando motor")
    parar_motor()

except KeyboardInterrupt:
    print("Programa interrompido")

finally:
    GPIO.cleanup()
