#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Adafruit_NeoPixel.h>
#include <map>
#include <string>
#include <stdlib.h>

// --- CONFIGURACIÓN DE UUIDs (DEBEN COINCIDIR CON LOS NODOS) ---
static BLEUUID serviceUUID("4FAFC201-1FB5-459E-8FC7-C2D046D5E98C");
static BLEUUID charUUID("BDAA24FF-B980-4D30-A466-417A16773B1C");

// --- CONFIGURACIÓN DE NODOS OBJETIVO ---
const char* TARGET_NODE_NAMES[] = {
  "ESP_NODO_1",
  "ESP_NODO_2",
  "ESP_NODO_3"
};
const int NUM_TARGET_NODES = sizeof(TARGET_NODE_NAMES) / sizeof(TARGET_NODE_NAMES[0]);

// --- CONFIGURACIÓN LED RGB ---
#define LED_PIN 8
#define NUMPIXELS 1
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// --- ESTRUCTURAS DE DATOS PARA MÚLTIPLES CONEXIONES ---
std::map<std::string, BLEAdvertisedDevice*> advertisedDevices;
std::map<std::string, BLEClient*> activeClients;
std::map<std::string, BLERemoteCharacteristic*> activeCharacteristics;

// --- ESTRUCTURAS PARA ACUMULACIÓN DE DATOS ---
struct NodeData {
  String accel;
  String tempHum;
  String aqi;
  int receivedCount = 0;
};

std::map<std::string, NodeData> accumulatedData;

// Variables de estado
bool doConnect = true;
bool isScanning = false;

// --- VARIABLES PARA ALERTAS COMUNES (MEJORADAS) ---
static String lastAlertTypeNodo1 = "";
static String lastAlertTypeNodo2 = "";
static String lastAlertTypeNodo3 = "";

// Función auxiliar para color LED
void colorLED(int r, int g, int b) {
  pixels.setPixelColor(0, pixels.Color(r, g, b));
  pixels.show();
}

// ===========================================
// 🆕 FUNCIÓN PARA EXTRAER TIPO DE ALERTA (SIN VALORES NUMÉRICOS)
// ===========================================
String extractAlertType(String alertMessage) {
  // Eliminar espacios al inicio/final
  alertMessage.trim();
  
  // Buscar el primer paréntesis (si existe)
  int parenPos = alertMessage.indexOf('(');
  
  if (parenPos != -1) {
    // Si hay paréntesis, tomar solo la parte antes del paréntesis
    String alertType = alertMessage.substring(0, parenPos);
    alertType.trim(); // Eliminar espacios al final
    return alertType;
  } else {
    // Si no hay paréntesis, devolver el mensaje completo
    return alertMessage;
  }
}

// ===========================================
// 🛠️ FUNCIÓN AUXILIAR DE EXTRACCIÓN
// ===========================================
float extractFloatValue(String data, const char* startTag, const char* endTag = NULL) {
    int startPos = data.indexOf(startTag);
    if (startPos == -1) return 0.0;
    
    startPos += strlen(startTag);
    
    int endPos;
    if (endTag != NULL) {
        endPos = data.indexOf(endTag, startPos);
    } else {
        endPos = data.length();
    }
    
    String valueStr = data.substring(startPos, endPos);
    valueStr.trim();
    
    if (valueStr.length() == 0) return 0.0;
    
    return atof(valueStr.c_str());
}

