// ATtiny85: OneWire on PB1, I2C (TinyWireS) on PB0/PB2, LED on PB3
#include <TinyWireS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <avr/pgmspace.h>

#define ONEWIRE_PIN 1
#define LED_PIN 3 // Status LED connected to PB3 (Arduino pin 3)

#define I2C_ADDR 0x42

// Your actual sensor addresses
DeviceAddress sensorsAddr[] = {
    {0x28, 0x6E, 0xAB, 0xA4, 0x00, 0x00, 0x00, 0x2A}, // Sensor B
  {0x28, 0x0D, 0x57, 0x69, 0x00, 0x00, 0x00, 0x93}, // Sensor A
  {0x28, 0xB3, 0x63, 0x68, 0x00, 0x00, 0x00, 0x93}, // Sensor C
  {0x28, 0x4E, 0x46, 0x69, 0x00, 0x00, 0x00, 0x0E}, // Sensor D
  {0x28, 0x0D, 0x53, 0x69, 0x00, 0x00, 0x00, 0x8C}  // Sensor E
};
const uint8_t NUM_SENS = 5;

OneWire oneWire(ONEWIRE_PIN);
DallasTemperature sensors(&oneWire);

uint8_t i2c_buf[16];
volatile uint8_t i2c_buf_len = 0;

// State machine variables
volatile bool conversionStarted = false;
volatile unsigned long conversionStartTime = 0;
volatile bool dataReady = false;

// Simple LED blinking function
void blink_led(uint8_t times, uint16_t delay_ms) {
  for (uint8_t i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(delay_ms);
    digitalWrite(LED_PIN, LOW);
    if (i + 1 < times) {
      delay(delay_ms);
    }
  }
}

int16_t temp_to_centi(float t) {
  if (t == DEVICE_DISCONNECTED_C) return 0x8000;
  int16_t v = (int16_t)round(t * 100.0);
  return v;
}

// Start temperature conversion (non-blocking)
void start_temperature_conversion() {
  blink_led(2, 80); // Two fast blinks to indicate start
  
  sensors.requestTemperatures();
  conversionStarted = true;
  conversionStartTime = millis();
  dataReady = false;
  
  // Pre-fill buffer with error values in case conversion fails
  uint8_t idx = 0;
  for (uint8_t i = 0; i < NUM_SENS; i++) {
    i2c_buf[idx++] = 0x00; // Default to 0 if not ready
    i2c_buf[idx++] = 0x80; // Error indicator
  }
  i2c_buf_len = idx;
}

// Check if conversion is complete and read results
void check_temperature_conversion() {
  if (!conversionStarted) return;
  
  if (millis() - conversionStartTime >= 800) {
    // Conversion complete - read results
    blink_led(1, 100); // Single blink to indicate reading
    
    uint8_t idx = 0;
    for (uint8_t i = 0; i < NUM_SENS; i++) {
      float t = sensors.getTempC(sensorsAddr[i]);
      int16_t c = temp_to_centi(t);
      i2c_buf[idx++] = (uint8_t)(c & 0xFF);
      i2c_buf[idx++] = (uint8_t)((c >> 8) & 0xFF);
    }
    i2c_buf_len = idx;
    
    conversionStarted = false;
    dataReady = true;
    
    blink_led(2, 80); // Two fast blinks to indicate completion
  } else {
    // Still waiting - blink slowly to show working state
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 400) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Toggle LED
      lastBlink = millis();
    }
  }
}

// callback: master requests bytes
void requestEvent() {
  if (!dataReady || i2c_buf_len == 0) {
    // If no data ready, start a new conversion
    start_temperature_conversion();
    
    // Send placeholder data (all zeros)
    for (uint8_t i = 0; i < NUM_SENS * 2; i++) {
      TinyWireS.send(0x00);
    }
  } else {
    // Send actual temperature data
    for (uint8_t i = 0; i < i2c_buf_len; i++) {
      TinyWireS.send(i2c_buf[i]);
    }
    dataReady = false; // Mark data as sent
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Blink 3 times on startup to confirm it's working
  blink_led(3, 300);

  sensors.begin();
  
  // Set resolution to 12 bits for each sensor and verify connection
  for (uint8_t i = 0; i < NUM_SENS; i++) {
    sensors.setResolution(sensorsAddr[i], 12);
    
    // Verify sensor connection
    if (!sensors.isConnected(sensorsAddr[i])) {
      // Blink rapidly to indicate sensor error
      for (int j = 0; j < 10; j++) {
        blink_led(1, 100);
        delay(100);
      }
    }
  }
  
  TinyWireS.begin(I2C_ADDR);
  TinyWireS.onRequest(requestEvent);
  
  // Start first conversion
  start_temperature_conversion();
}

void loop() {
  // Check if temperature conversion is complete
  if (conversionStarted) {
    check_temperature_conversion();
  } else if (dataReady) {
    // Data is ready and waiting - slow blink to indicate ready state
    static unsigned long lastReadyBlink = 0;
    if (millis() - lastReadyBlink > 1000) {
      digitalWrite(LED_PIN, HIGH);
      delay(50);
      digitalWrite(LED_PIN, LOW);
      lastReadyBlink = millis();
    }
  } else {
    // No conversion in progress and no data ready - start one
    start_temperature_conversion();
  }
  
  // Handle I2C communication
  TinyWireS_stop_check();
  
  // Small delay to prevent overwhelming the MCU
  delay(10);
}