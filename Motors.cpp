#include "Motors.hpp"

std::atomic<int> Motors::_pulsos(0);
std::chrono::steady_clock::time_point Motors::_ultimoTempo = std::chrono::steady_clock::now();
// Variável global para sinalizar quando o programa deve terminar
std::atomic<bool> running(true);

Motors::Motors(){
	i2c_device = "/dev/i2c-1";
	_fdServo = open(i2c_device.c_str(), O_RDWR);
	if (_fdServo < 0)
		throw std::runtime_error("Error open I2C");
	if ( ioctl(_fdServo, I2C_SLAVE, _servoAddr) < 0){
		close (_fdServo);
		throw std::runtime_error("Erro ao configurar endereço I2C do servo.");
	}

	// Configurar endereço do motor

	_fdMotor = open(i2c_device.c_str(), O_RDWR);
	if (_fdMotor < 0){
		close(_fdServo);
		throw std::runtime_error("Erro ao configurar endereço I2C do motor.");
	}

	if (ioctl(_fdMotor, I2C_SLAVE, _motorAddr) < 0){
		close(_fdServo);
		close (_fdMotor);
		throw std::runtime_error("Erro ao configurar endereço I2C do motor.");
	}
	std::cout << "JetCar inicializado com sucesso!" << std::endl;
}

Motors::~Motors(){
	setSpeed(0);
	close(_fdMotor);
	close(_fdServo);
    std::cout << "destructor call\n";
}

bool Motors::init_servo(){
	try{
		    // Reset PCA9685
            writeByteData(_fdServo, 0x00, 0x06);
            usleep(100000); // Aguarda 100 ms
            
            // Setup servo control
            writeByteData(_fdServo, 0x00, 0x10);
            usleep(100000);

            // Set frequency (~50Hz)
            writeByteData(_fdServo, 0xFE, 0x79);
            usleep(100000);

            // Configure MODE2
            writeByteData(_fdServo, 0x01, 0x04);
            usleep(100000);

            // Enable auto-increment
            writeByteData(_fdServo, 0x00, 0x20);
            usleep(100000);
		return true;
	}
	catch(const std::exception &e){
		std::cerr << "Erro ao inicializar o servo: " << e.what() << std::endl;
        return false;
	}
}

bool Motors::setServoPwm(const int channel, int on_value, int off_value){
	try{
		 // Calcula o endereço base do registrador para o canal específico
        int base_reg = 0x06 + (channel * 4);

        // Escreve os valores de "on" e "off" nos registradores
        writeByteData(_fdServo, base_reg, on_value & 0xFF);         // Escreve o byte baixo de "on_value"
        writeByteData(_fdServo, base_reg + 1, on_value >> 8);       // Escreve o byte alto de "on_value"
        writeByteData(_fdServo, base_reg + 2, off_value & 0xFF);    // Escreve o byte baixo de "off_value"
        writeByteData(_fdServo, base_reg + 3, off_value >> 8);      // Escreve o byte alto de "off_value"
		return(true);
	}
	catch ( const std::exception &e){
		std::cerr << "Erro ao configurar PWM do servo: " << e.what() << std::endl;
        return false;
	}
}


bool Motors::init_motors(){
	try{
		// Configure motor controller
		writeByteData(_fdMotor, 0x00, 0x20);

		// Set frequency to 60Hz
		int 	preScale;
		uint8_t oldMode, newMode;

		preScale = static_cast<int>(std::floor(25000000.0 / 4096.0 / 60 - 1));
		oldMode = readByteData(_fdMotor, 0x00);
		newMode = (oldMode & 0x7F) | 0x10;

		 // Configurar o novo modo e frequência
		writeByteData(_fdMotor, 0x00, newMode);
		writeByteData(_fdMotor, 0xFE, preScale);
		writeByteData(_fdMotor, 0x00, oldMode);

		usleep(5000);

		// Ativar auto-incremento
		writeByteData(_fdMotor, 0x00, oldMode | 0xa1);
		return true;
	}
	catch(const std::exception &e){
		std::cerr << "Erro na inicialização dos motores: " << e.what() << std::endl;
		return false;
	}
}

bool Motors::setMotorPwm(const int channel, int value){
	value = std::min(std::max(value, 0), 4095);
	try{
		writeByteData(_fdMotor, 0x06 + 4 * channel, 0);
		writeByteData(_fdMotor, 0x07 + 4 * channel, 0);
		writeByteData(_fdMotor, 0x08 + 4 * channel, value & 0xFF);
		writeByteData(_fdMotor, 0x09 + 4 * channel, value >> 8);
		return true;

	}
	catch(const std::exception &e){
		std::cerr << "Erro PWM dos motores: " << e.what() << std::endl;
		return false;
	}
}

