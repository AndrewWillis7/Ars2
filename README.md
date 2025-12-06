# Overview

Ars2 is a modular sensor framework designed for real-time robotics and embedded systems.
It provides:

* A hardware-configuration pipeline using JSON auto-generated headers

* A dedicated I2CUtils library for safe multiplexer access and I²C bus control

* A unified SensorBase class supporting blocking and task-driven sampling

* Automatic bus recovery to prevent I²C lockups

* A thread-safe, multi-core concurrency model compatible with FreeRTOS tasks

The framework allows you to build complex sensor systems—color sensors, optical odometry sensors, encoders, and more—while maintaining reliability and clean separation of responsibilities.