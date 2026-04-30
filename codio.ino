#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>

/* ============================================================
   CONFIGURAÇÃO - tudo em um lugar só
   ============================================================ */

// --- Wi-Fi (Wokwi) ---
const char* WIFI_SSID     = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// --- MQTT / FIWARE ---
const char* MQTT_BROKER   = "136.116.47.189";
const int   MQTT_PORT     = 1883;
const char* MQTT_TOPIC    = "/TEF/chaveiro001/attrs";
const char* MQTT_CLIENT_ID = "chaveiro001";

// --- Limiares da flor (modo TESTE - bate com flower_stages.md) ---
const int LIMITE_SPROUT = 10;
const int LIMITE_BUD    = 30;
const int LIMITE_BLOOM  = 60;
const int LIMITE_FULL   = 100;

// --- Detector de passo ---
const float         PASSO_LIMIAR        = 12.0;
const unsigned long PASSO_DEBOUNCE_MS   = 300;

// --- Frequência de envio MQTT ---
const int           PASSOS_POR_ENVIO    = 5;
const unsigned long ENVIO_TIMEOUT_MS    = 15000;

// --- Inatividade ---
const unsigned long INATIVIDADE_MS      = 30000;
const unsigned long REPETE_ALERTA_MS    = 30000;

// --- Pinos ---
const int PIN_BTN_NEXT   = 14;
const int PIN_BTN_SELECT = 27;
const int PIN_BUZZER     = 13;

// --- OLED ---
const int OLED_LARGURA = 128;
const int OLED_ALTURA  = 64;

/* ============================================================
   OBJETOS GLOBAIS
   ============================================================ */

Adafruit_MPU6050 mpu;
Adafruit_SSD1306 oled(OLED_LARGURA, OLED_ALTURA, &Wire, -1);
WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);

/* ============================================================
   ESTADO DO SISTEMA
   ============================================================ */

int passos             = 0;
int estagio            = 0;
int passosUltimoEnvio  = 0;
unsigned long ultimoEnvioMs   = 0;
unsigned long ultimoPassoMs   = 0;
unsigned long ultimoAlertaMs  = 0;
unsigned long ultimoPicoMs    = 0;

bool modoSilencioso = false;

// --- Menu ---
enum TelaAtual { TELA_FLOR, TELA_MENU, TELA_STATUS };
TelaAtual telaAtual = TELA_FLOR;
int menuSelecionado = 0;
const char* opcoesMenu[] = { "Voltar", "Status", "Resetar passos", "Modo silencioso" };
const int NUM_OPCOES_MENU = 4;
unsigned long telaStatusAteMs = 0;

// --- Debounce dos botões ---
bool ultimoEstadoNext   = HIGH;
bool ultimoEstadoSelect = HIGH;
unsigned long ultimoToqueNextMs   = 0;
unsigned long ultimoToqueSelectMs = 0;
const unsigned long DEBOUNCE_BOTAO_MS = 200;

/* ============================================================
   SETUP
   ============================================================ */

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n=== Tamagotchi Flora ===");

  pinMode(PIN_BTN_NEXT, INPUT_PULLUP);
  pinMode(PIN_BTN_SELECT, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  Wire.begin();

  // OLED
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Erro: OLED nao encontrado");
    while (true) delay(100);
  }
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(10, 28);
  oled.println("Iniciando...");
  oled.display();

  // MPU-6050
  if (!mpu.begin()) {
    Serial.println("Erro: MPU6050 nao encontrado");
    oled.clearDisplay();
    oled.setCursor(0, 28);
    oled.println("Erro MPU!");
    oled.display();
    while (true) delay(100);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println("MPU-6050 ok");

  conectarWiFi();

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  conectarMQTT();

  ultimoPassoMs  = millis();
  ultimoEnvioMs  = millis();
  ultimoAlertaMs = millis();

  // Splash screen de abertura "Health Plus"
  mostrarSplash();

  Serial.println("Setup completo");
}

/* ============================================================
   SPLASH SCREEN - animação de abertura "Health Plus"
   ============================================================ */

