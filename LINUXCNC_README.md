# LinuxCNC Integration Guide for Arduino Connector

## Overview
This guide explains how to connect your Arduino Nano (with the provided sketch) to LinuxCNC using the included `arduino-connector.py` script. It covers:
- Feed override potentiometer (A0)
- Spindle override potentiometer (A1)
- Power on switch (D2)
- Status LED (WS2812B on D3)

---

## 1. Prerequisites
- **LinuxCNC** installed and working.
- **Python 3** and the `pyserial` library installed:
  ```bash
  sudo apt-get install python3-serial
  ```
  or
  ```bash
  pip3 install pyserial
  ```
- The `arduino-connector.py` script from this project.

---

## 2. Connect the Arduino
- Upload the provided `.ino` sketch to your Arduino Nano.
- Connect the Arduino to your LinuxCNC PC via USB.
- Find the serial port (e.g., `/dev/ttyUSB0` or `/dev/ttyACM0`):
  ```bash
  dmesg | grep tty
  ```

---

## 3. Configure the Python Script
- Open `arduino-connector.py` and set the correct serial port:
  ```python
  connection = '/dev/ttyACM0'  # or your detected port, e.g. '/dev/ttyUSB0'
  ```
- Set the correct number of inputs and analog inputs for your hardware:
  ```python
  Inputs = 1
  InPinmap = [2]  # D2 for power on switch
  SInputs = 0
  sInPinmap = []
  AInputs = 2
  AInPinmap = [0, 1]  # A0 (feed override), A1 (spindle override)
  DLEDcount = 1
  # Other features (Outputs, PWM, etc.) can be set to 0 if unused
  Outputs = 0
  PwmOutputs = 0
  LPoti = 0
  BinSelKnob = 0
  QuadEncs = 0
  JoySticks = 0
  Keypad = 0
  MultiplexLED = 0
  ```

---

## 4. Update Your LinuxCNC HAL File
- In your machine's `.hal` file, add:
  ```hal
  # Start the Arduino connector
  loadusr -Wn arduino python3 /path/to/arduino-connector.py

  # Map Arduino pins to LinuxCNC signals
  net feed-override     arduino.ain.0 => motion.feed-override
  net spindle-override  arduino.ain.1 => motion.spindle-override
  net power-switch      arduino.din.2 => iocontrol.0.user-enable-out
  net machine-enabled   iocontrol.0.user-enable-out => arduino.dled.0
  ```
  > **Note:** Pin names are as created by the script: `ain.0`, `ain.1`, `din.2`, `dled.0`.

---

## 5. Testing
- Start LinuxCNC.
- Move the potentiometers and toggle the switch; observe the corresponding changes in LinuxCNC.
- The status LED should indicate connection status (see main README for LED behavior).

---

## 6. Troubleshooting
- If pins do not appear, check the Python script output for errors.
- Ensure the serial port is correct and not in use by another process.
- Use `halmeter` or `halscope` to monitor pin values.

---

**For more details, see the original project page:**  
https://theartoftinkering.com/linuxcnc-arduino-connector/ 