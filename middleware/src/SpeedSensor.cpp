#include <wiringPi.h>
#include <iostream>
#include <chrono>
#include <thread>

// Configurações
const int SENSOR_PIN = 0;  // Pin 17 na Raspberry Pi corresponde ao 0 no wiringPi
const double RODA_DIAMETRO = 0.065;  // Diâmetro da roda em metros
const int FUROS = 36;

// Variáveis globais
volatile int pulsos = 0;
auto ultimo_tempo = std::chrono::steady_clock::now();

// Função de callback para detectar o pulso
void pulso_detectado() {
    pulsos++;
}

// Função para calcular a velocidade
double calcular_velocidade(int pulsos, double tempo) {
    double voltas = static_cast<double>(pulsos) / FUROS;
    double distancia = voltas * (RODA_DIAMETRO * 3.14159);  // Distância em metros
    double velocidade_ms = distancia / tempo;
    double velocidade_kmh = velocidade_ms * 3.6;
    return velocidade_kmh;
}

int main() {
    // Inicializa a biblioteca wiringPi
    wiringPiSetup();

    // Configura o pino do sensor como entrada com pull-up interno
    pinMode(SENSOR_PIN, INPUT);
    pullUpDnControl(SENSOR_PIN, PUD_UP);

    // Configura a interrupção para detectar a borda de subida
    wiringPiISR(SENSOR_PIN, INT_EDGE_RISING, &pulso_detectado);

    std::cout << "Aguardando leitura de velocidade. Pressione Ctrl+C para encerrar." << std::endl;

    while (true) {
        auto tempo_atual = std::chrono::steady_clock::now();
        double duracao = std::chrono::duration_cast<std::chrono::seconds>(tempo_atual - ultimo_tempo).count();

        if (duracao >= 1) {
            double kmh = calcular_velocidade(pulsos, 1);
            std::cout << "Velocidade: " << kmh << " km/h" << std::endl;
            pulsos = 0;
            ultimo_tempo = tempo_atual;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Reduz o uso da CPU
    }

    return 0;
}
