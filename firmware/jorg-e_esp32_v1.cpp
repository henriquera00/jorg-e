#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h> 
#include "BluetoothSerial.h"

// ===== CONFIGURAÇÕES DE HARDWARE =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define PINO_LED 5    
#define PINO_TOUCH 4  

// ===== OBJETOS =====
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
BluetoothSerial SerialBT;

// ===== EXPRESSÕES =====
const uint8_t FELIZ = 0;
const uint8_t RAIVA = 1;
const uint8_t TRISTE = 2;
uint8_t expressao = FELIZ;
uint32_t fimExpressao = 0;
const uint16_t DURACAO_EXPRESSAO = 3000;

// ===== GEOMETRIA DO ROSTO =====
const int CENTRO_ESQ_X = 35;
const int CENTRO_DIR_X = 93;
const int OLHOS_Y = 24;
const int RAIO_X = 12;
const int RAIO_Y_ABERTO = 16;
const int RAIO_Y_FECHADO = 2;
const int BOCA_X = 64;
const int BOCA_Y = 50;

// ===== CONTROLE DE ANIMAÇÃO =====
const uint16_t INTERVALO_FRAME = 33; // ~30 fps
uint32_t ultimoFrame = 0;

float posX = 0;           
int alvoX = 0;            
int raioYAtual = RAIO_Y_ABERTO; 

enum EstadoPiscada { ABERTO, FECHANDO, ABRINDO };
EstadoPiscada estadoPiscada = ABERTO;

uint32_t proximoOlhar = 0;
uint32_t proximaPiscada = 0;

int ultimoX = 9999;
int ultimoRaioY = -1;
uint8_t ultimaExpressao = 255;

void setup() {
  // Inicializa USB (para o Processing) e Bluetooth (para o celular)
  Serial.begin(9600); 
  SerialBT.begin("JORG-E"); // Nome do dispositivo Bluetooth
  
  pinMode(PINO_LED, OUTPUT);
  digitalWrite(PINO_LED, LOW);

  Wire.begin(21, 22);
  
  // Inicializa Display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha no OLED"));
    for(;;);
  }
  
  telaIntro();

  // Inicializa Acelerômetro
  if (!accel.begin()) {
    Serial.println("Falha no ADXL345. Verifique as ligacoes.");
  } else {
    accel.setRange(ADXL345_RANGE_16_G);
  }
  
  // Tempos iniciais da animação
  uint32_t agora = millis();
  proximoOlhar = agora + 1500;
  proximaPiscada = agora + 2000;
}

void loop() {
  uint32_t agora = millis();

  lerSerial(agora);
  lerBluetooth(agora);
  lerAcelerometro(agora);
  lerTouch(agora);

  if (expressao != FELIZ && agora >= fimExpressao) {
    expressao = FELIZ;
  }

  // Controle de FPS da animação
  if (agora - ultimoFrame < INTERVALO_FRAME) return;
  ultimoFrame = agora;

  atualizarOlhar(agora);
  atualizarPiscada(agora);

  int tremor = (expressao == RAIVA) ? (int)random(-2, 3) : 0;
  int xFinal = (int)round(posX) + tremor;

  // Só atualiza o display se algo mudou (economia de processamento I2C)
  if (xFinal != ultimoX || raioYAtual != ultimoRaioY || expressao != ultimaExpressao) {
    desenharRosto(xFinal, raioYAtual);
    ultimoX = xFinal;
    ultimoRaioY = raioYAtual;
    ultimaExpressao = expressao;
  }
}

// ===== TELA DE INTRO =====
void telaIntro() {
  const char* texto = "JORG-E"; 
  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(texto, 0, 0, &x1, &y1, &w, &h);
  int16_t x = (SCREEN_WIDTH - w) / 2 - x1;
  int16_t y = (SCREEN_HEIGHT - h) / 2 - y1;

  for (uint8_t i = 1; i <= 6; i++) {
    display.clearDisplay();
    display.setCursor(x, y);
    for (uint8_t j = 0; j < i; j++) display.write(texto[j]);
    display.display();
    delay(150);
  }
  delay(1000);
}

// ===== COMUNICAÇÃO =====
void lerSerial(uint32_t agora) {
  while (Serial.available()) {
    processarComando(Serial.read(), agora, false);
  }
}

void lerBluetooth(uint32_t agora) {
  while (SerialBT.available()) {
    processarComando(SerialBT.read(), agora, true);
  }
}

void processarComando(char c, uint32_t agora, bool isBT) {
  if (c == 'R') definirExpressao(RAIVA, agora);
  else if (c == 'T') definirExpressao(TRISTE, agora);
  else if (c == 'F') definirExpressao(FELIZ, agora);
  else if (c == 'L') {
    digitalWrite(PINO_LED, HIGH);
    if(isBT) SerialBT.println("LED_LIGADO");
    else Serial.println("LED_LIGADO");
  }
  else if (c == 'D') {
    digitalWrite(PINO_LED, LOW);
    if(isBT) SerialBT.println("LED_DESLIGADO");
    else Serial.println("LED_DESLIGADO");
  }
}