// ===========================================
// 🔔 CALLBACK PARA RECEPCIÓN DE NOTIFICACIONES
// ===========================================
void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  String receivedData = "";
  for (int i = 0; i < length; i++) {
    receivedData += (char)pData[i];
  }
  receivedData.trim();

  // Obtener el nombre del nodo
  std::string nodeName = "Desconocido";
  std::string peerAddr = pBLERemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress().toString().c_str();

  for (auto const& [name, client] : activeClients) {
    if (std::string(client->getPeerAddress().toString().c_str()) == peerAddr) {
      nodeName = name;
      break;
    }
  }
  
  if (nodeName == "Desconocido") {
    Serial.println("❌ Dato recibido de nodo desconocido, ignorado.");
    return;
  }

  if (accumulatedData.count(nodeName) == 0) {
    accumulatedData[nodeName] = NodeData();
  }

  NodeData& currentData = accumulatedData[nodeName];
  String outputLine = "";
  
  // --- IDENTIFICACIÓN DE DATOS Y LÓGICA DE PROCESAMIENTO ---
  if (receivedData.indexOf("X:") != -1 && receivedData.indexOf("Y:") != -1) {
    currentData.accel = receivedData;
    currentData.receivedCount++;
    Serial.print("✅ Dato Accel Recibido (Nodo: ");
    Serial.print(nodeName.c_str());
    Serial.print(" | Acumulados: ");
    Serial.print(currentData.receivedCount);
    Serial.print("/3): ");
    Serial.println(receivedData);

  } else if (receivedData.indexOf("Temp:") != -1 && receivedData.indexOf("Hum:") != -1) {
    currentData.tempHum = receivedData;
    currentData.receivedCount++;
    Serial.print("✅ Dato Temp/Hum Recibido (Nodo: ");
    Serial.print(nodeName.c_str());
    Serial.print(" | Acumulados: ");
    Serial.print(currentData.receivedCount);
    Serial.print("/3): ");
    Serial.println(receivedData);

  } else if (receivedData.indexOf("AQI:") != -1 && receivedData.indexOf("eCO2:") != -1) {
    currentData.aqi = receivedData;
    currentData.receivedCount++;
    Serial.print("✅ Dato AQI Recibido (Nodo: ");
    Serial.print(nodeName.c_str());
    Serial.print(" | Acumulados: ");
    Serial.print(currentData.receivedCount);
    Serial.print("/3): ");
    Serial.println(receivedData);
    
  } else {
    // PROCESAMIENTO DE ALERTAS
    if (receivedData.indexOf("N1:") != -1 || receivedData.indexOf("N2:") != -1 || receivedData.indexOf("N3:") != -1) {
        
        // Extraer el mensaje de alerta limpio
        int colonPos = receivedData.indexOf(':');
        String alertMessage = "";
        if (colonPos != -1) {
          alertMessage = receivedData.substring(colonPos + 1);
          alertMessage.trim();
        } else {
          alertMessage = receivedData;
        }

        // --- FORMATEO DE ALERTA INDIVIDUAL ---
        outputLine += "[";
        outputLine += nodeName.c_str();
        outputLine += "]";
        outputLine += "ALERT:";
        outputLine += alertMessage;

        // Enviar alerta individual
        Serial.println("\n--- 🚨 ALERTA ENVIADA A NUCLEO (Serial1) ---");
        Serial.println(outputLine);
        Serial1.println(outputLine);
        Serial.println("------------------------------------------");
        
        //----------------------------------------------
        // 🔥 DETECCIÓN DE ALERTA COMÚN ENTRE 3 NODOS (MEJORADA)
        //----------------------------------------------
        
        // Extraer el TIPO de alerta (sin valores numéricos)
        String alertType = extractAlertType(alertMessage);
        
        // Guardar el tipo de alerta según nodo
        if (nodeName == "ESP_NODO_1") lastAlertTypeNodo1 = alertType;
        if (nodeName == "ESP_NODO_2") lastAlertTypeNodo2 = alertType;
        if (nodeName == "ESP_NODO_3") lastAlertTypeNodo3 = alertType;

        // Verificar coincidencia de TIPOS (no valores exactos)
        if (lastAlertTypeNodo1.length() > 0 &&
            lastAlertTypeNodo2.length() > 0 &&
            lastAlertTypeNodo3.length() > 0 &&
            lastAlertTypeNodo1 == lastAlertTypeNodo2 &&
            lastAlertTypeNodo2 == lastAlertTypeNodo3) {

            String alertaComun = "[GENERAL]ALERT:" + lastAlertTypeNodo1;

            Serial.println("\n--- 🚨🚨 ALERTA COMÚN DETECTADA ---");
            Serial.println(alertaComun);
            Serial1.println(alertaComun);
            Serial1.print("\n");
            Serial.println("----------------------------------");

            // Limpiar para esperar una nueva coincidencia
            lastAlertTypeNodo1 = "";
            lastAlertTypeNodo2 = "";
            lastAlertTypeNodo3 = "";
        }

        Serial.print("🚨 Alerta Inmediata Procesada (Nodo: ");
        Serial.print(nodeName.c_str());
        Serial.print("): ");
        Serial.println(receivedData);

    } else {
      Serial.print("⚠️ Dato Inesperado (Ignorado): ");
      Serial.println(receivedData);
    }
  }

  // LÓGICA DE RE-TRANSMISIÓN SERIAL1 (DATOS COMPLETOS)
  if (currentData.receivedCount >= 3) {
    float accelX = extractFloatValue(currentData.accel, "X: ");
    float accelY = extractFloatValue(currentData.accel, "Y: ", " Z:");
    float accelZ = extractFloatValue(currentData.accel, "Z: ");
    
    float temp = extractFloatValue(currentData.tempHum, "Temp: ", " C");
    float hum = extractFloatValue(currentData.tempHum, "Hum: ", " %");

    int aqi = (int)extractFloatValue(currentData.aqi, "AQI: ", ",");
    int tvoc = (int)extractFloatValue(currentData.aqi, "TVOC: ", " ppb");
    int eco2 = (int)extractFloatValue(currentData.aqi, "eCO2: ", " ppm");
    
    outputLine = "";
    outputLine += "[";
    outputLine += nodeName.c_str();
    outputLine += "]";
    outputLine += "X:"; outputLine += String(accelX, 2); outputLine += ";";
    outputLine += "Y:"; outputLine += String(accelY, 2); outputLine += ";";
    outputLine += "Z:"; outputLine += String(accelZ, 2); outputLine += ";";
    outputLine += "Temp:"; outputLine += String(temp, 2); outputLine += ";";
    outputLine += "Hum:"; outputLine += String(hum, 2); outputLine += ";";
    outputLine += "AQI:"; outputLine += String(aqi); outputLine += ";";
    outputLine += "TVOC:"; outputLine += String(tvoc); outputLine += ";";
    outputLine += "eCO2:"; outputLine += String(eco2);
    
    Serial.println("\n--- 🚀 LÍNEA ENVIADA A NUCLEO (Serial1) ---");
    Serial.println(outputLine);
    Serial.println("------------------------------------------");
    
    Serial1.println(outputLine);

    currentData.receivedCount = 0;
    currentData.accel = "";
    currentData.tempHum = "";
    currentData.aqi = "";
  }
    
  colorLED(0, 255, 0);
  delay(50);

  if (activeClients.size() > 0) {
    colorLED(0, 0, 255);
  } else {
    colorLED(255, 0, 0);
  }
}

