<div align="center">

# 🚀 STM32 Cortex-M4 Autonomous Perception & Motion Control Platform

### 🎯 Real-Time Color Perception • 🤖 Autonomous Navigation • ⚙️ Deterministic Motion Control

<img src="https://img.shields.io/badge/STM32F446RE-Microcontroller-03234B?style=for-the-badge&logo=stmicroelectronics&logoColor=white">
<img src="https://img.shields.io/badge/ARM-Cortex--M4-FF6B35?style=for-the-badge&logo=arm&logoColor=white">
<img src="https://img.shields.io/badge/Language-Embedded_C-00599C?style=for-the-badge&logo=c&logoColor=white">
<img src="https://img.shields.io/badge/IDE-STM32CubeIDE-7B1FA2?style=for-the-badge">
<img src="https://img.shields.io/badge/Framework-STM32_HAL-009688?style=for-the-badge">
<img src="https://img.shields.io/badge/Communication-I2C%20%7C%20UART%20%7C%20PWM-FF5722?style=for-the-badge">
<img src="https://img.shields.io/badge/Sensor-TCS34725_RGB-E91E63?style=for-the-badge">
<img src="https://img.shields.io/badge/Motor_Control-Dual_Timer_PWM-4CAF50?style=for-the-badge">
<img src="https://img.shields.io/badge/Navigation-Autonomous-FFC107?style=for-the-badge&logoColor=black">

### 🏆 Embedded Robotics • 🎨 Color Perception • ⚡ Real-Time Processing • 🤖 Autonomous Intelligence

</div>

---

# 📖 Overview

This project presents an **Autonomous Mobile Robotics Platform** developed on the **STM32F446RE ARM Cortex-M4 Microcontroller**. The system integrates **real-time obstacle detection**, **RGB color perception**, **deterministic motion control**, and **autonomous decision-making** to enable intelligent navigation without external computation.

The robot continuously traverses its environment while monitoring for obstacles using an IR sensor. Upon detection, the platform immediately applies active braking, stabilizes itself, captures multiple RGB measurements through a TCS34725 color sensor, classifies the detected color, and executes a corresponding navigation action.

The entire perception, processing, and control pipeline runs locally on the STM32 microcontroller.

---

# 🎯 Project Objectives

✅ Develop an autonomous mobile robot using STM32 Cortex-M4

✅ Implement reliable obstacle detection

✅ Integrate RGB color perception using I2C communication

✅ Execute color-based navigation decisions

✅ Achieve deterministic motor control using hardware PWM

✅ Demonstrate embedded real-time autonomous behavior

---

# ✨ Key Features

🚀 Autonomous Navigation

🎨 RGB Color Perception

🛑 Real-Time Obstacle Detection

⚡ Hardware PWM Motor Control

🔄 Differential Drive Architecture

🧠 Autonomous Decision Engine

📊 Multi-Sample Voting Algorithm

🌙 Ambient Light Compensation

📡 UART Runtime Diagnostics

🔌 I2C Sensor Communication

🛡️ Active Braking Mechanism

⚙️ Deterministic Motion Control

---

# 🛠 Hardware Stack

## 🧠 STM32F446RE Nucleo Board

The STM32F446RE serves as the central processing unit of the platform.

### Specifications

- ARM Cortex-M4 Core
- 32-bit Architecture
- Multiple Hardware Timers
- I2C Peripheral Interface
- UART Communication
- GPIO Control
- Real-Time Processing Capability

---

## ⚙️ L298N Dual H-Bridge Motor Driver

The L298N acts as the power interface between the STM32 and the drive motors.

### Functions

- Bidirectional Motor Control
- PWM Speed Regulation
- Motor Direction Switching
- Power Amplification

---

## 🛞 TT Geared DC Motors

Two TT geared motors provide locomotion through differential drive control.

### Functions

- Forward Motion
- Turning Control
- U-Turn Execution
- Autonomous Navigation

---

## 🎨 TCS34725 RGB Color Sensor

Used for environmental color perception.

### Measured Channels

- Red
- Green
- Blue
- Clear

---

## 📍 IR Obstacle Detection Sensor

Provides front obstacle detection capability.

### Functions

- Obstacle Monitoring
- Navigation Trigger Generation
- Real-Time Event Detection

---

# 💻 Software Stack

| Category | Technology |
|-----------|------------|
| Programming Language | Embedded C |
| IDE | STM32CubeIDE |
| Configuration Tool | STM32CubeMX |
| Version Control | Git |
| Repository Hosting | GitHub |
| Sensor Interface | I2C |
| Debug Interface | UART |
| Motor Control | PWM |
| Logic Control | GPIO |

---

# 🏗 System Architecture

The platform follows a perception-action workflow.

```text
Move Forward
      ↓
Detect Obstacle
      ↓
Apply Active Brake
      ↓
Stabilize System
      ↓
Acquire RGB Samples
      ↓
Normalize Values
      ↓
Color Classification
      ↓
Navigation Decision
      ↓
Execute Action
      ↓
Resume Navigation
```

---

# ⚙️ Motion Control Architecture

The robot utilizes a differential drive mechanism powered by two TT geared motors.

Independent wheel control enables:

- Forward Movement
- Left Turn
- Right Turn
- U-Turn
- Active Braking
- Motion Correction

This architecture provides a simple yet effective solution for autonomous navigation.

---

# 🔥 PWM Motor Control

Hardware-generated PWM signals regulate motor speed.

