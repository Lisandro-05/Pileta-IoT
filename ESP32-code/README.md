# Código ESP32
### Este código es para cargar al esp32, el mismo entrega lecturas cada 3 horas
#### Se usa un sensor "Atlas Scientific EZO-pH Kit + PT-1000" de base para la configuración de pines y programación de payload
---
**El esp tendría el siguiente comportamiento:**

- Se levanta un punto de acceso WiFi propio.
- Monta un servidor web para ingresar:  
        ¬ SSID y contraseña del WiFi  
        ¬ IP y puerto del broker MQTT  

- Estos datos se guardan en RAM (no en flash todavía, para que se borren al reiniciar).
- Se conecta al Wi-Fi, al broker, y lee valores reales desde el sensor
- Obviamente envía datos al broker MQTT.

**Por el momento solo está programado para leer PH y Temperatura ya que para medir el tds en ppm se necesita un sensor de conductividad eléctrica como el Atlas Scientific EZO-EC**

---

### Esquema de conexión física
- El sensor de pH se conecta al ESP32:  
        ¬ **TX del Atlas → Pin RX2 (GPIO16) del ESP32**  
        ¬ **RX del Atlas → Pin TX2 (GPIO17) del ESP32**  
        ¬ **VCC/GND según especificaciones**

- El sensor de TDS se conecta al ESP32:
        ¬ **VCC → 3.3V**
        ¬ **GND → GND**
        ¬ **Señal → GPIO 35**
