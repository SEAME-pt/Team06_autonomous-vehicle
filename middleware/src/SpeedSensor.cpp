#include "../inc/SpeedSensor.hpp"


SpeedSensor::SpeedSensor()
    : _pulseCount(0), running_(false) {
    setupGPIO();
}

SpeedSensor::~SpeedSensor() {
    stop();
    cleanupGPIO();
}

void SpeedSensor::setupGPIO() {
    exportGPIO(_sensorPin);
    setDirection(_sensorPin, "in");
}

void SpeedSensor::cleanupGPIO() {
    unexportGPIO(_sensorPin);
}

void SpeedSensor::exportGPIO(int pin) {
    std::ofstream exportFile("/sys/class/gpio/export");
    if (!exportFile) {
        throw std::runtime_error("Erro ao exportar GPIO");
    }
    exportFile << pin;
}

void SpeedSensor::unexportGPIO(int pin) {
    std::ofstream unexportFile("/sys/class/gpio/unexport");
    if (!unexportFile) {
        throw std::runtime_error("Erro ao desexportar GPIO");
    }
    unexportFile << pin;
}

void SpeedSensor::setDirection(int pin, const std::string& direction) {
    std::ofstream directionFile("/sys/class/gpio/gpio" + std::to_string(pin) + "/direction");
    if (!directionFile) {
        throw std::runtime_error("Erro ao configurar direção do GPIO");
    }
    directionFile << direction;
}

int SpeedSensor::readGPIO(int pin) {
    std::ifstream valueFile("/sys/class/gpio/gpio" + std::to_string(pin) + "/value");
    if (!valueFile) {
        throw std::runtime_error("Erro ao ler valor do GPIO");
    }
    int value;
    valueFile >> value;
    return value;
}

void SpeedSensor::monitorPulses() {
    while (running_) {
        if (readGPIO(_sensorPin) == 1) {
            _pulseCount++;
            usleep(10000); // Debounce de 10ms
        }
    }
}

void SpeedSensor::start() {
    running_ = true;
    monitorThread_ = std::thread(&SpeedSensor::monitorPulses, this);
}

void SpeedSensor::stop() {
    running_ = false;
    if (monitorThread_.joinable()) {
        monitorThread_.join();
    }
}

double SpeedSensor::getSpeedKmh() {
    if (!running_) return 0.0;

    int pulses = _pulseCount;
    double tempoSegundos = 1.0; // Simulando leitura por 1 segundo
    double voltas = static_cast<double>(pulses) / _furos;
    double distancia = voltas * (_rodaDiametro * M_PI); // Distância em metros
    double velocidadeMs = distancia / tempoSegundos;    // Velocidade em m/s
    return velocidadeMs * 3.6;                          // Converter para km/h
}

void SpeedSensor::resetPulses() {
    _pulseCount = 0;
}


bool running = true;

void handleSignal(int signal) {
    running = false;
}

int main() {
    signal(SIGINT, handleSignal);

    try {
        SpeedSensor sensor;
        sensor.start();

        std::cout << "Monitor de Velocidade iniciado. Pressione Ctrl+C para sair.\n";

        while (running) {
            double velocidade = sensor.getSpeedKmh();
            std::cout << "Velocidade: " << velocidade << " km/h\n";
            sensor.resetPulses();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        sensor.stop();
        std::cout << "Encerrando monitoramento.\n";
    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << "\n";
    }

    return 0;
}