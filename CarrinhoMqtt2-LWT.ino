#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <WiFiManager.h>

#define LED_PIN   2
#define RELE_L1   4
#define RELE_L2   5

WiFiClient espClient;
PubSubClient client(espClient);

const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_status_topic = "carrinho/status";
const char* mqtt_command_topic = "carrinho/comando";

bool wifiConectado = false;
bool portalAtivo = false;
unsigned long previousMillis = 0;
const long intervaloPiscar = 100;

WiFiManager wm;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELE_L1, OUTPUT);
  pinMode(RELE_L2, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(RELE_L1, LOW);
  digitalWrite(RELE_L2, LOW);

  // Primeiro tenta conectar com dados salvos
  Serial.println("Tentando conectar ao Wi-Fi salvo...");
  WiFi.begin();
  unsigned long tempoInicial = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - tempoInicial < 5000) {
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi conectado!");
    wifiConectado = true;
    digitalWrite(LED_PIN, HIGH);
  } else {
    Serial.println("Wi-Fi falhou. Iniciando portal...");
    wm.setConfigPortalBlocking(false); // NÃO BLOQUEAR
    wm.startConfigPortal("CarrinhoMQTT");
    portalAtivo = true;
  }

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (portalAtivo) {
    wm.process();  // Mantém o portal funcionando

    piscarLedRapido();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Wi-Fi conectado via portal!");
      portalAtivo = false;
      wifiConectado = true;
      digitalWrite(LED_PIN, HIGH);
    }
    return;
  }

  if (wifiConectado) {
    if (!client.connected()) {
      digitalWrite(LED_PIN, LOW); // LED apagado se perder MQTT
      reconnect();
    }
    client.loop();
  }
}

void piscarLedRapido() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= intervaloPiscar) {
    previousMillis = currentMillis;
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");

    String clientId = "ESP32Carrinho";
    if (client.connect(clientId.c_str(), NULL, NULL,
                       mqtt_status_topic, 0, true, "OFFLINE")) {
      Serial.println("Conectado ao MQTT!");
      digitalWrite(LED_PIN, HIGH);
      client.publish(mqtt_status_topic, "ONLINE", true);
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
