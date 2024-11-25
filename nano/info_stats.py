import psutil
import time
from typing import Dict
import math
import socket
from canvas import SSD1306
from battery_monitor import BatteryMonitor  

class SystemMonitorDisplay:
    def __init__(self):
        self.display = SSD1306()
        self.battery = BatteryMonitor()
        self.last_update = 0
        self.update_interval = 1

    def get_system_stats(self) -> Dict:
        """Coleta estatísticas do sistema"""
        try:
            # Obtém informações da bateria
            battery_info = self.battery.get_battery_info()
            
            stats = {
                "cpu_percent": psutil.cpu_percent(interval=0.1),
                "memory_percent": psutil.virtual_memory().percent,
                "memory_used": f"{psutil.virtual_memory().used / (1024 ** 3):.1f}GB",
                "temperature": 0,
                "battery_percent": battery_info["percentage"],
                "battery_voltage": battery_info["voltage"],
                "ip_address": self.get_ip_address()
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

    def get_ip_address(self, prefix="10.") -> str:
        """Obtém o endereço IP começando com o prefixo especificado"""
        try:
            addrs = psutil.net_if_addrs()
            for interface, addresses in addrs.items():
                for addr in addresses:
                    if hasattr(addr, 'address') and addr.address.startswith(prefix):
                        return addr.address
            return socket.gethostname()
        except:
            return "No IP"

    def draw_stats(self, stats: Dict):
        """Desenha as estatísticas no display"""
        self.display.clear()

        # IP no topo
        ip_text = f"IP:{stats['ip_address']}"
        self.display.draw_text(2, 0, ip_text[:20])  

        # CPU Usage com barra - linha 1
        cpu_width = int((stats["cpu_percent"] / 100) * 40)
        self.display.draw_text(2, 8, f"CPU:")
        self.display.draw_rect(25, 9, 40, 5, fill=False)
        self.display.draw_rect(25, 9, cpu_width, 5, fill=True)
        self.display.draw_text(70, 8, f"{stats['cpu_percent']}%")

        # Memory Usage com barra - linha 2
        mem_width = int((stats["memory_percent"] / 100) * 40)
        self.display.draw_text(2, 16, f"MEM:")
        self.display.draw_rect(25, 17, 40, 5, fill=False)
        self.display.draw_rect(25, 17, mem_width, 5, fill=True)
        self.display.draw_text(70, 16, f"{stats['memory_percent']}%")

        # Bateria com barra - linha 3
        bat_width = int((stats["battery_percent"] / 100) * 40)
        self.display.draw_text(2, 24, f"BAT:")
        self.display.draw_rect(25, 25, 40, 5, fill=False)
        self.display.draw_rect(25, 25, bat_width, 5, fill=True)
        self.display.draw_text(70, 24, f"{stats['battery_percent']:.0f}%")

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