#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "HardwareSerial.h"

// ----------------------------------------------------
// --- Configuración de Wi-Fi ---
const char* ssid = "iPhone de Sarak";
const char* password = "kj6yic2ufhf9k";

// --- Configuración de MQTT ---
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic_huella = "resqnet/huella";
const char* mqtt_client_id = "ESP32_Nucleo_Live_Client"; 

// --- Configuración de UART (Comunicación con Nucleo) ---
#define RXD2 16 // Pin RX para UART2 (Conectar al TX de la Nucleo)
#define TXD2 17 // Pin TX para UART2 (Conectar al RX de la Nucleo)
#define BAUD_RATE_NUCLEO 115200 

// ----------------------------------------------------
// Inicialización de clientes
WiFiClient espClient;
PubSubClient client(espClient);

// Buffer para la lectura UART
String inputString = "";         
bool stringComplete = false;     

// ----------------------------------------------------

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

// 3. Manejar y Publicar Datos UART
void handle_uart_data() {
    // Si la cadena está completa (recibió \n)
    if (stringComplete) {
        Serial.print("Datos UART Recibidos: ");
        Serial.println(inputString);
        
        // Formato esperado: PersonasActivas|ID|Estado|Nombre (ej: 5|12|IN|JORGE\r\n)

        // Crear una copia para usar con strtok
        char tempStr[inputString.length() + 1];
        inputString.toCharArray(tempStr, sizeof(tempStr));
        
        // 1. Personas Activas
        char *token = strtok(tempStr, "|");
        int personas_activas = token ? atoi(token) : 0;
        
        // 2. ID
        token = strtok(NULL, "|");
        int id_usuario = token ? atoi(token) : 0;

        // 3. Estado (IN/OUT)
        token = strtok(NULL, "|");
        String estado = token ? String(token) : "ERR";
        estado.trim(); 

        // 4. NOMBRE 
        token = strtok(NULL, "|");
        String nombre = token ? String(token) : "N/A";
        nombre.trim();
        
        // --- Creación del JSON ---
        StaticJsonDocument<256> doc; // Se aumenta el tamaño
        doc["personas_activas"] = personas_activas;
        doc["id_usuario"] = id_usuario;
        doc["estado"] = estado;
        doc["nombre"] = nombre; 
        
        char payload[256];
        size_t len = serializeJson(doc, payload);

        // --- Publicación MQTT ---
        if (client.publish(mqtt_topic_huella, payload, len)) {
            Serial.print("Publicado HUELLA a '");
            Serial.print(mqtt_topic_huella);
            Serial.print("': ");
            Serial.println(payload);
        } else {
            Serial.println("Error al publicar HUELLA.");
        }

        // Limpiar la cadena para la siguiente lectura
        inputString = "";
        stringComplete = false;
    }
}

// --- Rutina de lectura UART (Se ejecuta continuamente en el loop) ---
void serialEvent2() {
    while (Serial2.available()) {
        char inChar = (char)Serial2.read();
        inputString += inChar;

        // El carácter de fin de línea de la Nucleo es '\n' (después de '\r')
        if (inChar == '\n') {
            stringComplete = true;
        }
    }
}

// --- Setup principal ---
void setup() {
    Serial.begin(115200); 
    // Inicializa la UART2 para la comunicación con la Nucleo
    Serial2.begin(BAUD_RATE_NUCLEO, SERIAL_8N1, RXD2, TXD2); 
    delay(100);

    client.setServer(mqtt_server, mqtt_port);
    setup_wifi(); 
}

// --- Loop principal ---
void loop() {
    if (!client.connected()) {
        reconnect_mqtt(); 
    }
    client.loop(); 

    // Revisar y procesar datos UART entrantes
    serialEvent2();
    handle_uart_data();
}