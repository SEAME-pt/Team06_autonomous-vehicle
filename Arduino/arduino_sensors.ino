/*
 * Simplified Arduino Sensors with Speed and Distance
 * Uses CAN_BUS_SHIELD library for communication
 */

 #include <SPI.h>
 #include "mcp2515_can.h"
 #include <Wire.h>

 // CAN-Bus Shield definitions
 #define CAN_2515
 const int SPI_CS_PIN = 9;
 const int CAN_INT_PIN = 2;
 mcp2515_can CAN(SPI_CS_PIN);

 // Speed sensor definitions
 const int ENCODER_PIN = 3;
 const unsigned long measurementInterval = 50; // Measurement interval in milliseconds (0.5s)

 // Speed measurement variables (based on working code)
 volatile unsigned int interruptCount = 0; // Counter for interrupts
 unsigned long lastMeasurementTime = 0; // Time of last measurement
 const unsigned long debounceDelay = 0; // Debounce time in milliseconds
 volatile unsigned long lastInterruptTime = 0; // Time of last interrupt
 volatile unsigned long totalPulses = 0; // Total pulses since startup

 // SRF08 variables
 #define SRF08_ADDRESS 0x70
 unsigned int distance = 100; // Initialize with reasonable default (100cm)
 unsigned long lastSRF08Update = 0;
 const unsigned long SRF08_UPDATE_INTERVAL = 50;

 // SRF08 non-blocking state machine
 enum SRF08_State {
     SRF08_IDLE,
     SRF08_RANGING,
     SRF08_READING
 };
 SRF08_State srf08State = SRF08_IDLE;
 unsigned long srf08CommandTime = 0;
 const unsigned long SRF08_RANGING_DELAY = 70; // 70ms for ranging to complete

 // Interrupt to count pulses (based on working code)
 void pulseISR() {
     unsigned long currentTime = millis();
     if (currentTime - lastInterruptTime > debounceDelay) { // Ignore interrupts within debounce delay
         interruptCount++; // Increment counter
         totalPulses++; // Increment total counter
         lastInterruptTime = currentTime; // Update last interrupt time
     }
 }

 void setup() {
     Serial.begin(9600);
     Serial.println("Initializing simplified Arduino sensors...");

     // Configure encoder pin as input with internal pull-up
     pinMode(ENCODER_PIN, INPUT_PULLUP);

     // Activate interrupt on encoder pin
     attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), pulseISR, RISING);
     Serial.println("Speed sensor initialized");

     // Initialize CAN-Bus Shield for 16MHz crystal at 500kbps
     Serial.print("Initializing CAN (16MHz, 500kbps)... ");
     while (CAN.begin(CAN_500KBPS, MCP_16MHz) != CAN_OK) {
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

     // Send speed sensor data (pulse counts)
     if (currentTime - lastMeasurementTime >= measurementInterval) {
         sendSpeedData();
         lastMeasurementTime = currentTime;
     }

     // Non-blocking SRF08 sensor processing
     updateSRF08NonBlocking();

     delay(10);
 }

 void sendSpeedData() {
     // Simple CAN message for Speed sensor (CAN ID 0x100)
     long id = 0x100;
     byte data[8];

     // Get pulse count since last message (based on working code)
     noInterrupts(); // Disable interrupts during reading
     unsigned int rawInterruptCount = interruptCount; // Store current count
     interruptCount = 0; // Reset counter for next measurement
     interrupts(); // Re-enable interrupts

     // Assume 2 interrupts per physical event (from working code)
     unsigned int pulsesInInterval = rawInterruptCount / 2; // Estimate physical events

     // FIXED: Don't divide total pulses - send the actual accumulated count
     // This was causing middleware to receive same total repeatedly -> pulse_delta = 0
     unsigned long correctedTotalPulses = totalPulses; // Send actual total, not divided

     // buffer[0-1]: Pulse count in this interval (16-bit, little endian)
     uint16_t pulsesDelta = (uint16_t)pulsesInInterval;
     data[0] = pulsesDelta & 0xFF;
     data[1] = (pulsesDelta >> 8) & 0xFF;

     // buffer[2-5]: Total pulse count since startup (32-bit, little endian)
     data[2] = correctedTotalPulses & 0xFF;
     data[3] = (correctedTotalPulses >> 8) & 0xFF;
     data[4] = (correctedTotalPulses >> 16) & 0xFF;
     data[5] = (correctedTotalPulses >> 24) & 0xFF;

     // buffer[6-7]: Reserved for future use
     data[6] = 0;
     data[7] = 0;

     // Send the packet
     CAN.sendMsgBuf(id, 0, 8, data);

     Serial.print("Pulses sent: +");
     Serial.print(pulsesDelta);
     Serial.print(" | Total: ");
     Serial.println(totalPulses);
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

 void updateSRF08NonBlocking() {
     unsigned long currentTime = millis();

     switch (srf08State) {
         case SRF08_IDLE:
             // Check if it's time to start a new measurement
             if (currentTime - lastSRF08Update >= SRF08_UPDATE_INTERVAL) {
                 // Start ranging command (0x51 = range in cm)
                 Wire.beginTransmission(SRF08_ADDRESS);
                 Wire.write(0x00);  // Command register
                 Wire.write(0x51);  // Range in cm
                 if (Wire.endTransmission() == 0) {
                     srf08State = SRF08_RANGING;
                     srf08CommandTime = currentTime;
                 } else {
                     Serial.println("SRF08 command failed");
                     lastSRF08Update = currentTime; // Try again next interval
                 }
             }
             break;

         case SRF08_RANGING:
             // Wait for ranging to complete (70ms)
             if (currentTime - srf08CommandTime >= SRF08_RANGING_DELAY) {
                 srf08State = SRF08_READING;
             }
             break;

         case SRF08_READING:
             // Read the result
             Wire.beginTransmission(SRF08_ADDRESS);
             Wire.write(0x02);  // Range high byte register
             if (Wire.endTransmission() == 0) {
                 Wire.requestFrom(SRF08_ADDRESS, 2);
                 if (Wire.available() >= 2) {
                     uint8_t highByte = Wire.read();
                     uint8_t lowByte = Wire.read();
                     unsigned int newDistance = (highByte << 8) | lowByte;

                     // Only update if we get a reasonable reading (not 0 and not max range)
                     if (newDistance > 0 && newDistance < 6000) {  // SRF08 max range is ~6m
                         distance = newDistance;
                         sendDistanceData();
                     } else {
                         Serial.print("SRF08 invalid reading: ");
                         Serial.println(newDistance);
                     }
                 } else {
                     Serial.println("SRF08 no data available");
                 }
             } else {
                 Serial.println("SRF08 read request failed");
             }

             // Reset state and update timer
             srf08State = SRF08_IDLE;
             lastSRF08Update = currentTime;
             break;
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
