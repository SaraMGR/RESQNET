#define RXD1 4   // ESP32 RX  <- STM32 TX
#define TXD1 5   // ESP32 TX  -> STM32 RX

void setup() {
  Serial.begin(115200);  // para ver en monitor USB
  Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1);
  Serial.println("📡 Esperando datos del STM32...");
}

void loop() {
  if (Serial1.available()) {
    String data = Serial1.readStringUntil('\n');
    Serial.print("📥 STM32 dice: ");
    Serial.println(data);
  }
}
