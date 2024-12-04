#pragma once

#include <atomic>
#include <thread>
#include <functional>
#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <csignal>

class SpeedSensor {
private:
    int		_sensorPin = 17;                			// Pino do sensor
    double _rodaDiametro = 0.065;          			// Diâmetro da roda
    int		_furos = 36;                    			// Número de furos no disco
    std::atomic<int> _pulseCount;  			// Contador de pulsos
    bool running_;                 			// Indicador de execução
    std::thread monitorThread_;    			// Thread para monitorar GPIO

    void setupGPIO();              			// Configurar GPIO
    void cleanupGPIO();            			// Limpar GPIO
    void monitorPulses();          			// Monitora pulsos do sensor
    void exportGPIO(int pin);      			// Exportar GPIO
    void unexportGPIO(int pin);    			// Desexportar GPIO
    void setDirection(int pin, const std::string& direction); // Configurar direção
    int readGPIO(int pin);         			// Ler estado do GPIO

public:
	SpeedSensor();
    ~SpeedSensor();

    void start();                  // Inicia o monitoramento do sensor
    void stop();                   // Para o monitoramento do sensor
    double getSpeedKmh();          // Retorna a velocidade em km/h
    void resetPulses();            // Reseta a contagem de pulsos
};