#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// --- Configuración de Wi-Fi ---
const char* ssid = "iPhone de Sarak";
const char* password = "kj6yic2ufhf9k";

// --- Configuración de MQTT ---
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "proyecto/sensores";
const char* mqtt_client_id = "ESP01S_Nucleo_Client"; // ID único para el broker

// Inicialización de clientes
WiFiClient espClient;
PubSubClient client(espClient);

// Intervalo de publicación (2 segundos = 2000 ms)
long lastMsg = 0;
const int publishInterval = 2000;

// --- Funciones ---

// 1. Conexión Wi-Fi
void setup_wifi() {
    delay(10);
    Serial.print("Conectando a ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi conectado!");
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());
}

// 2. Reconexión MQTT
void reconnect_mqtt() {
    // Bucle hasta reconectar
    while (!client.connected()) {
        Serial.print("Intentando conexión MQTT...");

        // Intentamos conectar
        if (client.connect(mqtt_client_id)) {
            Serial.println("Conectado!");
        } else {
            Serial.print("Falló, rc=");
            Serial.print(client.state());
            Serial.println(" Intentando de nuevo en 5 segundos");
            // Esperar 5 segundos antes de reintentar
            delay(5000);
        }
    }
}

// 3. Simulación y Publicación de Datos
void publish_data() {
    // 1. Crear el documento JSON
    // El tamaño (200) es el espacio en memoria (en bytes) para el JSON
    StaticJsonDocument<200> doc; 

    // 2. Simular datos (Esto será reemplazado por la lectura de la Nucleo)
    doc["gas"] = round(random(30000, 60000) / 100.0); // 300.00 a 600.00
    doc["vibracion"] = round(random(0, 200) / 100.0); // 0.00 a 2.00
    doc["huella"] = random(1, 6); // 1 a 5

    // 3. Serializar (Convertir) el JSON a una cadena de texto (payload)
    char payload[200];
    serializeJson(doc, payload);

    unsigned int length = strlen(payload);

    // 4. Publicar la cadena
    // El QoS se establece en 2, como en tu código Python
    if (client.publish(mqtt_topic, (const uint8_t*)payload, length, true)) { // 'true' = retained
    // ----------------------------------------------------
        Serial.print("Publicado: ");
        Serial.println(payload);
    } else {
        Serial.println("Error al publicar.");
    }
}

// --- Setup principal (corre una vez) ---
void setup() {
    Serial.begin(115200); // Tasa de baudios para depuración
    delay(100);

    // Configurar cliente MQTT
    client.setServer(mqtt_server, mqtt_port);

    setup_wifi(); // Conectar a la red Wi-Fi
}

// --- Loop principal (corre continuamente) ---
void loop() {
    if (!client.connected()) {
        reconnect_mqtt(); // Asegurar conexión MQTT
    }
    client.loop(); // Mantener la conexión activa y procesar mensajes pendientes

    long now = millis();
    if (now - lastMsg > publishInterval) {
        lastMsg = now;
        publish_data(); // Publicar datos
    }
}