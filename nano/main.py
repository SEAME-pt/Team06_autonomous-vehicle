import time
import smbus2

class JetracerI2CController:
    def __init__(self, barramento=1, endereco=0x40):
        """
        Inicializa a comunicação I2C
        
        Args:
        - barramento: Número do barramento I2C (geralmente 1 no Jetson Nano)
        - endereco: Endereço I2C do controlador de motores
        """
        try:
            # Inicializa o barramento I2C
            self.bus = smbus2.SMBus(barramento)
            self.endereco = endereco
            print(f"I2C inicializado no barramento {barramento}, endereço {hex(endereco)}")
        except Exception as e:
            print(f"Erro ao inicializar I2C: {e}")
            self.bus = None

    def enviar_comando_motor(self, motor_id, velocidade, direcao):
        """
        Envia comando para controle de motor via I2C
        
        Args:
        - motor_id: Identificador do motor (0 ou 1)
        - velocidade: Valor de -100 a 100
        - direcao: Direção do motor
        """
        try:
            # Prepara os bytes de comando
            # Formato pode variar dependendo do controlador específico
            velocidade_abs = abs(int(velocidade))
            direcao_byte = 1 if velocidade < 0 else 0
            
            # Sequência de bytes para controle de motor
            # Exemplo: [motor_id, velocidade, direção]
            comando = [
                motor_id,   # Identificador do motor
                velocidade_abs,  # Valor de velocidade
                direcao_byte     # Direção (0: frente, 1: trás)
            ]
            
            # Envia os bytes via I2C
            self.bus.write_i2c_block_data(self.endereco, 0x00, comando)
            
            print(f"Comando I2C - Motor {motor_id}: Velocidade {velocidade}, Direção {direcao_byte}")
        
        except Exception as e:
            print(f"Erro ao enviar comando I2C: {e}")

    def mover_frente(self, velocidade=50, duracao=None):
        """Move o carro para frente"""
        print(f"Movendo para frente - Velocidade: {velocidade}")
        # Motor esquerdo
        self.enviar_comando_motor(0, velocidade, 0)
        # Motor direito
        self.enviar_comando_motor(1, velocidade, 0)
        
        if duracao:
            time.sleep(duracao)
            self.parar()

    def mover_re(self, velocidade=50, duracao=None):
        """Move o carro para trás"""
        print(f"Movendo para trás - Velocidade: {velocidade}")
        # Motor esquerdo
        self.enviar_comando_motor(0, -velocidade, 1)
        # Motor direito
        self.enviar_comando_motor(1, -velocidade, 1)
        
        if duracao:
            time.sleep(duracao)
            self.parar()

    def girar_direita(self, velocidade=40, duracao=None):
        """Gira o carro para a direita"""
        print(f"Girando para direita - Velocidade: {velocidade}")
        # Motor esquerdo para frente
        self.enviar_comando_motor(0, velocidade, 0)
        # Motor direito para trás
        self.enviar_comando_motor(1, -velocidade, 1)
        
        if duracao:
            time.sleep(duracao)
            self.parar()

    def girar_esquerda(self, velocidade=40, duracao=None):
        """Gira o carro para a esquerda"""
        print(f"Girando para esquerda - Velocidade: {velocidade}")
        # Motor esquerdo para trás
        self.enviar_comando_motor(0, -velocidade, 1)
        # Motor direito para frente
        self.enviar_comando_motor(1, velocidade, 0)
        
        if duracao:
            time.sleep(duracao)
            self.parar()

    def parar(self):
        """Para completamente os motores"""
        print("Parando motores")
        self.enviar_comando_motor(0, 0, 0)
        self.enviar_comando_motor(1, 0, 0)

    def teste_completo(self):
        """Sequência completa de testes de movimento"""
        try:
            print("Iniciando teste completo de movimentos I2C")
            
            # Teste de movimentos básicos
            self.mover_frente(velocidade=60, duracao=2)
            time.sleep(1)
            
            self.mover_re(velocidade=60, duracao=2)
            time.sleep(1)
            
            self.girar_direita(velocidade=50, duracao=1)
            time.sleep(1)
            
            self.girar_esquerda(velocidade=50, duracao=1)
            
            print("Teste de movimentos I2C concluído")
        
        except Exception as e:
            print(f"Erro durante o teste: {e}")
        
        finally:
            self.parar()

    def ler_status_motor(self, motor_id):
        try:
            # Comando para ler status (pode variar conforme o controlador)
            status = self.bus.read_i2c_block_data(self.endereco, motor_id, 3)
            
            print(f"Status Motor {motor_id}: {status}")
            return status
        
        except Exception as e:
            print(f"Erro ao ler status do motor: {e}")
            return None

# Configuração inicial do I2C
def configurar_i2c():
    import subprocess
    
    try:
        # Verifica dispositivos I2C
        print("Dispositivos I2C detectados:")
        resultado = subprocess.run(['i2cdetect', '-y', '1'], capture_output=True, text=True)
        print(resultado.stdout)
    except Exception as e:
        print(f"Erro na configuração I2C: {e}")

# Execução do script
if __name__ == "__main__":
    # Configura I2C
    configurar_i2c()
    
    # Cria uma instância do controlador I2C
    jetracer_i2c = JetracerI2CController()
    
    # Executa o teste completo
    jetracer_i2c.teste_completo()
    
    # Opcional: verificar status dos motores
    jetracer_i2c.ler_status_motor(0)
    jetracer_i2c.ler_status_motor(1)