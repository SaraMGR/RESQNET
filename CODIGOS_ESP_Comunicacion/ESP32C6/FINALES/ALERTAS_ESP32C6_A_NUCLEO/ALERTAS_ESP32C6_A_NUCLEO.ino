#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Adafruit_NeoPixel.h>
#include <map>
#include <string>
#include <stdlib.h> // Necesario para 'atof' en el helper

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

// --- ESTRUCTURAS NUEVAS PARA ACUMULACIÓN DE DATOS ---
struct NodeData {
  String accel;
  String tempHum;
  String aqi;
  int receivedCount = 0; // Contador para saber cuántos de 3 datos se han recibido
};

// Mapa para almacenar los datos acumulados por nombre de nodo
std::map<std::string, NodeData> accumulatedData;

// Variables de estado
bool doConnect = true;
bool isScanning = false;

// Función auxiliar para color LED
void colorLED(int r, int g, int b) {
  pixels.setPixelColor(0, pixels.Color(r, g, b));
  pixels.show();
}

// ===========================================
// 🛠️ FUNCIÓN AUXILIAR DE EXTRACCIÓN
// ===========================================
// Extrae el valor flotante de una subcadena de sensor (ej: "X: 0.05 Y:...")
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
    
    // Asegurarse de que el valor extraído no tenga espacios innecesarios
    String valueStr = data.substring(startPos, endPos);
    valueStr.trim();
    
    // Si la cadena está vacía después del trim, devuelve 0.0
    if (valueStr.length() == 0) return 0.0;
    
    // Usa atof para convertir la cadena a float
    return atof(valueStr.c_str());
}

// ===========================================
// 🔔 CALLBACK PARA RECEPCIÓN DE NOTIFICACIONES (MODIFICADA PARA ALERTAS)
// ===========================================
void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  String receivedData = "";
  for (int i = 0; i < length; i++) {
    receivedData += (char)pData[i];
  }
  receivedData.trim(); // Limpia espacios en blanco

  // Obtener el nombre del nodo asociado a la característica
  std::string nodeName = "Desconocido";
  std::string peerAddr = pBLERemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress().toString().c_str();

  // Buscar el nombre del nodo en activeClients
  for (auto const& [name, client] : activeClients) {
    if (std::string(client->getPeerAddress().toString().c_str()) == peerAddr) {
      nodeName = name;
      break;
    }
  }
  
  // Si no es un nodo objetivo, se descarta (no debería pasar con la lógica de conexión)
  if (nodeName == "Desconocido") {
    Serial.println("❌ Dato recibido de nodo desconocido, ignorado.");
    return;
  }

  // Asegurar que el nodo esté inicializado en el mapa de acumulación
  if (accumulatedData.count(nodeName) == 0) {
    accumulatedData[nodeName] = NodeData();
  }

  // Referencia al objeto de datos del nodo actual
  NodeData& currentData = accumulatedData[nodeName];
  
  // Variable para la línea de salida (puede ser alerta o datos)
  String outputLine = "";
  
  // --- IDENTIFICACIÓN DE DATOS Y LÓGICA DE PROCESAMIENTO ---
  // 1. DATO DE ACELERÓMETRO (X: Y: Z:)
  if (receivedData.indexOf("X:") != -1 && receivedData.indexOf("Y:") != -1) {
    currentData.accel = receivedData;
    currentData.receivedCount++;
    Serial.print("✅ Dato Accel Recibido (Nodo: ");
    Serial.print(nodeName.c_str());
    Serial.print(" | Acumulados: ");
    Serial.print(currentData.receivedCount);
    Serial.print("/3): ");
    Serial.println(receivedData);

  // 2. DATO DE TEMPERATURA/HUMEDAD (Temp: Hum:)
  } else if (receivedData.indexOf("Temp:") != -1 && receivedData.indexOf("Hum:") != -1) {
    currentData.tempHum = receivedData;
    currentData.receivedCount++;
    Serial.print("✅ Dato Temp/Hum Recibido (Nodo: ");
    Serial.print(nodeName.c_str());
    Serial.print(" | Acumulados: ");
    Serial.print(currentData.receivedCount);
    Serial.print("/3): ");
    Serial.println(receivedData);

  // 3. DATO DE CALIDAD DEL AIRE (AQI: eCO2:)
  } else if (receivedData.indexOf("AQI:") != -1 && receivedData.indexOf("eCO2:") != -1) {
    currentData.aqi = receivedData;
    currentData.receivedCount++;
    Serial.print("✅ Dato AQI Recibido (Nodo: ");
    Serial.print(nodeName.c_str());
    Serial.print(" | Acumulados: ");
    Serial.print(currentData.receivedCount);
    Serial.print("/3): ");
    Serial.println(receivedData);
    
  // 4. ALERTA O DATO INESPERADO (NO CONTIENE PALABRAS CLAVE DE SENSORES)
  // **¡Esta es la modificación clave!**
  } else {
    // Si contiene la identificación del nodo al inicio (N1: N2: etc.), lo tratamos como Alerta.
    // Buscamos patrones como "N1:" o "N2:"
    if (receivedData.indexOf("N1:") != -1 || receivedData.indexOf("N2:") != -1 || receivedData.indexOf("N3:") != -1) {
        
        // --- FORMATEO DE ALERTA ---
        // Nuevo formato de Alerta: [NodeName]ALERT:Mensaje Completo
        outputLine += "[";
        outputLine += nodeName.c_str();
        outputLine += "]";
        outputLine += "ALERT:";
        // Quitamos la identificación del nodo del mensaje para hacerlo más limpio
        int colonPos = receivedData.indexOf(':');
        if (colonPos != -1) {
          String alertMessage = receivedData.substring(colonPos + 1);
          alertMessage.trim();
          outputLine += alertMessage;
        } else {
          outputLine += receivedData; // Usamos el mensaje completo si no tiene ':'
        }

        // Se envía de inmediato
        Serial.println("\n--- 🚨 ALERTA ENVIADA A NUCLEO (Serial1) ---");
        Serial.println(outputLine);
        Serial1.println(outputLine);
        Serial.println("------------------------------------------");
        
        // NO se incrementa currentData.receivedCount.
        Serial.print("🚨 Alerta Inmediata Procesada (Nodo: ");
        Serial.print(nodeName.c_str());
        Serial.print("): ");
        Serial.println(receivedData);

    } else {
      // Si no es un dato de sensor conocido NI una alerta reconocida, se ignora.
      Serial.print("⚠️ Dato Inesperado (Ignorado): ");
      Serial.println(receivedData);
    }
  }

  // *******************************************************
  // *** LÓGICA CLAVE: RE-TRANSMISIÓN SERIAL1 (CLAVE:VALOR) ***
  // *** SOLO SE EJECUTA SI SE COMPLETAN LOS 3 TIPOS DE DATOS ***
  // *******************************************************
  if (currentData.receivedCount >= 3) {
    // 1. Extraer los 8 valores numéricos
    float accelX = extractFloatValue(currentData.accel, "X: ");
    float accelY = extractFloatValue(currentData.accel, "Y: ", " Z:");
    float accelZ = extractFloatValue(currentData.accel, "Z: ");
    
    float temp = extractFloatValue(currentData.tempHum, "Temp: ", " C");
    float hum = extractFloatValue(currentData.tempHum, "Hum: ", " %");

    int aqi = (int)extractFloatValue(currentData.aqi, "AQI: ", ",");
    int tvoc = (int)extractFloatValue(currentData.aqi, "TVOC: ", " ppb");
    int eco2 = (int)extractFloatValue(currentData.aqi, "eCO2: ", " ppm");
    
    // 2. Construir la cadena de DATOS SENSOR en el formato simple Clave:Valor;Clave:Valor
    // Se reutiliza 'outputLine' (que estaría vacía ya que las alertas se envían antes)
    outputLine = "";
    
    // Parte de Identificación: [NodeName]
    outputLine += "[";
    outputLine += nodeName.c_str();
    outputLine += "]";
    
    // Parte de Datos: Clave:Valor;...
    outputLine += "X:"; outputLine += String(accelX, 2); outputLine += ";";
    outputLine += "Y:"; outputLine += String(accelY, 2); outputLine += ";";
    outputLine += "Z:"; outputLine += String(accelZ, 2); outputLine += ";";
    outputLine += "Temp:"; outputLine += String(temp, 2); outputLine += ";";
    outputLine += "Hum:"; outputLine += String(hum, 2); outputLine += ";";
    outputLine += "AQI:"; outputLine += String(aqi); outputLine += ";";
    outputLine += "TVOC:"; outputLine += String(tvoc); outputLine += ";";
    outputLine += "eCO2:"; outputLine += String(eco2); // El último no lleva punto y coma
    
    // 3. Imprimir en el Monitor Serial (PARA VERIFICACIÓN)
    Serial.println("\n--- 🚀 LÍNEA ENVIADA A NUCLEO (Serial1) ---");
    Serial.println(outputLine);
    Serial.println("------------------------------------------");
    
    // 4. Enviar a la Nucleo (Serial1)
    Serial1.println(outputLine);

    // 5. Limpiar el contador y los datos para el siguiente ciclo
    currentData.receivedCount = 0;
    currentData.accel = "";
    currentData.tempHum = "";
    currentData.aqi = "";
  }
  // *******************************************************
    
  // Ejemplo de flash verde al recibir (EXISTENTE)
  colorLED(0, 255, 0);
  delay(50);

  // Si hay al menos un cliente conectado, se mantiene en azul (EXISTENTE)
  if (activeClients.size() > 0) {
    colorLED(0, 0, 255);
  } else {
    colorLED(255, 0, 0);
  }
}

