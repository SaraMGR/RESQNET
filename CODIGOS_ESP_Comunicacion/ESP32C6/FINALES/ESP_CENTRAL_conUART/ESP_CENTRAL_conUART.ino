#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Adafruit_NeoPixel.h>
#include <map>

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

// Variables de estado
bool doConnect = true;
bool isScanning = false;

// Función auxiliar para color LED
void colorLED(int r, int g, int b) {
	pixels.setPixelColor(0, pixels.Color(r, g, b));
	pixels.show();
}

// ===========================================
// 🔔 CALLBACK PARA RECEPCIÓN DE NOTIFICACIONES (MODIFICADA)
// ===========================================
void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
	String receivedData = "";
	for (int i = 0; i < length; i++) {
		receivedData += (char)pData[i];
	}

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

	Serial.print("✅ Dato Recibido (Cliente: ");
	Serial.print(peerAddr.c_str());
	Serial.print(" | Nodo: ");
	Serial.print(nodeName.c_str());
	Serial.print("): ");
	Serial.println(receivedData);

  // *******************************************************
	// *** CÓDIGO AÑADIDO: RE-TRANSMISIÓN POR SERIAL1 (UART1) ***
	// *******************************************************
    // Formato de envío a la Nucleo: [NodeName]Data\n
    // Usamos Serial1 (GPIO5 TX) para enviar a la Nucleo.
    Serial1.print("[");
    Serial1.print(nodeName.c_str());
    Serial1.print("]");
    Serial1.println(receivedData);
    // *******************************************************
    
	// Ejemplo de flash verde al recibir
	colorLED(0, 255, 0);
	delay(50);

	// Si hay al menos un cliente conectado, se mantiene en azul
	if (activeClients.size() > 0) {
		colorLED(0, 0, 255);
	} else {
		colorLED(255, 0, 0);
	}
}

// ===========================================
// 🔗 CALLBACKS DE CONEXIÓN
// (El resto del código MyClientCallback, MyAdvertisedDeviceCallbacks, connectToServer, setup, y loop 
// se mantiene igual que en tu versión original para manejar la lógica BLE multiconexión).
// ===========================================
class MyClientCallback : public BLEClientCallbacks {
	// Se llama cuando una conexión se establece
	void onConnect(BLEClient* pclient) {
		Serial.print("🔗 Conectado (dirección: ");
		Serial.print(pclient->getPeerAddress().toString().c_str());
		Serial.println(").");
		colorLED(0, 0, 255); // Azul: Conectado
	}

