#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// --- CONFIGURACIÓN UART ---
// Define los pines de la UART que usas para comunicarte con la STM32
#define RXD1 4   // ESP32 RX <- STM32 TX
#define TXD1 5   // ESP32 TX -> STM32 RX
const long UART_BAUD = 115200; // Baudios de la comunicación UART

// UUIDs ÚNICOS (Mismos que el Cliente)
#define SERVICE_UUID        "4FAFC201-1FB5-459E-8FC7-C2D046D5E98C"
#define CHARACTERISTIC_UUID "BDAA24FF-B980-4D30-A466-417A16773B1C"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;

// Callbacks del Servidor
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("📡 Cliente BLE conectado.");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("❌ Cliente BLE desconectado. Reiniciando publicidad...");
      BLEDevice::startAdvertising(); // Permite la reconexión
    }
};

void setup() {
  Serial.begin(115200);   // Monitor Serial (USB/PC)
  
  // Inicialización de la UART (Serial1) para la comunicación con la STM32
  Serial1.begin(UART_BAUD, SERIAL_8N1, RXD1, TXD1);
  Serial.println("UART-BLE Gateway Iniciado.");
  Serial.println("Esperando datos del STM32...");

  // --- INICIALIZACIÓN BLE (Igual que antes) ---
  BLEDevice::init("ESP32C6_Servidor");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();

  Serial.println("Esperando que el Cliente BLE se conecte...");
}

void loop() {
  // 1. Verificar si hay datos nuevos en la UART (desde la STM32)
  if (Serial1.available()) {
    // Leer toda la cadena de datos hasta el caracter de nueva línea ('\n')
    String receivedData = Serial1.readStringUntil('\n');
    receivedData.trim(); // Limpiar espacios en blanco al inicio/final

    Serial.print("📥 UART Recibido: ");
    Serial.println(receivedData);

    // 2. Verificar si hay un cliente BLE conectado
    if (deviceConnected) {
      // 3. Enviar los datos por BLE (NOTIFY)
      
      // USAR SOLUCIÓN DE ENVÍO SEGURO
      // LÍNEA CORREGIDA:
      pCharacteristic->setValue((const uint8_t*)receivedData.c_str(), receivedData.length()); 
      pCharacteristic->notify(); 

      Serial.print("📤 BLE Enviado: ");
      Serial.println(receivedData);
    } else {
      Serial.println("⚠️ No hay Cliente BLE conectado para enviar datos.");
    }
  }

  // Dejar que el procesador descanse un poco
  delay(10); 
}