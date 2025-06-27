#include <ps5Controller.h>

// Definições de pinos
#define LED_PIN   2    // LED da placa ESP32
#define RELE_L1   4    // Relé controlado por L1
#define RELE_L2   5    // Relé controlado por L2

// Flags para evitar prints repetidos
bool l1Pressed = false;
bool r2Pressed = false;

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(RELE_L1, OUTPUT);
  pinMode(RELE_L2, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(RELE_L1, LOW);
  digitalWrite(RELE_L2, LOW);

  ps5.begin("58:10:31:bd:52:38"); // Substitua com o MAC do seu controle
  Serial.println("Pronto para conectar ao controle PS5...");
}

void loop() {
  if (ps5.isConnected()) {
    // LED permanece aceso enquanto conectado
    digitalWrite(LED_PIN, HIGH);

    // L1: relé + mensagem única
    if (ps5.L1()) {
      digitalWrite(RELE_L1, HIGH);
      if (!l1Pressed) {
        Serial.println("L1 pressionado");
        l1Pressed = true;
      }
    } else {
      digitalWrite(RELE_L1, LOW);
      l1Pressed = false;
    }

    // L2: relé simples (sem mensagem na serial)
    digitalWrite(RELE_L2, ps5.L2() ? HIGH : LOW);

    // R2: mensagem única
    if (ps5.R2()) {
      if (!r2Pressed) {
        Serial.println("R2 pressionado");
        r2Pressed = true;
      }
    } else {
      r2Pressed = false;
    }

  } else {
    // Pisca LED enquanto não conectado
    digitalWrite(LED_PIN, HIGH);
    delay(300);
    digitalWrite(LED_PIN, LOW);
    delay(300);

    // Desliga os relés por segurança
    digitalWrite(RELE_L1, LOW);
    digitalWrite(RELE_L2, LOW);

    // Reseta flags para evitar erro ao reconectar
    l1Pressed = false;
    r2Pressed = false;
  }
}
