import os, json, time, random, math, logging
from datetime import datetime, timezone
import paho.mqtt.client as mqtt

# Configurar logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

# Variables de configuración
HOST = os.getenv("MQTT_HOST", "mosquitto")
PUERTO = int(os.getenv("MQTT_PORT", "1883"))
TOPICO = os.getenv("MQTT_TOPIC", "pool/metrics")
INTERVALO = int(os.getenv("SIM_INTERVAL_SECONDS", "10"))

PH_BASE = float(os.getenv("PH_BASE", "7.4"))
TEMP_BASE = float(os.getenv("TEMP_BASE_C", "25.0"))
TDS_BASE = float(os.getenv("TDS_BASE_PPM", "500"))

# Variables globales para control de conexión
cliente_conectado = False
intentos_reconexion = 0
MAX_INTENTOS_RECONEXION = 5

def al_conectar(cliente, userdata, flags, rc):
    """Callback cuando se establece conexión MQTT"""
    global cliente_conectado, intentos_reconexion
    if rc == 0:
        cliente_conectado = True
        intentos_reconexion = 0
        logger.info(f"✅ Conectado al broker MQTT: {HOST}:{PUERTO}")
    else:
        cliente_conectado = False
        logger.error(f"❌ Error de conexión MQTT. Código: {rc}")

def al_desconectar(cliente, userdata, rc):
    """Callback cuando se pierde la conexión MQTT"""
    global cliente_conectado
    cliente_conectado = False
    if rc != 0:
        logger.warning("⚠️ Desconexión inesperada del broker MQTT")
    else:
        logger.info("📡 Desconectado del broker MQTT")

def al_publicar(cliente, userdata, mid):
    """Callback cuando se confirma publicación"""
    logger.debug(f"📤 Mensaje publicado. ID: {mid}")

def reconectar_mqtt(cliente):
    """Intenta reconectar al broker MQTT"""
    global intentos_reconexion, cliente_conectado
    
    intentos_reconexion += 1
    logger.info(f"🔄 Intento de reconexión #{intentos_reconexion}")
    
    if intentos_reconexion > MAX_INTENTOS_RECONEXION:
        logger.error(f"💥 Se alcanzó el máximo de intentos de reconexión ({MAX_INTENTOS_RECONEXION})")
        return False
    
    try:
        cliente.reconnect()
        return True
    except Exception as e:
        logger.error(f"❌ Error al reconectar: {e}")
        tiempo_espera = min(30, 5 * intentos_reconexion)  # Backoff exponencial limitado
        logger.info(f"⏳ Esperando {tiempo_espera}s antes del siguiente intento...")
        time.sleep(tiempo_espera)
        return False

def crear_cliente_mqtt():
    """Crear y configurar cliente MQTT"""
    cliente = mqtt.Client(client_id="sim-pileta", clean_session=True)
    
    # Configurar callbacks
    cliente.on_connect = al_conectar
    cliente.on_disconnect = al_desconectar
    cliente.on_publish = al_publicar
    
    # Configurar opciones de conexión
    cliente.reconnect_delay_set(min_delay=1, max_delay=120)
    
    return cliente

def generar_lectura(ph_previo, angulo):
    """Generar datos de sensores simulados"""
    # Variaciones suaves: seno + ruido
    ph = PH_BASE + 0.15 * math.sin(angulo) + random.uniform(-0.05, 0.05)
    temperatura = TEMP_BASE + 1.5 * math.sin(angulo/3) + random.uniform(-0.3, 0.3)
    tds = TDS_BASE + 30 * math.sin(angulo/4) + random.uniform(-10, 10)

    # Tendencia basada en diferencia actual vs previa
    delta = ph - ph_previo
    if delta > 0.01:
        tendencia = "subiendo"
        valor_tendencia = 1
    elif delta < -0.01:
        tendencia = "bajando" 
        valor_tendencia = -1
    else:
        tendencia = "estable"
        valor_tendencia = 0
    
    payload = {
        "ph": round(ph, 2),
        "temperature_c": round(temperatura, 2),
        "tds_ppm": int(round(tds)),
        "trend": tendencia,
        "trend_value": valor_tendencia,
        "timestamp": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    }
    
    return payload, ph

def main():
    """Función principal del simulador"""
    global cliente_conectado
    
    logger.info("🚀 Iniciando simulador de métricas de piscina...")
    
    # Crear cliente MQTT
    cliente = crear_cliente_mqtt()
    
    # Conectar inicialmente
    try:
        logger.info(f"🔌 Conectando a {HOST}:{PUERTO}...")
        cliente.connect(HOST, PUERTO, keepalive=60)
        cliente.loop_start()  # ¡IMPORTANTE! Inicia el bucle de red en segundo plano
    except Exception as e:
        logger.error(f"❌ Error inicial de conexión: {e}")
        return
    
    # Esperar conexión inicial
    tiempo_espera = 0
    while not cliente_conectado and tiempo_espera < 10:
        time.sleep(0.5)
        tiempo_espera += 0.5
    
    if not cliente_conectado:
        logger.error("💥 No se pudo establecer conexión inicial")
        return
    
    # Variables de simulación
    ph_previo = PH_BASE
    angulo = 0.0
    contador_lecturas = 0
    
    logger.info("📊 Comenzando envío de lecturas...")
    
    try:
        while True:
            # Verificar conexión y reconectar si es necesario
            if not cliente_conectado:
                logger.warning("⚠️ Conexión perdida, intentando reconectar...")
                if not reconectar_mqtt(cliente):
                    continue
            
            # Generar nueva lectura
            angulo += 0.2
            payload, ph_actual = generar_lectura(ph_previo, angulo)
            
            # Publicar lectura
            try:
                resultado = cliente.publish(TOPICO, json.dumps(payload), qos=1, retain=False)
                
                if resultado.rc == mqtt.MQTT_ERR_SUCCESS:
                    contador_lecturas += 1
                    logger.info(f"📤 Lectura #{contador_lecturas} enviada - pH: {payload['ph']}, Temp: {payload['temperature_c']}°C")
                else:
                    logger.error(f"❌ Error al publicar. Código: {resultado.rc}")
                    
            except Exception as e:
                logger.error(f"❌ Excepción al publicar: {e}")
            
            ph_previo = ph_actual
            time.sleep(INTERVALO)
            
    except KeyboardInterrupt:
        logger.info("⏹️ Deteniendo simulador...")
    except Exception as e:
        logger.error(f"💥 Error inesperado: {e}")
    finally:
        cliente.loop_stop()
        cliente.disconnect()
        logger.info("👋 Simulador terminado")

if __name__ == "__main__":
    main()