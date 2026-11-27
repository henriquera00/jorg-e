# JORG-E: Robô Conversacional e Interativo

![Status](https://img.shields.io/badge/Status-Em_Desenvolvimento-orange)
![Arduino](https://img.shields.io/badge/Arduino-ESP32-blue)
![Processing](https://img.shields.io/badge/Processing-Java-purple)
![IA](https://img.shields.io/badge/IA-Whisper-green)

O JORG-E é um robô de mesa interativo com um avatar expressivo em uma tela OLED. Ele une o poder de um microcontrolador (ESP32) a uma interface desktop (Processing) equipada com Inteligência Artificial local para reconhecimento de voz em português.

O projeto foi desenhado com uma estética retrô minimalista e responde a comandos de voz, além de reagir fisicamente a estímulos do ambiente, como toques e movimentos bruscos.

## Funcionalidades Principais

*   **Reconhecimento de Voz Local (Offline):** Utiliza o modelo Whisper (IA da OpenAI) rodando localmente via Java/Processing, garantindo privacidade e funcionamento sem internet.
*   **Avatar OLED Animado:** O rosto do robô pisca e move os olhos aleatoriamente. Ele expressa emoções como Feliz, Triste, Raiva e Amor (Olhos de Coração).
*   **Reações Físicas (Sensores):**
    *   Fica com Raiva: Se for sacudido bruscamente (detectado pelo acelerômetro).
    *   Fica Triste: Se for virado de cabeça para baixo.
    *   Fica Apaixonado: Ao receber um toque (detectado pelo sensor touch externo).
*   **Feedback Sonoro:** Emite um "bip" sonoro no PC ao reconhecer um comando de voz válido.
*   **Interface Desktop:** Interface retrô colorida (Creme e Laranja) com botões de controle, texto para comandos manuais e seleção de microfone.
*   **Conectividade Dupla:** Comunicação via Serial (USB) para o PC e via Bluetooth para controle remoto pelo celular.

## Tecnologias e Bibliotecas Utilizadas

### Hardware (ESP32)
*   **Linguagem:** C++ (Arduino IDE)
*   **Display:** Adafruit SSD1306 & Adafruit GFX (OLED 128x64 I2C)
*   **Sensor de Movimento:** Adafruit ADXL345 (Acelerômetro I2C)
*   **Sensor de Toque:** Módulo Touch TTP223 (Digital)
*   **Comunicação:** Serial e BluetoothSerial

### Software (Interface e IA)
*   **Linguagem:** Java (Processing 4)
*   **IA de Voz:** [Whisper-JNI](https://github.com/GiviMAD/whisper-jni) (Modelo `ggml-small-q5_1` em português)
*   **Comunicação:** processing.serial

## Componentes de Hardware

Para replicar este projeto, você vai precisar de:
*   1x Placa ESP32 DevKit V1
*   1x Display OLED 0.96" (SSD1306 I2C)
*   1x Acelerômetro ADXL345 (I2C)
*   1x Módulo Sensor de Toque (TTP223)
*   1x LED (Qualquer cor) + Resistor (220Ω a 330Ω)
*   Fios Jumper e Protoboard

### Esquema de Ligação

| Componente | Pino do Componente | Pino da ESP32 |
| :--- | :--- | :--- |
| **OLED & ADXL345** | SDA | GPIO 21 |
| | SCL | GPIO 22 |
| | VCC | 3.3V |
| | GND | GND |
| **ADXL345 (Extra)** | CS | 3.3V *(Obrigatório para modo I2C)* |
| | SDO | GND |
| **LED** | Anodo (+) | GPIO 5 |
| | Catodo (-) | GND (via resistor) |
| **Sensor Touch** | VCC | 3.3V |
| | GND | GND |
| | I/O (SIG) | GPIO 4 |

## Como Executar o Projeto

### 1. Gravando a ESP32 (Firmware)
1. Abra o arquivo `.ino` na Arduino IDE.
2. Instale as bibliotecas: `Adafruit SSD1306`, `Adafruit GFX Library`, `Adafruit ADXL345`, `Adafruit Unified Sensor`.
3. Selecione a placa "DOIT ESP32 DEVKIT V1" e a porta correta.
4. Faça o upload do código.

### 2. Executando a Interface (Processing)
1. Instale o [Processing 4](https://processing.org/).
2. Na pasta do sketch, crie uma pasta chamada `data` e coloque o modelo da IA: `ggml-small-q5_1.bin`.
3. Crie uma pasta chamada `code` e coloque a biblioteca Java: `whisper-jni.jar`.
4. Abra o arquivo `.pde` no Processing e clique em Run.
5. Conecte a ESP32 via USB e clique em CONECTAR ESP na interface.

## Comandos de Voz e Interações

| Ação / Emoção | Como Acionar |
| :--- | :--- |
| **Ligar LED** | Falar: "Ligar led", "Acender luz", "Ligar a lâmpada" |
| **Desligar LED** | Falar: "Desligar led", "Apagar luz", "Apagar a lâmpada" |
| **Expressão Feliz** | Falar: "Ficar feliz", "Sorrir", "Ficar alegre" |
| **Expressão Triste** | Falar: "Ficar triste" |
| **Expressão Raiva** | Falar: "Ficar bravo", "Ficar com raiva" |
| **Olhos de Coração** | Tocar no sensor de toque físico |

## Contexto Acadêmico

Este projeto foi desenvolvido como parte dos requisitos da disciplina de Programação de Computadores. O objetivo foi integrar hardware e software, aplicando conceitos de:
*   Internet das Coisas (IoT) e Sistemas Embarcados.
*   Processamento de Linguagem Natural (PLN) e IA local.
*   Design de Interfaces (UI/UX).
*   Desenvolvimento Orientado a Objetos.

---
Desenvolvido por Henrique R. A.
