#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h> 

// ===== CONFIGURAÇÕES DE HARDWARE =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define PINO_LED 5    
#define PINO_TOUCH 4  

// ===== OBJETOS =====
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// ===== EXPRESSÕES =====
const uint8_t FELIZ = 0;
const uint8_t RAIVA = 1;
const uint8_t TRISTE = 2;
const uint8_t AMOR = 3; 
uint8_t expressao = FELIZ;
uint32_t fimExpressao = 0;

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
const uint16_t INTERVALO_FRAME = 33;
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

// ===== CONTROLE DO TOUCH =====
int estadoRepousoTouch = HIGH; 
bool estaTocando = false; 

// ===== CONTROLE DE ERRO =====
bool piscandoErro = false;
int contagemPisca = 0;
uint32_t tempoPiscaErro = 0;
bool estadoPiscaErro = false;
bool ledEstavaLigado = false;

void setup() {
  Serial.begin(9600); 
  Serial.println("Sistema Iniciado. Aguardando comandos...");
  
  pinMode(PINO_LED, OUTPUT);
  digitalWrite(PINO_LED, LOW);
  
  pinMode(PINO_TOUCH, INPUT_PULLUP); 
  
  // Auto-calibração do estado de repouso do sensor
  delay(100);
  estadoRepousoTouch = digitalRead(PINO_TOUCH);

  Wire.begin(21, 22);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha no OLED"));
    for(;;);
  }
  
  telaIntro();

  if (!accel.begin()) {
    Serial.println("Falha no ADXL345.");
  } else {
    accel.setRange(ADXL345_RANGE_16_G);
  }
  
  uint32_t agora = millis();
  proximoOlhar = agora + 1500;
  proximaPiscada = agora + 2000;
}

void loop() {
  uint32_t agora = millis();

  lerSerial(agora);
  lerAcelerometro(agora);
  lerTouch(agora);
  atualizarPiscaErro(agora);

  // O TEMPORIZADOR GERAL: Se o tempo acabar, ele volta ao normal
  if (expressao != FELIZ && agora >= fimExpressao) {
    expressao = FELIZ;
  }

  if (agora - ultimoFrame < INTERVALO_FRAME) return;
  ultimoFrame = agora;

  atualizarOlhar(agora);
  atualizarPiscada(agora);

  int tremor = (expressao == RAIVA) ? (int)random(-2, 3) : 0;
  int xFinal = (int)round(posX) + tremor;

  if (xFinal != ultimoX || raioYAtual != ultimoRaioY || expressao != ultimaExpressao) {
    desenharRosto(xFinal, raioYAtual);
    ultimoX = xFinal;
    ultimoRaioY = raioYAtual;
    ultimaExpressao = expressao;
  }
}

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

// ===== COMUNICAÇÃO SERIAL =====
void lerSerial(uint32_t agora) {
  while (Serial.available()) {
    processarComando(Serial.read(), agora);
  }
}

void processarComando(char c, uint32_t agora) {
  if (c == 'R') {
    definirExpressao(RAIVA, agora);
    Serial.println("OK: Expressao Raiva");
  }
  else if (c == 'T') {
    definirExpressao(TRISTE, agora);
    Serial.println("OK: Expressao Triste");
  }
  else if (c == 'F') {
    definirExpressao(FELIZ, agora);
    Serial.println("OK: Expressao Feliz");
  }
  else if (c == 'A') {
    definirExpressao(AMOR, agora); 
    Serial.println("OK: Expressao Amor");
  }
  else if (c == 'L') {
    if (!piscandoErro) digitalWrite(PINO_LED, HIGH);
    ledEstavaLigado = true;
    Serial.println("LED_LIGADO");
  }
  else if (c == 'D') {
    if (!piscandoErro) digitalWrite(PINO_LED, LOW);
    ledEstavaLigado = false;
    Serial.println("LED_DESLIGADO");
  }
  else if (c == '\n' || c == '\r') {
    // Ignora Enter
  }
  else {
    Serial.print("ERRO: Comando desconhecido (");
    Serial.print(c);
    Serial.println(")");
    
    piscandoErro = true;
    contagemPisca = 0;
    tempoPiscaErro = agora;
    estadoPiscaErro = true;
    digitalWrite(PINO_LED, HIGH);
  }
}

