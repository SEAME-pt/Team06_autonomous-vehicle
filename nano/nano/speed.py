import RPi.GPIO as GPIO 
import time 
 
# Set up GPIO 
SENSOR_PIN = 17  # GPIO pin connected to the LM393 output 
GPIO.setmode(GPIO.BCM) 
GPIO.setup(SENSOR_PIN, GPIO.IN) 
 
# Global variable to count pulses 
pulse_count = 0 
 
def count_pulse(channel): 
    global pulse_count 
    pulse_count += 1 
 
# Set up event detection on the sensor pin 
GPIO.add_event_detect(SENSOR_PIN, GPIO.RISING, callback=count_pulse) 
 
try: 
    print("Counting pulses. Press Ctrl+C to stop.") 
    while True: 
        # Reset the pulse count every second 
        pulse_count = 0 
        time.sleep(1) 
         
        # Calculate speed (pulses per second) 
        speed = pulse_count  # This is the number of pulses in one second 
        print(f"Speed: {speed} pulses/second") 
 
except KeyboardInterrupt: 
    print("Stopping...") 
finally: 
    GPIO.cleanup()



pulsos = 0
ultimo_tempo = time.time()


pulsos = 0
ultimo_tempo = time.time()


RODA_DIAMETRO = 0.065  # Diâmetro da roda em metros
FUROS = 36

pulsos = 0
ultimo_tempo = time.time()

def pulso_detectado(channel):
   global pulsos
   pulsos += 1

def calcular_velocidade(pulsos, tempo):
   voltas = pulsos / FUROS
   distancia = voltas * (RODA_DIAMETRO * 3.14159)  # Distância em metros
   velocidade_ms = distancia / tempo
   velocidade_kmh = velocidade_ms * 3.6
   return velocidade_kmh


try:
    car = JetCar()
    canvas = Canvas()
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(17, GPIO.IN)  # DO pin


    GPIO.add_event_detect(17, GPIO.RISING, callback=pulso_detectado)

    car.set_speed(100)
    kmh =0
    while True:
        canvas.clear()
        if time.time() - ultimo_tempo >= 1:
           kmh = calcular_velocidade(pulsos, 1)
           print(f"Velocidade {kmh:.2f} km/h")
           pulsos = 0
           ultimo_tempo = time.time()
        canvas.draw_text(1,2,f" {kmh:.2f} km/h",1)
        time.sleep(0.1)
        canvas.update()
        

    #car.start()
    #3while car.running:
    #    time.sleep(0.1)
except Exception as e:
    print(f"Error: {e}")
finally:
    car.stop()
    GPIO.cleanup()
    print("Program ended") 