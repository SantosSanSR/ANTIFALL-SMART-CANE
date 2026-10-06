// ✅ CÓDIGO ACTUALIZADO PARA BASTÓN INTELIGENTE V2.1
// ✅ Con acumulación de triggers, tolerancia a sensores desconectados y mejor gestión de estados
// Incluye: LED como indicador de nivel de alerta, lógica de detección de caídas más robusta, mensajes claros en Serial

#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <MPU6050_tockn.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "heartRate.h"

// Pines
#define IMPACT_PIN_1    27
#define IMPACT_PIN_2    26
#define IMPACT_PIN_3    33
#define BUZZER_PIN      25
#define BUTTON_PIN      36
#define PIR_PIN         0
#define LED_PIN         23
#define SDA_PIN         21
#define SCL_PIN         22

// Pantalla
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Sensores
MPU6050 mpu(Wire);
MAX30105 particleSensor;

// Telegram
const char* ssid = "***********";
const char* password = "**********";
#define BOT_TOKEN "*********************************"
#define CHAT_ID "***************************"
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// Estructuras
struct Sensors {
  bool impact1 = false;
  bool impact2 = false;
  bool impact3 = false;
  bool mpu = false;
  bool heart = false;
  bool pir = false;
  bool wifi = false;
};

struct Triggers {
  bool impact1 = false;
  bool impact2 = false;
  bool impact3 = false;
  bool fall = false;
  bool heart = false;
  bool motion = false;
};

Sensors sensors;
Triggers triggers;

// Estados
enum State {
  CALIBRATING,
  NORMAL,
  ALERT_PENDING,
  AWAIT_CONFIRMATION,
  EMERGENCY,
  CANCELLED
};

State currentState = CALIBRATING;

// Variables globales
unsigned long lastMotion = 0;
unsigned long lastTriggerTime = 0;
unsigned long ledTimer = 0;
unsigned long bpmLast = 0;
int triggerCount = 0;
int avgBPM = 0;
int totalFunctionalSensors = 0;
bool ledState = false;
bool heartAvailable = false;
bool mpuCalibrated = false;
float pitchBase = 0, rollBase = 0;
const float accelThreshold = 2.3;
const int BPM_MIN = 45, BPM_MAX = 130;
const unsigned long TIMEOUT_TRIGGER = 10000;
const unsigned long TIMEOUT_CONFIRM = 10000;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(IMPACT_PIN_1, INPUT_PULLUP);
  pinMode(IMPACT_PIN_2, INPUT_PULLUP);
  pinMode(IMPACT_PIN_3, INPUT_PULLUP);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10, 10);
  display.println("Calibrando...");
  display.display();

  mpu.begin();
  mpu.calcGyroOffsets(true);
  delay(2000);
  pitchBase = mpu.getAngleX();
  rollBase = mpu.getAngleY();
  mpuCalibrated = true;

  if (particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    particleSensor.setup();
    heartAvailable = true;
  }

  connectWiFi();
  currentState = NORMAL;
  Serial.println("Sistema listo");
}

void loop() {
  readSensors();
  updateLED();

  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Botón presionado. Cancelando alarma.");
    resetTriggers();
    currentState = CANCELLED;
  }

  switch (currentState) {
    case NORMAL:
      if (triggerCount > 0) {
        currentState = ALERT_PENDING;
        lastTriggerTime = millis();
        Serial.println("[ALERTA PENDIENTE] Primer trigger activado");
      }
      break;

    case ALERT_PENDING:
      if (triggerCount >= 2 && millis() - lastTriggerTime > 5000) {
        currentState = AWAIT_CONFIRMATION;
        Serial.println("[ESPERANDO CONFIRMACION] Triggers acumulados: " + String(triggerCount));
      } else if (millis() - lastTriggerTime > 5000) {
        resetTriggers();
        currentState = NORMAL;
        Serial.println("[ALERTA DESCARTADA] Solo un trigger activo");
      }
      break;

    case AWAIT_CONFIRMATION:
      if (triggerCount >= 4) {
        currentState = EMERGENCY;
        Serial.println("[EMERGENCIA DETECTADA]");
        sendAlert();
      } else if (millis() - lastTriggerTime > TIMEOUT_CONFIRM) {
        resetTriggers();
        currentState = NORMAL;
        Serial.println("[CANCELADO] No se acumuló suficiente evidencia");
      }
      break;

    case EMERGENCY:
      digitalWrite(BUZZER_PIN, HIGH);
      break;

    case CANCELLED:
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(LED_PIN, LOW);
      resetTriggers();
      currentState = NORMAL;
      break;
  }

  delay(50);
}

void readSensors() {
  triggerCount = 0;

  if (mpuCalibrated) {
    mpu.update();
    float acc = sqrt(pow(mpu.getAccX(), 2) + pow(mpu.getAccY(), 2) + pow(mpu.getAccZ(), 2));
    if (acc > accelThreshold) {
      triggers.fall = true;
      triggerCount++;
    }
  }

  if (digitalRead(IMPACT_PIN_1) == LOW) { triggers.impact1 = true; triggerCount++; }
  if (digitalRead(IMPACT_PIN_2) == LOW) { triggers.impact2 = true; triggerCount++; }
  if (digitalRead(IMPACT_PIN_3) == LOW) { triggers.impact3 = true; triggerCount++; }

  if (heartAvailable) {
    long ir = particleSensor.getIR();
    if (checkForBeat(ir)) {
      long dt = millis() - bpmLast;
      if (dt > 250) {
        bpmLast = millis();
        int bpm = 60.0 / (dt / 1000.0);
        if (bpm > BPM_MAX || bpm < BPM_MIN) {
          triggers.heart = true;
          triggerCount++;
        }
      }
    }
  }

  if (digitalRead(PIR_PIN) == LOW && millis() - lastMotion > 15000) {
    triggers.motion = true;
    triggerCount++;
  } else if (digitalRead(PIR_PIN) == HIGH) {
    lastMotion = millis();
  }
}

void resetTriggers() {
  triggers = Triggers();
  triggerCount = 0;
  Serial.println("Triggers reseteados");
}

void sendAlert() {
  if (WiFi.status() == WL_CONNECTED) {
    String msg = "🚨 CAÍDA DETECTADA 🚨\n";
    msg += "Impactos: ";
    msg += (triggers.impact1 ? "1 " : "");
    msg += (triggers.impact2 ? "2 " : "");
    msg += (triggers.impact3 ? "3 " : "");
    msg += "\nMPU: ";
    msg += (triggers.fall ? "✔️" : "✖️");
    msg += "\nBPM: ";
    msg += (triggers.heart ? "Anómalos" : "OK");
    msg += "\nMovimiento: ";
    msg += (triggers.motion ? "NO" : "Sí");
    bot.sendMessage(CHAT_ID, msg, "");
  }
}

void updateLED() {
  int freq = triggerCount * 2;
  if (freq == 0) {
    digitalWrite(LED_PIN, LOW);
    return;
  }

  if (millis() - ledTimer > 1000 / freq) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
    ledTimer = millis();
  }
}

void connectWiFi() {
  WiFi.begin(ssid, password);
  int t = 0;
  while (WiFi.status() != WL_CONNECTED && t < 20) {
    delay(500);
    Serial.print(".");
    t++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    client.setInsecure();
    Serial.println("\nWiFi conectado: " + WiFi.localIP().toString());
    bot.sendMessage(CHAT_ID, "🟢 Sistema listo", "");
  } else {
    Serial.println("\n❌ Sin conexión WiFi");
  }
}
