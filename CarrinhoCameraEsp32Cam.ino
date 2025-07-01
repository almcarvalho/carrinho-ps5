#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include "esp_camera.h"

// Definições da câmera ESP32-CAM AI-Thinker
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27

#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

#define LED_FLASH_PIN     4
#define LED_STATUS_PIN   33  // LED externo de status

const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_topic  = "carrinho/camera";

WiFiClient espClient;
PubSubClient client(espClient);
WiFiManager wm;

bool wifiConectado = false;
bool portalAtivo = false;
unsigned long previousMillis = 0;
const long intervaloPiscar = 100;

void setup() {
  Serial.begin(115200);
  pinMode(LED_FLASH_PIN, OUTPUT);
  pinMode(LED_STATUS_PIN, OUTPUT);
  digitalWrite(LED_FLASH_PIN, LOW);
  digitalWrite(LED_STATUS_PIN, LOW);

  Serial.println("Verificando PSRAM...");
  if (psramFound()) {
    Serial.println("PSRAM detectada");
  } else {
    Serial.println("PSRAM NÃO detectada");
  }

  initCamera();

  Serial.println("Tentando conectar ao Wi-Fi salvo...");
  WiFi.begin();
  unsigned long tempoInicial = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - tempoInicial < 5000) {
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi conectado!");
    wifiConectado = true;
    digitalWrite(LED_STATUS_PIN, HIGH);
  } else {
    Serial.println("Wi-Fi falhou. Iniciando portal...");
    wm.setConfigPortalBlocking(false); // NÃO BLOQUEANTE
    wm.startConfigPortal("ESP32CAM");
    portalAtivo = true;
  }

  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (portalAtivo) {
    wm.process();
    piscarLedStatus();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Wi-Fi conectado via portal!");
      wifiConectado = true;
      portalAtivo = false;
      digitalWrite(LED_STATUS_PIN, HIGH);
    }
    return;
  }

  if (wifiConectado) {
    if (!client.connected()) {
      reconnectMQTT();
    }
    client.loop();

    static unsigned long lastCapture = 0;
    if (millis() - lastCapture > 5000) {
      lastCapture = millis();
      capturarEEnviarImagem();
    }
  }
}

void piscarLedStatus() {
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink >= intervaloPiscar) {
    lastBlink = millis();
    digitalWrite(LED_STATUS_PIN, !digitalRead(LED_STATUS_PIN));
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    String clientId = "ESP32CAM-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado ao MQTT");
    } else {
      Serial.print(".");
      delay(1000);
    }
  }
}

void initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QQVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Erro ao iniciar câmera: 0x%x\n", err);
  } else {
    Serial.println("Câmera iniciada com sucesso");
  }
}

void capturarEEnviarImagem() {
  Serial.println("Capturando imagem...");
  delay(100); // Tempo para estabilizar

  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Falha ao capturar imagem");
    return;
  }

  Serial.printf("Imagem capturada: %d bytes\n", fb->len);
  client.beginPublish(mqtt_topic, fb->len, false);
  client.write(fb->buf, fb->len);
  client.endPublish();

  esp_camera_fb_return(fb);
}