void mostrarSplash() {
  Serial.println("Mostrando splash...");

  // Fase 1: flor crescendo no centro (estágios 0..4)
  for (int est = 0; est <= 4; est++) {
    oled.clearDisplay();
    desenharFlor(64, 32, est);
    oled.display();
    delay(250);
  }

  // Fase 2: flor florida estabiliza
  delay(500);

  // Fase 3: nome aparece embaixo da flor
  oled.clearDisplay();
  desenharFlor(64, 24, 4);   // flor mais pra cima
  oled.setTextSize(2);
  oled.setTextColor(SSD1306_WHITE);
  // "Health Plus" tem 11 chars * 12px = 132px (passa de 128).
  // Em vez de quebrar, separamos em 2 linhas: "HEALTH" e "PLUS".
  oled.setCursor(40, 48);
  oled.print("HEALTH");
  oled.display();
  delay(400);

  // Fase 4: tela cheia "HEALTH PLUS" centralizado
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(28, 16);
  oled.print("HEALTH");
  oled.setCursor(40, 36);
  oled.print("PLUS");
  // Linha decorativa embaixo
  oled.drawLine(20, 58, 108, 58, SSD1306_WHITE);
  oled.display();
  delay(700);

  oled.clearDisplay();
  oled.display();
  delay(150);
}

/* ============================================================
   LOOP PRINCIPAL
   ============================================================ */

void loop() {
  if (!mqtt.connected()) {
    conectarMQTT();
  }
  mqtt.loop();

  detectarPasso();
  lerBotoes();
  verificarEnvioMQTT();
  verificarInatividade();
  desenharTela();

  delay(20);
}

/* ============================================================
   FLOR - cálculo do estágio e nomes
   ============================================================ */

int calcularEstagio(int p) {
  if (p < LIMITE_SPROUT) return 0;
  if (p < LIMITE_BUD)    return 1;
  if (p < LIMITE_BLOOM)  return 2;
  if (p < LIMITE_FULL)   return 3;
  return 4;
}

const char* nomeEstagio(int e) {
  switch (e) {
    case 0: return "Semente";
    case 1: return "Broto";
    case 2: return "Folhas";
    case 3: return "Botao";
    case 4: return "Florida!";
    default: return "?";
  }
}

/* ============================================================
   DETECTOR DE PASSO
   ============================================================ */

void detectarPasso() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float modulo = sqrt(
    a.acceleration.x * a.acceleration.x +
    a.acceleration.y * a.acceleration.y +
    a.acceleration.z * a.acceleration.z
  );

  unsigned long agora = millis();

  if (modulo > PASSO_LIMIAR &&
      (agora - ultimoPicoMs) > PASSO_DEBOUNCE_MS) {
    passos++;
    estagio = calcularEstagio(passos);
    ultimoPicoMs  = agora;
    ultimoPassoMs = agora;
    Serial.printf("Passo %d (estagio %d) | accel=%.2f\n",
                  passos, estagio, modulo);
  }
}

/* ============================================================
   BOTÕES E MENU
   ============================================================ */

void lerBotoes() {
  unsigned long agora = millis();

  bool estadoNext = digitalRead(PIN_BTN_NEXT);
  if (estadoNext == LOW && ultimoEstadoNext == HIGH &&
      (agora - ultimoToqueNextMs) > DEBOUNCE_BOTAO_MS) {
    ultimoToqueNextMs = agora;
    onBotaoNext();
  }
  ultimoEstadoNext = estadoNext;

  bool estadoSelect = digitalRead(PIN_BTN_SELECT);
  if (estadoSelect == LOW && ultimoEstadoSelect == HIGH &&
      (agora - ultimoToqueSelectMs) > DEBOUNCE_BOTAO_MS) {
    ultimoToqueSelectMs = agora;
    onBotaoSelect();
  }
  ultimoEstadoSelect = estadoSelect;
}

void onBotaoNext() {
  if (telaAtual == TELA_FLOR) {
    telaAtual = TELA_MENU;
    menuSelecionado = 0;
  } else if (telaAtual == TELA_MENU) {
    menuSelecionado = (menuSelecionado + 1) % NUM_OPCOES_MENU;
  }
}

