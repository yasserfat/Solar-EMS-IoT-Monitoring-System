#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ── WiFi credentials ──────────────────────────────────
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// ── Firebase credentials ──────────────────────────────
#define API_KEY       "AIzaSyDew2TFnUlK2hwMh80_Nnz7DmkCS30rXGE"   // your real AIza... key
#define DATABASE_URL  "ttps://micro-grid-caf7e-default-rtdb.europe-west1.firebasedatabase.app/"

// ── Battery voltage divider ───────────────────────────
// GPIO34 is ADC-only (safe for input). Adjust R1/R2 to match
// your resistor values. Max measurable = 3.3 * (R1+R2)/R2
// Example: R1=30kΩ, R2=10kΩ → max ≈ 13.2 V (good for 12 V systems)
#define BAT_ADC_PIN      34
#define R1               30000.0f   // upper resistor (Ω)
#define R2               10000.0f   // lower resistor (Ω)
#define ADC_REF          3.3f       // ESP32 ADC reference voltage
#define ADC_RESOLUTION   4095.0f    // 12-bit ADC

// ── Firebase objects ──────────────────────────────────
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;
unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 2000; // send every 2 seconds

// ── Read battery voltage via voltage divider ──────────
float readBatteryVoltage() {
  int raw = analogRead(BAT_ADC_PIN);
  float vAdc = (raw / ADC_RESOLUTION) * ADC_REF;
  return vAdc * (R1 + R2) / R2;
}

// ── Setup ─────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  analogReadResolution(12); // ensure 12-bit ADC resolution

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());

  // Configure Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  // Anonymous sign-in (matches open DB rules)
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase signup OK");
    signupOK = true;
  } else {
    Serial.printf("Firebase signup error: %s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Setup complete — reading battery voltage");
}

// ── Loop ──────────────────────────────────────────────
void loop() {
  if (!Firebase.ready() || !signupOK) return;

  unsigned long now = millis();
  if (now - lastSendTime < SEND_INTERVAL) return;
  lastSendTime = now;

  float batV = readBatteryVoltage();

  if (Firebase.RTDB.setFloat(&fbdo, "/battery/voltage", batV)) {
    Serial.printf("[OK] Battery: %.2f V\n", batV);
  } else {
    Serial.printf("[ERR] Firebase write failed: %s\n", fbdo.errorReason().c_str());
  }
}
