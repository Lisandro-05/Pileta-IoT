<h1 align="center">Pileta IoT 💧 - Sistema de Monitoreo Inteligente</h1>

<div align="center">
       Sistema IoT completo para el monitoreo en tiempo real de parámetros de calidad de agua en piscinas y/o piletas. Desarrollado como proyecto de la materia **Práctica Profesionalizante I** del **ISPC de Córdoba**.
</div>

## Descripción

Este proyecto implementa una solución integral de monitoreo IoT que permite supervisar continuamente los parámetros críticos de calidad del agua de piletas mediante sensores conectados a un ESP32, almacenamiento de datos en tiempo real y visualización mediante dashboards interactivos.

## Características Principales

- **Monitoreo en Tiempo Real**: Lectura continua de pH, temperatura y TDS (Total Dissolved Solids)
- **Comunicación MQTT**: Arquitectura basada en mensajería pub/sub para comunicación asíncrona
- **Almacenamiento de Series Temporales**: Base de datos InfluxDB para historial de mediciones
- **Dashboards Interactivos**: Visualización de métricas con Grafana
- **Configuración Web**: Interfaz web integrada en el ESP32 para configuración inicial
- **Simulador de Datos**: Herramienta para pruebas sin hardware físico
- **Docker Compose**: Despliegue simplificado con contenedores

## Arquitectura del Sistema

```
┌─────────────┐
│   ESP32     │  ──► Sensores (pH, Temp, TDS)
│  (Hardware) │  ──► Simulador (opcional)
└──────┬──────┘
       │ MQTT
       ▼
┌─────────────┐
│  Mosquitto  │  ──► Broker MQTT
│   (MQTT)    │
└──────┬──────┘
       │
       └──► Telegraf ──► InfluxDB ──► Grafana
```

## Componentes

### Hardware
- **ESP32 NodeMCU**: Microcontrolador con WiFi integrado
- **Sensor Atlas Scientific EZO-pH**: Sensor de pH con comunicación serial
- **Sensor PT-1000**: Sensor de temperatura
- **Sensor TDS**: Sensor analógico de sólidos disueltos totales

### Software
- **Mosquitto**: Broker MQTT para mensajería
- **Telegraf**: Agente de recolección de métricas
- **InfluxDB 2.7**: Base de datos de series temporales
- **Grafana 11.2.0**: Plataforma de visualización y análisis
- **Simulador Python**: Generador de datos de prueba

## Requisitos Previos

- Docker y Docker Compose instalados
- Git
- (Opcional) PlatformIO para compilar el firmware del ESP32

## Instalación y Configuración

### 1. Clonar el Repositorio

```bash
git clone https://github.com/Lisandro-05/Pileta-IoT.git
cd pileta-iot-test
```

### 2. Iniciar los Servicios

```bash
docker-compose up -d
```

Esto iniciará todos los servicios:
- **Mosquitto**: `localhost:1883`
- **InfluxDB**: `localhost:8086`
- **Grafana**: `localhost:3000`

### 3. Configurar InfluxDB

1. Accede a `http://localhost:8086`
2. Completá el setup inicial con las credenciales del `.env`

### 4. Configurar Grafana

1. Accedé a `http://localhost:3000`
2. Iniciá sesión con las credenciales del `.env`
3. El dashboard de Pileta se cargará automáticamente

## Configuración del ESP32

### Compilar y Subir el Firmware

1. Instalá [PlatformIO](https://platformio.org/)
2. Abrí el proyecto en VS Code o tu IDE preferido
3. Compilá y subí el código al ESP32:

```bash
cd ESP32-code
pio run -t upload
```

### Configuración Inicial

1. Al encender el ESP32, se crea un punto de acceso WiFi llamado `ESP32_Config`
2. Conectate a esta red (contraseña: `12345678`)
3. Abri un navegador y andá a `http://192.168.4.1`
4. Completá el formulario con:
   - SSID de tu red WiFi
   - Contraseña de WiFi
   - IP del broker MQTT (IP de tu servidor)
   - Puerto MQTT (por defecto: 1883)
5. Guardá la configuración

El ESP32 se reiniciará y comenzará a enviar datos al broker MQTT cada 5 segundos.  
***Nota:** Los datos no son persistentes por el estado actual del proyecto (desarrollo).*

## Uso del Simulador

Si no tenés hardware físico, podés usar el simulador:

```bash
docker-compose up lecture-simulator
```

El simulador generará datos de prueba y los publicará en el tópico `pool/metrics`.

## Estructura del Proyecto

```
pileta-iot-test/
├── ESP32-code/              # Código del firmware ESP32
│   ├── src/
│   │   └── main.cpp        # Código principal del ESP32
│   └── platformio.ini      # Configuración de PlatformIO
├── lecture-simulator/      # Simulador de datos
│   ├── publisher.py        # Script de simulación
│   └── requirements.txt    # Dependencias Python
├── mosquitto/              # Configuración del broker MQTT
│   └── config/
│       └── mosquitto.conf
├── telegraf/               # Configuración de Telegraf
│   └── telegraf.conf
├── influxdb/               # Datos persistentes de InfluxDB
├── grafana/                # Dashboards y configuración
│   ├── dashboards/
│   │   └── Pileta-dashboard.json
│   └── provisioning/
├── docker-compose.yml      # Orquestación de servicios
└── README.md               # Este archivo
```

## Tópicos MQTT

- **`pool/metrics`**: Tópico donde se publican las métricas con el siguiente formato:

```json
{
  "ph": 7.4,
  "temperature_c": 25.0,
  "tds_ppm": 500,
  "trend": "stable",
  "trend_value": 0.0,
  "timestamp": 1234567890
}
```

## Resumen de las Tecnologías Utilizadas

- **Hardware**: ESP32, Sensores Atlas Scientific
- **Protocolo**: MQTT
- **Base de Datos**: InfluxDB 2.7
- **Visualización**: Grafana 11.2.0
- **Agente de Métricas**: Telegraf 1.30
- **Broker MQTT**: Eclipse Mosquitto 2
- **Lenguajes**: C++ (Arduino), Python
- **Orquestación**: Docker Compose

## Métricas Monitoreadas

- **pH**: Rango óptimo 7.2 - 7.8
- **Temperatura**: Monitoreo en grados Celsius
- **TDS**: Sólidos disueltos totales en ppm (partes por millón)
- **Tendencia**: Indicador de estabilidad de los parámetros

## Imágenes

![Grafana funcionando](/resources/Grafana.png)
![Página de conexión ESP](/resources/Pagina-ESP32.png)

## Autores

- **Lisandro Juncos** - *Desarrollo inicial* - [Github](https://github.com/Lisandro-05)
