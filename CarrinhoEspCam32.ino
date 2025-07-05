#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <base64.h>

// ======= Config Wi-Fi =======
const char* ssid = "4G-UFI-A18";
const char* password = "1234567890";

// ======= MQTT =======
const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_command_topic = "carrinho/comando";
WiFiClient wifiClient;
PubSubClient client(wifiClient);
bool farolLigado = false;

// ======= HTTPS endpoint =======
const char* serverUrl = "https://carrinho-mqtt-b118.herokuapp.com/upload64";

// ======= Pinos ESP32-CAM AI-Thinker =======
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define LED_GPIO_NUM       4

void disableBrownout() {
  WRITE_PERI_REG(0x600080A4, 0);
}

// ======= Função callback MQTT =======
void callback(char* topic, byte* message, unsigned int length) {
  String comando = "";
  for (unsigned int i = 0; i < length; i++) {
    comando += (char)message[i];
  }

  Serial.println("Comando MQTT recebido: " + comando);

  if (comando == "farol") {
    farolLigado = !farolLigado;
    digitalWrite(LED_GPIO_NUM, farolLigado ? HIGH : LOW);
    Serial.println(farolLigado ? "LED ligado" : "LED desligado");
  }
}

// ======= Reconectar MQTT =======
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.println("Conectando ao MQTT...");
    if (client.connect("ESP32CAMClient")) {
      Serial.println("MQTT conectado");
      client.subscribe(mqtt_command_topic);
    } else {
      Serial.print("Erro MQTT: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  disableBrownout();

  pinMode(LED_GPIO_NUM, OUTPUT);
  digitalWrite(LED_GPIO_NUM, LOW);

  // ======= Wi-Fi =======
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi...");
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFalha ao conectar no Wi-Fi. Reiniciando...");
    ESP.restart();
  }

  Serial.println("\nWi-Fi conectado!");
  Serial.println(WiFi.localIP());

  // ======= Inicializa Câmera =======
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_QQVGA;
  config.jpeg_quality = 12;
  config.fb_count     = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Erro ao iniciar a câmera. Reiniciando...");
    delay(1000);
    ESP.restart();
  }

  // ======= MQTT =======
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi desconectado. Reiniciando...");
    delay(1000);
    ESP.restart();
  }

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Erro ao capturar imagem. Reiniciando...");
    delay(1000);
    ESP.restart();
  }

  String imagemBase64 = base64::encode(fb->buf, fb->len);
  esp_camera_fb_return(fb);

  String payload = "{\"imagemBase64\":\"" + imagemBase64 + "\"}";

  HTTPClient https;
  https.begin(serverUrl);
  https.addHeader("Content-Type", "application/json");

  int httpResponseCode = https.POST(payload);

  if (httpResponseCode > 0) {
    Serial.printf("POST enviado! Código HTTP: %d\n", httpResponseCode);
    Serial.println(https.getString());
  } else {
    Serial.printf("Erro no POST (%s). Reiniciando...\n", https.errorToString(httpResponseCode).c_str());
    https.end();
    delay(1000);
    ESP.restart();
  }

  https.end();
  delay(3000);
}
