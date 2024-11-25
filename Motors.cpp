#include "Motors.hpp"

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
	close(_fdMotor);
	close(_fdServo);
}

bool Motors::init_servo(){
	try{
		    // Reset PCA9685
            write_byte_data(_fdServo, 0x00, 0x06);
            usleep(100000); // Aguarda 100 ms
            
            // Setup servo control
            write_byte_data(_fdServo, 0x00, 0x10);
            usleep(100000);

            // Set frequency (~50Hz)
            write_byte_data(_fdServo, 0xFE, 0x79);
            usleep(100000);

            // Configure MODE2
            write_byte_data(_fdServo, 0x01, 0x04);
            usleep(100000);

            // Enable auto-increment
            write_byte_data(_fdServo, 0x00, 0x20);
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
        write_byte_data(_fdServo, base_reg, on_value & 0xFF);         // Escreve o byte baixo de "on_value"
        write_byte_data(_fdServo, base_reg + 1, on_value >> 8);       // Escreve o byte alto de "on_value"
        write_byte_data(_fdServo, base_reg + 2, off_value & 0xFF);    // Escreve o byte baixo de "off_value"
        write_byte_data(_fdServo, base_reg + 3, off_value >> 8);      // Escreve o byte alto de "off_value"
		return(true);
	}
	catch ( const std::exception &e){
		std::cerr << "Erro ao configurar PWM do servo: " << e.what() << std::endl;
        return false;
	}
}


//void Motors::init_motors(){}

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


void Motors::write_byte_data(int fd, uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg, value};
    if (write(fd, buffer, 2) != 2) {
        throw std::runtime_error("Erro ao escrever no dispositivo I2C.");
    }
}

int main() {
    try {
        Motors car;
        if (car.init_servo()) {
            std::cout << "Servo inicializado com sucesso!" << std::endl;
			car.setServoPwm(0, 0, 300);
			usleep(1000);
			car.set_steering(0);
			usleep(1000);

		} else {
            std::cerr << "Falha na inicialização do servo." << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Erro fatal: " << e.what() << std::endl;
    }

    return 0;
}