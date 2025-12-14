# 🌡️ ATtiny85 I2C Multi-Sensor Gateway

## Table of Contents
* [Overview](#overview)
* [Features](#features)
* [Hardware Requirements](#hardware-requirements)
* [Wiring Diagrams](#wiring-diagrams)
* [Software & Setup](#software--setup)
* [How it Works](#how-it-works)
* [Contribution](#contribution)
* [License](#license)

## Overview
This project demonstrates how to use an **ATtiny85** microcontroller as an **I2C Slave** to manage and poll multiple **DS18B20 1-Wire Digital Temperature Sensors**. This offloads the intensive 1-Wire communication from the primary **Arduino Uno** (Master), allowing the master to simply request temperature data via the I2C bus (address `0x42`).

This is ideal for projects requiring many DS18B20 sensors where the main microcontroller is resource-constrained or needs to prioritize other tasks.

## Features
* **Offloads 1-Wire Bus Management:** ATtiny85 handles the timing-critical 1-Wire protocol.
* **I2C Interface:** Simple 2-wire communication with the master (SDA/SCL).
* **Non-Blocking Conversion:** Slave uses a state machine to manage the 800ms DS18B20 conversion time.
* **Error Reporting:** Sends back specific error codes for disconnected sensors (`0x8000`) or ongoing conversions (`0x0000`).
* **Status LED:** Visual feedback on the slave's state (conversion start, data ready, error).

## Hardware Requirements
* 1x Arduino Uno (Master/Programmer)
* 1x ATtiny85 Microcontroller (Slave)
* 5x DS18B20 1-Wire Temperature Sensors (Can be adjusted)
* 1x 4.7 kΩ Pull-up Resistor (For the 1-Wire Bus)
* 2x 4.7 kΩ Pull-up Resistors (For the I2C Bus, optional depending on cable length)
* 1x LED and 220 Ω Resistor (Status Indicator)

## Wiring Diagrams

### 1. ATtiny85 Programming (via Arduino ISP)
Before the final circuit, the ATtiny85 must be programmed. The connection is:

* *Arduino Pin 10* -> ATtiny85 **Reset**
* *Arduino Pin 11* -> ATtiny85 **MOSI** (Pin 5)
* *Arduino Pin 12* -> ATtiny85 **MISO** (Pin 6)
* *Arduino Pin 13* -> ATtiny85 **SCK** (Pin 7)
* *Arduino 5V/GND* -> ATtiny85 **VCC/GND**

### 2. Final I2C Circuit
This diagram shows the final circuit connections for the multi-sensor gateway.

* **1-Wire Bus:** ATtiny85 **PB1** (Pin 6) connected to all DS18B20 Data lines, with a **4.7 kΩ pull-up resistor** to VCC.
* **I2C Bus:**
    * Arduino **A4 (SDA)** -> ATtiny85 **PB0** (Pin 5)
    * Arduino **A5 (SCL)** -> ATtiny85 **PB2** (Pin 7)
    * *Note: I2C Pull-ups are usually on the Arduino or within the ATtiny library but can be added externally.*
* **Status LED:** ATtiny85 **PB3** (Pin 2) -> **220 Ω Resistor** -> LED -> GND.

## Software & Setup

### A. Arduino Master Setup
1.  **Dependencies:** Ensure you have the `Wire.h` library (standard).
2.  **Code:** Upload the contents of `Arduino_Master/Arduino_Master.ino` to your Arduino Uno.
3.  **Monitor:** Open the Serial Monitor at **115200 baud** to view the readings.

### B. ATtiny85 Slave Setup
1.  **Dependencies:**
    * Install the **ATtinyCore** from the Board Manager.
    * Install the **TinyWireS** library (for I2C slave on ATtiny).
    * Install the **OneWire** and **DallasTemperature** libraries.
2.  **Programming:**
    * Connect the Arduino Uno as an **ISP** (In-System Programmer) to the ATtiny85 (See Diagram 1).
    * **Burn Bootloader** (to set fuses for 8MHz internal clock).
    * Select **"ATtiny25/45/85"** as the board and **"8 MHz (internal)"** as the clock.
    * Upload the contents of `ATtiny85_Slave/ATtiny85_Slave.ino` using the "Upload Using Programmer" option.

## How it Works
The master Arduino requests data by calling `Wire.requestFrom(SLAVE_ADDR, NUM_SENS * 2)`.

1.  **If conversion is in progress** on the slave, the slave sends a buffer of `0x0000` (Waiting) and immediately starts a new conversion.
2.  **If data is ready** on the slave, the slave sends the 16-bit temperature data (in hundredths of Celsius, e.g., $25.50^\circ\text{C}$ is $2550$ or `0x09F6`) and sets its state to "conversion needed".
3.  **Error Code:** A return value of `0x8000` indicates a disconnected DS18B20 sensor.