// ===========================================
// 🔗 CALLBACKS DE CONEXIÓN
// ===========================================
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.print("🔗 Conectado (dirección: ");
    Serial.print(pclient->getPeerAddress().toString().c_str());
    Serial.println(").");
    colorLED(0, 0, 255);
  }

  void onDisconnect(BLEClient* pclient) {
    Serial.print("❌ Desconectado (dirección: ");
    Serial.print(pclient->getPeerAddress().toString().c_str());
    Serial.println(").");

    std::string disconnectedAddr = pclient->getPeerAddress().toString().c_str();

    for (auto it = activeClients.begin(); it != activeClients.end(); ) {
      if (std::string(it->second->getPeerAddress().toString().c_str()) == disconnectedAddr) {
        Serial.print("⚠️ Eliminando conexión activa para nodo: ");
        Serial.println(it->first.c_str());

        activeCharacteristics.erase(it->first);
        it = activeClients.erase(it);

        doConnect = true;
      } else {
        ++it;
      }
    }

    if (activeClients.empty()) {
      colorLED(255, 0, 0);
    }
  }
};

// ===========================================
// 🔍 CALLBACK DE ESCANEO
// ===========================================
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveName()) {
      for (int i = 0; i < NUM_TARGET_NODES; i++) {
        std::string deviceName = advertisedDevice.getName().c_str();

        if (deviceName == TARGET_NODE_NAMES[i]) {
          Serial.print("⭐ ¡Nodo Objetivo Encontrado! ");
          Serial.println(deviceName.c_str());

          if (advertisedDevices.count(deviceName) == 0) {
            advertisedDevices[deviceName] = new BLEAdvertisedDevice(advertisedDevice);
          } 

          doConnect = true;
          break;
        }
      }
    }
  }
};

