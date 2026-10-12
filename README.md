# Solución IoT para Monitoreo Ambiental y Automatización Domótica en Aulas

## 1. Descripción general

Este proyecto implementa una solución IoT para supervisar condiciones ambientales en un aula y automatizar dispositivos de iluminación y ventilación según la presencia de personas, la temperatura y la calidad del aire. La arquitectura combina sensores conectados a un ESP32, comunicación MQTT, un broker local, almacenamiento en InfluxDB y visualización en Grafana.

El sistema está pensado para ambientes educativos o de trabajo donde se desea:

- monitorear temperatura y humedad,
- detectar presencia mediante sensor PIR,
- evaluar calidad del aire con un sensor MQ135,
- automatizar iluminación y ventilación,
- registrar series temporales para análisis y visualización,
- centralizar la información mediante MQTT e InfluxDB.

---

## 2. Objetivos del sistema

### Objetivo principal
Diseñar una solución de bajo costo y fácil replicación para automatizar condiciones ambientales en aulas usando Internet de las Cosas.

### Objetivos específicos
- Capturar variables de entorno desde sensores reales o simulados.
- Enviar los datos por MQTT a un broker central.
- Persistir los datos en una base de series de tiempo.
- Activar actuadores según reglas automáticas.
- Exponer la información en una interfaz de monitoreo.

---

## 3. Arquitectura del sistema

El flujo del sistema es el siguiente:

1. El ESP32 lee los sensores:
   - DHT22 para temperatura y humedad.
   - PIR para presencia.
   - MQ135 para calidad del aire.
2. El microcontrolador procesa la información y activa relés:
   - Relé 1: iluminación.
   - Relé 2: ventilación.
3. Se publican mensajes JSON en el tópico MQTT:
   - `aula/domotica/telemetria`
4. El gateway Python recibe esos mensajes y los almacena en InfluxDB.
5. Grafana consulta InfluxDB para crear dashboards y alertas.

Diagrama funcional:

```text
ESP32 + sensores + relés
        │
        ▼
   MQTT Broker (Mosquitto)
        │
        ▼
  Gateway Python (mqtt_to_influx.py)
        │
        ▼
      InfluxDB 2.x
        │
        ▼
      Grafana
```

---

## 4. Componentes del proyecto

### 4.1 Firmware (ESP32)
Ubicación: `firmware/`

Archivos principales:
- `sketch.ino`: lógica principal del nodo IoT.
- `diagram.json`: circuito simulado en Wokwi.
- `libraries.txt`: librerías requeridas en Arduino.

El firmware configura:
- red Wi‑Fi,
- conexión MQTT,
- lectura de sensores,
- control de relés,
- publicación de JSON en MQTT,
- actualización de pantalla OLED.

### 4.2 Broker MQTT
Ubicación: `docker/mosquitto/`

Se despliega con Mosquitto mediante Docker. La configuración incluye:
- puerto `1883` para MQTT,
- puerto `9001` para WebSockets,
- persistencia de datos,
- logs de operación.

Archivo de configuración:
- `docker/mosquitto/config/mosquitto.conf`

### 4.3 Gateway de integración
Ubicación: `gateway/`

El script `mqtt_to_influx.py` se conecta al broker MQTT, recibe mensajes del tópico `aula/domotica/telemetria` y escribe los datos en InfluxDB.

Dependencias:
- `paho-mqtt`
- `influxdb-client`

### 4.4 Infraestructura con Docker
Ubicación: `docker/docker-compose.yml`

Servicios incluidos:
- `mosquitto`: broker MQTT.
- `influxdb`: base de datos de series de tiempo.
- `grafana`: plataforma de visualización.

---

## 5. Hardware y sensores

El esquema del nodo IoT está compuesto por:

- ESP32 DevKit C V4
- Sensor DHT22 (temperatura y humedad)
- Sensor PIR (detección de presencia)
- Sensor MQ135 (calidad del aire / gases)
- Módulo de relés para iluminación y ventilación
- Pantalla OLED SSD1306 para diagnóstico local

### Pines principales configurados

- `DHTPIN 4`
- `PIR_PIN 13`
- `MQ135_PIN 34`
- `RELAY_LUZ_PIN 26`
- `RELAY_VENT_PIN 27`
- `OLED_SDA 23`
- `OLED_SCL 22`

---

## 6. Lógica de automatización

El firmware toma decisiones automáticas en base a los sensores.

### Regla de iluminación
- Si el sensor PIR detecta presencia (`HIGH`), se enciende la luz.
- Si no hay presencia, la luz se apaga.

### Regla de ventilación
- Se activa la ventilación si:
  - la calidad del aire supera el umbral configurado,
  - o la temperatura supera el umbral configurado.

Valores configurados en el firmware:
- `PARAM_INTERVALO_TELEMETRIA = 5000` ms
- `PARAM_UMBRAL_CALIDAD_AIRE = 1800`
- `PARAM_UMBRAL_TEMPERATURA = 25.0`

Esto permite un sistema de control reactivo y sostenible para aulas.

---

## 7. Topologías de datos y mensajes MQTT

### Tópico de publicación
- `aula/domotica/telemetria`

### Ejemplo de payload publicado por el ESP32

```json
{
  "device_id": "ESP32_AULA_508",
  "ubicacion": "Aula_Pabellon_A",
  "temperatura": 24.7,
  "humedad": 46,
  "calidad_aire_ppm": 1720,
  "presencia": 1,
  "rele_iluminacion": "ON",
  "rele_ventilacion": "OFF"
}
```

### Tópico de actuadores
- `aula/domotica/actuadores`

El firmware también publica un JSON auxiliar con el estado de los actuadores para control remoto o monitoreo.

---

## 8. Flujo de datos real

