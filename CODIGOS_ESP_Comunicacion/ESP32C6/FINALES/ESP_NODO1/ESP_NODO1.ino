#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Adafruit_NeoPixel.h>

// ==========================================
// 🔧 CONFIGURACIÓN DEL NODO (¡CAMBIA ESTO!)
// ==========================================
// Cambia este valor para cada ESP32 (1, 2 o 3)
#define NODE_NUMBER "1" 

// El nombre que verá el receptor al escanear (ej. "ESP_NODO_1")
#define DEVICE_NAME "ESP_NODO_" NODE_NUMBER 

// El prefijo que se enviará con los datos (ej. "N1:Hola")
const String DATA_PREFIX = "N" NODE_NUMBER ":"; 
// ==========================================


// --- CONFIGURACIÓN LED RGB ---
#define LED_PIN    8   
#define NUMPIXELS  1   
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// --- CONFIGURACIÓN UART ---
#define RXD1 4   
#define TXD1 5   
const long UART_BAUD = 115200;

// UUIDs (Deben ser iguales en todos los nodos para que el receptor los reconozca)
#define SERVICE_UUID        "4FAFC201-1FB5-459E-8FC7-C2D046D5E98C"
#define CHARACTERISTIC_UUID "BDAA24FF-B980-4D30-A466-417A16773B1C"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Función auxiliar para color LED
void colorLED(int r, int g, int b) {
  pixels.setPixelColor(0, pixels.Color(r, g, b));
  pixels.show();
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("📡 Receptor conectado a este Nodo.");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("❌ Receptor desconectado.");
      BLEDevice::startAdvertising(); 
    }
};

void setup() {
  Serial.begin(115200); 
  
  // LED Inicio
  pixels.begin();
  pixels.setBrightness(30); 
  colorLED(255, 0, 0); // Rojo al iniciar

  // UART
  Serial1.begin(UART_BAUD, SERIAL_8N1, RXD1, TXD1);
  Serial.print("Iniciando ");
  Serial.println(DEVICE_NAME);

  // --- INICIALIZACIÓN BLE CON NOMBRE ÚNICO ---
  BLEDevice::init(DEVICE_NAME); // Aquí usamos el nombre dinámico "ESP_NODO_X"
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      //BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY 
                      //BLECharacteristic::PROPERTY_INDICATE
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising(); // Empezamos a "gritar" nuestra existencia

  Serial.println("Esperando conexión del Receptor...");
}

void loop() {
  // Gestión LED de estado (Rojo/Azul)
  if (deviceConnected && !oldDeviceConnected) {
      oldDeviceConnected = true;
      colorLED(0, 0, 255); // AZUL: Conectado
  }
  if (!deviceConnected && oldDeviceConnected) {
      oldDeviceConnected = false;
      colorLED(255, 0, 0); // ROJO: Desconectado
  }

  // --- LECTURA Y ENVÍO DE DATOS ---
  if (Serial1.available()) {
    String rawData = Serial1.readStringUntil('\n');
    rawData.trim();

    // 1. PREPARAR EL PAQUETE CON EL IDENTIFICADOR
    // Si recibes "Temperatura:25", se convierte en "N1:Temperatura:25"
    String dataPacket = DATA_PREFIX + rawData;

    Serial.print("📥 UART: ");
    Serial.println(rawData);

    if (deviceConnected) {
      // 2. ENVIAR PAQUETE COMPLETO
      pCharacteristic->setValue((const uint8_t*)dataPacket.c_str(), dataPacket.length()); 
      pCharacteristic->notify(); 
      
      Serial.print("📤 BLE Enviado: ");
      Serial.println(dataPacket);

      // Flash Verde
      colorLED(0, 255, 0); 
      delay(50);           
      colorLED(0, 0, 255); 
      
    } else {
      Serial.println("⚠️ Dato recibido pero no hay receptor conectado.");
      colorLED(0, 0, 0);
      delay(50);
      colorLED(255, 0, 0); 
    }
  }
  delay(10); 
}