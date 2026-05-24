#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ── WiFi credentials ──────────────────────────────────
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// ── Firebase credentials ──────────────────────────────
#define API_KEY       "AIzaSy..."   // your real AIza... key
#define DATABASE_URL  "https://micro-grid-caf7e-default-rtdb.europe-west1.firebasedatabase.app/"

// ── Firebase objects ──────────────────────────────────
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;
unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 2000; // send every 2 seconds

// ── Simulation state ──────────────────────────────────
float simTime = 0.0;

// ── Simulate sensor readings ──────────────────────────
float simPVVoltage() {
  return 15.0 + 3.0 * sin(simTime / 20.0) + random(-10, 10) * 0.04;
}

float simPVCurrent() {
  return 3.2 + 0.8 * sin(simTime / 25.0 + 1.0) + random(-10, 10) * 0.015;
}

float simSOC() {
  float soc = 55.0 + 20.0 * sin(simTime / 120.0) + random(-10, 10) * 0.2;
  return constrain(soc, 0.0, 100.0);
}

float simACVoltage() {
  return 220.0 + 5.0 * sin(simTime / 15.0 + 0.5) + random(-10, 10) * 0.15;
}

float simACCurrent() {
  return 4.5 + 1.2 * sin(simTime / 18.0 + 2.0) + random(-10, 10) * 0.02;
}

// ── Setup ─────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

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

  Serial.println("Setup complete — starting simulation loop");
}

// ── Loop ──────────────────────────────────────────────
void loop() {
  if (!Firebase.ready() || !signupOK) return;

  unsigned long now = millis();
  if (now - lastSendTime < SEND_INTERVAL) return;
  lastSendTime = now;

  simTime += (SEND_INTERVAL / 1000.0); // advance sim clock

  // Read simulated values
  float pvV  = simPVVoltage();
  float pvI  = simPVCurrent();
  float soc  = simSOC();
  float acV  = simACVoltage();
  float acI  = simACCurrent();
  float power = pvV * pvI;

  // ── Push to Firebase ──
  bool ok = true;

  ok &= Firebase.RTDB.setFloat(&fbdo, "/solar/voltage", pvV);
  ok &= Firebase.RTDB.setFloat(&fbdo, "/solar/current", pvI);
  ok &= Firebase.RTDB.setFloat(&fbdo, "/solar/soc",     soc);
  ok &= Firebase.RTDB.setFloat(&fbdo, "/ac/voltage",    acV);
  ok &= Firebase.RTDB.setFloat(&fbdo, "/ac/current",    acI);

  // Serial monitor feedback
  if (ok) {
    Serial.printf(
      "[OK] PV: %.2fV / %.2fA / %.1fW | SOC: %.1f%% | AC: %.1fV / %.2fA\n",
      pvV, pvI, power, soc, acV, acI
    );
  } else {
    Serial.printf("[ERR] Firebase write failed: %s\n", fbdo.errorReason().c_str());
  }
}