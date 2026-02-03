#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ElegantOTA.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>

// ==================== KONFIGURATION ====================
// Pin Definitionen
#define BEEPER_PIN 5
#define LED_PIN 17
#define LED_COUNT 1
#define ONE_WIRE_BUS 18
#define RADAR_OUT_PIN 40  // LD2410 OUT Signal (digitaler Präsenzausgang)

// Intervalle
const long tempReadInterval = 15000;
const long radarCheckInterval = 100;  // OUT-Signal alle 100ms prüfen
const long mqttReconnectInterval = 5000;

// Sensor Parameter
const float temp_offset = 0.5;

// ==================== MQTT TOPICS ====================
char mqtt_server[40] = "";
char mqtt_port[6] = "1883";
char mqtt_user[40] = "";
char mqtt_password[40] = "";
char mqtt_topic[100] = "sensorcluster/data";
char mqtt_status_topic[110] = "sensorcluster/status";
char mqtt_beeper_topic[110] = "sensorcluster/beeper";
char mqtt_led_topic[110] = "sensorcluster/led";
char mqtt_temperature_topic[110] = "sensorcluster/temperature";
char mqtt_radar_presence_topic[110] = "sensorcluster/radar/presence";

// ==================== GLOBALE OBJEKTE ====================
WiFiClient espClient;
PubSubClient mqttClient(espClient);
WebServer server(80);
WiFiManager wifiManager;
Preferences preferences;
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ==================== STATE VARIABLEN ====================
// Beeper
int beepCount = 0, currentBeep = 0;
unsigned long lastBeepTime = 0;
bool beeping = false;

// NeoPixel
uint8_t ledRed = 0, ledGreen = 0, ledBlue = 0;
int blinkOnTime = 0, blinkOffTime = 0, blinkCount = 0, currentBlink = 0;
unsigned long lastLedChange = 0;
bool ledState = false, blinkingLed = false;

// Temperatur
float lastTemperature = -127.0;
unsigned long lastTempRead = 0;

// Radar
bool radarPresence = false;
unsigned long lastRadarCheck = 0;

// MQTT
unsigned long lastMqttReconnect = 0;

// ==================== FUNKTIONEN ====================

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  // Beeper Steuerung
  if (String(topic) == mqtt_beeper_topic) {
    int count = message.toInt();
    if (count > 0 && count <= 20) {
      beepCount = count;
      currentBeep = 0;
      beeping = true;
    }
  }
  
  // LED Steuerung (JSON Format: {"color":[R,G,B], "on":500, "off":500, "count":5})
  else if (String(topic) == mqtt_led_topic) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (!error) {
      // Farbe auslesen
      if (doc.containsKey("color") && doc["color"].is<JsonArray>()) {
        JsonArray color = doc["color"];
        if (color.size() >= 3) {
          ledRed = color[0];
          ledGreen = color[1];
          ledBlue = color[2];
        }
      }
      
      // Blink-Zeiten auslesen
      if (doc.containsKey("on")) {
        blinkOnTime = doc["on"];
      }
      if (doc.containsKey("off")) {
        blinkOffTime = doc["off"];
      }
      
      // Anzahl auslesen
      if (doc.containsKey("count")) {
        blinkCount = doc["count"];
      }
      
      // LED-Modus setzen
      if (blinkOnTime == 0 && blinkOffTime == 0) {
        // Feste Farbe (kein Blinken)
        blinkingLed = false;
        ledState = true;
        strip.setPixelColor(0, strip.Color(ledRed, ledGreen, ledBlue));
        strip.show();
      } else {
        // Blink-Modus
        blinkingLed = true;
        currentBlink = 0;
        ledState = false;
        lastLedChange = millis();
      }
    }
  }
}

void reconnectMQTT() {
  if (millis() - lastMqttReconnect > mqttReconnectInterval) {
    lastMqttReconnect = millis();
    if (!mqttClient.connected()) {
      String clientId = "ESP32-" + String(WiFi.macAddress());
      // LWT: QoS 1, retain true, message "offline"
      if (mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_password, 
                            mqtt_status_topic, 1, true, "offline")) {
        mqttClient.publish(mqtt_status_topic, "online", true);
        mqttClient.subscribe(mqtt_topic);
        mqttClient.subscribe(mqtt_beeper_topic);
        mqttClient.subscribe(mqtt_led_topic);
      }
    }
  }
}

void saveConfig() {
  preferences.begin("mqtt", false);
  preferences.putString("server", mqtt_server);
  preferences.putString("port", mqtt_port);
  preferences.putString("user", mqtt_user);
  preferences.putString("password", mqtt_password);
  preferences.putString("topic", mqtt_topic);
  preferences.end();
}

void loadConfig() {
  preferences.begin("mqtt", true);
  preferences.getString("server", "broker.hivemq.com").toCharArray(mqtt_server, 40);
  preferences.getString("port", "1883").toCharArray(mqtt_port, 6);
  preferences.getString("user", "").toCharArray(mqtt_user, 40);
  preferences.getString("password", "").toCharArray(mqtt_password, 40);
  preferences.getString("topic", "sensorcluster/data").toCharArray(mqtt_topic, 100);
  preferences.end();
}

