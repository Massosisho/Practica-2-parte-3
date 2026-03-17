#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// =========================
// CONFIGURACION WIFI AP
// =========================
const char* AP_SSID = "ESP32-Massó-Frecuencia";
const char* AP_PASS = "12345678";

// =========================
// PINES
// =========================
const uint8_t INPUT_PIN = 18;      // Pin donde medimos la señal
const uint8_t TEST_OUT_PIN = 25;   // Pin que genera señal de prueba

// =========================
// SENAL DE PRUEBA
// true  -> el ESP32 genera una señal de prueba en GPIO25
// false -> usar una señal externa real en GPIO18
// =========================
bool enableTestSignal = true;
const uint32_t TEST_FREQ_HZ = 10;  // Frecuencia de prueba

// =========================
// COLA CIRCULAR
// =========================
const size_t BUFFER_SIZE = 20;

volatile unsigned long periodsUs[BUFFER_SIZE];
volatile size_t writeIndex = 0;
volatile size_t storedSamples = 0;
volatile unsigned long lastEdgeTimeUs = 0;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// =========================
// SERVIDOR WEB
// =========================
WebServer server(80);

// =========================
// ISR DE ENTRADA
// =========================
void IRAM_ATTR onSignalEdge() {
    unsigned long nowUs = micros();

    portENTER_CRITICAL_ISR(&mux);

    if (lastEdgeTimeUs != 0) {
        unsigned long period = nowUs - lastEdgeTimeUs;
        periodsUs[writeIndex] = period;
        writeIndex = (writeIndex + 1) % BUFFER_SIZE;

        if (storedSamples < BUFFER_SIZE) {
            storedSamples++;
        }
    }

    lastEdgeTimeUs = nowUs;

    portEXIT_CRITICAL_ISR(&mux);
}

// =========================
// ESTRUCTURA DE ESTADISTICAS
// =========================
struct Stats {
    unsigned long tMin;
    unsigned long tMax;
    float tAvg;
    float freqAvg;
    size_t samples;
    bool valid;
};

// =========================
// CALCULO DE ESTADISTICAS
// =========================
Stats calculateStats() {
    Stats s;
    s.tMin = 0;
    s.tMax = 0;
    s.tAvg = 0.0f;
    s.freqAvg = 0.0f;
    s.samples = 0;
    s.valid = false;

    unsigned long localBuffer[BUFFER_SIZE];
    size_t localCount = 0;

    portENTER_CRITICAL(&mux);

    localCount = storedSamples;
    for (size_t i = 0; i < localCount; i++) {
        localBuffer[i] = periodsUs[i];
    }

    portEXIT_CRITICAL(&mux);

    if (localCount == 0) {
        return s;
    }

    unsigned long minVal = ULONG_MAX;
    unsigned long maxVal = 0;
    unsigned long long sum = 0;

    for (size_t i = 0; i < localCount; i++) {
        unsigned long v = localBuffer[i];
        if (v == 0) continue;

        if (v < minVal) minVal = v;
        if (v > maxVal) maxVal = v;
        sum += v;
    }

    float avg = (float)sum / (float)localCount;
    float freq = 0.0f;

    if (avg > 0.0f) {
        freq = 1000000.0f / avg;
    }

    s.tMin = minVal;
    s.tMax = maxVal;
    s.tAvg = avg;
    s.freqAvg = freq;
    s.samples = localCount;
    s.valid = true;

    return s;
}