	// Se llama cuando una conexión se pierde
	void onDisconnect(BLEClient* pclient) {
		Serial.print("❌ Desconectado (dirección: ");
		Serial.print(pclient->getPeerAddress().toString().c_str());
		Serial.println(").");

		// CORRECCIÓN 1: Convertir explícitamente la String de Arduino a std::string de C++
		std::string disconnectedAddr = pclient->getPeerAddress().toString().c_str();

		for (auto it = activeClients.begin(); it != activeClients.end(); ) {
			// CORRECCIÓN 2: Convertir la String de Arduino a std::string antes de la comparación
			if (std::string(it->second->getPeerAddress().toString().c_str()) == disconnectedAddr) {
				Serial.print("⚠️ Eliminando conexión activa para nodo: ");
				Serial.println(it->first.c_str());

				// Eliminar de activeCharacteristics y activeClients
				activeCharacteristics.erase(it->first);
				it = activeClients.erase(it);

				// Como se desconectó, forzamos un re-escaneo
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
// ===========================================
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
	void onResult(BLEAdvertisedDevice advertisedDevice) {
		if (advertisedDevice.haveName()) {
			// 1. Revisar si el dispositivo escaneado es uno de nuestros Nodos objetivo
			for (int i = 0; i < NUM_TARGET_NODES; i++) {
				std::string deviceName = advertisedDevice.getName().c_str();

				if (deviceName == TARGET_NODE_NAMES[i]) {
					Serial.print("⭐ ¡Nodo Objetivo Encontrado! ");
					Serial.println(deviceName.c_str());

					// Guardar el dispositivo para la conexión (sobrescribe si ya existe)
					if (advertisedDevices.count(deviceName) == 0) {
						advertisedDevices[deviceName] = new BLEAdvertisedDevice(advertisedDevice);
					} else {
						// No se crea uno nuevo si ya está, solo se asegura la bandera de conexión.
					}

					// Establecer la bandera de conexión para que el loop intente conectar
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

	// 1. Crear un nuevo cliente para esta conexión (CLAVE PARA MÚLTIPLES CONEXIONES)
	BLEClient* pNewClient = BLEDevice::createClient();
	pNewClient->setClientCallbacks(new MyClientCallback());

	Serial.print("Intentando conectar a: ");
	Serial.println(nodeName);

	// 2. Intentar la conexión
	if (!pNewClient->connect(pAdvertisedDevice)) {
		Serial.print("❌ Error de Conexión BLE a: ");
		Serial.println(nodeName);
		delete pNewClient;
		return false;
	}

	// 3. Buscar Servicio y Característica
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

	// 4. Suscribirse a Notificaciones
	if (pRemoteCharacteristic->canNotify()) {
		pRemoteCharacteristic->registerForNotify(notifyCallback);
		Serial.println("🔔 Suscrito a Notificaciones.");

		// 5. Almacenar la conexión activa en los mapas
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
	// NOTA: Serial.begin(115200) es el UART0 por defecto, que usamos para la comunicación
    // con la Nucleo, y también para la depuración en tu PC.
	Serial.begin(115200);
	Serial.println("Iniciando Cliente BLE Receptor Multiconexión...");

  // 💡 UART1 (Serial1): Para comunicación con la Nucleo-H755
  // RX en GPIO4, TX en GPIO5, a 115200 baudios.
  Serial1.begin(115200, SERIAL_8N1, 4, 5);
  Serial.println("Serial1 (UART1) inicializado en Pines: RX=4, TX=5.");

	// LED Inicio
	pixels.begin();
	pixels.setBrightness(30);
	colorLED(255, 0, 0); // Rojo al iniciar/Escaneando

	// Inicialización BLE
	BLEDevice::init("");

	// Configuración del escaneo
	BLEScan* pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setActiveScan(true);
	pBLEScan->setInterval(100);
	pBLEScan->setWindow(99);

	// ⚡ Iniciar el escaneo al final del setup
	Serial.println("Iniciando escaneo BLE...");
	pBLEScan->start(5, false); // Escanear por 5 segundos
	isScanning = true;
}

// ===========================================
// 🔄 LOOP
// ===========================================
void loop() {
	// 1. Manejo del proceso de conexión/reconexión
	if (doConnect) {
		doConnect = false;

		// Iterar sobre TODOS los nodos objetivo listados
		for (int i = 0; i < NUM_TARGET_NODES; i++) {
			std::string nameString = TARGET_NODE_NAMES[i];

			// 2. Intentar conectar SOLO si el nodo NO está ya en la lista de activos
			if (advertisedDevices.count(nameString) > 0 && activeClients.count(nameString) == 0) {
				if (connectToServer(TARGET_NODE_NAMES[i])) {
					Serial.print("✅ Conexión establecida y suscrito con éxito a: ");
					Serial.println(TARGET_NODE_NAMES[i]);
				}
			}
		}

		// 3. Re-escanear si la conexión fue parcial
		if (activeClients.size() < NUM_TARGET_NODES && !isScanning) {
			Serial.println("Intentando re-escanear para encontrar nodos faltantes...");

			// Limpiar la lista de dispositivos encontrados para el nuevo escaneo
			for (auto const& [key, val] : advertisedDevices) {
				delete val; // Liberar memoria
			}
			advertisedDevices.clear();

			colorLED(255, 0, 0); // Rojo: Escaneando
			BLEDevice::getScan()->start(5, false); // Escanear por 5 segundos
			isScanning = true;
		}
	}

	// 4. CORRECCIÓN 3: Verificar el estado de escaneo sin error de tipeo
	if (BLEDevice::getScan()->isScanning()) {
		isScanning = true;
	} else {
		isScanning = false;
	}

	delay(100);
}