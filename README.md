# ESP32 Frequency Meter (Interrupts + Web Interface)

This project implements a **frequency measurement system on ESP32** using hardware interrupts and a circular buffer. The measured data is processed in real time and displayed through a **web interface hosted by the ESP32** over its own WiFi network.

---

## 🚀 Features

* Interrupt-based signal measurement (GPIO)
* High-resolution timing using `micros()`
* Circular buffer for data storage
* Real-time calculation of:

  * Minimum period (Tmin)
  * Maximum period (Tmax)
  * Average period (Tavg)
  * Frequency (Hz)
* Embedded web server with auto-refresh UI
* Optional internal test signal generation

---

## 🛠️ Hardware Requirements

* ESP32 development board
* USB cable
* Jumper wire (for test mode)

### Test Setup (No external generator required)

Connect:

```
GPIO 25 → GPIO 18
```

The ESP32 generates a test signal internally and measures it.

---

## ⚙️ Development Environment

* **IDE:** Visual Studio Code
* **Extension:** PlatformIO
* **Framework:** Arduino
* **Board:** ESP32 Dev Module

---

## 📁 Project Structure

```
project/
├── platformio.ini
├── src/
│   └── main.cpp
└── README.md
```

---

## 🌐 How It Works

1. The ESP32 generates or receives a signal on a GPIO pin.
2. Each rising edge triggers an interrupt.
3. The time between interrupts is measured using `micros()`.
4. Values are stored in a circular buffer.
5. Statistical values are computed:

   * Tmin, Tmax, Tavg
   * Frequency
6. A web server displays the results in real time.

---

## 📡 WiFi Access

The ESP32 creates its own WiFi network:

* **SSID:** ESP32-FrequencyMeter
* **Password:** 12345678

Open your browser and go to:

```
http://192.168.4.1
```

---

## 🖥️ Web Interface

The web page displays:

* Tmin (µs)
* Tavg (µs)
* Tmax (µs)
* Frequency (Hz)
* Number of samples

The data updates automatically every few seconds.

---

## 🧠 Key Concepts

* Hardware interrupts (GPIO)
* Interrupt Service Routine (ISR)
* Circular buffer implementation
* Real-time signal processing
* Embedded web server on ESP32
* WiFi Access Point (AP mode)

---

## 📌 Notes

* Works with both internal test signal and external signal source
* Frequency accuracy depends on signal stability
* `micros()` provides microsecond resolution

---

## 🎯 Purpose

Educational project to understand **interrupt-driven measurement systems** and how to integrate **real-time data visualization using a web interface on ESP32**.

---

## 👤 Author

**Miguel Massó**

---

## 📄 License

This project is for educational purposes.
