#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

// PARÁMETROS CONFIGURABLES DE LA SOLUCIÓN EMPRESARIAL
const char* PARAM_DEVICE_ID = "ESP32_AULA_508";
const char* PARAM_UBICACION = "Aula_Pabellon_A";
const unsigned long PARAM_INTERVALO_TELEMETRIA = 5000;
const int PARAM_UMBRAL_CALIDAD_AIRE = 1800;
const float PARAM_UMBRAL_TEMPERATURA = 25.0;

// CONFIGURACIÓN DE RED Y BROKER MQTT
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

const char* TOPICO_TELEMETRIA = "aula/domotica/telemetria";
const char* TOPICO_ACTUADORES = "aula/domotica/actuadores";

// ASIGNACIÓN DE PINES Y HARDWARE
#define DHTPIN 4
#define DHTTYPE DHT22
#define PIR_PIN 13
#define MQ135_PIN 34
#define RELAY_LUZ_PIN 26
#define RELAY_VENT_PIN 27

#define OLED_SDA 23
#define OLED_SCL 22

// INICIALIZACIÓN DE PERIFÉRICOS Y VARIABLES
DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(128, 64, &Wire, -1);
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsgTime = 0;

// DECLARACIÓN DE FUNCIONES
void setup_wifi();
void reconnect_mqtt();
void actualizar_pantalla(float temp, float hum, int gas, int pres, bool estadoLuz, bool estadoVent);

// CONFIGURACIÓN INICIAL DEL SISTEMA
void setup() {
  Serial.begin(115200);

  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_LUZ_PIN, OUTPUT);
  pinMode(RELAY_VENT_PIN, OUTPUT);

  digitalWrite(RELAY_LUZ_PIN, LOW);
  digitalWrite(RELAY_VENT_PIN, LOW);

  dht.begin();

  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Fallo al inicializar pantalla OLED SSD1306");
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("INICIANDO NODO IOT...");
  display.println("SISTEMA DOMOTICO AULAS");
  display.display();

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

// CONEXIÓN A LA RED WI-FI
void setup_wifi() {
  delay(10);

  Serial.println();
  Serial.print("Conectando a red Wi-Fi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConexión Wi-Fi Establecida.");
  Serial.print("Dirección IP asignada: ");
  Serial.println(WiFi.localIP());
}

// CONEXIÓN Y RECONEXIÓN AL BROKER MQTT
void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Intentando conexión con Broker MQTT (" + String(mqtt_server) + ")...");

    String clientId = "ESP32Client-" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println(" CONECTADO.");
    } else {
      Serial.print(" Falló conexión, rc=");
      Serial.print(client.state());
      Serial.println(". Reintentando en 3 segundos...");
      delay(3000);
    }
  }
}

// LECTURA DE SENSORES, AUTOMATIZACIÓN Y PUBLICACIÓN MQTT
void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }

  client.loop();

  unsigned long now = millis();

  if (now - lastMsgTime > PARAM_INTERVALO_TELEMETRIA) {
    lastMsgTime = now;

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    int pres = digitalRead(PIR_PIN);
    int gas = analogRead(MQ135_PIN);

    if (isnan(temp) || isnan(hum)) {
      temp = 0.0;
      hum = 0.0;
    }

    bool estadoLuz = (pres == HIGH);
    digitalWrite(RELAY_LUZ_PIN, estadoLuz ? HIGH : LOW);

    bool estadoVent = (
      gas > PARAM_UMBRAL_CALIDAD_AIRE ||
      temp > PARAM_UMBRAL_TEMPERATURA
    );

    digitalWrite(RELAY_VENT_PIN, estadoVent ? HIGH : LOW);

    StaticJsonDocument<256> doc;
    doc["device_id"] = PARAM_DEVICE_ID;
    doc["ubicacion"] = PARAM_UBICACION;
    doc["temperatura"] = temp;
    doc["humedad"] = hum;
    doc["calidad_aire_ppm"] = gas;
    doc["presencia"] = pres;
    doc["rele_iluminacion"] = estadoLuz ? "ON" : "OFF";
    doc["rele_ventilacion"] = estadoVent ? "ON" : "OFF";

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);

    client.publish(TOPICO_TELEMETRIA, jsonBuffer);

    StaticJsonDocument<128> docActuadores;
    docActuadores["device_id"] = PARAM_DEVICE_ID;
    docActuadores["iluminacion"] = estadoLuz ? 1 : 0;
    docActuadores["ventilacion"] = estadoVent ? 1 : 0;

    char jsonActuadorBuffer[128];
    serializeJson(docActuadores, jsonActuadorBuffer);

    client.publish(TOPICO_ACTUADORES, jsonActuadorBuffer);

    Serial.print("[MQTT PUBLISH] Payload: ");
    Serial.println(jsonBuffer);

    actualizar_pantalla(temp, hum, gas, pres, estadoLuz, estadoVent);
  }
}

// ACTUALIZACIÓN DE LA PANTALLA OLED
void actualizar_pantalla(float temp, float hum, int gas, int pres, bool estadoLuz, bool estadoVent) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);

  display.println("NODO AULA 508 - IOT");
  display.println("---------------------");
  display.println("Temp: " + String(temp, 1) + " C | Hum: " + String(hum, 0) + "%");
  display.println("Aire: " + String(gas) + " PPM");
  display.println("Presencia: " + String(pres == 1 ? "SI" : "NO"));
  display.println("Luz (R1): " + String(estadoLuz ? "ON" : "OFF"));
  display.println("Vent (R2): " + String(estadoVent ? "ON" : "OFF"));

  display.display();
}