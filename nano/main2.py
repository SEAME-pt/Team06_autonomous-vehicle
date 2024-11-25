
import time
import smbus2

            
class JetracerI2CController:
    def __init__(self, barramento=1, endereco=0x40):
        try:
            self.bus = smbus2.SMBus(barramento)
            self.endereco = endereco
            print(f"I2C inicializado no barramento {barramento}, endereço {hex(endereco)}")
            
            # Adiciona verificações iniciais
            self.verificar_comunicacao()
            self.diagnostico_i2c()
        
        except Exception as e:
            print(f"Erro ao inicializar I2C: {e}")
            self.bus = None

    # [Restante do código anterior permanece igual]

    def teste_avancado(self):
        """Teste mais robusto de movimentação"""
        try:
            print("Teste Avançado de Movimentação")
            
            # Sequência de testes
            velocidades = [30, 50, 70, 100]
            
            for vel in velocidades:
                print(f"\nTestando velocidade: {vel}")
                self.mover_frente(velocidade=vel, duracao=1)
                time.sleep(0.5)
                
                self.mover_re(velocidade=vel, duracao=1)
                time.sleep(0.5)
            
            print("Teste avançado concluído")
        
        except Exception as e:
            print(f"Erro no teste avançado: {e}")
        
        finally:
            self.parar()

    # Substitua a função de configuração I2C por:
    def configurar_i2c():
        try:
            import subprocess
            resultado = subprocess.run(['i2cdetect', '-y', '1'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            print("Dispositivos I2C detectados:")
            print(resultado.stdout)
        except Exception as e:
            print(f"Erro na configuração I2C: {e}")

    def enviar_comando_motor(self, motor_id, velocidade, direcao):
        try:
            # Ajuste para lidar com valores negativos
            velocidade_abs = min(abs(int(velocidade)), 100)
            direcao_byte = 1 if velocidade < 0 else 0
            
            # Exemplo de comando mais robusto
            comando = [
                motor_id,        # ID do motor
                velocidade_abs,  # Velocidade absoluta
                direcao_byte     # Direção
            ]
            
            # Método de escrita I2C
            self.bus.write_i2c_block_data(self.endereco, motor_id, comando)
            
            print(f"Comando I2C - Motor {motor_id}: Velocidade {velocidade}, Direção {direcao_byte}")
        
        except Exception as e:
            print(f"Erro ao enviar comando I2C: {e}")
    def diagnostico_i2c(self):
        """Função de diagnóstico detalhado"""
        try:
            # Verifica dispositivos no barramento
            dispositivos = []
            for endereco in range(0x03, 0x78):
                try:
                    self.bus.read_byte(endereco)
                    dispositivos.append(hex(endereco))
                except:
                    pass
            
            print("Dispositivos I2C detectados:")
            for dev in dispositivos:
                print(f"- Endereço: {dev}")
            
            return dispositivos
        
        except Exception as e:
            print(f"Erro no diagnóstico I2C: {e}")
            return []
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

    def parar(self):
        """Para completamente os motores"""
        print("Parando motores")
        self.enviar_comando_motor(0, 0, 0)
        self.enviar_comando_motor(1, 0, 0)
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
    def verificar_comunicacao(self):
        """Testa comunicação básica"""
        try:
            # Leitura de teste
            teste = self.bus.read_i2c_block_data(self.endereco, 0, 3)
            print("Comunicação I2C OK")
            print("Dados de teste:", teste)
            return True
        except Exception as e:
            print(f"Erro de comunicação I2C: {e}")
            return False    
# Execução
if __name__ == "__main__":
    jetracer = JetracerI2CController()
    #jetracer.teste_completo()
    jetracer.teste_avancado()