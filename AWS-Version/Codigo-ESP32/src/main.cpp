#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <math.h>
#include "secretidirigillo.h"

// Declaraciones de funciones
void handleRoot();
void handleSave();
void connectWiFi();
void connectMQTT();
void configurarCertificados();
void generarLectura();
void publicarMetricas();

// Variables de configuración AWS desde secretidirigillo.h
const char* servidorMqtt = MQTT_HOST;
const int puertoMqtt = MQTT_PORT;
const char* topicoMqtt = MQTT_TOPIC;

// Variables de configuración WiFi
String redWiFi = "";
String claveWiFi = "";

// Objetos globales - usando WiFiClientSecure para conexión segura
WebServer servidor(80);
WiFiClientSecure clienteEspSeguro;
PubSubClient clienteMqtt(clienteEspSeguro);

// Variables de simulación
float phBase = 7.4;
float temperaturaBase = 25.0;
float tdsBase = 500.0;

float phAnterior = 7.4;
float anguloSimulacion = 0.0;
unsigned long contadorLecturas = 0;

// Variables de tiempo
unsigned long ultimoEnvio = 0;
const unsigned long intervaloEnvio = 60000; // 60 segundos

// Variables de control
bool configuracionRecibida = false;
bool clienteConectado = false;
bool certificadosConfigurados = false;
int intentosReconexion = 0;
const int maxIntentosReconexion = 5;

void setup() {
  Serial.begin(9600);
  
  Serial.println("🚀 Iniciando ESP32 Simulador de Pileta con AWS IoT...");

  // Configurar certificados para conexión segura
  configurarCertificados();

  // Inicia punto de acceso
  WiFi.softAP("ESP32_Pileta", "12345678");
  Serial.println("📡 Punto de acceso iniciado: SSID=ESP32_Pileta, PASS=12345678");
  Serial.print("📍 IP del AP: ");
  Serial.println(WiFi.softAPIP());

  // Configurar servidor web
  servidor.on("/", handleRoot);
  servidor.on("/guardar", HTTP_POST, handleSave);
  servidor.begin();
  Serial.println("🌐 Servidor web iniciado");
}