void atualizarPiscaErro(uint32_t agora) {
  if (!piscandoErro) return;
  
  if (agora - tempoPiscaErro >= 100) {
    tempoPiscaErro = agora;
    estadoPiscaErro = !estadoPiscaErro;
    
    if (estadoPiscaErro) {
      digitalWrite(PINO_LED, HIGH);
    } else {
      digitalWrite(PINO_LED, LOW);
      contagemPisca++;
    }
    
    if (contagemPisca >= 3) {
      piscandoErro = false;
      digitalWrite(PINO_LED, ledEstavaLigado ? HIGH : LOW); 
    }
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

  if (forca > 16.0 && !estaTocando) {
    if (expressao != RAIVA) {
      definirExpressao(RAIVA, agora);
    }
    static uint32_t ultimoPrintSacudida = 0;
    if (agora - ultimoPrintSacudida > 1000) {
      Serial.println("EVENTO:SACUDIDO");
      ultimoPrintSacudida = agora;
    }
  }
  else if (z < -7.5 && !estaTocando) {
    if (expressao != TRISTE) {
      definirExpressao(TRISTE, agora);
    }
    static uint32_t ultimoPrintTriste = 0;
    if (agora - ultimoPrintTriste > 1000) {
      Serial.println("EVENTO_CABECA_BAIXO");
      ultimoPrintTriste = agora;
    }
  }
}

// ===== LEITURA DO TOUCH (COM DELAY DE 0.5S NO FINAL) =====
void lerTouch(uint32_t agora) {
  int leituraAtual = digitalRead(PINO_TOUCH);

  // Se o dedo está encostado
  if (leituraAtual != estadoRepousoTouch) {
    if (!estaTocando) {
      Serial.println("EVENTO:CARINHO");
      estaTocando = true;
    }
    
    expressao = AMOR;
    // Enquanto estiver segurando, empurra o tempo para frente
    fimExpressao = agora + 500; 
  } 
  // Se soltou o dedo
  else {
    if (estaTocando) {
      estaTocando = false;
      // Define o tempo para meio segundo a partir do momento que soltou
      fimExpressao = agora + 500; 
    }
  }
}

void definirExpressao(uint8_t nova, uint32_t agora) {
  expressao = nova;
  fimExpressao = agora + 3000; // Comandos via Serial duram 3s
}

// ===== LÓGICA DE ANIMAÇÃO =====
void atualizarOlhar(uint32_t agora) {
  if (expressao == AMOR) {
    alvoX = (millis() % 200 < 100) ? 2 : -2; 
  } else if (expressao != FELIZ) {   
    alvoX = 0;
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
  if (expressao == AMOR) {
    raioYAtual = RAIO_Y_ABERTO;
    return;
  }

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
  if (expressao == FELIZ || expressao == AMOR || raioY <= RAIO_Y_FECHADO + 2) return; 
  
  bool descePelaDireita = (expressao == RAIVA) == esquerdo;
  int topo = OLHOS_Y - raioY - 1;
  int queda = (raioY * 7) / 8 + 1;
  int xPonta = descePelaDireita ? cx + RAIO_X + 1 : cx - RAIO_X - 1;
  
  display.fillTriangle(cx - RAIO_X - 1, topo, cx + RAIO_X + 1, topo, xPonta, topo + queda, SSD1306_BLACK);
}

void desenharOlhoCoracao(int cx, int cy) {
  display.fillCircle(cx - 5, cy - 5, 7, SSD1306_WHITE);
  display.fillCircle(cx + 5, cy - 5, 7, SSD1306_WHITE);
  display.fillTriangle(cx - 11, cy - 2, cx + 11, cy - 2, cx, cy + 14, SSD1306_WHITE);
  display.fillCircle(cx - 7, cy - 7, 2, SSD1306_BLACK);
}

int curvaBoca(int x) {
  if (expressao == RAIVA)  return (x * x) / 24;
  if (expressao == TRISTE) return (x * x) / 12;
  if (expressao == AMOR)   return 6 - (x * x) / 10; 
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
  
  if (expressao == AMOR) {
    desenharOlhoCoracao(CENTRO_ESQ_X + offsetX, OLHOS_Y);
    desenharOlhoCoracao(CENTRO_DIR_X + offsetX, OLHOS_Y);
  } else {
    fillElipse(CENTRO_ESQ_X + offsetX, OLHOS_Y, RAIO_X, raioY);
    fillElipse(CENTRO_DIR_X + offsetX, OLHOS_Y, RAIO_X, raioY);
    desenharPalpebra(CENTRO_ESQ_X + offsetX, true, raioY);
    desenharPalpebra(CENTRO_DIR_X + offsetX, false, raioY);
  }
  
  desenharBoca(offsetX);
  display.display();
}