void onBotaoSelect() {
  if (telaAtual == TELA_MENU) {
    switch (menuSelecionado) {
      case 0:  // Voltar
        telaAtual = TELA_FLOR;
        break;
      case 1:  // Status
        telaAtual = TELA_STATUS;
        telaStatusAteMs = millis() + 3000;
        break;
      case 2:  // Resetar passos
        passos = 0;
        estagio = 0;
        passosUltimoEnvio = 0;
        publicarEstado();
        telaAtual = TELA_FLOR;
        break;
      case 3:  // Modo silencioso
        modoSilencioso = !modoSilencioso;
        telaAtual = TELA_FLOR;
        break;
    }
  }
}

/* ============================================================
   DESENHO NO OLED
   ============================================================ */

void desenharTela() {
  if (telaAtual == TELA_STATUS && millis() > telaStatusAteMs) {
    telaAtual = TELA_FLOR;
  }

  oled.clearDisplay();

  if (telaAtual == TELA_FLOR) {
    desenharTelaFlor();
  } else if (telaAtual == TELA_MENU) {
    desenharMenu();
  } else if (telaAtual == TELA_STATUS) {
    desenharStatus();
  }

  oled.display();
}

void desenharTelaFlor() {
  // Cabeçalho com passos
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.printf("Passos: %d/%d", passos, LIMITE_FULL);
  if (modoSilencioso) {
    oled.setCursor(108, 0);
    oled.print("M");
  }

  // Linha separadora
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Centro: a flor (visual reaproveitado do código antigo)
  desenharFlor(64, 38, estagio);

  // Nome do estágio embaixo
  oled.setCursor(0, 56);
  oled.print("Estagio: ");
  oled.print(nomeEstagio(estagio));
}

/* --- FLOR (visual reaproveitado do código antigo, levemente
   ajustado para caber na área disponível 11..55) --- */
void desenharFlor(int cx, int cy, int est) {
  switch (est) {
    case 0:  // Semente: bolinha no chão
      oled.fillCircle(cx, cy + 8, 3, SSD1306_WHITE);
      oled.drawLine(cx - 10, cy + 12, cx + 10, cy + 12, SSD1306_WHITE);
      break;

    case 1:  // Broto: caule curto + folhinha
      oled.drawLine(cx, cy + 12, cx, cy + 4, SSD1306_WHITE);
      oled.fillCircle(cx, cy + 2, 2, SSD1306_WHITE);
      oled.drawLine(cx - 10, cy + 12, cx + 10, cy + 12, SSD1306_WHITE);
      break;

    case 2:  // Folhas: caule + 2 folhas triangulares
      oled.drawLine(cx, cy + 12, cx, cy - 4, SSD1306_WHITE);
      oled.fillTriangle(cx, cy + 4, cx - 6, cy + 2, cx, cy + 8, SSD1306_WHITE);
      oled.fillTriangle(cx, cy,     cx + 6, cy - 2, cx, cy + 4, SSD1306_WHITE);
      oled.drawLine(cx - 12, cy + 12, cx + 12, cy + 12, SSD1306_WHITE);
      break;

    case 3:  // Botão: caule alto + folhas + botão fechado
      oled.drawLine(cx, cy + 12, cx, cy - 8, SSD1306_WHITE);
      oled.fillTriangle(cx, cy + 2, cx - 6, cy,     cx, cy + 6, SSD1306_WHITE);
      oled.fillTriangle(cx, cy - 2, cx + 6, cy - 4, cx, cy + 2, SSD1306_WHITE);
      oled.fillCircle(cx, cy - 10, 4, SSD1306_WHITE);
      oled.drawLine(cx - 14, cy + 12, cx + 14, cy + 12, SSD1306_WHITE);
      break;

    case 4:  // Florida: 4 pétalas + miolo preto
      oled.drawLine(cx, cy + 12, cx, cy - 6, SSD1306_WHITE);
      oled.fillTriangle(cx, cy + 4, cx - 7, cy + 2, cx, cy + 8, SSD1306_WHITE);
      oled.fillTriangle(cx, cy,     cx + 7, cy - 2, cx, cy + 4, SSD1306_WHITE);
      oled.fillCircle(cx - 5, cy - 10, 3, SSD1306_WHITE);
      oled.fillCircle(cx + 5, cy - 10, 3, SSD1306_WHITE);
      oled.fillCircle(cx,     cy - 14, 3, SSD1306_WHITE);
      oled.fillCircle(cx,     cy - 6,  3, SSD1306_WHITE);
      oled.fillCircle(cx,     cy - 10, 2, SSD1306_BLACK);
      oled.drawLine(cx - 14, cy + 12, cx + 14, cy + 12, SSD1306_WHITE);
      break;
  }
}