void loop() {
  servidor.handleClient();

  if (configuracionRecibida && certificadosConfigurados) {
    // Verificar conexión WiFi
    if (WiFi.status() != WL_CONNECTED) {
      connectWiFi();
    }
    
    // Verificar conexión MQTT
    if (!clienteMqtt.connected()) {
      connectMQTT();
    }
    
    clienteMqtt.loop();

    // Generar y enviar lecturas simuladas cada intervalo
    if (millis() - ultimoEnvio > intervaloEnvio) {
      ultimoEnvio = millis();
      generarLectura();
      publicarMetricas();
    }
  }
  
  delay(100); // Pequeña pausa para no saturar el procesador
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Configuracion ESP32 - Pileta Simulada</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Arial', sans-serif;
            min-height: 100vh;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 50%, #87ceeb 100%);
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 1rem;
        }

        .contenedorPrincipal {
            background: rgba(255, 255, 255, 0.95);
            padding: 2.5rem;
            border-radius: 20px;
            box-shadow: 0 15px 35px rgba(0, 0, 0, 0.3);
            width: 100%;
            max-width: 450px;
            backdrop-filter: blur(15px);
            border: 1px solid rgba(255, 255, 255, 0.2);
        }

        .iconoAgua {
            text-align: center;
            font-size: 4rem;
            margin-bottom: 1rem;
            background: linear-gradient(135deg, #2a5298, #87ceeb);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
        }

        .titulo {
            text-align: center;
            color: #1e3c72;
            margin-bottom: 0.5rem;
            font-size: 1.8rem;
            font-weight: bold;
        }

        .subtitulo {
            text-align: center;
            color: #2a5298;
            margin-bottom: 2rem;
            font-size: 0.9rem;
            opacity: 0.8;
        }

        .grupoCampo {
            margin-bottom: 1.5rem;
        }

        .etiqueta {
            display: block;
            margin-bottom: 0.7rem;
            color: #2a5298;
            font-weight: 600;
            font-size: 0.95rem;
        }

        .campoEntrada {
            width: 100%;
            padding: 1rem;
            border: 2px solid #e0e0e0;
            border-radius: 12px;
            font-size: 1rem;
            transition: all 0.3s ease;
            background: rgba(255, 255, 255, 0.9);
        }

        .campoEntrada:focus {
            outline: none;
            border-color: #2a5298;
            box-shadow: 0 0 20px rgba(42, 82, 152, 0.2);
            background: white;
            transform: translateY(-2px);
        }

        .botonGuardar {
            width: 100%;
            padding: 1rem;
            background: linear-gradient(135deg, #2a5298, #1e3c72);
            color: white;
            border: none;
            border-radius: 12px;
            font-size: 1.1rem;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.3s ease;
            margin-top: 1.5rem;
            text-transform: uppercase;
            letter-spacing: 1px;
        }

        .botonGuardar:hover {
            transform: translateY(-3px);
            box-shadow: 0 8px 25px rgba(42, 82, 152, 0.4);
        }

        .botonGuardar:active {
            transform: translateY(-1px);
        }

        .infoConexion {
            background: rgba(135, 206, 235, 0.1);
            padding: 1rem;
            border-radius: 10px;
            margin-bottom: 1.5rem;
            border-left: 4px solid #2a5298;
        }

        .infoConexion p {
            margin: 0.3rem 0;
            font-size: 0.85rem;
            color: #1e3c72;
        }

        .certificadosStatus {
            background: rgba(46, 204, 113, 0.1);
            padding: 0.8rem;
            border-radius: 8px;
            margin-bottom: 1rem;
            border-left: 4px solid #2ecc71;
        }

        .certificadosStatus p {
            margin: 0;
            font-size: 0.8rem;
            color: #27ae60;
            font-weight: 600;
        }

        @media (max-width: 480px) {
            .contenedorPrincipal {
                margin: 0.5rem;
                padding: 2rem;
            }
            
            .titulo {
                font-size: 1.5rem;
            }

            .iconoAgua {
                font-size: 3rem;
            }
        }
    </style>
</head>
<body>
    <div class="contenedorPrincipal">
        <div class="iconoAgua">🏊‍♂️</div>
        <h1 class="titulo">Pileta Simulada</h1>
        <p class="subtitulo">Sistema de monitoreo virtual conectado a AWS IoT Core</p>
        
        <div class="certificadosStatus">
            <p>🛡️ Conexión segura con certificados SSL/TLS configurados</p>
        </div>
        
        <div class="infoConexion">
            <p><strong>📡 Broker MQTT:</strong> AWS IoT Core (SSL/TLS)</p>
            <p><strong>📊 Datos:</strong> pH, Temperatura, TDS simulados</p>
            <p><strong>⏱️ Intervalo:</strong> 1 minuto</p>
        </div>
        
        <form action="/guardar" method="POST">
            <div class="grupoCampo">
                <label class="etiqueta" for="ssid">🌐 Red WiFi (SSID):</label>
                <input type="text" id="ssid" name="ssid" class="campoEntrada" 
                       placeholder="Nombre de tu red WiFi" required>
            </div>
            
            <div class="grupoCampo">
                <label class="etiqueta" for="pass">🔒 Contraseña WiFi:</label>
                <input type="password" id="pass" name="pass" class="campoEntrada" 
                       placeholder="Contraseña de la red" required>
            </div>
            
            <button type="submit" class="botonGuardar">Conectar y Simular</button>
        </form>
    </div>
</body>
</html>
)rawliteral";

  servidor.send(200, "text/html; charset=UTF-8", html);
}

void handleSave() {
  String nuevoSSID = servidor.arg("ssid");
  String nuevaClave = servidor.arg("pass");
  
  // Validaciones básicas
  if (nuevoSSID.length() == 0) {
    servidor.send(400, "text/html; charset=UTF-8", 
      "<html><body style='font-family:Arial;text-align:center;padding:2rem;'>"
      "<h2 style='color:#e74c3c;'>❌ Error</h2>"
      "<p>El SSID no puede estar vacío</p>"
      "<button onclick='history.back()'>Volver</button>"
      "</body></html>");
    return;
  }
  
  if (nuevaClave.length() < 8) {
    servidor.send(400, "text/html; charset=UTF-8", 
      "<html><body style='font-family:Arial;text-align:center;padding:2rem;'>"
      "<h2 style='color:#e74c3c;'>❌ Error</h2>"
      "<p>La contraseña debe tener al menos 8 caracteres</p>"
      "<button onclick='history.back()'>Volver</button>"
      "</body></html>");
    return;
  }
  
  // Validar que los certificados estén configurados
  if (!certificadosConfigurados) {
    servidor.send(500, "text/html; charset=UTF-8", 
      "<html><body style='font-family:Arial;text-align:center;padding:2rem;'>"
      "<h2 style='color:#e74c3c;'>❌ Error de Seguridad</h2>"
      "<p>Los certificados AWS IoT no están configurados correctamente</p>"
      "<button onclick='history.back()'>Volver</button>"
      "</body></html>");
    return;
  }
  
  // Guardar credenciales
  redWiFi = nuevoSSID;
  claveWiFi = nuevaClave;

  Serial.println("✅ Credenciales WiFi guardadas:");
  Serial.println("SSID: " + redWiFi);
  Serial.println("PASS: " + String(nuevaClave.length()) + " caracteres");

  servidor.send(200, "text/html; charset=UTF-8", 
    "<html><body style='font-family:Arial;text-align:center;padding:2rem;background:linear-gradient(135deg,#1e3c72,#87ceeb);color:white;'>"
    "<div style='background:rgba(255,255,255,0.1);padding:2rem;border-radius:15px;'>"
    "<h2>🎉 ¡Configuración Guardada!</h2>"
    "<p>Conectando a la red WiFi y comenzando simulación...</p>"
    "<p>Los datos se enviarán de forma segura a AWS IoT Core</p>"
    "<div style='margin-top:1rem;padding:1rem;background:rgba(46,204,113,0.2);border-radius:8px;'>"
    "<p>🛡️ Conexión SSL/TLS habilitada con certificados</p>"
    "</div>"
    "</div>"
    "</body></html>");
    
  configuracionRecibida = true;
}

