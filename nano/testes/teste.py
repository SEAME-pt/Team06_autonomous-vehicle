import smbus2
import time

def scan_i2c():
    print("Escaneando dispositivos I2C...")
    bus = smbus2.SMBus(1)  # 1 indica /dev/i2c-1
    
    devices_found = []
    
    for address in range(0x00, 0x7F):
        try:
            bus.read_byte(address)
            hex_address = hex(address)
            print(f"Dispositivo encontrado no endereço: {hex_address}")
            devices_found.append(hex_address)
            
            # Pequena pausa para não sobrecarregar o bus
            time.sleep(0.1)
            
        except Exception as e:
            pass
    
    print("\nResumo dos dispositivos encontrados:")
    if devices_found:
        print("Endereços encontrados:", ", ".join(devices_found))
        print("\nEndereços comuns:")
        print("0x40 - PCA9685 (PWM Controller)")
        print("0x3C - SSD1306 (OLED Display)")
        print("0x60 - Motor Driver comum")
        print("0x61 - Motor Driver alternativo")
    else:
        print("Nenhum dispositivo I2C encontrado!")
    
    bus.close()

if __name__ == "__main__":
    try:
        scan_i2c()
    except Exception as e:
        print(f"Erro ao escanear I2C: {e}")
        print("Certifique-se de que o I2C está habilitado e você está rodando como root (sudo)")