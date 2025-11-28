#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// --- ⚙️ Configuración de Wi-Fi ---
const char* ssid = "iPhone de Sarak";
const char* password = "kj6yic2ufhf9k";

// --- ⚙️ Configuración de MQTT ---
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
// Tópico ÚNICO para enviar el objeto JSON con AMBOS valores:
const char* mqtt_topic = "sensores/data_completa"; 
const char* mqtt_client_id = "ESP32_Data_Sender";

// --- 💡 Configuración del LED ---
const int ledPin = 2; // Pin del LED integrado del ESP32-WROOM-32
const int ledOnDuration = 100; // LED encendido por 100 milisegundos

// Inicialización de clientes
WiFiClient espClient;
PubSubClient client(espClient);

// Intervalo de publicación (3 segundos = 3000 ms)
long lastMsg = 0;
const int publishInterval = 3000; 

// ----------------------------------------------------

// 1. Conexión Wi-Fi (SIN CAMBIOS)
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
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());
}

// 2. Reconexión MQTT (SIN CAMBIOS)
void reconnect_mqtt() {
    while (!client.connected()) {
        Serial.print("Intentando conexión MQTT...");
        if (client.connect(mqtt_client_id)) {
            Serial.println("Conectado!");
        } else {
            Serial.print("Falló, rc=");
            Serial.print(client.state());
            Serial.println(" Intentando de nuevo en 5 segundos");
            delay(5000);
        }
    }
}

// 3. Simulación y Publicación de Datos (MODIFICADA)
void publish_data() {
    StaticJsonDocument<150> doc; // Aumentamos el tamaño del documento JSON

    // --- SIMULACIÓN DE GAS (150 - 700 ppm) ---
    int simulatedGasLevel = random(150, 700);

    // --- SIMULACIÓN DE VIBRACIÓN (0.1 - 2.5 G) ---
    // Generamos un número flotante aleatorio
    float simulatedVibration = (float)random(10, 250) / 100.0; 

    // Creamos un objeto JSON con AMBAS claves
    doc["gas_promedio"] = simulatedGasLevel;
    doc["vibration_promedio"] = simulatedVibration; // Nuevo valor de vibración/aceleración
    
    // Serializar el JSON
    char payload[150];
    serializeJson(doc, payload);

    // Publicar la cadena
    if (client.publish(mqtt_topic, payload)) {
        
        // INDICADOR LED: Parpadea al publicar
        digitalWrite(ledPin, HIGH); 
        delay(ledOnDuration); 
        digitalWrite(ledPin, LOW);
        
        Serial.print("Publicado en '");
        Serial.print(mqtt_topic);
        Serial.print("': ");
        Serial.println(payload);
    } else {
        Serial.println("Error al publicar.");
    }
}

// --- 🚀 Setup principal (SIN CAMBIOS) ---
void setup() {
    Serial.begin(115200); 
    delay(100);
    randomSeed(analogRead(0)); 

    pinMode(ledPin, OUTPUT); 

    client.setServer(mqtt_server, mqtt_port);

    setup_wifi(); 
}

// --- Loop principal (SIN CAMBIOS) ---
void loop() {
    if (!client.connected()) {
        reconnect_mqtt(); 
    }
    client.loop(); 

    long now = millis();
    if (now - lastMsg > publishInterval) {
        lastMsg = now;
        publish_data(); 
    }
}