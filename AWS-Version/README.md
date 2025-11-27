# 🏊‍♂️ Pileta IoT - Versión AWS Cloud

> 💡 **Nota**: Versión de desarrollo. Carece de contenedores adaptados para despliegue en cloud, solo una conexión exitosa con AWS IoT Core

## 🌐 ¿Qué es esta versión?

Esta es la **versión cloud** del proyecto Pileta IoT, diseñada para conectarse directamente a **AWS IoT Core** en lugar de usar infraestructura local. El ESP32 se comunica de forma segura con los servicios de AWS mediante certificados SSL/TLS, eliminando la necesidad de servidores locales, brokers MQTT propios y bases de datos locales.

## ✨ Características Principales

- 🔐 **Conexión Segura**: Autenticación mediante certificados X.509 con AWS IoT Core
- ☁️ **Cloud-Native**: Sin necesidad de infraestructura local (Docker, servidores, etc.)
- 📊 **Monitoreo Simulado**: Genera datos de pH, temperatura y TDS cada 60 segundos
- 🌐 **Configuración Web**: Interfaz web integrada para configuración WiFi inicial
- 🚀 **Despliegue Simplificado**: Solo necesitás el ESP32 y una conexión a internet

## 🔑 Diferencias con la Versión Local

| Característica | Versión Local | Versión AWS |
|---------------|---------------|-------------|
| Broker MQTT | Mosquitto (local) | AWS IoT Core (cloud) |
| Base de Datos | InfluxDB (local) | AWS (gestionado) |
| Visualización | Grafana (local) | AWS (opcional) |
| Infraestructura | Docker Compose | Solo ESP32 |
| Seguridad | Usuario/Contraseña | Certificados X.509 |
| Mantenimiento | Requiere servidor | Gestionado por AWS |

## 📦 Requisitos

- **ESP32** con WiFi
- **Certificados AWS IoT** configurados
- **Conexión a internet**
- **PlatformIO** para compilar y subir el firmware

## 🚀 Inicio Rápido

1. Configurá tus certificados AWS IoT en `secretidirigillo.h`
2. Compilá y subí el código al ESP32
3. Conectate al punto de acceso `ESP32_Pileta`
4. Configurá tu red WiFi desde la interfaz web
5. ¡Listo! Los datos se enviarán automáticamente a AWS IoT Core

## 📊 Métricas Enviadas

- **pH**: Nivel de acidez del agua (simulado)
- **Temperatura**: Temperatura en grados Celsius (simulado)
- **TDS**: Sólidos disueltos totales en ppm (simulado)

**Intervalo de envío**: Cada 60 segundos

## 🛡️ Seguridad

- Autenticación mediante certificados X.509
- Comunicación encriptada con SSL/TLS
- Validación de certificados en tiempo real
- Sincronización NTP para validación temporal
