
import psutil
import time
from typing import Dict
import math
import socket
from pathlib import Path
from canvas import SSD1306 

class SystemMonitorDisplay:
    def __init__(self):
        self.display = SSD1306()
        self.last_update = 0
        self.update_interval = 1
        self.ip_address = self.get_ip_address()

    def get_ip_address(self, prefix="10.") -> str:
        """Obtém o endereço IP começando com o prefixo especificado"""
        try:
            # Tenta todos os IPs até encontrar um que começa com o prefixo
            addrs = psutil.net_if_addrs()
            for interface, addresses in addrs.items():
                for addr in addresses:
                    if hasattr(addr, 'address') and addr.address.startswith(prefix):
                        return addr.address
            return socket.gethostname()
        except:
            return "No IP"

    def get_battery_info(self) -> Dict:

           
            return {
                "percent": 50,
                "power_plugged": True,
                "available": True
            }
        
 

    def get_system_stats(self) -> Dict:
        """Coleta estatísticas do sistema"""
        try:
            stats = {
                "cpu_percent": psutil.cpu_percent(interval=0.1),
                "memory_percent": psutil.virtual_memory().percent,
                "memory_used": f"{psutil.virtual_memory().used / (1024 ** 3):.1f}GB",
                "temperature": 0,
                "battery": self.get_battery_info(),
                "ip": self.ip_address
            }
            
            # Tenta obter temperatura da CPU
            try:
                temps = psutil.sensors_temperatures()
                if 'coretemp' in temps:
                    stats["temperature"] = temps['coretemp'][0].current
                elif 'cpu_thermal' in temps:
                    stats["temperature"] = temps['cpu_thermal'][0].current
            except:
                pass

            return stats
        except Exception as e:
            return {"error": str(e)}

    def draw_battery_icon(self, x: int, y: int, percent: float, is_charging: bool):
        """Desenha um ícone de bateria"""
        # Desenha o contorno da bateria
        self.display.draw_rect(x, y, 12, 6, fill=False)
        self.display.draw_rect(x+12, y+1, 2, 4, fill=True)  # Terminal da bateria
        
        # Desenha o nível da bateria
        fill_width = int((percent / 100) * 10)
        if fill_width > 0:
            self.display.draw_rect(x+1, y+1, fill_width, 4, fill=True)
        
        # Indicador de carregamento
        if is_charging:
            self.display.draw_text(x+15, y, "+")

    def draw_stats(self, stats: Dict):
        self.display.clear()
        self.display.draw_text(2, 0, f"IP:{stats['ip']}", scale=1)

        # CPU e Memória na segunda linha
        cpu_text = f"C:{stats['cpu_percent']}%"
        mem_text = f"M:{stats['memory_percent']}%"
        self.display.draw_text(2, 12, cpu_text)
        self.display.draw_text(64, 12, mem_text)

        # Temperatura e bateria na terceira linha
        if stats["temperature"] > 0:
            self.display.draw_text(2, 24, f"T:{stats['temperature']:.1f}C")

        # Desenha informações da bateria
        battery = stats["battery"]

        self.draw_battery_icon(64, 24, battery["percent"], battery["power_plugged"])
        self.display.draw_text(85, 24, f"{battery['percent']}%")

        self.display.update()

    def run(self):
        """Loop principal do monitor"""
        try:
            while True:
                current_time = time.time()
                if current_time - self.last_update >= self.update_interval:
                    stats = self.get_system_stats()
                    self.draw_stats(stats)
                    self.last_update = current_time
                time.sleep(0.1)
                
        except KeyboardInterrupt:
            self.display.clear()
            self.display.update()
            print("\nMonitoramento encerrado ")
        except Exception as e:
            print(f"Erro: {str(e)}")

if __name__ == "__main__":
    monitor = SystemMonitorDisplay()
    monitor.run()