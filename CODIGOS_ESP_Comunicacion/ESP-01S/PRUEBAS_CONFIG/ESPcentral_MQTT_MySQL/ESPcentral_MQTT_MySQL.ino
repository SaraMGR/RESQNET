#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// --- WiFi ---
const char* ssid = "iPhone de Sarak";
const char* password = "kj6yic2ufhf9k";

// --- MQTT ---
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_client_id = "ESP01S_Resqnet_Client";

// --- TOPICS REALES USADOS POR TU PYTHON ---
const char* T_PERSONA       = "resqnet/persona";
const char* T_HUELLA        = "resqnet/huella";
const char* T_NODO          = "resqnet/nodo";
const char* T_AIRE          = "resqnet/aire";
const char* T_ACELEROMETRO  = "resqnet/acelerometro";

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
const int publishInterval = 4000;



// ---------------- WIFI ----------------
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


// -------------- MQTT RECONNECT --------------
void reconnect_mqtt() {
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


// -------------- PUBLICACIÓN GENÉRICA --------------
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



// ============= PUBLICACIÓN DE TODOS LOS DATOS =============
void publish_all_data() {

  StaticJsonDocument<256> doc;
  char payload[256];

  // =========================================================
  // 1. PUBLICAR PERSONA
  // =========================================================
  doc.clear();
  doc["persona_id"] = 12;
  doc["nombre"]     = "UsuarioDemo";
  serializeJson(doc, payload);
  publish_json(T_PERSONA, payload);


  // =========================================================
  // 2. PUBLICAR EVENTO DE HUELLA (ENTRADA / SALIDA)
  // =========================================================
  doc.clear();
  doc["persona_id"]  = 12;
  doc["tipo_evento"] = "entrada";
  doc["nodo_id"]     = 1;
  doc["fecha"]       = "2025-11-24 12:00:00";
  serializeJson(doc, payload);
  publish_json(T_HUELLA, payload);


  // =========================================================
  // 3. PUBLICAR NODO
  // =========================================================
  doc.clear();
  doc["nodo_id"] = 1;
  serializeJson(doc, payload);
  publish_json(T_NODO, payload);


  // =========================================================
  // 4. PUBLICAR SENSOR DE AIRE
  // =========================================================
  doc.clear();
  doc["nodo_id"]     = 1;
  doc["temperatura"] = 24.5;
  doc["humedad"]     = 55.2;
  doc["aqi"]         = 45;
  doc["tvoc"]        = 320;
  doc["eco2"]        = 650;
  doc["tipo_evento"] = "lectura";
  doc["fecha"]       = "2025-11-24 12:00:05";
  serializeJson(doc, payload);
  publish_json(T_AIRE, payload);


  // =========================================================
  // 5. PUBLICAR ACELERÓMETRO
  // =========================================================
  doc.clear();
  doc["nodo_id"]     = 1;
  doc["x"]           = 0.02;
  doc["y"]           = -0.01;
  doc["z"]           = 0.98;
  doc["tipo_evento"] = "vibracion";
  doc["fecha"]       = "2025-11-24 12:00:07";
  serializeJson(doc, payload);
  publish_json(T_ACELEROMETRO, payload);
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

  long now = millis();
  if (now - lastMsg > publishInterval) {
      lastMsg = now;

      // Envía los 5 mensajes como tu Python los espera
      publish_all_data();
  }
}
