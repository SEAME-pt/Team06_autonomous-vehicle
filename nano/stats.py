import psutil
import asyncio
import json
import math
from pathlib import Path
from typing import Dict, Optional, Tuple

class SystemMonitor:
    def __init__(self, ip_prefix: str = "10.19"):
        self.ip_prefix = ip_prefix
        self.model_name, self.gpu_load_path = self._get_gpu_load_path()

    def _get_gpu_load_path(self) -> Tuple[str, str]:
        """
        Determines the GPU load path based on the system model.
        Returns tuple of (model_name, gpu_load_path)
        """
        try:
            model_path = Path("/proc/device-tree/model")
            model_name = model_path.read_text().strip() if model_path.exists() else "Unknown"
            
            # Default GPU load path
            gpu_load_path = "/sys/devices/gpu.0/load"
            
            # Check for Orin and adjust path accordingly
            if "orin" in model_name.lower():
                gpu_load_path = "/sys/devices/platform/gpu.0/load"
                
            return model_name, gpu_load_path
            
        except Exception as e:
            return f"Error: {str(e)}", ""

    def get_gpu_load(self) -> float:
        """
        Reads GPU load from the system.
        Returns GPU load as percentage or 0 if unavailable.
        """
        try:
            if Path(self.gpu_load_path).exists():
                return float(Path(self.gpu_load_path).read_text().strip()) / 10
            return 0.0
        except Exception:
            return 0.0

    def get_ip_address(self) -> Optional[str]:
        """
        Gets the IP address starting with the specified prefix.
        """
        network_interfaces = psutil.net_if_addrs()
        for interface, addresses in network_interfaces.items():
            for addr in addresses:
                if addr.address.startswith(self.ip_prefix):
                    return addr.address
        return None

    def get_system_stats(self) -> Dict:
        """
        Collects comprehensive system statistics including CPU, GPU, memory, and swap usage.
        """
        try:
            # Initialize data structure
            stats = {
                "cpu_percent": 0,
                "gpu_percent": 0.0,
                "memory_percent": 0,
                "memory_used": "0.00 GB",
                "cached_files": "0.00 GB",
                "swap_used": "0.00 MB",
                "physical_memory": "0.00 GB",
                "ip_address": None
            }

            # CPU Usage
            stats["cpu_percent"] = psutil.cpu_percent(interval=1)
            
            # GPU Usage
            stats["gpu_percent"] = self.get_gpu_load()

            # Memory Information
            memory = psutil.virtual_memory()
            stats["memory_percent"] = memory.percent
            stats["memory_used"] = f"{memory.used / (1024 ** 3):.2f} GB"
            stats["cached_files"] = f"{memory.cached / (1024 ** 3):.2f} GB"
            stats["physical_memory"] = f"{math.ceil(memory.total / (1024 ** 3)):.2f} GB"

            # Swap Information
            swap = psutil.swap_memory()
            swap_gb = swap.used / (1024 ** 3)
            stats["swap_used"] = f"{swap_gb * 1024:.2f} MB" if swap_gb < 1 else f"{swap_gb:.2f} GB"

            # IP Address
            stats["ip_address"] = self.get_ip_address()

            return stats

        except Exception as e:
            return {"error": str(e)}




if __name__ == "__main__":
    monitor = SystemMonitor()
    stats = monitor.get_system_stats()
    print(json.dumps(stats, indent=2))
    