## Left Motor

```text
Timer   : TIM3
Channel : CH2
Output  : D9
```

## Right Motor

```text
Timer   : TIM2
Channel : CH3
Output  : D11
```

PWM-based speed control enables smooth motor operation while minimizing power loss.

---

# 🧠 Why Two Separate Hardware Timers?

Instead of generating both PWM signals from a single timer, the system uses:

```text
TIM3 Channel 2 → Left Motor
TIM2 Channel 3 → Right Motor
```

### 🎯 Independent Speed Calibration

Each motor can be tuned individually.

Example:

```text
Left Motor PWM  = 450
Right Motor PWM = 430
```

This compensates for manufacturing differences between motors.

---

### 🚧 Challenge: Robot Drift

During testing, the robot consistently drifted while attempting straight-line motion.

### Root Cause

The TT motors exhibited unequal rotational characteristics.

### Solution

Separate timer resources enabled independent PWM calibration.

Result:

✅ Improved straight-line stability

✅ Better navigation accuracy

---

### 🎯 Enhanced Motion Accuracy

Independent timer channels provide greater flexibility for:

- Turn Calibration
- Braking Optimization
- Motion Correction
- Navigation Tuning

---

# 🛑 Active Braking System

### Problem

Removing motor power alone resulted in excessive stopping distance due to inertia.

### Solution

An active reverse braking mechanism was implemented.

### Procedure

```text
Reverse Motor Direction
        ↓
Apply Reverse Torque
        ↓
Cut Motor Power
```

### Benefits

✅ Reduced Overshoot

✅ Faster Stopping

✅ Improved Positioning

✅ Better Sensor Stability

---

# 🎨 Color Perception Engine

The TCS34725 sensor measures:

```text
Red Intensity
Green Intensity
Blue Intensity
Clear Intensity
```

These values are processed to classify the detected color.

---

# 🌙 Ambient Light Compensation

Raw RGB values vary under different lighting conditions.

To overcome this challenge, color values are normalized using the Clear channel.

### Formula

```text
R_scaled = (Red × 255) / Clear
G_scaled = (Green × 255) / Clear
B_scaled = (Blue × 255) / Clear
```

### Benefits

✅ Improved Classification Accuracy

✅ Robust Low-Light Operation

✅ Reduced Lighting Dependency

---

# 🗳 Multi-Sample Voting Engine

### Problem

Single sensor measurements occasionally produced unstable classifications.

### Solution

Five independent samples are collected.

Each sample votes for:

- Red
- Green
- Blue
- White

The final decision is determined through majority voting.

### Example

```text
Sample 1 → Green
Sample 2 → Green
Sample 3 → Green
Sample 4 → Blue
Sample 5 → Green

Final Result → Green
```

Result:

✅ Improved Reliability

✅ Noise Reduction

✅ Better Classification Confidence

---

# 🚦 Navigation Decision Engine

| Detected Color | Action |
|---------------|---------|
| ⚪ White | U-Turn |
| 🔴 Red | Stop for 5 Seconds |
| 🟢 Green | Turn Right |
| 🔵 Blue | Turn Left |

The robot autonomously translates sensor perception into navigation behavior.

---

# 📡 Communication Interfaces

## I2C1

Used For:

- TCS34725 Color Sensor

Benefits:

✅ Low Pin Count

✅ Reliable Communication

✅ Industry Standard Interface

---

## UART2

Used For:

- Runtime Diagnostics
- Sensor Monitoring
- Debug Logging

Example Output:

```text
Obstacle Detected

Sample 1:
R=220 G=40 B=35

Decision = RED
```

UART logging significantly accelerated development and troubleshooting.

---

# 🌟 Technical Highlights

✅ ARM Cortex-M4 Architecture

✅ Dual Hardware Timer PWM Control

✅ Differential Drive Navigation

✅ Active Braking Mechanism

✅ Multi-Sample Voting Algorithm

✅ Ambient-Light Compensation

✅ I2C Bus Recovery Logic

✅ Real-Time Embedded Processing

✅ Autonomous Decision Making

✅ Deterministic Motion Control

---

# 📑 Documentation

A detailed engineering report is included within this repository.

The report covers:

- Hardware Architecture
- Software Design
- STM32 Peripheral Configuration
- Pin Mapping
- System Flowcharts
- Motor Control Design
- Perception Algorithms
- Engineering Challenges
- Root Cause Analysis
- Implemented Solutions
- Validation Results
- Future Enhancements

📄 Refer to:

```text
Project_Report.pdf
```

for complete technical documentation.

---

# 🏆 Results

The platform successfully demonstrates:

✅ Autonomous Navigation

✅ Real-Time Obstacle Detection

✅ Reliable Color Recognition

✅ Color-Based Route Selection

✅ Stable Motion Control

✅ Embedded Decision-Making

All sensing, processing, classification, and navigation functions execute entirely onboard the STM32 Cortex-M4 microcontroller.

---

# 🚀 Future Enhancements

- FreeRTOS Integration
- Encoder-Based Closed Loop Control
- Bluetooth Connectivity
- Wireless Telemetry
- Camera-Based Vision Processing
- Edge AI Integration
- Object Detection
- SLAM Navigation
- Autonomous Path Planning

---

# 👨‍💻 Author

### Anora Sharon Tessie

Embedded Systems • IoT • Robotics • Edge AI

---

# 📜 License

This project is intended for educational, research, and embedded systems development purposes.