// ===========================================
// 🤝 FUNCIÓN DE CONEXIÓN
// ===========================================
bool connectToServer(const char* nodeName) {
  std::string nameString = nodeName;

  if (advertisedDevices.count(nameString) == 0) {
    return false;
  }

  BLEAdvertisedDevice* pAdvertisedDevice = advertisedDevices[nameString];

  if (pAdvertisedDevice == nullptr) return false;

  BLEClient* pNewClient = BLEDevice::createClient();
  pNewClient->setClientCallbacks(new MyClientCallback());

  Serial.print("Intentando conectar a: ");
  Serial.println(nodeName);

  if (!pNewClient->connect(pAdvertisedDevice)) {
    Serial.print("❌ Error de Conexión BLE a: ");
    Serial.println(nodeName);
    delete pNewClient;
    return false;
  }

  BLERemoteService* pRemoteService = pNewClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.println("❌ Error: No se encontró el Servicio BLE.");
    pNewClient->disconnect();
    return false;
  }

  BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.println("❌ Error: No se encontró la Característica BLE.");
    pNewClient->disconnect();
    return false;
  }

  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
    Serial.println("🔔 Suscrito a Notificaciones.");

    activeClients[nameString] = pNewClient;
    activeCharacteristics[nameString] = pRemoteCharacteristic;

    return true;
  }

  Serial.println("⚠️ La Característica no soporta NOTIFY.");
  pNewClient->disconnect();
  return false;
}

// ===========================================
// 🚀 SETUP
// ===========================================
void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando Cliente BLE Receptor Multiconexión...");

  Serial1.begin(115200, SERIAL_8N1, 4, 5);
  Serial.println("Serial1 (UART1) inicializado en Pines: RX=4, TX=5.");

  pixels.begin();
  pixels.setBrightness(30);
  colorLED(255, 0, 0);

  BLEDevice::init("");

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);

  Serial.println("Iniciando escaneo BLE...");
  pBLEScan->start(5, false);
  isScanning = true;
}

// ===========================================
// 🔄 LOOP
// ===========================================
void loop() {
  if (doConnect) {
    doConnect = false;

    for (int i = 0; i < NUM_TARGET_NODES; i++) {
      std::string nameString = TARGET_NODE_NAMES[i];

      if (advertisedDevices.count(nameString) > 0 && activeClients.count(nameString) == 0) {
        if (connectToServer(TARGET_NODE_NAMES[i])) {
          Serial.print("✅ Conexión establecida y suscrito con éxito a: ");
          Serial.println(TARGET_NODE_NAMES[i]);
        }
      }
    }

    if (activeClients.size() < NUM_TARGET_NODES && !isScanning) {
      Serial.println("Intentando re-escanear para encontrar nodos faltantes...");

      for (auto const& [key, val] : advertisedDevices) {
        delete val; 
      }
      advertisedDevices.clear();

      colorLED(255, 0, 0); 
      BLEDevice::getScan()->start(5, false); 
      isScanning = true;
    }
  }

  if (BLEDevice::getScan()->isScanning()) {
    isScanning = true;
  } else {
    isScanning = false;
  }

  delay(100);
}