/*
 * Simplified Arduino Sensors with Speed and Distance
 * Uses CAN_BUS_SHIELD library for communication
 */

#include <SPI.h>
#include "mcp2515_can.h"
#include <Wire.h>

// CAN-Bus Shield definitions
#define CAN_2515
#define MCP_16MHZ    0
#define MCP_8MHZ     1
const int SPI_CS_PIN = 9;
const int CAN_INT_PIN = 2;
mcp2515_can CAN(SPI_CS_PIN);

// Speed sensor definitions
const int ENCODER_PIN = 3;
const unsigned int pulsesPerRevolution = 18; // 18 holes in the disc
const float wheelDiameter_mm = 67.0; // Wheel diameter in millimeters
const float wheelCircumference_m = (wheelDiameter_mm / 1000.0) * PI; // Circumference in meters
const unsigned long measurementInterval = 500; // Measurement interval in milliseconds (0.5s)

volatile unsigned long pulseCount = 0;
volatile unsigned long totalPulses = 0; // Total pulses since startup
unsigned long lastMeasurementTime = 0;
float currentSpeed_mps = 0.0; // m/s

// SRF08 variables
#define SRF08_ADDRESS 0x70
unsigned int distance = 0;
unsigned long lastSRF08Update = 0;
const unsigned long SRF08_UPDATE_INTERVAL = 50;

// Interrupt to count pulses
void pulseISR() {
    pulseCount++;
    totalPulses++;
}

void setup() {
    Serial.begin(9600);
    Serial.println("Initializing simplified Arduino sensors...");

    // Configure encoder pin as input with internal pull-up
    pinMode(ENCODER_PIN, INPUT_PULLUP);

    // Activate interrupt on encoder pin
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), pulseISR, RISING);
    Serial.println("Speed sensor initialized");

    // Initialize CAN-Bus Shield
    Serial.print("Initializing CAN... ");
    while (CAN.begin(CAN_500KBPS, MCP_8MHZ) != CAN_OK) {
        Serial.println("CAN BUS Initialization Failed! Retrying...");
        delay(1000);
    }
    Serial.println("CAN BUS Initialized successfully!");

    // Initialize I2C for SRF08
    Serial.print("Initializing I2C... ");
    Wire.begin();
    Serial.println("OK");

    // Initialize SRF08
    Serial.print("Initializing SRF08... ");
    initSRF08();
    Serial.println("OK");

    Serial.println("Simplified sensors initialized!");
    Serial.println("CAN IDs: Speed=0x100, SRF08=0x101");
}

void loop() {
    unsigned long currentTime = millis();

    // Update speed sensor
    if (currentTime - lastMeasurementTime >= measurementInterval) {
        updateSpeedSensor();
        sendSpeedData();
        lastMeasurementTime = currentTime;
    }

    // Update SRF08 sensor
    if (currentTime - lastSRF08Update >= SRF08_UPDATE_INTERVAL) {
        updateSRF08Sensor();
        sendDistanceData();
        lastSRF08Update = currentTime;
    }

    delay(10);
}

void updateSpeedSensor() {
    // Calculate the number of pulses in the time interval
    unsigned long pulsesInInterval = pulseCount;

    // Reset counter for next measurement
    pulseCount = 0;

    // Calculate speed
    float revolutions = (float)pulsesInInterval / pulsesPerRevolution;
    currentSpeed_mps = (revolutions * wheelCircumference_m) / (measurementInterval / 1000.0);

    // Display speed in Serial Monitor (for debugging)
    Serial.print("Speed: ");
    Serial.print(currentSpeed_mps * 3.6, 2); // Convert to km/h
    Serial.println(" km/h");
}

void sendSpeedData() {
    // Simple CAN message for Speed sensor (CAN ID 0x100)
    long id = 0x100;
    byte data[8];

    // Convert speed to km/h * 10 for precision
    uint16_t speedValue = (uint16_t)(currentSpeed_mps * 3.6 * 10);

    // buffer[0-1]: Speed value (16-bit, little endian) - km/h * 10
    data[0] = speedValue & 0xFF;
    data[1] = (speedValue >> 8) & 0xFF;

    // buffer[2-3]: Pulse count since last message (16-bit) - for odometry
    static unsigned long lastSentPulses = 0;
    uint16_t pulseDelta = (uint16_t)(totalPulses - lastSentPulses);
    data[2] = pulseDelta & 0xFF;
    data[3] = (pulseDelta >> 8) & 0xFF;
    lastSentPulses = totalPulses;

    // buffer[4-7]: Reserved for future use
    for (int i = 4; i < 8; i++) {
        data[i] = 0;
    }

    // Send the packet
    CAN.sendMsgBuf(id, 0, 8, data);

    Serial.print("Speed sent: ");
    Serial.print(currentSpeed_mps * 3.6);
    Serial.print(" km/h | Pulses: +");
    Serial.println(pulseDelta);
}

void initSRF08() {
    Wire.beginTransmission(SRF08_ADDRESS);
    Wire.write(0x02);
    Wire.write(0xFF);
    Wire.endTransmission();
    delay(100);

    Wire.beginTransmission(SRF08_ADDRESS);
    Wire.write(0x01);
    Wire.write(0x1F);
    Wire.endTransmission();
    delay(100);
}

void updateSRF08Sensor() {
    Wire.beginTransmission(SRF08_ADDRESS);
    Wire.write(0x00);
    Wire.write(0x51);
    Wire.endTransmission();
    delay(70);

    Wire.beginTransmission(SRF08_ADDRESS);
    Wire.write(0x02);
    Wire.endTransmission();

    Wire.requestFrom(SRF08_ADDRESS, 2);
    if (Wire.available() >= 2) {
        uint8_t highByte = Wire.read();
        uint8_t lowByte = Wire.read();
        distance = (highByte << 8) | lowByte;
    }
}

void sendDistanceData() {
    long id = 0x101;
    byte data[8];

    // buffer[0-1]: Distance value (16-bit, little endian) - cm
    data[0] = distance & 0xFF;
    data[1] = (distance >> 8) & 0xFF;

    // buffer[2-7]: Reserved for future use
    for (int i = 2; i < 8; i++) {
        data[i] = 0;
    }

    CAN.sendMsgBuf(id, 0, 8, data);

    Serial.print("Distance sent: ");
    Serial.print(distance);
    Serial.println(" cm");
}
