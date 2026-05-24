# Solar EMS — IoT Monitoring System

A real-time solar energy monitoring system built with an **ESP32**, **Firebase Realtime Database**, and a **vanilla HTML/JS dashboard**. The system tracks PV panel voltage & current, calculates power, monitors battery state of charge (SOC), and displays AC bus readings — all live in the browser.

---

## Project Structure

```
solar-ems/
├── dashboard/
│   └── index.html          # Web dashboard (single-file app)
├── esp32/
│   └── solar_monitor.ino   # ESP32 Arduino sketch
└── README.md
```

---

## System Architecture

```
[PV Panel]
    │
    ▼
[ESP32]  ──────────────────────────────────►  [Firebase RTDB]
  │  Sensors:                                      │
  │  · Voltage sensor (PV)                         │  Realtime listeners
  │  · Hall effect current sensor (PV)             │  (on 'value' event)
  │  · SOC from battery BMS                        ▼
  │  · AC voltage sensor                    [Web Dashboard]
  │  · AC current sensor                     index.html
  │
  └── Pushes every 2 seconds via WiFi
```

---

## Firebase Database Structure

```
micro-grid-caf7e (root)
│
├── solar/
│   ├── voltage       →  18.4        (float, V)
│   ├── current       →  2.35        (float, A)
│   └── soc           →  72.0        (float, %)
│
└── ac/
    ├── voltage       →  221.3       (float, V)
    └── current       →  4.8         (float, A)
```

Power is **not stored** in Firebase — it is calculated client-side using:

```
P = I × V
```

---

## ESP32 Firmware

### Hardware Requirements

| Component | Purpose |
|---|---|
| ESP32 DevKit | Main microcontroller |
| Voltage divider circuit | PV panel voltage sensing |
| Hall effect sensor (e.g. ACS712) | PV panel current sensing |
| Battery BMS with UART/analog output | SOC reading |
| AC voltage sensor (e.g. ZMPT101B) | AC bus voltage |
| AC current sensor (e.g. SCT-013) | AC bus current |

### Required Libraries

Install via **Arduino Library Manager**:

- `Firebase ESP Client` by Mobizt
- `WiFi` (built-in with ESP32 board package)

### Configuration

Open `esp32/solar_monitor.ino` and fill in:

```cpp
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define API_KEY       "AIzaSy..."        // Firebase Web API key
#define DATABASE_URL  "https://your-project-default-rtdb.region.firebasedatabase.app/"
```

### Simulation Mode

The sketch ships in **simulation mode** — it generates realistic sinusoidal sensor readings without any physical hardware. This lets you verify the full Firebase → Dashboard pipeline before connecting real sensors.

To switch to real sensors, replace the `simXxx()` function calls in `loop()` with your actual `analogRead()` / sensor library calls.

### Upload Steps

1. Install the [ESP32 board package](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html) in Arduino IDE
2. Select board: `ESP32 Dev Module`
3. Select the correct COM port
4. Flash the sketch
5. Open **Serial Monitor** at `115200 baud`

Expected output once connected:

```
Connecting to WiFi......
Connected! IP: 192.168.1.45
Firebase signup OK
Setup complete — starting simulation loop
[OK] PV: 16.24V / 3.41A / 55.4W | SOC: 63.2% | AC: 222.1V / 4.87A
[OK] PV: 15.98V / 3.38A / 54.0W | SOC: 63.4% | AC: 221.8V / 5.01A
```

---

## Web Dashboard

### Features

| Section | What it shows |
|---|---|
| PV Panel | Voltage card, current card, efficiency %, real-time voltage graph, real-time current graph |
| Power | Calculated `P = I × V` with live formula display and max-power fill bar |
| Battery SOC | Percentage display, color-coded status, animated progress bar and battery icon |
| AC Bus | Voltage card, current card, real-time voltage graph, real-time current graph |

### Firebase Integration

The dashboard uses the **Firebase Compat SDK** loaded via CDN — no build tools or npm required.

Open `dashboard/index.html` and update the config block:

```js
const FIREBASE_CONFIG = {
  apiKey:      "AIzaSy...",
  authDomain:  "your-project.firebaseapp.com",
  databaseURL: "https://your-project-default-rtdb.region.firebasedatabase.app/",
  projectId:   "your-project-id",
};
```

The two Firebase listeners used:

```js
db.ref('solar').on('value', snap => { /* updates PV cards, power, SOC, charts */ });
db.ref('ac').on('value',    snap => { /* updates AC cards and charts */ });
```

### Running Locally

No server needed. Just open `index.html` directly in a browser:

```bash
open dashboard/index.html
# or on Windows:
start dashboard/index.html
```

For live reload during development:

```bash
npx live-server dashboard/
```

---

## Firebase Setup

### 1. Create a project

Go to [console.firebase.google.com](https://console.firebase.google.com) → **Add project**

### 2. Enable Realtime Database

Firebase Console → Build → **Realtime Database** → Create database → choose your region

### 3. Get your API key

Project Settings (gear icon) → General → Your apps → **apiKey** (starts with `AIzaSy...`)

> ⚠️ The **App ID** (`1:295364...:web:...`) is NOT the API key. Make sure you use the `apiKey` field.

### 4. Set database rules (development)

Firebase Console → Realtime Database → **Rules** tab:

```json
{
  "rules": {
    ".read": true,
    ".write": true
  }
}
```

> ⚠️ These rules are open — suitable for development only. Lock them down before any public deployment.

### 5. Manually test the structure

In Firebase Console → Realtime Database → click **+** and add:

```
solar
  voltage   18.4
  current   2.35
  soc       72
ac
  voltage   221.3
  current   4.8
```

If the dashboard charts update immediately, the connection is working.

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| `Uncaught SyntaxError: Cannot use import` | Using ES module `import` in a regular script tag | Use the compat SDK with `<script src="...firebase-app-compat.js">` |
| `PERMISSION_DENIED` in console | Database rules block reads | Set rules to `".read": true` temporarily |
| Charts invisible, cards show values | Only one data point in DB | ESP32 not yet pushing; or seed the chart with first value twice |
| `Firebase signup error` in Serial Monitor | Wrong API key | Use the `AIzaSy...` key, not the App ID |
| ESP32 stuck on `Connecting to WiFi...` | Wrong credentials or 5GHz network | ESP32 only supports 2.4GHz WiFi |
| Dashboard shows `—` everywhere | Firebase config not updated in HTML | Replace placeholder values in `FIREBASE_CONFIG` |

---

## Future Improvements

- Add authentication (Firebase Auth) before going public
- Store historical data using `Firebase.pushFloat()` for trend analysis
- Add alerting (email / push notification) when SOC drops below 20%
- Add a time-series chart showing daily energy yield (Wh)
- Replace simulation with real sensor calibration routines
- Deploy dashboard to Firebase Hosting for remote access

---

## License

MIT — free to use and modify for personal and commercial projects.