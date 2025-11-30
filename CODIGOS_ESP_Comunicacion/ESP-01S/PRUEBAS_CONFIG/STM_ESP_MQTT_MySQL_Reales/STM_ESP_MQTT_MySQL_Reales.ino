#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// --- WiFi ---
const char* ssid = "VivianM";
const char* password = "Vivimormal286";

// --- MQTT ---
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_client_id = "ESP01S_Resqnet_Router_Client";

// --- TOPICS ---
const char* T_NODO = "resqnet/nodo";
const char* T_AIRE = "resqnet/aire";
const char* T_ACELEROMETRO = "resqnet/acelerometro";
const char* T_ALERTAS_NODOS = "resqnet/alertas_nodos";  // Alertas INDIVIDUALES
const char* T_ALERTAS = "resqnet/alertas";              // Alertas COMUNES

const char* TOPICS[] = {T_NODO, T_AIRE, T_ACELEROMETRO, T_ALERTAS_NODOS, T_ALERTAS};
const int NUM_TOPICS = sizeof(TOPICS) / sizeof(TOPICS[0]);

WiFiClient espClient;
PubSubClient client(espClient);
String incomingData = "";
bool alertaActiva = false;

// ============================================================
// SETUP CONEXIONES
// ============================================================
void setup_wifi() {
    delay(10);
    Serial.print("Conectando a ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { 
        delay(500); 
        Serial.print("."); 
    }
    Serial.println("\nWiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

void reconnect_mqtt() {
    while (!client.connected()) {
        Serial.print("Intentando conexión MQTT... ");
        if (client.connect(mqtt_client_id)) {
            Serial.println("Conectado.");
            for (int i = 0; i < NUM_TOPICS; i++) {
                client.subscribe(TOPICS[i]);
                Serial.print("✓ Suscrito a: ");
                Serial.println(TOPICS[i]);
            }
        } else {
            Serial.print("Fallo rc=");
            Serial.print(client.state());
            Serial.println(" esperando 5 segundos...");
            delay(5000);
        }
    }
}

void publish_json(const char* topic, const char* payload) {
    if (client.publish(topic, payload)) {
        Serial.print("[OK] Enviado a ");
        Serial.print(topic);
        Serial.print(" → ");
        Serial.println(payload);
    } else {
        Serial.print("[ERROR] No se pudo enviar a ");
        Serial.println(topic);
    }
}

// ============================================================
// PARSING DE DATOS
// ============================================================
String extractValue(String data, String key) {
    int startIdx = data.indexOf(key + ":");
    if (startIdx == -1) return "";
    
    startIdx += key.length() + 1;
    int endIdx = data.indexOf(";", startIdx);
    if (endIdx == -1) endIdx = data.length();
    
    return data.substring(startIdx, endIdx);
}

String extractNodeName(String data) {
    int startIdx = data.indexOf("[");
    int endIdx = data.indexOf("]");
    
    if (startIdx != -1 && endIdx != -1 && endIdx > startIdx) {
        return data.substring(startIdx + 1, endIdx);
    }
    return "";
}

String extractNodeNumberFromAlert(String data) {
    // "🚨 ALERTA NODO #:" → extraer el número después de "NODO "
    int nodoIdx = data.indexOf("NODO ");
    if (nodoIdx == -1) return "";
    
    nodoIdx += 5; // Avanzar después de "NODO "
    
    // Buscar el siguiente carácter que sea un dígito
    for (int i = nodoIdx; i < data.length(); i++) {
        if (isdigit(data.charAt(i))) {
            return String(data.charAt(i));
        }
    }
    return "";
}

String extractAlertMessage(String data) {
    int colonIdx = data.lastIndexOf(":"); // Último ":" para capturar el mensaje
    if (colonIdx == -1) return "";
    
    String alertMsg = data.substring(colonIdx + 1);
    alertMsg.trim();
    alertMsg.replace("\r", "");
    alertMsg.replace("\n", "");
    
    return alertMsg;
}

bool isAlertaNodo(String data) {
    return data.indexOf("ALERTA NODO") != -1;
}

bool isAlertaComun(String data) {
    return (data.indexOf("ALERTA_GENERAL") != -1 || 
            data.indexOf("ALERTA COMÚN") != -1);
}

// ============================================================
// PROCESAR ALERTAS INDIVIDUALES (NODO)
// ============================================================
void process_alerta_nodo(String data) {
    Serial.println("[ALERTA NODO] Detectada alerta individual");
    alertaActiva = true;
    
    String nodoNum = extractNodeNumberFromAlert(data);
    String alertMessage = extractAlertMessage(data);
    
    // PUBLICAR SOLO EN ALERTAS_NODOS (individual del nodo)
    StaticJsonDocument<256> doc_alertas_nodos;
    char payload_alertas_nodos[256];
    
    doc_alertas_nodos["tipo_evento"] = alertMessage;
    doc_alertas_nodos["nodo"] = nodoNum.toInt();
    serializeJson(doc_alertas_nodos, payload_alertas_nodos);
    publish_json(T_ALERTAS_NODOS, payload_alertas_nodos);
}

// ============================================================
// PROCESAR ALERTAS COMUNES
// ============================================================
void process_alerta_comun(String data) {
    Serial.println("[ALERTA COMÚN] Detectada alerta general");
    alertaActiva = true;
    
    String alertMessage = extractAlertMessage(data);
    
    // PUBLICAR SOLO EN ALERTAS (general/común)
    StaticJsonDocument<128> doc_alertas;
    char payload_alertas[128];
    
    doc_alertas["evento"] = alertMessage;
    serializeJson(doc_alertas, payload_alertas);
    publish_json(T_ALERTAS, payload_alertas);
}

// ============================================================
// PROCESAR DATOS UART
// ============================================================
void process_uart_data() {
    while (Serial.available()) {
        char inChar = (char)Serial.read();
        incomingData += inChar;

        if (inChar == '\n') {
            
            // 1️⃣ DETECTAR Y PROCESAR ALERTAS COMUNES
            if (isAlertaComun(incomingData)) {
                process_alerta_comun(incomingData);
                incomingData = "";
                return;
            }
            
            // 2️⃣ DETECTAR Y PROCESAR ALERTAS INDIVIDUALES DE NODOS
            if (isAlertaNodo(incomingData)) {
                process_alerta_nodo(incomingData);
                incomingData = "";
                return;
            }
            
            // 3️⃣ PROCESAR DATOS DE SENSORES SOLO SI NO HAY ALERTA
            if (!alertaActiva) {
                String nodo_label = extractNodeName(incomingData);
                float acel_x = extractValue(incomingData, "X").toFloat();
                float acel_y = extractValue(incomingData, "Y").toFloat();
                float acel_z = extractValue(incomingData, "Z").toFloat();
                float temperatura = extractValue(incomingData, "Temp").toFloat();
                float humedad = extractValue(incomingData, "Hum").toFloat();
                int aqi = extractValue(incomingData, "AQI").toInt();
                int tvoc = extractValue(incomingData, "TVOC").toInt();
                float eco2 = extractValue(incomingData, "eCO2").toFloat();
                float prom_eco2 = extractValue(incomingData, "PromECO2").toFloat();
                float prom_xy = extractValue(incomingData, "PromXY").toFloat();
                
                int nodo_id = nodo_label.substring(nodo_label.lastIndexOf("_") + 1).toInt();
                
                StaticJsonDocument<256> doc_out;
                char payload_out[256];

                // PUBLICAR NODO
                doc_out.clear();
                doc_out["nodo_id"] = nodo_id;
                serializeJson(doc_out, payload_out);
                publish_json(T_NODO, payload_out);

                // PUBLICAR AIRE
                doc_out.clear();
                doc_out["nodo_id"] = nodo_id;
                doc_out["temperatura"] = temperatura;
                doc_out["humedad"] = humedad;
                doc_out["aqi"] = aqi;
                doc_out["tvoc"] = tvoc;
                doc_out["eco2"] = eco2;
                doc_out["prom_eco2"] = prom_eco2;
                serializeJson(doc_out, payload_out);
                publish_json(T_AIRE, payload_out);

                // PUBLICAR ACELERÓMETRO
                doc_out.clear();
                doc_out["nodo_id"] = nodo_id;
                doc_out["x"] = acel_x;
                doc_out["y"] = acel_y;
                doc_out["z"] = acel_z;
                doc_out["prom_xy"] = prom_xy;
                serializeJson(doc_out, payload_out);
                publish_json(T_ACELEROMETRO, payload_out);
            } else {
                alertaActiva = false; // Resetear flag después de un ciclo
            }

            incomingData = "";
        }
    }
}

// ============================================================
// SETUP Y LOOP
// ============================================================
void setup() {
    Serial.begin(115200); 
    delay(1000);
    
    Serial.println("\n\n========== ESP8266 INICIANDO ==========");
    client.setServer(mqtt_server, mqtt_port);
    setup_wifi();
    reconnect_mqtt();
    Serial.println("========== LISTO PARA RECIBIR DATOS Y ALERTAS ==========\n");
}

void loop() {
    if (!client.connected()) {
        reconnect_mqtt();
    }
    client.loop();
    process_uart_data();
}