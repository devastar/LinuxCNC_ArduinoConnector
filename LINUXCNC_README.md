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

### **Feed and Spindle Override with HALUI**
LinuxCNC does **not** provide `motion.feed-override` or `motion.spindle-override` pins. Instead, use the HALUI pins:
- `halui.feed-override.value` (float, in)
- `halui.spindle.0.override.value` (float, in)

These expect a value in the range 0.5–1.2 (feed override, 50%–120%) and 0.5–1.0 (spindle override, 50%–100%) by default. You must scale the Arduino analog input (0–1023) to this range.

### **Sample HAL Snippet**
```hal
# Start the Arduino connector
loadusr arduino-connector

# Scale Arduino analog input (0–1023) to feed override (0.5–1.2)
loadrt scale count=2
addf scale.0 servo-thread
addf scale.1 servo-thread
setp scale.0.gain [expr (1.2-0.5)/1023.0]   # ~0.000684
setp scale.0.offset 0.5
setp scale.1.gain [expr (1.0-0.5)/1023.0]   # ~0.000489
setp scale.1.offset 0.5

# Enable direct value mode for HALUI overrides
setp halui.feed-override.direct-value true
setp halui.spindle.0.override.direct-value true

# Connect Arduino analog inputs to scale components
net feed-raw      arduino.ain.0 => scale.0.in
net spindle-raw   arduino.ain.1 => scale.1.in

# Connect scaled outputs to HALUI override pins
net feed-override     scale.0.out => halui.feed-override.value
net spindle-override  scale.1.out => halui.spindle.0.override.value

# Connect power switch and status LED as before
net power-switch      arduino.din.2 => iocontrol.0.user-enable-out
net machine-enabled   iocontrol.0.user-enable-out => arduino.dled.0
```

> **Note:** Adjust the scaling as needed for your desired override range.

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