void Motors::set_steering(int angle){
    /* """Set steering angle (-90 to +90 degrees)""" */
	angle = std::max(-_maxAngle, std::min(_maxAngle, angle));
	
	int pwm;
	if (angle < 0){
		// Calcula o PWM para ângulo negativo
		pwm = static_cast<int>(_servoCenterPwm + (static_cast<float>(angle) / _maxAngle) * (_servoCenterPwm - _servoLeftPwm));
	}
	else if (angle > 0){
		 // Calcula o PWM para ângulo positivo
		 pwm = static_cast<int>(_servoCenterPwm + (static_cast<float>(angle) / _maxAngle) * (_servoRightPwm - _servoCenterPwm));
	}
	else 
		pwm = _servoCenterPwm;
	setServoPwm(_sterringChannel, 0 , pwm);
	_currentAngle = angle;
}

void Motors::setSpeed(int speed){
	int pwmValue;
	speed = std::max(-100, std::min(100, speed));
	pwmValue = static_cast<int>(std::abs(speed) / 100.0 * 4095);

	if (speed > 0){ //forward
		setMotorPwm(0, pwmValue); //IN1
		setMotorPwm(1, 0);        //IN2
		setMotorPwm(2, pwmValue); //ENA
		setMotorPwm(5, pwmValue); //IN3
		setMotorPwm(6, 0);		  //IN4
		setMotorPwm(7, pwmValue); //ENB
	}
	else if (speed < 0){ //backward
		setMotorPwm(0, pwmValue); //IN1
		setMotorPwm(1, pwmValue);        //IN2
		setMotorPwm(2, 0); //ENA
		setMotorPwm(6, pwmValue); //IN3
		setMotorPwm(7, pwmValue); //IN4
		setMotorPwm(8, 0); //ENB
	}
	else{
		for (int channel = 0; channel < 9; ++channel) {
                setMotorPwm(channel, 0);
            }
	}
	_currentSpeed = speed;
}

uint8_t Motors::readByteData(int fd, uint8_t reg){
	if(write(fd, &reg, 1) != 1)
		throw std::runtime_error("Erro ao enviar o registrador ao dispositivo I2C.");
	uint8_t value;
	if (read(fd, &value, 1) != 1)
		throw std::runtime_error("Erro ao ler o registrador ao dispositivo I2C.");
	return value;
}


void Motors::writeByteData(int fd, uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg, value};
    if (write(fd, buffer, 2) != 2) {
        throw std::runtime_error("Erro ao escrever no dispositivo I2C.");
    }
}

// Função de callback chamada em interrupções
void Motors::pulsoDetectado(struct gpiod_line_event event) {
    if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) { // Detecta borda de subida
        _pulsos++;
    }
	std::cout << "pulso:  " << _pulsos << "\n";
}

// Calcula a velocidade com base nos pulsos e no tempo decorrido
double Motors::calcularVelocidade(int pulsos, double tempo) {
    double voltas = static_cast<double>(pulsos) / FUROS;
    double distancia = voltas * (RODA_DIAMETRO * M_PI); // Distância percorrida
    double velocidade_ms = distancia / tempo;           // Velocidade em m/s
    return velocidade_ms * 3.6;                         // Converte para km/h
}

// Thread para monitorar GPIO e processar interrupções
void Motors::monitorGPIO(struct gpiod_line *line) {
    struct timespec timeout;
    timeout.tv_sec = 1; // Tempo de espera em segundos
    timeout.tv_nsec = 0; // Tempo de espera em nanosegundos

    while (running.load()) { // Usa .load() para ler de forma atômica
        int ret = gpiod_line_event_wait(line, &timeout); // Aguarda por um evento ou timeout
        std::cout << "ret:  " << ret << "\n";
        if (ret > 0) {
            struct gpiod_line_event event;
            gpiod_line_event_read(line, &event); // Lê o evento
            pulsoDetectado(event);
        } else if (ret < 0) {
            std::cerr << "Erro ao esperar por eventos GPIO!" << std::endl;
            break;
        } else {
            // Se ret == 0, isso significa que houve um timeout
            std::cout << "Timeout atingido na espera do evento GPIO.\n";
        }
    }
    std::cout << "Thread monitorGPIO closed.\n";
}

