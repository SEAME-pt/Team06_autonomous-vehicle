import psutil
import time
from typing import Dict
import math
from canvas import SSD1306  

class SystemMonitorDisplay:
    def __init__(self):
        self.display = SSD1306()  # Inicializa o display OLED
        self.last_update = 0
        self.update_interval = 1  # Atualiza a cada 1 segundo

    def get_system_stats(self) -> Dict:
        """Coleta estatísticas do sistema"""
        try:
            stats = {
                "cpu_percent": psutil.cpu_percent(interval=0.1),
                "memory_percent": psutil.virtual_memory().percent,
                "memory_used": f"{psutil.virtual_memory().used / (1024 ** 3):.1f}GB",
                "temperature": 0
            }
            
            # Tenta obter temperatura da CPU (pode variar dependendo do sistema)
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
            # Tenta todos os IPs até encontrar um que começa com o prefixo
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

        # Desenha uma borda
        self.display.draw_rect(0, 0, 128, 32, fill=False)

        # CPU Usage com barra de progresso
        cpu_width = int((stats["cpu_percent"] / 100) * 50)
        self.display.draw_text(2, 2, f"CPU:{stats['cpu_percent']}%")
        self.display.draw_rect(70, 2, 50, 6, fill=False)
        self.display.draw_rect(70, 2, cpu_width, 6, fill=True)

        # Memory Usage com barra de progresso
        mem_width = int((stats["memory_percent"] / 100) * 50)
        self.display.draw_text(2, 12, f"MEM:{stats['memory_percent']}%")
        self.display.draw_rect(70, 12, 50, 6, fill=False)
        self.display.draw_rect(70, 12, mem_width, 6, fill=True)

        # Temperatura e Memória Usada
        if "temperature" in stats and stats["temperature"] > 0:
            self.display.draw_text(2, 22, f"T:{stats['temperature']:.1f}C")
        self.display.draw_text(70, 22, stats["memory_used"])

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
                time.sleep(0.1)  #  pausa para não sobrecarregar o CPU
                
        except KeyboardInterrupt:
            self.display.clear()
            self.display.update()
        except Exception as e:
            print(f"Erro: {str(e)}")

if __name__ == "__main__":
    monitor = SystemMonitorDisplay()
    monitor.run()