// =========================
// PAGINA WEB
// =========================
String buildHtml() {
    Stats st = calculateStats();

    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta http-equiv="refresh" content="2">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Frequency Meter</title>
  <style>
    body {
      margin: 0;
      font-family: Arial, sans-serif;
      background: #0f172a;
      color: #e2e8f0;
      text-align: center;
      padding: 30px 15px;
    }
    .container {
      max-width: 800px;
      margin: auto;
    }
    h1 {
      margin-bottom: 10px;
      font-size: 2rem;
    }
    p {
      color: #94a3b8;
    }
    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
      gap: 15px;
      margin-top: 30px;
    }
    .card {
      background: #1e293b;
      border-radius: 18px;
      padding: 20px;
      box-shadow: 0 8px 24px rgba(0,0,0,0.25);
    }
    .label {
      font-size: 0.95rem;
      color: #94a3b8;
      margin-bottom: 8px;
    }
    .value {
      font-size: 1.8rem;
      font-weight: bold;
      color: #38bdf8;
    }
    .footer {
      margin-top: 30px;
      color: #94a3b8;
      font-size: 0.9rem;
    }
    .status {
      margin-top: 20px;
      font-size: 1rem;
      color: #22c55e;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32 Frequency Meter</h1>
    <p>Measurement of time between interrupts using a circular buffer</p>
)rawliteral";

    if (!st.valid) {
        html += R"rawliteral(
    <div class="status">Waiting for interrupts...</div>
)rawliteral";
    } else {
        html += "<div class='grid'>";
        html += "<div class='card'><div class='label'>Tmin</div><div class='value'>" + String(st.tMin) + " us</div></div>";
        html += "<div class='card'><div class='label'>Tavg</div><div class='value'>" + String(st.tAvg, 2) + " us</div></div>";
        html += "<div class='card'><div class='label'>Tmax</div><div class='value'>" + String(st.tMax) + " us</div></div>";
        html += "<div class='card'><div class='label'>Frequency</div><div class='value'>" + String(st.freqAvg, 2) + " Hz</div></div>";
        html += "<div class='card'><div class='label'>Samples</div><div class='value'>" + String(st.samples) + "</div></div>";
        html += "</div>";
    }

    html += "<div class='footer'>AP IP: 192.168.4.1 | Input pin: GPIO " + String(INPUT_PIN) + "</div>";
    html += "</div></body></html>";

    return html;
}

// =========================
// RUTAS WEB
// =========================
void handleRoot() {
    server.send(200, "text/html", buildHtml());
}

// =========================
// SENAL DE PRUEBA CON LEDC
// =========================
void setupTestSignal() {
    const int channel = 0;
    const int resolution = 8;

    ledcSetup(channel, TEST_FREQ_HZ, resolution);
    ledcAttachPin(TEST_OUT_PIN, channel);
    ledcWrite(channel, 128); // 50% duty cycle
}

// =========================
// SETUP
// =========================
void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(INPUT_PIN, INPUT);

    if (enableTestSignal) {
        setupTestSignal();
        Serial.println("Test signal enabled.");
        Serial.println("Connect GPIO 25 to GPIO 18.");
    }

    attachInterrupt(digitalPinToInterrupt(INPUT_PIN), onSignalEdge, RISING);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);

    IPAddress ip = WiFi.softAPIP();

    server.on("/", handleRoot);
    server.begin();

    Serial.println();
    Serial.println("=== ESP32 Frequency Meter ===");
    Serial.print("WiFi AP: ");
    Serial.println(AP_SSID);
    Serial.print("Password: ");
    Serial.println(AP_PASS);
    Serial.print("Open in browser: http://");
    Serial.println(ip);
}

// =========================
// LOOP
// =========================
void loop() {
    server.handleClient();

    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 2000) {
        lastPrint = millis();

        Stats st = calculateStats();

        if (!st.valid) {
            Serial.println("Waiting for interrupts...");
        } else {
            Serial.println("------ MEASUREMENTS ------");
            Serial.print("Tmin: ");
            Serial.print(st.tMin);
            Serial.println(" us");

            Serial.print("Tavg: ");
            Serial.print(st.tAvg, 2);
            Serial.println(" us");

            Serial.print("Tmax: ");
            Serial.print(st.tMax);
            Serial.println(" us");

            Serial.print("Frequency: ");
            Serial.print(st.freqAvg, 2);
            Serial.println(" Hz");

            Serial.print("Samples: ");
            Serial.println(st.samples);
            Serial.println();
        }
    }
}