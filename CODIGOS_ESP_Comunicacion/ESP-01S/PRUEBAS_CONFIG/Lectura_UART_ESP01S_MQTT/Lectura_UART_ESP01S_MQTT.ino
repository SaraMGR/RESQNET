#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> // Necesario para deserializar y serializar

// --- WiFi ---
const char* ssid = "iPhone de Sarak";
const char* password = "kj6yic2ufhf9k";

// --- MQTT ---
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_client_id = "ESP01S_Resqnet_Router_Client";

// --- TOPICS REALES (Los destinos finales) ---
const char* T_PERSONA       = "resqnet/persona";
const char* T_HUELLA        = "resqnet/huella";
const char* T_NODO          = "resqnet/nodo";
const char* T_AIRE          = "resqnet/aire";
const char* T_ACELEROMETRO  = "resqnet/acelerometro";


WiFiClient espClient;
PubSubClient client(espClient);

String incomingData = ""; // Buffer para datos seriales


// ---------------- CONFIGURACIONES COMUNES ----------------
void setup_wifi() {
    delay(10);
    Serial.begin(115200); // Misma velocidad que la Nucleo

    // ... (Código de conexión Wi-Fi igual que antes)
    Serial.print("Conectando a ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.println("\nWiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

void reconnect_mqtt() {
    // ... (Código de reconexión MQTT igual que antes)
    while (!client.connected()) {
        Serial.print("Intentando conexión MQTT... ");
        if (client.connect(mqtt_client_id)) {
            Serial.println("Conectado.");
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


// -------------- PROCESAR Y ENRUTAR DATOS UART --------------
void process_uart_data() {
    while (Serial.available()) {
        char inChar = (char)Serial.read();
        incomingData += inChar;

        // El mensaje termina con un salto de línea ('\n')
        if (inChar == '\n') {
            
            // Usamos un tamaño grande, ya que el JSON combinado es más grande
            StaticJsonDocument<512> doc_in;
            
            // Deserializar el JSON completo recibido de la Nucleo
            DeserializationError error = deserializeJson(doc_in, incomingData);
            
            if (error) {
                Serial.print(F("[ERROR] Falla al deserializar UART: "));
                Serial.println(error.f_str());
                incomingData = ""; // Limpiar el buffer
                return;
            }
            
            Serial.println("[INFO] JSON UART recibido y decodificado. Enrutando...");
            
            // Buffers para los 5 mensajes de salida
            StaticJsonDocument<256> doc_out;
            char payload_out[256];

            // Recuperar campos comunes (siempre deberían existir)
            int persona_id = doc_in["persona_id"].as<int>();
            int nodo_id = doc_in["nodo_id"].as<int>();
            const char* fecha = doc_in["fecha"] | "N/A";


            // =========================================================
            // 1. PUBLICAR PERSONA (T_PERSONA)
            // =========================================================
            doc_out.clear();
            doc_out["persona_id"] = persona_id;
            doc_out["nombre"]     = doc_in["nombre"];
            serializeJson(doc_out, payload_out);
            publish_json(T_PERSONA, payload_out);


            // =========================================================
            // 2. PUBLICAR EVENTO DE HUELLA (T_HUELLA)
            // =========================================================
            doc_out.clear();
            doc_out["persona_id"]  = persona_id;
            doc_out["tipo_evento"] = doc_in["huella_tipo_evento"];
            doc_out["nodo_id"]     = nodo_id;
            doc_out["fecha"]       = fecha;
            serializeJson(doc_out, payload_out);
            publish_json(T_HUELLA, payload_out);


            // =========================================================
            // 3. PUBLICAR NODO (T_NODO)
            // =========================================================
            doc_out.clear();
            doc_out["nodo_id"] = nodo_id;
            serializeJson(doc_out, payload_out);
            publish_json(T_NODO, payload_out);


            // =========================================================
            // 4. PUBLICAR SENSOR DE AIRE (T_AIRE)
            // =========================================================
            doc_out.clear();
            doc_out["nodo_id"]     = nodo_id;
            doc_out["temperatura"] = doc_in["aire_temperatura"];
            doc_out["humedad"]     = doc_in["aire_humedad"];
            doc_out["aqi"]         = doc_in["aire_aqi"];
            doc_out["tvoc"]        = doc_in["aire_tvoc"];
            doc_out["eco2"]        = doc_in["aire_eco2"];
            doc_out["tipo_evento"] = "lectura"; // Asumimos constante
            doc_out["fecha"]       = fecha;
            serializeJson(doc_out, payload_out);
            publish_json(T_AIRE, payload_out);


            // =========================================================
            // 5. PUBLICAR ACELERÓMETRO (T_ACELEROMETRO)
            // =========================================================
            doc_out.clear();
            doc_out["nodo_id"]     = nodo_id;
            doc_out["x"]           = doc_in["acel_x"];
            doc_out["y"]           = doc_in["acel_y"];
            doc_out["z"]           = doc_in["acel_z"];
            doc_out["tipo_evento"] = "vibracion"; // Asumimos constante
            doc_out["fecha"]       = fecha;
            serializeJson(doc_out, payload_out);
            publish_json(T_ACELEROMETRO, payload_out);

            // Limpiar el buffer para el siguiente mensaje de la Nucleo
            incomingData = "";
        }
    }
}


// ---------------- SETUP ----------------
void setup() {
    Serial.begin(115200); 

    client.setServer(mqtt_server, mqtt_port);
    setup_wifi();
}


// ---------------- LOOP ----------------
void loop() {

    if (!client.connected()) {
        reconnect_mqtt();
    }

    client.loop();

    // Tarea asíncrona: Leer el puerto UART (en espera del JSON de la Nucleo)
    // Esto se ejecutará cada vez que la Nucleo envíe un mensaje (cada 4s)
    process_uart_data();
}