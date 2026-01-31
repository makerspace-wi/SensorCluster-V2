# SensorCluster V2.0

<img src="https://github.com/user-attachments/assets/65ef6d3d-df8e-42d8-acc4-e89d973fbf49" width="400">

Ein ESP32-S2 basiertes IoT-Gerät mit Radar-Präsenzerkennung, Temperaturmessung und MQTT-Integration.

## 🎯 Features

- **LD2410 Radar-Präsenzerkennung** - Erkennt Anwesenheit über OUT-Signal (digital)
- **DS18B20 Temperatursensor** - Präzise Temperaturmessung mit Offset-Korrektur
- **NeoPixel LED** - RGB-Statusanzeige mit JSON-basierter Steuerung
- **Piezo-Beeper** - Akustische Signalisierung
- **WiFiManager** - Einfache WLAN-Konfiguration über Captive Portal
- **MQTT Integration** - Vollständige Home-Automation-Anbindung
- **OTA Updates** - Drahtlose Firmware-Updates über ElegantOTA
- **Web Interface** - Status-Anzeige und Konfiguration

## 🔌 Hardware

### Komponenten
- **ESP32-S2** Mikrocontroller (Lolin S2 Mini)
- **LD2410** 24GHz Radar-Sensor (nur OUT-Pin verwendet)
- **DS18B20** Digitaler Temperatursensor (OneWire)
- **WS2812B** NeoPixel LED
- **Piezo-Beeper**

### Pin-Belegung

| Komponente | Pin | Beschreibung |
|------------|-----|--------------|
| Beeper | GPIO 5 | Piezo-Summer |
| NeoPixel | GPIO 17 | WS2812B LED |
| DS18B20 | GPIO 18 | OneWire Temperatursensor |
| LD2410 OUT | GPIO 40 | Digitaler Präsenzausgang (HIGH=Präsenz) |

## 📡 MQTT Topics

### Status & Heartbeat
- `sensorcluster/status` - Online/Offline Status (LWT)
- `sensorcluster/ip` - IP-Adresse des Geräts
- `sensorcluster/data` - Heartbeat (alle 10s) mit Device-Info

### Sensoren (Published)
- `sensorcluster/temperature` - Temperatur in °C (alle 15s)
- `sensorcluster/radar/presence` - Präsenzerkennung (0/1)

### Steuerung (Subscribe)

#### Beeper
**Topic:** `sensorcluster/beeper`  
**Format:** Zahl (1-20)  
**Beispiel:** `3` (3x piepen)

#### LED (JSON)
**Topic:** `sensorcluster/led`  
**Format:** JSON-String mit optionalen Parametern  

**Parameter:**
- `color` - RGB Array `[R, G, B]` (Werte: 0-255)
- `on` - Einschaltzeit in Millisekunden
- `off` - Ausschaltzeit in Millisekunden
- `count` - Anzahl Blink-Wiederholungen (0 = endlos)

**Beispiele:**
```json
// Feste rote LED
{"color":[255,0,0], "on":0, "off":0}

// Grün blinken 3x
{"color":[0,255,0], "on":500, "off":500, "count":3}

// Blau endlos blinken
{"color":[0,0,255], "on":200, "off":800, "count":0}

// Nur Farbe ändern (behält aktuelles Blink-Muster)
{"color":[255,255,0]}

// Nur Blink-Muster ändern (behält aktuelle Farbe)
{"on":100, "off":100, "count":10}

// LED ausschalten
{"color":[0,0,0], "on":0, "off":0}
```

## ⚙️ Konfiguration

### WiFi Setup
1. Beim ersten Start öffnet das Gerät einen Access Point: **SensorCluster-AP**
2. Passwort: `12345678`
3. Mit dem AP verbinden - Captive Portal öffnet sich automatisch
4. WLAN-Zugangsdaten und MQTT-Server konfigurieren
5. Gerät startet neu und verbindet sich

### MQTT Konfiguration
Kann über das WiFi-Portal konfiguriert werden:
- **MQTT Server** - IP oder Hostname
- **MQTT Port** - Standard: 1883
- **MQTT User** - Optional
- **MQTT Passwort** - Optional
- **MQTT Topic Prefix** - Standard: sensorcluster/data

### WiFi Reset
- Web-Interface aufrufen: `http://<IP-Adresse>/reset`
- Oder Reset-Button beim Boot gedrückt halten

## 🚀 Installation

### PlatformIO
```bash
# Repository klonen
git clone <repository-url>
cd "SensorCluster V2.0"

# Dependencies installieren
pio pkg install

# Projekt bauen
pio run

# Hochladen (USB)
pio run --target upload

# Serial Monitor
pio device monitor -b 115200
```

### OTA Update
Nach der ersten Installation per USB:
1. Web-Interface öffnen: `http://<IP-Adresse>`
2. "Firmware Update" anklicken
3. `.bin` Datei aus `.pio/build/lolin_s2_mini/` auswählen
4. Hochladen und auf Neustart warten

## 🔧 Technische Details

### Temperatursensor
- **Sensor:** DS18B20 (Dallas/Maxim)
- **Auflösung:** 12-bit (0.0625°C Genauigkeit)
- **Leseintervall:** 15 Sekunden
- **Offset-Korrektur:** -3.0°C (anpassbar im Code: `temp_offset`)
- **MQTT:** Nur bei Änderung wird gesendet (spart Bandbreite)

