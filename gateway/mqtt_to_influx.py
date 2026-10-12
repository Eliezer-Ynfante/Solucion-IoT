import paho.mqtt.client as mqtt
import json
from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS

# Configuración MQTT (Debe coincidir con el servidor de Wokwi)
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPIC = "aula/domotica/telemetria"

# Configuración InfluxDB
INFLUX_URL = "http://localhost:8086"
INFLUX_TOKEN = "my-super-secret-auth-token"
INFLUX_ORG = "utp_iot"
INFLUX_BUCKET = "telemetria_aulas"

client_influx = InfluxDBClient(url=INFLUX_URL, token=INFLUX_TOKEN, org=INFLUX_ORG)
write_api = client_influx.write_api(write_options=SYNCHRONOUS)

def on_connect(client, userdata, flags, rc):
    print(f"Conectado al Broker MQTT ({MQTT_BROKER}) con código de resultado: {rc}")
    client.subscribe(MQTT_TOPIC)
    print(f"Suscrito al tópico: {MQTT_TOPIC}")

def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode('utf-8'))
        print(f"\n[MENSAJE RECIBIDO] {payload}")

        point = Point("mediciones_aula") \
            .tag("device_id", str(payload.get("device_id", "ESP32_AULA"))) \
            .tag("ubicacion", str(payload.get("ubicacion", "Aula_General"))) \
            .field("temperatura", float(payload.get("temperatura", 0.0))) \
            .field("humedad", float(payload.get("humedad", 0.0))) \
            .field("calidad_aire_ppm", int(payload.get("calidad_aire_ppm", 0))) \
            .field("presencia", int(payload.get("presencia", 0))) \
            .field("rele_iluminacion", 1 if payload.get("rele_iluminacion") == "ON" else 0) \
            .field("rele_ventilacion", 1 if payload.get("rele_ventilacion") == "ON" else 0)

        write_api.write(bucket=INFLUX_BUCKET, org=INFLUX_ORG, record=point)
        print("-> Registro almacenado con éxito en InfluxDB.")
    except Exception as e:
        print(f"Error procesando mensaje: {e}")

mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
mqtt_client.loop_forever()