// ===========================================
// 🔗 CALLBACKS DE CONEXIÓN
// (ESTE CÓDIGO SE MANTIENE DE TU VERSIÓN ORIGINAL)
// ===========================================
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.print("🔗 Conectado (dirección: ");
    Serial.print(pclient->getPeerAddress().toString().c_str());
    Serial.println(").");
    colorLED(0, 0, 255); // Azul: Conectado
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
      colorLED(255, 0, 0); // Rojo: Desconectado/Escaneando (si no queda nadie)
    }
  }
};

// ===========================================
// 🔍 CALLBACK DE ESCANEO
// (ESTE CÓDIGO SE MANTIENE DE TU VERSIÓN ORIGINAL)
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
// (ESTE CÓDIGO SE MANTIENE DE TU VERSIÓN ORIGINAL)
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
// (ESTE CÓDIGO SE MANTIENE DE TU VERSIÓN ORIGINAL)
// ===========================================
void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando Cliente BLE Receptor Multiconexión...");

  Serial1.begin(115200, SERIAL_8N1, 4, 5);
  Serial.println("Serial1 (UART1) inicializado en Pines: RX=4, TX=5.");

  pixels.begin();
  pixels.setBrightness(30);
  colorLED(255, 0, 0); // Rojo al iniciar/Escaneando

  BLEDevice::init("");

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);

  Serial.println("Iniciando escaneo BLE...");
  pBLEScan->start(5, false); // Escanear por 5 segundos
  isScanning = true;
}

// ===========================================
// 🔄 LOOP
// (ESTE CÓDIGO SE MANTIENE DE TU VERSIÓN ORIGINAL)
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