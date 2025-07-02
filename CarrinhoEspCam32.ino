#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>

// === CONFIG Wi-Fi ===
const char* ssid = "HP1202";
const char* password = "47asdfsdfahasdfo";

// === CONFIG SERVIDOR HTTPS ===
const char* serverName = "https://carrinho-mqtt-b4118.herokuapp.com/upload";

WiFiClientSecure client;

// === PINOS ESP32-CAM (AI THINKER) ===
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

// === LED do flash (GPIO 4 no AI-Thinker) ===
#define FLASH_GPIO_NUM 4

void disableBrownout() {
  WRITE_PERI_REG(0x600080A4, 0);
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  delay(1000);

  disableBrownout();

  // Inicializa pino do flash
  pinMode(FLASH_GPIO_NUM, OUTPUT);
  digitalWrite(FLASH_GPIO_NUM, LOW);

  // Conexão Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Conectando ao Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado!");
  Serial.println(WiFi.localIP());

  client.setInsecure();

  // Inicializar câmera
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

 //config.frame_size = FRAMESIZE_QQVGA; // 160x120 (baixa)
  config.frame_size = FRAMESIZE_UXGA; // 1600x1200 (máxima)
 // config.jpeg_quality = 12;
 // Qualidade entre 0 (máx) e 63 (mín). Use 10~12.
  config.jpeg_quality = 10;
 // config.fb_count = 1;
 config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Erro ao iniciar câmera: 0x%x\n", err);
    return;
  }
}

void loop() {
  // Liga o flash
  digitalWrite(FLASH_GPIO_NUM, HIGH);
  delay(200); // Tempo para iluminar a cena antes da captura

  camera_fb_t * fb = esp_camera_fb_get();

  // Desliga o flash
  digitalWrite(FLASH_GPIO_NUM, LOW);

  if (!fb) {
    Serial.println("Erro ao capturar imagem");
    delay(5000);
    return;
  }

  Serial.printf("Imagem capturada (%d bytes). Enviando...\n", fb->len);

  if (client.connect("carrinho-mqtt-b118.herokuapp.com", 443)) {
    String boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    String head = "--" + boundary + "\r\n" +
                  "Content-Disposition: form-data; name=\"image\"; filename=\"cam.jpg\"\r\n" +
                  "Content-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--" + boundary + "--\r\n";

    uint32_t contentLength = head.length() + fb->len + tail.length();

    client.println("POST /upload HTTP/1.1");
    client.println("Host: carrinho-mqttfb118.herokuapp.com");
    client.println("Content-Type: multipart/form-data; boundary=" + boundary);
    client.println("Content-Length: " + String(contentLength));
    client.println();
    client.print(head);
    client.write(fb->buf, fb->len);
    client.print(tail);

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") break;
    }

    String response = client.readString();
    Serial.println("Resposta do servidor:");
    Serial.println(response);
  } else {
    Serial.println("Falha na conexão HTTPS");
  }

  esp_camera_fb_return(fb);
  delay(1000);
}