void saveConfigCallback() {
  saveConfig();
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Konfiguration laden
  loadConfig();
  
  // Hardware initialisieren
  pinMode(BEEPER_PIN, OUTPUT);
  digitalWrite(BEEPER_PIN, LOW);
  
  strip.begin();
  strip.setBrightness(50);
  strip.setPixelColor(0, strip.Color(0, 0, 0));
  strip.show();
  
  sensors.begin();
  if (sensors.getDeviceCount() > 0) {
    sensors.setResolution(12);
  }
  
  // LD2410 OUT Pin initialisieren
  pinMode(RADAR_OUT_PIN, INPUT);
  Serial.println("LD2410 OUT-Pin (IO40) initialisiert");
  
  // WiFiManager Setup
  WiFiManagerParameter custom_mqtt_server("server", "MQTT Server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "MQTT User", mqtt_user, 40);
  WiFiManagerParameter custom_mqtt_password("password", "MQTT Passwort", mqtt_password, 40);
  WiFiManagerParameter custom_mqtt_topic("topic", "MQTT Topic", mqtt_topic, 100);
  
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_mqtt_topic);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setConfigPortalTimeout(180);
  
  if (!wifiManager.autoConnect("SensorCluster-AP", "12345678")) {
    ESP.restart();
  }
  
  // MQTT Konfiguration speichern
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(mqtt_password, custom_mqtt_password.getValue());
  strcpy(mqtt_topic, custom_mqtt_topic.getValue());
  saveConfig();
  
  // MQTT Setup
  mqttClient.setServer(mqtt_server, atoi(mqtt_port));
  mqttClient.setCallback(mqttCallback);
  
  // Web Server Setup
  server.on("/", []() {
    server.send(200, "text/html", 
      "<h1>SensorCluster V2.0</h1>"
      "<p><a href='/update'>Firmware Update</a></p>"
      "<p><a href='/reset'>WiFi Reset</a></p>"
      "<p>IP: " + WiFi.localIP().toString() + "</p>"
      "<p>MAC: " + WiFi.macAddress() + "</p>"
      "<p>MQTT: " + String(mqtt_server) + ":" + String(mqtt_port) + "</p>"
    );
  });
  
  server.on("/reset", []() {
    server.send(200, "text/html", 
      "<h1>WiFi Einstellungen werden zurückgesetzt...</h1>"
      "<p>Bitte warten Sie 10 Sekunden, dann startet der AP neu.</p>"
    );
    delay(1000);
    wifiManager.resetSettings();
    delay(2000);
    ESP.restart();
  });
  
  ElegantOTA.begin(&server);
  server.begin();
  
  Serial.println("SensorCluster V2.0 bereit - IP: " + WiFi.localIP().toString());
}

// ==================== LOOP ====================
void loop() {
  if (!mqttClient.connected()) reconnectMQTT();
  mqttClient.loop();
  server.handleClient();
  ElegantOTA.loop();
  
  unsigned long now = millis();
  
  // Beeper
  if (beeping && currentBeep < beepCount) {
    if (now - lastBeepTime >= 200) {
      digitalWrite(BEEPER_PIN, !digitalRead(BEEPER_PIN));
      if (digitalRead(BEEPER_PIN) == LOW) currentBeep++;
      lastBeepTime = now;
    }
  } else if (beeping) {
    beeping = false;
    digitalWrite(BEEPER_PIN, LOW);
  }
  
  // NeoPixel Blink
  if (blinkingLed && (blinkOnTime > 0 || blinkOffTime > 0)) {
    if (blinkCount > 0 && currentBlink >= blinkCount) {
      blinkingLed = false;
      strip.setPixelColor(0, strip.Color(0, 0, 0));
      strip.show();
    } else {
      unsigned long interval = ledState ? blinkOnTime : blinkOffTime;
      if (now - lastLedChange >= interval) {
        ledState = !ledState;
        strip.setPixelColor(0, ledState ? strip.Color(ledRed, ledGreen, ledBlue) : strip.Color(0, 0, 0));
        strip.show();
        if (!ledState && blinkCount > 0) currentBlink++;  // Zähle beim Wechsel von ON zu OFF
        lastLedChange = now;
      }
    }
  }
  
  // Temperatur
  if (now - lastTempRead >= tempReadInterval) {
    lastTempRead = now;
    sensors.requestTemperatures();
    float temperature = sensors.getTempCByIndex(0) - temp_offset;
    if (temperature != DEVICE_DISCONNECTED_C && temperature != -127.0 && temperature != lastTemperature) {
      lastTemperature = temperature;
      if (mqttClient.connected()) {
        char tempString[8];
        dtostrf(temperature, 1, 2, tempString);
        mqttClient.publish(mqtt_temperature_topic, tempString, true);
      }
    }
  }
  
  // Radar OUT Signal prüfen
  if (now - lastRadarCheck >= radarCheckInterval) {
    lastRadarCheck = now;
    
    // OUT-Pin lesen (HIGH = Präsenz erkannt, LOW = keine Präsenz)
    bool currentPresence = digitalRead(RADAR_OUT_PIN) == HIGH;
    
    // Nur bei Änderung senden
    if (currentPresence != radarPresence) {
      radarPresence = currentPresence;
      if (mqttClient.connected()) {
        mqttClient.publish(mqtt_radar_presence_topic, radarPresence ? "1" : "0", true);
        Serial.print("Radar Präsenz (OUT): ");
        Serial.println(radarPresence ? "JA" : "NEIN");
      }
    }
  }
  
  // Status Heartbeat
  static unsigned long lastMsg = 0;
  if (now - lastMsg > 10000) {
    lastMsg = now;
    if (mqttClient.connected()) {
      String payload = "{\"device\":\"SensorCluster\",\"ip\":\"" + WiFi.localIP().toString() + "\",\"uptime\":" + String(millis()/1000) + "}";
      mqttClient.publish(mqtt_topic, payload.c_str());
    }
  }
}