### Radar-Sensor (LD2410)
- **Technologie:** 24GHz FMCW Radar
- **Modus:** Einfacher Präsenzausgang (OUT-Pin)
- **Signal:** Digitales HIGH = Präsenz erkannt, LOW = keine Präsenz
- **Prüfintervall:** 100ms (schnelle Reaktion)
- **Keine serielle Kommunikation:** Nur OUT-Pin wird verwendet

**Wichtig:** Der LD2410 muss separat konfiguriert werden:
- Empfindlichkeit (stationär/bewegt)
- Verzögerungszeiten (out-hold-time)
- Maximal-Distanz pro Gate

**Konfiguration mit:**
- HLKRadarTool App (Android/iOS)
- Windows/Linux Config-Tools
- Andere ESP32 mit serieller Verbindung

### NeoPixel LED
- **Typ:** WS2812B
- **Helligkeit:** 50/255 (ca. 20%)
- **Steuerung:** JSON-basiert via MQTT
- **Modi:**
  - Feste Farbe (on=0, off=0)
  - Blinken mit konfigurierbaren Zeiten
  - Zählbare Wiederholungen oder Endlos
- **Flexibilität:** Alle Parameter einzeln oder kombiniert änderbar

### Beeper
- **Typ:** Passiver Piezo-Summer
- **Steuerung:** Anzahl-basiert (1-20 Pieptöne)
- **Timing:** 250ms ein, 250ms aus pro Piepton

## 📋 Dependencies

```ini
[lib_deps]
tzapu/WiFiManager @ ^2.0.17
knolleary/PubSubClient @ ^2.8
ayushsharma82/ElegantOTA @ ^3.1.5
adafruit/Adafruit NeoPixel @ ^1.12.3
milesburton/DallasTemperature @ ^3.11.0
bblanchon/ArduinoJson @ ^7.2.1
```

Siehe [platformio.ini](platformio.ini) für vollständige Konfiguration.

## 🏠 Home Assistant Integration

### MQTT Sensoren
```yaml
mqtt:
  sensor:
    - name: "SensorCluster Temperatur"
      state_topic: "sensorcluster/temperature"
      unit_of_measurement: "°C"
      device_class: temperature
      
    - name: "SensorCluster Präsenz"
      state_topic: "sensorcluster/radar/presence"
      payload_on: "1"
      payload_off: "0"
      device_class: occupancy
```

### LED Steuerung
```yaml
script:
  sensorcluster_led_rot:
    sequence:
      - service: mqtt.publish
        data:
          topic: "sensorcluster/led"
          payload: '{"color":[255,0,0], "on":0, "off":0}'
          
  sensorcluster_led_warnung:
    sequence:
      - service: mqtt.publish
        data:
          topic: "sensorcluster/led"
          payload: '{"color":[255,128,0], "on":200, "off":200, "count":5}'
```

### Automation Beispiel
```yaml
automation:
  - alias: "Präsenz erkannt - LED grün"
    trigger:
      - platform: mqtt
        topic: "sensorcluster/radar/presence"
        payload: "1"
    action:
      - service: mqtt.publish
        data:
          topic: "sensorcluster/led"
          payload: '{"color":[0,255,0], "on":0, "off":0}'
      - service: mqtt.publish
        data:
          topic: "sensorcluster/beeper"
          payload: "2"
```

## 🛠️ Entwicklung

### Projekt-Struktur
```
SensorCluster V2.0/
├── platformio.ini          # PlatformIO Konfiguration
├── src/
│   └── main.cpp           # Hauptprogramm
├── include/               # Header-Dateien
├── lib/                   # Lokale Libraries
└── test/                  # Unit-Tests
```

### Serial Monitor
- **Baud Rate:** 115200
- **Ausgabe:** Debug-Informationen, Sensor-Werte, Fehler
- **Nützlich für:** Debugging, Radar-Status, WiFi-Verbindung

## 🐛 Troubleshooting

### WiFi verbindet nicht
- Prüfe SSID und Passwort
- WiFi Reset durchführen (`/reset` im Browser)
- Serial Monitor prüfen auf Fehlermeldungen

### MQTT verbindet nicht
- MQTT Broker erreichbar? (Ping testen)
- Zugangsdaten korrekt?
- Port 1883 offen?
- Serial Monitor zeigt Verbindungsstatus

### Radar reagiert nicht
- **Verkabelung prüfen:**
  - OUT → GPIO 40
  - VCC → 5V
  - GND → GND
- **LD2410 konfiguriert?** OUT-Modus muss aktiviert sein
- **Serial Monitor:** Zeigt Zustandsänderungen

### Temperatur -127°C
- DS18B20 nicht angeschlossen
- Falscher Pin (muss GPIO 18 sein)
- Pull-up Widerstand fehlt (meist intern im Modul)
- Sensor defekt

### LED funktioniert nicht
- NeoPixel an GPIO 17?
- 5V Stromversorgung ausreichend?
- Datenleitung korrekt?
- JSON-Format in MQTT korrekt?

## 📝 Lizenz

Dieses Projekt ist Open Source.

## 🔗 Links

- [LD2410 Datasheet](https://www.hlktech.net/index.php?id=988)
- [ESP32-S2 Dokumentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/)
- [PlatformIO Docs](https://docs.platformio.org/)
- [ArduinoJson Documentation](https://arduinojson.org/)
- [Home Assistant MQTT](https://www.home-assistant.io/integrations/mqtt/)