// ===== SENSORES =====
void lerAcelerometro(uint32_t agora) {
  static uint32_t ultimaLeitura = 0;
  if (agora - ultimaLeitura < 50) return; 
  ultimaLeitura = agora;
  
  sensors_event_t event;
  accel.getEvent(&event);

  float x = event.acceleration.x;
  float y = event.acceleration.y;
  float z = event.acceleration.z;

  float forca = sqrt((x*x) + (y*y) + (z*z));

  // Detecta sacudida (Raiva)
  if (forca > 16.0) {
    if (expressao != RAIVA) {
      definirExpressao(RAIVA, agora);
      Serial.println("EVENTO:SACUDIDO");
    }
  }
  // Detecta se virou de cabeça para baixo (Triste)
  else if (z < -7.5) {
    if (expressao != TRISTE) {
      definirExpressao(TRISTE, agora);
      Serial.println("EVENTO_CABECA_BAIXO");
    }
  }
}

void lerTouch(uint32_t agora) {
  static uint32_t ultimoTouch = 0;
  if (agora - ultimoTouch < 1000) return; 

  if (touchRead(PINO_TOUCH) < 20) { 
    if (expressao != FELIZ) {
      definirExpressao(FELIZ, agora);
      ultimoTouch = agora;
      Serial.println("EVENTO:CARINHO");
    }
  }
}

void definirExpressao(uint8_t nova, uint32_t agora) {
  expressao = nova;
  fimExpressao = agora + DURACAO_EXPRESSAO;
}

// ===== LÓGICA DE ANIMAÇÃO =====
void atualizarOlhar(uint32_t agora) {
  if (expressao != FELIZ) {   
    alvoX = 0;
    proximoOlhar = agora + 1000;
  } else if (agora >= proximoOlhar) {
    const int opcoes[] = {-18, 0, 18, 0};
    alvoX = opcoes[random(4)];
    proximoOlhar = agora + random(1000, 2500);
  }
  
  float diff = alvoX - posX;
  if (fabs(diff) < 0.5) posX = alvoX;
  else posX += diff * 0.3; 
}

void atualizarPiscada(uint32_t agora) {
  switch (estadoPiscada) {
    case ABERTO:
      if (agora >= proximaPiscada) estadoPiscada = FECHANDO;
      break;
    case FECHANDO:
      raioYAtual -= 5;
      if (raioYAtual <= RAIO_Y_FECHADO) {
        raioYAtual = RAIO_Y_FECHADO;
        estadoPiscada = ABRINDO;
      }
      break;
    case ABRINDO:
      raioYAtual += 4;
      if (raioYAtual >= RAIO_Y_ABERTO) {
        raioYAtual = RAIO_Y_ABERTO;
        estadoPiscada = ABERTO;
        proximaPiscada = agora + (random(10) < 1 ? 300 : random(2000, 5000));
      }
      break;
  }
}

// ===== DESENHO DO ROSTO =====
void fillElipse(int cx, int cy, int rx, int ry) {
  if (ry <= 0) return; 
  for (int dy = -ry; dy <= ry; dy++) {
    int dx = rx * sqrt(1.0 - (float)(dy * dy) / (float)(ry * ry));
    display.drawFastHLine(cx - dx, cy + dy, dx * 2 + 1, SSD1306_WHITE);
  }
}

void desenharPalpebra(int cx, bool esquerdo, int raioY) {
  if (expressao == FELIZ || raioY <= RAIO_Y_FECHADO + 2) return; 
  
  bool descePelaDireita = (expressao == RAIVA) == esquerdo;
  int topo = OLHOS_Y - raioY - 1;
  int queda = (raioY * 7) / 8 + 1;
  int xPonta = descePelaDireita ? cx + RAIO_X + 1 : cx - RAIO_X - 1;
  
  display.fillTriangle(cx - RAIO_X - 1, topo, cx + RAIO_X + 1, topo, xPonta, topo + queda, SSD1306_BLACK);
}

int curvaBoca(int x) {
  if (expressao == RAIVA)  return (x * x) / 24;
  if (expressao == TRISTE) return (x * x) / 12;
  return 4 - (x * x) / 16;
}

void desenharBoca(int offsetX) {
  int cx = BOCA_X + offsetX;
  for (int x = -8; x <= 8; x++) {
    int y = curvaBoca(x);
    display.drawPixel(cx + x, BOCA_Y + y, SSD1306_WHITE);
    display.drawPixel(cx + x, BOCA_Y + y + 1, SSD1306_WHITE);
  }
}

void desenharRosto(int offsetX, int raioY) {
  display.clearDisplay();
  fillElipse(CENTRO_ESQ_X + offsetX, OLHOS_Y, RAIO_X, raioY);
  fillElipse(CENTRO_DIR_X + offsetX, OLHOS_Y, RAIO_X, raioY);
  desenharPalpebra(CENTRO_ESQ_X + offsetX, true, raioY);
  desenharPalpebra(CENTRO_DIR_X + offsetX, false, raioY);
  desenharBoca(offsetX);
  display.display();
}
