#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <WiFiManager.h>  // Adicione a biblioteca via Library Manager

#define LED_PIN   2
#define RELE_L1   4
#define RELE_L2   5

WiFiClient espClient;
PubSubClient client(espClient);

const char* mqtt_server = "broker.hivemq.com"; // ou o seu Heroku via ponte

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELE_L1, OUTPUT);
  pinMode(RELE_L2, OUTPUT);

  WiFiManager wm;

  bool res = wm.autoConnect("CarrinhoMQTT");
  if (!res) {
    Serial.println("Falha na conexão Wi-Fi");

     // Força o reset das configurações salvas (SSID/senha)
     wm.resetSettings();

  
    ESP.restart();
  }

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    if (client.connect("ESP32Carrinho")) {
      Serial.println("conectado!");
      client.subscribe("carrinho/comando");
    } else {
      Serial.print(".");
      delay(1000);
    }
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  String comando = "";
  for (int i = 0; i < length; i++) comando += (char)message[i];
  Serial.println("Comando recebido: " + comando);

  if (comando == "frente") {
    digitalWrite(RELE_L1, HIGH);
    digitalWrite(RELE_L2, HIGH);
    waitAndStop();
  } else if (comando == "tras") {
   //NÃO IMPLEMENTADO
  } else if (comando == "esquerda") {
    digitalWrite(RELE_L1, LOW);
    digitalWrite(RELE_L2, HIGH);
    waitAndStop();
  } else if (comando == "direita") {
    digitalWrite(RELE_L1, HIGH);
    digitalWrite(RELE_L2, LOW);
    waitAndStop();
  } else if (comando == "stop") {
    waitAndStop();
  }
}

void waitAndStop(){
  delay(500);
  digitalWrite(RELE_L1, LOW);
  digitalWrite(RELE_L2, LOW);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();
}
