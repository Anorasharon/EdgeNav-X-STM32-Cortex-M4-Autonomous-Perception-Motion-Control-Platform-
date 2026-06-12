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

# 🎯 What I Built

In this project, I designed and developed an autonomous mobile robot capable of perceiving its environment and making navigation decisions without external processing.

The platform combines obstacle detection, RGB color sensing, motor control, and autonomous navigation into a single embedded system running entirely on the STM32F446RE ARM Cortex-M4 microcontroller.

Key implementations include:

- IR-based obstacle detection
- TCS34725 RGB color sensing
- Color-based navigation logic
- Differential drive motor control
- Active braking mechanism
- Multi-sample voting algorithm
- UART debugging interface
- Independent motor calibration using dual hardware timers

---

# 🛠 Hardware Used

- STM32F446RE Nucleo Board
- TCS34725 RGB Color Sensor
- IR Obstacle Detection Sensor
- L298N Dual H-Bridge Motor Driver
- TT Geared DC Motors
- Buck Converter
- Lithium Battery Pack
- Toggle Switch

---

# 💻 Software Stack

- Embedded C
- STM32CubeIDE
- STM32CubeMX
- STM32 HAL
- Git
- GitHub

---

# 🧠 What I Learned

Through the development of this project, I gained practical experience in:

- STM32 peripheral configuration
- GPIO programming
- UART communication
- I2C sensor interfacing
- PWM generation and motor control
- Differential drive robotics
- Embedded debugging and testing
- Hardware-software integration
- Git and GitHub workflow
- Technical documentation

---

# 🚧 Engineering Challenges & Solutions

### Motor Drift

The TT motors exhibited different rotational characteristics, causing the robot to drift during straight-line movement.

**Solution:** I utilized TIM2 and TIM3 independently to calibrate the left and right motor speeds.

### Color Detection Instability

Ambient lighting conditions affected RGB sensor readings.

**Solution:** I implemented RGB normalization and a five-sample voting algorithm to improve classification reliability.

### I2C Bus Lockups

The color sensor occasionally became unresponsive due to electrical noise.

**Solution:** I developed an I2C recovery sequence that resets the bus during startup.

### Stopping Accuracy

The robot continued moving after power removal due to inertia.

**Solution:** I implemented an active braking mechanism using reverse motor torque before disabling the motors.

---

# 🏆 Results

- Successfully implemented autonomous navigation
- Reliable obstacle detection
- Stable RGB color recognition
- Accurate left, right, and U-turn execution
- Improved straight-line motion through dual-timer calibration
- Real-time decision making on STM32 Cortex-M4

---

# 📑 Documentation

The repository includes:

- STM32CubeIDE Project
- Embedded C Source Code
- Hardware Images
- Project Report
- System Documentation

---

# 👨‍💻 Author

### Anora Sharon Tessie

Embedded Systems • Robotics • IoT • Edge AI

---

# 📜 License

This project is intended for educational, research, and embedded systems learning purposes.

---