void connectWiFi() {
  if (redWiFi.length() == 0) return;
  
  Serial.println("📡 Conectando a WiFi: " + redWiFi);
  WiFi.begin(redWiFi.c_str(), claveWiFi.c_str());
  
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 30) {
    delay(1000);
    Serial.print(".");
    intentos++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi conectado!");
    Serial.print("📍 IP asignada: ");
    Serial.println(WiFi.localIP());
        
    // Configurar sincronización de tiempo para certificados
    configTime(0, 0, "time.google.com", "time.windows.com");
    Serial.println("🕐 Sincronizando tiempo para validación de certificados...");

    struct tm timeinfo;
    while (!getLocalTime(&timeinfo)){
      Serial.println("⏳ Esperando hora NTP...");
      delay(1000);
    }
    Serial.println("✅ Hora sincronizada con NTP");
    Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
  }
}

void configurarCertificados() {
  Serial.println("🔐 Configurando certificados para AWS IoT...");
  
  try {
    // Configurar CA certificate (Root CA)
    clienteEspSeguro.setCACert(cacert);
    Serial.println("✅ Certificado CA configurado");
    
    // Configurar certificado del cliente
    clienteEspSeguro.setCertificate(client_cert);
    Serial.println("✅ Certificado de cliente configurado");
    
    // Configurar clave privada
    clienteEspSeguro.setPrivateKey(privkey);
    Serial.println("✅ Clave privada configurada");
    
    // Configurar timeout para conexiones
    clienteEspSeguro.setTimeout(10);
    
    certificadosConfigurados = true;
    Serial.println("🛡️ Configuración de certificados completada");
    
  } catch (...) {
    Serial.println("❌ Error configurando certificados");
    certificadosConfigurados = false;
  }
}

void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED || !certificadosConfigurados) {
    if (!certificadosConfigurados) {
      Serial.println("❌ No se puede conectar a MQTT: certificados no configurados");
    }
    return;
  }
  
  // Configurar servidor MQTT con puerto seguro
  clienteMqtt.setServer(servidorMqtt, puertoMqtt);
  
  intentosReconexion++;
  Serial.printf("🔄 Conectando a AWS IoT MQTT seguro (intento %d)...\n", intentosReconexion);
  Serial.printf("🔗 Servidor: %s:%d\n", servidorMqtt, puertoMqtt);
  
  String clienteId = THINGNAME;

  // Codigo para diferentes nombres
  //String clienteId = "ESP32_Pileta_" + String(random(0xffff), HEX);
  
  // Conexión con certificados (sin usuario/contraseña ya que usa certificados)
  if (clienteMqtt.connect(clienteId.c_str())) {
    clienteConectado = true;
    intentosReconexion = 0;
    Serial.println("✅ Conectado de forma segura al broker AWS IoT");
    Serial.printf("🆔 Cliente ID: %s\n", clienteId.c_str());
  } else {
    clienteConectado = false;
    Serial.printf("❌ Fallo conexión MQTT segura, código: %d\n", clienteMqtt.state());
    
    // Códigos de error específicos
    switch(clienteMqtt.state()) {
      case -4: Serial.println("   Timeout de conexión"); break;
      case -3: Serial.println("   Conexión perdida"); break;
      case -2: Serial.println("   Fallo en la conexión de red"); break;
      case -1: Serial.println("   Cliente desconectado"); break;
      case 1: Serial.println("   Versión de protocolo incorrecta"); break;
      case 2: Serial.println("   ID de cliente rechazado"); break;
      case 3: Serial.println("   Servidor no disponible"); break;
      case 4: Serial.println("   Credenciales incorrectas"); break;
      case 5: Serial.println("   No autorizado"); break;
      default: Serial.println("   Error desconocido"); break;
    }
    
    if (intentosReconexion >= maxIntentosReconexion) {
      Serial.println("💥 Máximo de intentos alcanzado, esperando...");
      delay(30000); // Esperar 30 segundos antes de reintentar
      intentosReconexion = 0;
    }
  }
}