void desenharMenu() {
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("== MENU ==");
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  for (int i = 0; i < NUM_OPCOES_MENU; i++) {
    int y = 14 + i * 11;
    if (i == menuSelecionado) {
      oled.fillRect(0, y - 1, 127, 10, SSD1306_WHITE);
      oled.setTextColor(SSD1306_BLACK);
    } else {
      oled.setTextColor(SSD1306_WHITE);
    }
    oled.setCursor(2, y);
    oled.print(opcoesMenu[i]);
    if (i == 3) {
      oled.print(modoSilencioso ? " [ON]" : " [OFF]");
    }
  }
  oled.setTextColor(SSD1306_WHITE);
}

void desenharStatus() {
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("== STATUS ==");
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  oled.setCursor(0, 16);
  oled.printf("Passos: %d", passos);
  oled.setCursor(0, 28);
  oled.printf("Meta:   %d", LIMITE_FULL);
  oled.setCursor(0, 40);
  oled.printf("Estagio: %s", nomeEstagio(estagio));
  oled.setCursor(0, 52);
  oled.printf("WiFi: %s",
              WiFi.status() == WL_CONNECTED ? "OK" : "X");
}

/* ============================================================
   WI-FI E MQTT
   ============================================================ */

void conectarWiFi() {
  Serial.printf("Conectando WiFi a %s ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nWiFi ok. IP: %s\n",
                  WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\nWiFi FALHOU");
  }
}

void conectarMQTT() {
  if (WiFi.status() != WL_CONNECTED) return;

  while (!mqtt.connected()) {
    Serial.printf("Conectando MQTT a %s:%d ... ", MQTT_BROKER, MQTT_PORT);
    if (mqtt.connect(MQTT_CLIENT_ID)) {
      Serial.println("ok");
    } else {
      Serial.printf("falhou rc=%d, tentando de novo em 3s\n",
                    mqtt.state());
      delay(3000);
    }
  }
}

void verificarEnvioMQTT() {
  unsigned long agora = millis();
  int passosNovos = passos - passosUltimoEnvio;
  bool porContagem = passosNovos >= PASSOS_POR_ENVIO;
  bool porTempo    = (agora - ultimoEnvioMs) >= ENVIO_TIMEOUT_MS &&
                     passosNovos > 0;

  if (porContagem || porTempo) {
    publicarEstado();
  }
}

void publicarEstado() {
  if (!mqtt.connected()) return;

  char timestamp[32];
  unsigned long s = millis() / 1000;
  snprintf(timestamp, sizeof(timestamp),
           "2026-04-28T00:00:%02luZ", s % 60);

  char payload[96];
  snprintf(payload, sizeof(payload),
           "s|%d|fs|%d|ts|%s",
           passos, estagio, timestamp);

  bool ok = mqtt.publish(MQTT_TOPIC, payload);
  Serial.printf("Publicou [%s]: %s -> %s\n",
                MQTT_TOPIC, payload, ok ? "ok" : "FALHOU");

  passosUltimoEnvio = passos;
  ultimoEnvioMs     = millis();
}

/* ============================================================
   ALERTA DE INATIVIDADE
   ============================================================ */

void verificarInatividade() {
  if (modoSilencioso) return;
  if (passos == 0)    return;

  unsigned long agora   = millis();
  unsigned long inativo = agora - ultimoPassoMs;

  if (inativo > INATIVIDADE_MS &&
      (agora - ultimoAlertaMs) > REPETE_ALERTA_MS) {
    tocarAlerta();
    ultimoAlertaMs = agora;
  }
}

void tocarAlerta() {
  Serial.println("Alerta: hora de se mexer!");
  int notas[] = { 523, 659, 784, 1047 };  // C5, E5, G5, C6
  for (int i = 0; i < 4; i++) {
    tone(PIN_BUZZER, notas[i], 150);
    delay(180);
  }
  noTone(PIN_BUZZER);
}