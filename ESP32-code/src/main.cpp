#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>

// Declaraciones de funciones
void handleRoot();
void handleSave();
void connectWiFi();
void connectMQTT();
float readPH();
float readTemperature();
float readTDS();
void publishMetrics(float ph, float temperatura, float tds);

// Pines y variables de hardware
#define PH_PIN 34
#define TDS_PIN 35
#define VREF 3.3      // Voltaje de referencia del ESP32
#define SCOUNT 30     // Número de muestras para promedio

float valorPh = 7.0;
float temperaturaAnterior = 25.0;
float valorTdsAnterior = 300.0;

// Buffer para lecturas analógicas del TDS
int bufferAnalogico[SCOUNT];
int indiceBuffer = 0;

// Variables de configuración
String redWiFi = "";
String claveWiFi = "";
String servidorMqtt = "";
int puertoMqtt = 1883;

// Objetos globales
WebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Flags
bool configuracionRecibida = false;

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600); // Para comunicación con Atlas Scientific EZO-pH

  // Configurar pines analógicos
  pinMode(TDS_PIN, INPUT);
  
  // Inicializar buffer
  for (int i = 0; i < SCOUNT; i++) {
    bufferAnalogico[i] = 0;
  }

  // Inicia punto de acceso
  WiFi.softAP("ESP32_Config", "12345678");
  Serial.println("Punto de acceso iniciado: SSID=ESP32_Config, PASS=12345678");

  // Configurar servidor web
  server.on("/", handleRoot);
  server.on("/guardar", HTTP_POST, handleSave);
  server.begin();
  Serial.println("Servidor web iniciado");
}

void loop() {
  server.handleClient();

  if (configuracionRecibida) {
    if (WiFi.status() != WL_CONNECTED) connectWiFi();
    if (!mqttClient.connected()) connectMQTT();
    mqttClient.loop();

    // Leer sensores y publicar cada 5 segundos
    static unsigned long ultimoEnvio = 0;
    if (millis() - ultimoEnvio > 5000) {
      ultimoEnvio = millis();
      float ph = readPH();
      float temperatura = readTemperature();
      float tds = readTDS();
      publishMetrics(ph, temperatura, tds);
    }
  }
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Configuracion ESP32 - Piletas</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: Arial, sans-serif;
            min-height: 100vh;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 50%, #87ceeb 100%);
            display: flex;
            justify-content: center;
            align-items: center;
        }

        .contenedor-principal {
            background: rgba(255, 255, 255, 0.95);
            padding: 2rem;
            border-radius: 15px;
            box-shadow: 0 8px 25px rgba(0, 0, 0, 0.2);
            width: 100%;
            max-width: 400px;
            backdrop-filter: blur(10px);
        }

        .titulo {
            text-align: center;
            color: #1e3c72;
            margin-bottom: 1.5rem;
            font-size: 1.5rem;
            font-weight: bold;
        }

        .grupo-campo {
            margin-bottom: 1rem;
        }

        .etiqueta {
            display: block;
            margin-bottom: 0.5rem;
            color: #2a5298;
            font-weight: 500;
        }

        .campo-entrada {
            width: 100%;
            padding: 0.75rem;
            border: 2px solid #87ceeb;
            border-radius: 8px;
            font-size: 1rem;
            transition: all 0.3s ease;
            background: rgba(255, 255, 255, 0.9);
        }

        .campo-entrada:focus {
            outline: none;
            border-color: #2a5298;
            box-shadow: 0 0 10px rgba(42, 82, 152, 0.3);
            background: white;
        }

        .boton-guardar {
            width: 100%;
            padding: 0.75rem;
            background: linear-gradient(135deg, #2a5298, #1e3c72);
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 1rem;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.3s ease;
            margin-top: 1rem;
        }

        .boton-guardar:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(42, 82, 152, 0.4);
        }

        .boton-guardar:active {
            transform: translateY(0);
        }

        .icono-agua {
            text-align: center;
            font-size: 3rem;
            color: #2a5298;
            margin-bottom: 1rem;
        }

        @media (max-width: 480px) {
            .contenedor-principal {
                margin: 1rem;
                padding: 1.5rem;
            }
            
            .titulo {
                font-size: 1.3rem;
            }
        }
    </style>
