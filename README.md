# Edge-AI-Based-Real-Time-Ambient-Quality-Classification

# Project Description

This project implements a distributed environmental monitoring system using two STM32 Discovery boards. The STM32F0 board acts as a sensor node, collecting temperature, humidity, and gas data via BME280 and MQ135 sensors and transmitting them to the STM32F4 processing unit using SPI with DMA for high-efficiency data transfer. The STM32F4 unit, running on FreeRTOS, processes these inputs through an Edge Impulse-trained machine learning model to classify ambient air quality into three categories: normal, dangerous, and abnormal. The final classification results and system status are continuously monitored in real-time via Serial Wire Output (SWO).

# Software Architecture

This project implements an Edge AI-powered environmental monitoring system using two STM32 Discovery boards. Sensor data is acquired by the STM32F0 and transmitted to the STM32F4 via SPI-DMA. The STM32F4, running FreeRTOS, executes a machine learning model to classify air quality (Normal/Dangerous/Abnormal) in real-time.

  * __STM32F0 Discovery Board Side__

This project utilizes a custom-written BME280 driver to measure temperature, humidity, and pressure, alongside an MQ135 sensor to monitor ambient gas levels. These sensor readings are transmitted from the STM32F0 Discovery board to the STM32F4 Discovery board via SPI using DMA. By offloading the data transfer process from the CPU, DMA ensures low-latency, high-speed communication and prevents potential bottlenecks in the main execution flow, allowing for more efficient resource management.

  * __STM32F4 Discovery Board Size__

The sensor data transmitted from the STM32F0 is received by the STM32F4 via SPI-DMA. Initially, the system operated without an RTOS, which led to timing conflicts and data corruption, causing the machine learning model to produce unreliable results. To overcome this and ensure a deterministic execution flow, FreeRTOS is integrated. The architecture consists of two main tasks: one dedicated to receiving sensor data and another for executing the Edge Impulse-trained machine learning model. A binary semaphore is implemented to synchronize these tasks, ensuring the model only initiates processing once the sensor data acquisition is successfully completed. This synchronization guarantees that the model always processes valid, consistent data, allowing it to accurately classify the ambient air quality into three distinct categories: 0 (Normal), 1 (Dangerous), and 2 (Abnormal).

Figure 1 : System Overview

<img src="https://github.com/ssenanb/Edge-AI-Based-Real-Time-Ambient-Quality-Classification/blob/main/system_overview" alt="System Overview" width="500"/>
