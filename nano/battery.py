import smbus
import time
from typing import Dict

class BatteryMonitor:
    def __init__(self, i2c_bus: int = 1, adc_address: int = 0x41):
        """
        Inicializa o monitor de bateria para 3x 18650 em série
        Args:
            i2c_bus: Número do barramento I2C (geralmente 1)
            adc_address: Endereço I2C do ADC (0x41 por padrão)
        """
        self.bus = smbus.SMBus(i2c_bus)
        self.adc_address = adc_address
        
        # Constantes para cálculo da tensão
        self.ADC_REF = 3.3         # Tensão de referência do ADC
        self.ADC_MAX = 65535       # Valor máximo do ADC (16 bits)
        self.VOLTAGE_DIVIDER = 17.0 # Ajustado para bateria 3S 18650 (12.6V máx)
        
        # Limites da bateria 18650 3S
        self.MAX_VOLTAGE = 12.6    # 3 x 4.2V (totalmente carregada)
        self.NOMINAL_VOLTAGE = 11.1 # 3 x 3.7V (nominal)
        self.MIN_VOLTAGE = 9.0     # 3 x 3.0V (mínimo seguro)
        
    def read_adc(self) -> int:
        """
        Lê o valor bruto do ADC
        Returns:
            Valor inteiro do ADC (0-65535)
        """
        try:
            data = self.bus.read_i2c_block_data(self.adc_address, 0, 2)
            value = (data[0] << 8) | data[1]
            return value
        except Exception as e:
            print(f"Erro na leitura do ADC: {e}")
            return 0
            
    def get_battery_voltage(self) -> float:
        """
        Lê e converte o valor do ADC para tensão
        Returns:
            Tensão da bateria em volts
        """
        adc_value = self.read_adc()
        v_measured = (adc_value * self.ADC_REF) / self.ADC_MAX
        v_battery = v_measured * self.VOLTAGE_DIVIDER
        return v_battery
        
    def get_battery_percentage(self) -> float:
        """
        Calcula a porcentagem da bateria baseado na tensão
        Returns:
            Porcentagem da bateria (0-100)
        """
        voltage = self.get_battery_voltage()
        percentage = ((voltage - self.MIN_VOLTAGE) / 
                     (self.MAX_VOLTAGE - self.MIN_VOLTAGE)) * 100
        return max(0, min(100, percentage))

    def get_battery_status(self, voltage: float) -> str:
        """
        Retorna o status da bateria baseado na tensão para 18650 3S
        """
        if voltage >= 12.0:    # 4.0V por célula
            return "FULL"
        elif voltage >= 11.1:  # 3.7V por célula (nominal)
            return "GOOD"
        elif voltage >= 10.2:  # 3.4V por célula
            return "LOW"
        else:
            return "CRITICAL"

    def get_cell_voltages(self, total_voltage: float) -> list:
        """
        Estima a tensão por célula (assumindo células balanceadas)
        """
        return [total_voltage / 3] * 3
        
    def get_battery_info(self) -> Dict:
        """
        Retorna informações completas da bateria
        Returns:
            Dicionário com todas as informações da bateria
        """
        voltage = self.get_battery_voltage()
        percentage = self.get_battery_percentage()
        adc_value = self.read_adc()
        adc_voltage = (adc_value * self.ADC_REF) / self.ADC_MAX
        cell_voltages = self.get_cell_voltages(voltage)
        
        return {
            "voltage": voltage,
            "percentage": percentage,
            "raw_adc": adc_value,
            "adc_voltage": adc_voltage,
            "status": self.get_battery_status(voltage),
            "cell_voltages": cell_voltages
        }

def main():
    """Função principal para teste"""
    battery = BatteryMonitor()
    print("Monitor de Bateria 18650 3S")
    print("Pressione Ctrl+C para sair\n")
    
    try:
        while True:
            info = battery.get_battery_info()
            print(f"Valor ADC: {info['raw_adc']}")
            print(f"Tensão ADC: {info['adc_voltage']:.3f}V")
            print(f"Tensão Total: {info['voltage']:.2f}V")
            print(f"Tensão por célula: {info['cell_voltages'][0]:.2f}V")
            print(f"Bateria: {info['percentage']:.1f}%")
            print(f"Status: {info['status']}")
            print("-" * 20)
            time.sleep(1)
            
    except KeyboardInterrupt:
        print("\nMonitoramento encerrado ")
    except Exception as e:
        print(f"Erro: {e}")

if __name__ == "__main__":
    main()