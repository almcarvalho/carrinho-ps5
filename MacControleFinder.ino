#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32Scanner");  // Nome do ESP32 para descoberta
  Serial.println("Iniciando scan Bluetooth...");

  bool scanResult = SerialBT.discoverAsync([](BTAdvertisedDevice* device) {
    Serial.print("Dispositivo encontrado: ");
    Serial.print(device->getName().c_str());
    Serial.print(" - MAC: ");
    Serial.println(device->getAddress().toString().c_str());
  });

  if (!scanResult) {
    Serial.println("Falha ao iniciar o scan.");
  }
}

void loop() {
  delay(10000);  // Tempo para escanear
}