```text
Sensor DHT22 / PIR / MQ135
        │
        ▼
   ESP32 Firmware
        │
        ▼
   Publica JSON MQTT
        │
        ▼
   Mosquitto Broker
        │
        ▼
  Gateway Python recibe y parsea
        │
        ▼
  InfluxDB almacena mediciones
        │
        ▼
  Grafana visualiza tendencias
```

El archivo `gateway/mqtt_to_influx.py` transforma cada payload en un punto de InfluxDB con tags y campos correspondientes:

- Tags: `device_id`, `ubicacion`
- Campos: temperatura, humedad, calidad_aire_ppm, presencia, rele_iluminacion, rele_ventilacion

---

## 9. Configuración de servicios Docker

### 9.1 Arranque de infraestructura
Desde la carpeta `docker/`, ejecutar:

```bash
docker compose up -d
```

Esto levanta:
- Mosquitto en el puerto 1883
- InfluxDB en el puerto 8086
- Grafana en el puerto 3000

### 9.2 Credenciales por defecto

InfluxDB:
- Usuario: `admin`
- Contraseña: `adminpassword123`
- Organización: `utp_iot`
- Bucket: `telemetria_aulas`
- Token: `my-super-secret-auth-token`

Grafana:
- Usuario: `admin`
- Contraseña: `admin`

---

## 10. Ejecución del gateway

La integración entre MQTT e InfluxDB se realiza con el script Python ubicado en `gateway/mqtt_to_influx.py`.

### Preparación del entorno

1. Ubicarse en la carpeta del proyecto.
2. Crear un entorno virtual (opcional pero recomendado).
3. Instalar dependencias:

```bash
pip install -r gateway/requirements.txt
```

### Ejecución

```bash
python gateway/mqtt_to_influx.py
```

El script quedará escuchando el tópico MQTT y escribiendo cada dato a InfluxDB.

---

## 11. Ejecución del sistema completo

### Paso 1: Simulación en Wokwi
1. Abrir la simulación en Wokwi con el enlace disponible en la carpeta del proyecto.
2. Cargar los archivos `firmware/sketch.ino` y `firmware/diagram.json`.
3. Ejecutar la simulación.

### Paso 2: Levantar infraestructura

```bash
cd docker
docker compose up -d
```

### Paso 3: Ejecutar el gateway

```bash
python gateway/mqtt_to_influx.py
```

### Paso 4: Verificar en Grafana
Abrir:

```text
http://localhost:3000
```

Iniciar sesión con `admin / admin` y configurar un datasource de InfluxDB con la organización y token del sistema.

---

## 12. Requisitos del sistema

### Hardware necesario
- ESP32 DevKit
- DHT22
- Sensor PIR
- Sensor MQ135
- Relés de 5V
- Pantalla OLED 128x64
- Fuente de alimentación adecuada

### Software necesario
- Arduino IDE o entorno compatible con ESP32
- Docker y Docker Compose
- Python 3.x
- Librerías Arduino y Python listadas en los archivos del proyecto

---

## 13. Mantenimiento y troubleshooting

### Problemas comunes

#### 1. El ESP32 no se conecta a Wi‑Fi
- Revisar el nombre de la red y la contraseña en el firmware.
- Confirmar que la red esté disponible.

#### 2. El MQTT no recibe mensajes
- Verificar que el broker Mosquitto esté corriendo.
- Confirmar que el topic del firmware coincide con el suscrito por el gateway.

#### 3. El gateway falla al escribir en InfluxDB
- Confirmar que InfluxDB esté activo en `http://localhost:8086`.
- Verificar token, organización y bucket.

#### 4. Grafana no conecta con InfluxDB
- Usar la URL correcta del servicio.
- Confirmar token de autenticación.
- Validar que el bucket existe.

#### 5. Sensores no responden
- Revisar conexiones físicas.
- Verificar alimentación y pinout.
- Comprobar librerías instaladas en Arduino.

---

## 14. Datos relevantes del proyecto

La solución combina tres capas principales:

- Capa de percepción: sensores y actuadores.
- Capa de transporte: MQTT.
- Capa de almacenamiento y análisis: InfluxDB + Grafana.

Esto permite un modelo escalable y útil para supervisión de infraestructura escolar o industrial de bajo costo.

---

## 15. Consideraciones de seguridad y despliegue

- En entornos productivos, se recomienda cambiar credenciales por defecto.
- No exponer servicios de Docker directamente sin firewall o segmentación de red.
- Usar TLS en broker MQTT e InfluxDB si se despliega fuera de una red local cerrada.
- Revisar permisos de acceso en Grafana.

---

## 16. Conclusión

La solución presentada permite construir un sistema IoT completo para monitoreo ambiental y automatización de aulas. La integración de ESP32, MQTT, InfluxDB y Grafana genera una base funcional y extensible para monitoreo en tiempo real, análisis histórico y automatización inteligente de instalaciones educativas.

Es una implementación práctica, fácil de replicar y expandir a más aulas, edificios o condiciones de operación.

---

## 18. Estructura del repositorio

```text
Solución IoT/
├── README.md
├── docker/
│   ├── docker-compose.yml
│   └── mosquitto/
│       ├── config/
│       │   └── mosquitto.conf
│       ├── data/
│       └── log/
├── evidencias/
├── firmware/
│   ├── diagram.json
│   ├── libraries.txt
│   └── sketch.ino
└── gateway/
    ├── mqtt_to_influx.py
    ├── requirements.txt
    └── __pycache__/
```

---

## 19. Comandos resumidos de uso

```bash
# Levantar infraestructura
cd docker
docker compose up -d

# Ejecutar el gateway
cd ..
python gateway/mqtt_to_influx.py

# Verificar Grafana
http://localhost:3000
```

