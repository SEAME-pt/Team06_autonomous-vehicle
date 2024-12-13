//#include <gpiod.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>

#define SENSOR_PIN 17 // Número do pino GPIO na placa
#define RODA_DIAMETRO 0.065 // Diâmetro da roda em metros
#define FUROS 36 // Quantidade de furos no sensor

volatile int pulsos = 0; // Contador de pulsos

// Função de callback para quando um pulso é detectado
void pulsoDetectado(int linha) {
    pulsos++;
}

double calcularVelocidade(int pulsos, double tempoSegundos) {
    double voltas = static_cast<double>(pulsos) / FUROS;
    double distancia = voltas * (RODA_DIAMETRO * M_PI); // Distância em metros
    double velocidade_ms = distancia / tempoSegundos;
    double velocidade_kmh = velocidade_ms * 3.6;
    return velocidade_kmh;
}

/* int main() {
    try {
        // Abre o chip GPIO
        gpiod_chip* chip = gpiod_chip_open("/dev/gpiochip0");
        if (!chip) {
            std::cerr << "Erro ao abrir o chip GPIO." << std::endl;
            return 1;
        }

        // Obtém a linha GPIO específica para o sensor
        gpiod_line* line = gpiod_chip_get_line(chip, SENSOR_PIN);
        if (!line) {
            std::cerr << "Erro ao obter a linha GPIO." << std::endl;
            gpiod_chip_close(chip);
            return 1;
        }

        // Configura a linha como uma entrada e define uma interrupção para a borda crescente
        if (gpiod_line_request_rising_edge_events(line, "pulsos_sensor") < 0) {
            std::cerr << "Erro ao configurar a linha para eventos de borda crescente." << std::endl;
            gpiod_chip_close(chip);
            return 1;
        }

        auto ultimoTempo = std::chrono::steady_clock::now();
        double velocidade_kmh = 0.0;

        std::cout << "Contando pulsos. Pressione Ctrl+C para parar." << std::endl;

        while (true) {
            // Espera por um evento de interrupção na linha
            gpiod_line_event evento;
            if (gpiod_line_event_wait(line, nullptr) < 0) {
                std::cerr << "Erro ao esperar pelo evento de interrupção." << std::endl;
                break;
            }

            if (gpiod_line_event_read(line, &evento) < 0) {
                std::cerr << "Erro ao ler o evento de interrupção." << std::endl;
                break;
            }

            // Atualiza a contagem de pulsos
            pulsoDetectado(SENSOR_PIN);

            // Atualiza o tempo atual
            auto agora = std::chrono::steady_clock::now();
            std::chrono::duration<double> duracao = agora - ultimoTempo;

            if (duracao.count() >= 1.0) { // Atualiza a cada segundo
                velocidade_kmh = calcularVelocidade(pulsos, 1.0); // Tempo = 1 segundo
                std::cout << "Velocidade: " << velocidade_kmh << " km/h" << std::endl;

                // Reseta contadores
                pulsos = 0;
                ultimoTempo = agora;
            }

            // Adiciona um atraso para evitar alta carga da CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Fecha o chip GPIO e libera a linha
        gpiod_chip_close(chip);
    } catch (const std::exception &e) {
        std::cerr << "Erro: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Erro desconhecido." << std::endl;
    }

    std::cout << "Encerrando programa..." << std::endl;
    return 0;
} */