void Motors::updateVol(){
  while (true) {
	    auto agora = std::chrono::steady_clock::now();
        double tempo_decorrido = std::chrono::duration<double>(agora - _ultimoTempo).count();

        if (tempo_decorrido >= 1.0) { // Atualiza a cada 1 segundo
            int pulsosAtual = _pulsos.exchange(0); // Reseta contador de pulsos
            double velocidade = calcularVelocidade(pulsosAtual, tempo_decorrido);

            std::cout << "Velocidade: " << velocidade << " km/h" << std::endl;
            _ultimoTempo = agora;
        }
        if (running == false){
            std::cout << "Thread do updateVol closed\n";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Pausa para reduzir uso da CPU
	}
}

// Função de tratamento de sinal para interromper o programa com Ctrl+C
void signalHandler(int signum) {
	(void)signum;
    std::cout << "\nInterrupção recebida, parando os motores..." << std::endl;
    running = false;
}

/* int main() {
    // Configura o manipulador de sinal para capturar Ctrl+C (SIGINT)
    std::signal(SIGINT, signalHandler);

    try {
        // Inicializa a classe Motors
        Motors jetCar;

        // Inicializa o servo e os motores
        if (!jetCar.init_servo()) {
            std::cerr << "Erro ao inicializar o servo." << std::endl;
            return 1;
        }
        if (!jetCar.init_motors()) {
            std::cerr << "Erro ao inicializar os motores." << std::endl;
            return 1;
        }

        std::cout << "Sistema inicializado com sucesso!" << std::endl;

        // Configura GPIO para a leitura do sensor de velocidade
        struct gpiod_chip *chip = gpiod_chip_open("/dev/gpiochip0");
        if (!chip) {
            std::cerr << "Erro ao abrir o chip GPIO!" << std::endl;
            return 1;
        }

        struct gpiod_line *line = gpiod_chip_get_line(chip, 17); // Substitua pelo número do pino correto
        if (!line) {
            std::cerr << "Erro ao obter a linha GPIO!" << std::endl;
            gpiod_chip_close(chip);
            return 1;
        }

        // Configura a linha para detectar bordas de subida e queda
        if (gpiod_line_request_both_edges_events(line, "sensor_velocidade") < 0) {
            std::cerr << "Erro ao configurar GPIO para eventos!" << std::endl;
            gpiod_chip_close(chip);
            return 1;
        }

        // Cria threads para monitorar a velocidade e controlar o movimento
        std::thread monitoramentoThread(&Motors::monitorGPIO, &jetCar, line);
        std::thread velocidadeThread(&Motors::updateVol, &jetCar);

        // Define a velocidade dos motores para iniciar o movimento
        jetCar.setSpeed(-10); // Ajuste a velocidade conforme necessário

        // Enquanto o programa estiver rodando, mantém o loop ativo
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1)); // Simples pausa para reduzir uso da CPU
        }

        // Para encerrar, interrompe as threads e fecha recursos
        std::cout << "Parando os motores..." << std::endl;
        running.store(false); // Alteração atômica para sinalizar que as threads devem parar
        monitoramentoThread.join();
        velocidadeThread.join();
        jetCar.setSpeed(0); // Para de os motores


        gpiod_line_release(line);
        gpiod_chip_close(chip);

        return 1;
    } catch (const std::exception &e) {
        std::cerr << "Erro: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
 */


int main() {
    struct gpiod_chip *chip = gpiod_chip_open("/dev/gpiochip0");
    if (!chip) {
        std::cerr << "Erro ao abrir o chip GPIO!" << std::endl;
        return 1;
    }

    struct gpiod_line *line = gpiod_chip_get_line(chip, 17); // Número do pino do sensor
    if (!line) {
        std::cerr << "Erro ao obter a linha GPIO!" << std::endl;
        gpiod_chip_close(chip);
        return 1;
    }

    // Configura a linha para detectar bordas de subida e descida
    if (gpiod_line_request_both_edges_events(line, "teste") < 0) {
        std::cerr << "Erro ao configurar a linha para eventos!" << std::endl;
        gpiod_chip_close(chip);
        return 1;
    }

    struct gpiod_line_event event;
    while (true) {
        int ret = gpiod_line_event_wait(line, nullptr); // Espera indefinidamente por um evento
        if (ret > 0) {
            gpiod_line_event_read(line, &event);
            std::cout << "Evento detectado: " << event.event_type << std::endl;
        } else if (ret < 0) {
            std::cerr << "Erro ao esperar por eventos GPIO!" << std::endl;
            break;
        }
    }

    gpiod_chip_close(chip);
    return 0;
}


        // Teste de controle do servo (direção)
        //std::cout << "Girando o volante para a esquerda (-45 graus)..." << std::endl;
        //jetCar.set_steering(-45);
        //usleep(1000000); // Aguarda 1 segundo
//
        //std::cout << "Girando o volante para o centro (0 graus)..." << std::endl;
        //jetCar.set_steering(0);
        //usleep(1000000);
//
        //std::cout << "Girando o volante para a direita (+45 graus)..." << std::endl;
        //jetCar.set_steering(45);
        //usleep(1000000);
//
		//std::cout << "Girando o volante para o centro (0 graus)..." << std::endl;
        //jetCar.set_steering(0);
        //usleep(1000000);

        // Teste de controle dos motores (velocidade)
      //  std::cout << "Acelerando para frente (50%)..." << std::endl;
      //  jetCar.setSpeed(100);
      //  usleep(2000000); // Aguarda 2 segundos
//
      //  std::cout << "Reduzindo para 0 (parando)..." << std::endl;
      //  jetCar.setSpeed(0);
      //  usleep(1000000);
//
      //  std::cout << "Recuo (marcha ré, 30%)..." << std::endl;
      //  jetCar.setSpeed(-100);
      //  usleep(2000000);
//
      //  std::cout << "Parando o veículo..." << std::endl;
      //  jetCar.setSpeed(0);
//
      //  std::cout << "Teste concluído com sucesso!" << std::endl;