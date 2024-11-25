import psutil
import asyncio
import json
import math


def get_gpu_load_path():
    try:
        # Read the model name from /proc/device-tree/model
        with open("/proc/device-tree/model", "r") as file:
            model_name = file.read().strip()
        # Previous to Orin, this symbolic link points to the GPU load file
        gpu_load_path = "/sys/devices/gpu.0/load" 
        if "orin" in model_name.lower():
            # The location has been changed on the orins - /sys/devices/platform/bus@0/17000000.gpu/load
            # There's a symbolic link here:
            gpu_load_path = "/sys/devices/platform/gpu.0/load"
             
        return model_name, gpu_load_path
    except FileNotFoundError:
        return "Model file not found."
    except Exception as e:
        return f"An error occurred: {e}"

# MODEL_NAME, GPU_LOAD_PATH = get_gpu_load_path()
# print(GPU_LOAD_PATH)
# print(MODEL_NAME)

def get_pc_data()
    try:
            
        latest_data = {
            "cpu_percent": 0,
            "gpu_percent": 0.0,
            "memory_percent": 0,
            "memory_used": "0.00 GB",
            "cached_files": "0.00 GB",
            "swap_used": "0.00 MB",
            "physical_memory": "0.00 GB"
        }

        latest_data["cpu_percent"] = psutil.cpu_percent(interval=1)

        memory_info = psutil.virtual_memory()
        latest_data["memory_percent"] = memory_info.percent
        latest_data["memory_used"] = f"{memory_info.used / (1024 ** 3):.2f} GB"
        latest_data["cached_files"] = f"{memory_info.cached / (1024 ** 3):.2f} GB"

        swap_used_value = psutil.swap_memory().used / (1024 ** 3)
        if swap_used_value < 1:
            latest_data["swap_used"] = f"{swap_used_value * 1024:.2f} MB"
        else:
            latest_data["swap_used"] = f"{swap_used_value:.2f} GB"
            
        latest_data["physical_memory"] = f"{math.ceil(memory_info.total / (1024 ** 3)):.2f} GB"
    
    return latest_data



def get_ip(start="10.19")
    addrs = psutil.net_if_addrs()
    for interface, address in addrs.items():
        for addr in address:
            if addr.address.startswith(start):
                    return addr.address