void generarLectura() {
  // Incrementar ángulo para variaciones suaves
  anguloSimulacion += 0.2;
  
  // Generar variaciones realistas usando funciones senoidales + ruido
  float variacionPh = 0.15 * sin(anguloSimulacion) + random(-50, 51) / 1000.0; // ±0.05
  float variacionTemp = 1.5 * sin(anguloSimulacion / 3.0) + random(-30, 31) / 100.0; // ±0.3
  float variacionTds = 30.0 * sin(anguloSimulacion / 4.0) + random(-10, 11); // ±10
  
  // Calcular valores actuales
  float phActual = phBase + variacionPh;
  float temperaturaActual = temperaturaBase + variacionTemp;
  float tdsActual = tdsBase + variacionTds;
  
  // Asegurar rangos válidos
  phActual = constrain(phActual, 6.0, 8.5);
  temperaturaActual = constrain(temperaturaActual, 20.0, 35.0);
  tdsActual = constrain(tdsActual, 200, 1000);
  
  // Almacenar valores para la próxima iteración
  phAnterior = phActual;
  contadorLecturas++;
  
  // Mostrar en consola
  Serial.printf("📊 Lectura #%lu - pH: %.2f, Temp: %.1f°C, TDS: %.0f ppm\n", 
                contadorLecturas, phActual, temperaturaActual, tdsActual);
}

void publicarMetricas() {
  if (!clienteMqtt.connected()) return;

  // Calcular tendencia basada en variaciones
  String tendencia = "estable";
  int valorTendencia = 0;
  
  // Determinar tendencia basada en el ángulo de simulación
  float deltaSeno = sin(anguloSimulacion) - sin(anguloSimulacion - 0.2);
  if (deltaSeno > 0.01) {
    tendencia = "subiendo";
    valorTendencia = 1;
  } else if (deltaSeno < -0.01) {
    tendencia = "bajando";
    valorTendencia = -1;
  }

  // Se crea un documento JSON dinámico
  DynamicJsonDocument doc(256);

  // Carga de métricas
  doc["ph"] = phAnterior;
  doc["temperature_c"] = temperaturaBase + 1.5 * sin(anguloSimulacion / 3.0);
  doc["tds_ppm"] = (int)(tdsBase + 30.0 * sin(anguloSimulacion / 4.0));
  doc["trend"] = tendencia;
  doc["trend_value"] = valorTendencia;
  doc["timestamp"] = millis() / 1000;   // Tiempo en seg desde arranque
  doc["device_id"] = THINGNAME;

  // Serializar JSON en un string
  String payload;
  serializeJson(doc, payload);

  // Publicación MQTT
  if (clienteMqtt.publish(topicoMqtt, payload.c_str(), false)) {
    Serial.println("✅ Publicado en MQTT:");
    Serial.println(payload);
  } else {
    Serial.println("❌ Error publicando en MQTT");
  }
}


// ---------------- Función publicar metricas antigua sin ArduinoJson --------------
// void publicarMetricas() {
//   if (!clienteMqtt.connected()) {
//     Serial.println("⚠️ Cliente MQTT desconectado, no se puede publicar");
//     return;
//   }
//   
//   // Calcular tendencia basada en variaciones
//   String tendencia = "estable";
//   int valorTendencia = 0;
//   
//   // Determinar tendencia basada en el ángulo de simulación
//   float deltaSeno = sin(anguloSimulacion) - sin(anguloSimulacion - 0.2);
//   if (deltaSeno > 0.01) {
//     tendencia = "subiendo";
//     valorTendencia = 1;
//   } else if (deltaSeno < -0.01) {
//     tendencia = "bajando";
//     valorTendencia = -1;
//   }
//   
//   // Crear timestamp en segundos desde el inicio
//   unsigned long tiempoActual = millis() / 1000;
//   
//   // Construir payload JSON
//   String payload = "{";
//   payload += "\"ph\":" + String(phAnterior, 2) + ",";
//   payload += "\"temperature_c\":" + String(temperaturaBase + 1.5 * sin(anguloSimulacion / 3.0), 1) + ",";
//   payload += "\"tds_ppm\":" + String((int)(tdsBase + 30.0 * sin(anguloSimulacion / 4.0))) + ",";
//   payload += "\"trend\":\"" + tendencia + "\",";
//   payload += "\"trend_value\":" + String(valorTendencia) + ",";
//   payload += "\"timestamp\":" + String(tiempoActual) + ",";
//   payload += "\"device_id\":\"ESP32_Pileta_Secure\"";
//   payload += "}";
//   
//   // Publicar al tópico MQTT
//   if (clienteMqtt.publish(topicoMqtt, payload.c_str(), true)) { // retained = true
//     Serial.println("📤 Datos publicados de forma segura a AWS IoT: " + payload);
//   } else {
//     Serial.println("❌ Error al publicar datos de forma segura");
//   }
// }