#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLERemoteCharacteristic.h> // <--- Se puede requerir esta inclusión en algunos entornos

// UUIDs deben ser los mismos que en el Servidor
static BLEUUID serviceUUID("4FAFC201-1FB5-459E-8FC7-C2D046D5E98C");
static BLEUUID charUUID("BDAA24FF-B980-4D30-A466-417A16773B1C");
static bool doConnect = false;
static bool connected = false;
static BLEAdvertisedDevice* myDevice;

// Nombre del Servidor a buscar
const char* targetDeviceName = "ESP32C6_Servidor"; 

BLEClient* pClient = NULL;
BLERemoteCharacteristic* pRemoteCharacteristic = NULL;

// Función que se ejecuta cuando el Servidor notifica un cambio (recibir datos)
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  String receivedString = String((char*)pData, length);
  Serial.print("-> Dato Recibido: ");
  Serial.println(receivedString);
}

// Clase para manejar eventos de conexión del Cliente
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pClient) {
    Serial.println("Cliente conectado al Servidor.");
  }
  void onDisconnect(BLEClient* pClient) {
    connected = false;
    Serial.println("Cliente desconectado. Reintentando...");
    doConnect = true; // Intentar reconectar
  }
};

// Intenta la conexión y configura la recepción de datos
bool connectToServer() {
  Serial.print("Formando conexión a: ");
  Serial.println(myDevice->getAddress().toString().c_str());

  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  
  // Conectar al Servidor
  if (!pClient->connect(myDevice)) {
    Serial.println("Fallo al conectar al Servidor.");
    return false;
  }
  
  // Obtener el servicio y la característica
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Fallo al encontrar el servicio: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }

  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Fallo al encontrar la caracteristica: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }

  // Activar la notificación (callback)
  if(pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
    Serial.println("Suscrito a Notificaciones del Servidor.");
  } else {
     Serial.println("La característica no admite notificaciones.");
     pClient->disconnect();
     return false;
  }

  connected = true;
  return true;
}

// Clase para escanear dispositivos
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // Comprobar el nombre y que tenga el servicio
    if (advertisedDevice.getName() == targetDeviceName) { 
      Serial.print("Servidor encontrado: ");
      Serial.println(advertisedDevice.toString().c_str());
      
      // Guardar el dispositivo para la conexión y detener el escaneo
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    } 
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando Cliente BLE...");

  // Inicializar el dispositivo BLE
  BLEDevice::init("");

  // Iniciar escaneo
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true); // Escaneo activo para obtener el nombre
  pBLEScan->start(5); // Escanear por 5 segundos, luego se detiene
}

void loop() {
  // Intentar conectar si se encontró el Servidor y no estamos conectados
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("Conexión exitosa.");
    } else {
      Serial.println("Error de conexión.");
    }
    doConnect = false; // Solo intentar conectar una vez por escaneo
  } 
  
  // Si estamos desconectados, reiniciar el escaneo para buscar el Servidor
  if (!connected) {
    Serial.println("Reiniciando escaneo...");
    BLEDevice::getScan()->start(0); // Escanear indefinidamente hasta encontrar el Servidor
  }

  delay(1000); 
}