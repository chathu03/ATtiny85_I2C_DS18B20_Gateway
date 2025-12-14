#include <Wire.h>

#define SLAVE_ADDR 0x42
#define NUM_SENS 5
#define MAX_RETRIES 3

// Sensor labels corresponding to your IDs
const char* sensorLabels[] = {"A", "B", "C", "D", "E"};

// Error counters
uint32_t successCount = 0;
uint32_t errorCount = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(); // join I2C bus as master
  Wire.setTimeout(1000); // Set I2C timeout to 1 second
  delay(200);
  
  Serial.println("Temperature Monitoring System Started");
  Serial.println("Slave Address: 0x42");
  Serial.println("Waiting for sensor data...");
  Serial.println();
}

bool read_temperature_data(uint8_t* buffer, uint8_t buffer_size) {
  uint8_t retries = 0;
  
  while (retries < MAX_RETRIES) {
    uint8_t received = Wire.requestFrom(SLAVE_ADDR, buffer_size);
    
    if (received == 0) {
      retries++;
      delay(100);
      continue;
    }
    
    if (received != buffer_size) {
      Serial.print("Warning: Expected ");
      Serial.print(buffer_size);
      Serial.print(" bytes, received ");
      Serial.println(received);
    }
    
    uint8_t idx = 0;
    while (Wire.available() && idx < buffer_size) {
      buffer[idx++] = Wire.read();
    }
    
    if (idx > 0) {
      return true; // Successfully read some data
    }
    
    retries++;
    delay(100);
  }
  
  return false; // All retries failed
}

void print_temperature_data(uint8_t* buf) {
  Serial.println("=== Temperature Readings ===");
  Serial.print("Readings: ");
  Serial.print(successCount);
  Serial.print(" | Errors: ");
  Serial.println(errorCount);
  Serial.println("----------------------------");
  
  bool allSensorsValid = true;
  
  for (int i = 0; i < NUM_SENS; i++) {
    int16_t raw = (int16_t)((buf[2 * i]) | (buf[2 * i + 1] << 8));
    
    if (raw == (int16_t)0x8000) {
      Serial.print("Sensor ");
      Serial.print(sensorLabels[i]);
      Serial.println(": ERROR - Disconnected");
      allSensorsValid = false;
    } else if (raw == 0) {
      Serial.print("Sensor ");
      Serial.print(sensorLabels[i]);
      Serial.println(": WAITING - Conversion in progress");
      allSensorsValid = false;
    } else {
      float t = raw / 100.0;
      Serial.print("Sensor ");
      Serial.print(sensorLabels[i]);
      Serial.print(": ");
      Serial.print(t, 2);
      Serial.println(" °C");
    }
  }
  
  if (allSensorsValid) {
    Serial.println("✓ All sensors reporting OK");
  } else {
    Serial.println("⚠ Some sensors have issues");
  }
  Serial.println("============================");
}

void loop() {
  uint8_t buf[NUM_SENS * 2];
  
  // Clear buffer
  memset(buf, 0, sizeof(buf));
  
  if (read_temperature_data(buf, sizeof(buf))) {
    successCount++;
    print_temperature_data(buf);
  } else {
    errorCount++;
    Serial.println("❌ ERROR: No response from slave device");
    Serial.println("Check connections and slave power");
    Serial.println();
  }
   
  delay(2000); // Read every 2 seconds
}