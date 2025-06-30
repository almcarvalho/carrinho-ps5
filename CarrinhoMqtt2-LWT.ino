#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <WiFiManager.h>  // Adicione a biblioteca via Library Manager

#define LED_PIN   2
#define RELE_L1   4
#define RELE_L2   5

WiFiClient espClient;
PubSubClient client(espClient);

const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_status_topic = "carrinho/status";
const char* mqtt_command_topic = "carrinho/comando";

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELE_L1, OUTPUT);
  pinMode(RELE_L2, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(RELE_L1, LOW);
  digitalWrite(RELE_L2, LOW);

  WiFiManager wm;
  bool res = wm.autoConnect("CarrinhoMQTT");
  if (!res) {
    Serial.println("Falha na conexão Wi-Fi");
    wm.resetSettings();
    ESP.restart();
  }

  // Define servidor e callback MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");

    // Define LWT: se desconectar inesperadamente, publica OFFLINE com retain
    String clientId = "ESP32Carrinho";
    if (client.connect(clientId.c_str(), NULL, NULL,
                      mqtt_status_topic, 0, true, "OFFLINE")) {
      Serial.println("Conectado ao MQTT!");
      
      // LED ligado = conectado
      digitalWrite(LED_PIN, HIGH);

      // Envia status ONLINE (com retain)
      client.publish(mqtt_status_topic, "ONLINE", true);

      // Se inscreve no tópico de comando
      client.subscribe(mqtt_command_topic);
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
    // Implemente se necessário
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
  if (!client.connected()) {
    digitalWrite(LED_PIN, LOW); // Desliga LED se perder conexão
    reconnect();
  }
  client.loop();
}