</head>
<body>
    <div class="contenedor-principal">
        <div class="icono-agua">💧</div>
        <h1 class="titulo">Configuracion ESP32</h1>
        
        <form action="/guardar" method="POST">
            <div class="grupo-campo">
                <label class="etiqueta" for="ssid">SSID WiFi:</label>
                <input type="text" id="ssid" name="ssid" class="campo-entrada" required>
            </div>
            
            <div class="grupo-campo">
                <label class="etiqueta" for="pass">Contrasena:</label>
                <input type="password" id="pass" name="pass" class="campo-entrada" required>
            </div>
            
            <div class="grupo-campo">
                <label class="etiqueta" for="broker">MQTT Broker IP:</label>
                <input type="text" id="broker" name="broker" class="campo-entrada" placeholder="192.168.1.100" required>
            </div>
            
            <div class="grupo-campo">
                <label class="etiqueta" for="port">MQTT Broker Puerto:</label>
                <input type="number" id="port" name="port" class="campo-entrada" value="1883" required>
            </div>
            
            <button type="submit" class="boton-guardar">Guardar</button>
        </form>
    </div>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleSave() {
  String nuevoSSID = server.arg("ssid");
  String nuevaClave = server.arg("pass");
  String nuevoBroker = server.arg("broker");
  String puertoStr = server.arg("port");
  
  // Validaciones
  if (nuevoSSID.length() == 0) {
    server.send(400, "text/html", "<html><body><h2>Error: SSID no puede estar vacío</h2></body></html>");
    return;
  }
  
  if (nuevaClave.length() < 8) {
    server.send(400, "text/html", "<html><body><h2>Error: Contraseña debe tener al menos 8 caracteres</h2></body></html>");
    return;
  }
  
  if (nuevoBroker.length() == 0) {
    server.send(400, "text/html", "<html><body><h2>Error: IP del broker no puede estar vacía</h2></body></html>");
    return;
  }
  
  int nuevoPuerto = puertoStr.toInt();
  if (nuevoPuerto <= 0 || nuevoPuerto > 65535) {
    server.send(400, "text/html", "<html><body><h2>Error: Puerto debe ser entre 1 y 65535</h2></body></html>");
    return;
  }
  
  // Si todas las validaciones pasan, guardar datos
  redWiFi = nuevoSSID;
  claveWiFi = nuevaClave;
  servidorMqtt = nuevoBroker;
  puertoMqtt = nuevoPuerto;

  Serial.println("Datos recibidos y validados:");
  Serial.println("SSID: " + redWiFi);
  Serial.println("PASS: " + claveWiFi);
  Serial.println("Broker: " + servidorMqtt);
  Serial.println("Puerto: " + String(puertoMqtt));

  server.send(200, "text/html", "<html><body><h2>Datos guardados correctamente. Reiniciando conexión...</h2></body></html>");
  configuracionRecibida = true;
}

void connectWiFi() {
  Serial.println("Conectando a WiFi...");
  WiFi.begin(redWiFi.c_str(), claveWiFi.c_str());
  
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 20) {
    delay(500);
    Serial.print(".");
    intentos++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi conectado: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nFallo al conectar WiFi");
  }
}

void connectMQTT() {
  mqttClient.setServer(servidorMqtt.c_str(), puertoMqtt);
  
  int intentos = 0;
  while (!mqttClient.connected() && intentos < 5) {
    Serial.print("Conectando a MQTT...");
    if (mqttClient.connect("ESP32Client")) {
      Serial.println("Conectado al broker");
      return;
    } else {
      Serial.print("Fallo, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Reintentando...");
      intentos++;
      delay(2000);
    }
  }
  
  if (!mqttClient.connected()) {
    Serial.println("Error: No se pudo conectar a MQTT después de varios intentos");
  }
}

