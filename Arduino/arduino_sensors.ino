/*
 * Enhanced Arduino Sensors with Pulse-Based Odometry
 * Uses CAN_BUS_SHIELD library and working speed calculation
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

// Collision detection parameters
const unsigned int MIN_SAFE_DISTANCE_CM = 50;
const unsigned int MAX_DETECTION_RANGE_CM = 400;
const unsigned int WARNING_RISK_THRESHOLD = 50;
const unsigned int EMERGENCY_RISK_THRESHOLD = 80;
const float SAFE_DISTANCE_MULTIPLIER = 2.0;

// Interrupt to count pulses
void pulseISR() {
    pulseCount++;
    totalPulses++;
}

void setup() {
    Serial.begin(9600);
    Serial.println("Initializing enhanced Arduino sensors...");

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

    Serial.println("Enhanced sensors initialized!");
    Serial.println("CAN IDs: Speed=0x100, SRF08=0x101");
}

void loop() {
    unsigned long currentTime = millis();

    // Update speed sensor
    if (currentTime - lastMeasurementTime >= measurementInterval) {
        updateSpeedSensor();
        sendEnhancedSpeedData();
        lastMeasurementTime = currentTime;
    }

    // Update SRF08 sensor
    if (currentTime - lastSRF08Update >= SRF08_UPDATE_INTERVAL) {
        updateSRF08Sensor();
        sendSRF08Data();
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
    Serial.print(currentSpeed_mps, 2); // 2 decimal places
    Serial.print(" m/s (");
    Serial.print(currentSpeed_mps * 3.6, 2); // Convert to km/h
    Serial.println(" km/h)");
}

void sendEnhancedSpeedData() {
    // Enhanced CAN message for Speed sensor (CAN ID 0x100)
    long id = 0x100;
    byte data[8];

    // Convert speed to km/h * 10 for precision
    uint16_t speedValue = (uint16_t)(currentSpeed_mps * 3.6 * 10);

    // buffer[0-1]: Speed value (16-bit, little endian) - km/h * 10
    data[0] = speedValue & 0xFF;
    data[1] = (speedValue >> 8) & 0xFF;

    // buffer[2-3]: Pulse count since last message (16-bit) - for precise odometry
    static unsigned long lastSentPulses = 0;
    uint16_t pulseDelta = (uint16_t)(totalPulses - lastSentPulses);
    data[2] = pulseDelta & 0xFF;
    data[3] = (pulseDelta >> 8) & 0xFF;
    lastSentPulses = totalPulses;

    // buffer[4-5]: Total pulses (low 16 bits) - for absolute odometry reference
    data[4] = totalPulses & 0xFF;
    data[5] = (totalPulses >> 8) & 0xFF;

    // buffer[6-7]: Reserved / wheel info
    data[6] = pulsesPerRevolution; // Pulses per revolution
    data[7] = (uint8_t)(wheelDiameter_mm); // Wheel diameter (mm)

    // Send the packet
    CAN.sendMsgBuf(id, 0, 8, data);

    Serial.print("Enhanced Speed sent: ");
    Serial.print(currentSpeed_mps * 3.6);
    Serial.print(" km/h | Pulses: +");
    Serial.print(pulseDelta);
    Serial.print(" (Total: ");
    Serial.print(totalPulses);
    Serial.println(")");
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
        if (distance > MAX_DETECTION_RANGE_CM) {
            distance = MAX_DETECTION_RANGE_CM;
        }
    }
}

void sendSRF08Data() {
    uint8_t riskLevel = calculateCollisionRisk();
    uint8_t alertFlag = calculateAlertFlag(riskLevel);

    long id = 0x101;
    byte data[8];

    data[0] = distance & 0xFF;
    data[1] = (distance >> 8) & 0xFF;
    data[2] = riskLevel;
    data[3] = alertFlag;

    uint16_t speedValue = (uint16_t)(currentSpeed_mps * 3.6 * 10);
    data[4] = speedValue & 0xFF;
    data[5] = (speedValue >> 8) & 0xFF;

    data[6] = 0;
    data[7] = 0;

    CAN.sendMsgBuf(id, 0, 8, data);

    Serial.print("SRF08 sent - Distance: ");
    Serial.print(distance);
    Serial.print("cm, Risk: ");
    Serial.print(riskLevel);
    Serial.print("%, Alert: ");
    Serial.println(alertFlag);
}

uint8_t calculateCollisionRisk() {
    if (distance >= MAX_DETECTION_RANGE_CM) return 0;

    float riskFactor = 0.0;
    if (distance < MIN_SAFE_DISTANCE_CM) {
        riskFactor = 100.0;
    } else if (currentSpeed_mps > 0) {
        float safeDistance = currentSpeed_mps * 3.6 * SAFE_DISTANCE_MULTIPLIER;
        if (distance < safeDistance) {
            float distanceRatio = (safeDistance - distance) / safeDistance;
            riskFactor = min(100.0, distanceRatio * distanceRatio * 100.0);
        }
    } else {
        if (distance < MIN_SAFE_DISTANCE_CM * 2) {
            riskFactor = 100.0 * (MIN_SAFE_DISTANCE_CM * 2 - distance) / (MIN_SAFE_DISTANCE_CM * 2);
        }
    }
    return (uint8_t)riskFactor;
}

uint8_t calculateAlertFlag(uint8_t riskLevel) {
    if (distance < MIN_SAFE_DISTANCE_CM || riskLevel >= EMERGENCY_RISK_THRESHOLD) {
        return 2;
    } else if (riskLevel >= WARNING_RISK_THRESHOLD) {
        return 1;
    } else {
        return 0;
    }
}