float readPH() {
  // Enviar comando para leer pH
  Serial2.print("R\r");
  delay(1000); // Esperar respuesta del sensor Atlas
  
  if (Serial2.available()) {
    String respuesta = Serial2.readStringUntil('\r');
    respuesta.trim();
    
    // Verificar si la respuesta es válida
    if (respuesta.length() > 0 && respuesta.charAt(0) != '*') {
      float ph = respuesta.toFloat();
      
      // Validar rango de pH
      if (ph >= 0 && ph <= 14) {
        valorPh = ph; // Actualizar valor anterior
        Serial.printf("Lectura pH: %.2f\n", ph);
        return ph;
      }
    }
  }
  
  // Si no hay respuesta válida, mantener último valor conocido
  Serial.println("Error leyendo pH, usando valor anterior");
  return valorPh;
}

float readTemperature() {
  // Enviar comando para leer temperatura del PT-1000
  Serial2.print("RT\r");
  delay(1000); // Esperar respuesta del sensor Atlas
  
  if (Serial2.available()) {
    String respuesta = Serial2.readStringUntil('\r');
    respuesta.trim();
    
    // Verificar si la respuesta es válida
    if (respuesta.length() > 0 && respuesta.charAt(0) != '*') {
      float temperatura = respuesta.toFloat();
      
      // Validar rango razonable de temperatura
      if (temperatura >= -10 && temperatura <= 60) {
        // Filtro simple para evitar lecturas erráticas
        if (abs(temperatura - temperaturaAnterior) <= 5) {
          temperaturaAnterior = temperatura;
          Serial.printf("Lectura Temperatura: %.1f°C\n", temperatura);
          return temperatura;
        }
      }
    }
  }
  
  // Si no hay respuesta válida, mantener último valor conocido
  Serial.println("Error leyendo temperatura, usando valor anterior");
  return temperaturaAnterior;
}

float readTDS() {
  // Leer valor analógico del sensor TDS
  bufferAnalogico[indiceBuffer] = analogRead(TDS_PIN);
  indiceBuffer = (indiceBuffer + 1) % SCOUNT;
  
  // Calcular promedio de las muestras
  long sumaBuffer = 0;
  for (int i = 0; i < SCOUNT; i++) {
    sumaBuffer += bufferAnalogico[i];
  }
  
  float promedioAnalogico = (float)sumaBuffer / SCOUNT;
  
  // Convertir a voltaje (ESP32 ADC: 0-4095 = 0-3.3V)
  float voltaje = (promedioAnalogico * VREF) / 4095.0;
  
  // Compensación por temperatura (coeficiente típico: 2%/°C)
  float coeficienteTemperatura = 1.0 + 0.02 * (temperaturaAnterior - 25.0);
  float voltajeCompensado = voltaje / coeficienteTemperatura;
  
  // Conversión a TDS en ppm (fórmula típica para sensores TDS genéricos)
  // Esta fórmula puede necesitar calibración según tu sensor específico
  float valorTds = (133.42 * voltajeCompensado * voltajeCompensado * voltajeCompensado 
                   - 255.86 * voltajeCompensado * voltajeCompensado 
                   + 857.39 * voltajeCompensado) * 0.5;
  
  // Validar rango razonable (0-2000 ppm para agua de pileta)
  if (valorTds >= 0 && valorTds <= 3000) {
    // Filtro para evitar cambios bruscos
    if (abs(valorTds - valorTdsAnterior) <= 100 || valorTdsAnterior == 300.0) {
      valorTdsAnterior = valorTds;
      Serial.printf("Lectura TDS: %.0f ppm (Voltaje: %.3fV)\n", valorTds, voltaje);
      return valorTds;
    }
  }
  
  // Si la lectura es inválida, mantener valor anterior
  Serial.println("Error leyendo TDS, usando valor anterior");
  return valorTdsAnterior;
}

void publishMetrics(float ph, float temperatura, float tds) {
  // Crear timestamp (segundos desde inicio)
  unsigned long tiempoActual = millis() / 1000;
  
  String payload = "{";
  payload += "\"ph\":" + String(ph, 2) + ",";
  payload += "\"temperature_c\":" + String(temperatura, 1) + ",";
  payload += "\"tds_ppm\":" + String(tds, 0) + ",";
  payload += "\"trend\":\"stable\",";
  payload += "\"trend_value\":0.0,";
  payload += "\"timestamp\":" + String(tiempoActual);
  payload += "}";
  
  mqttClient.publish("pool/metrics", payload.c_str());
  Serial.println("Publicado: " + payload);
}