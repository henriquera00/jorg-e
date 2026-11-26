import processing.serial.*;
import java.text.Normalizer;
import java.io.File;
import java.io.ByteArrayOutputStream;
import java.nio.file.Path;
import javax.sound.sampled.*;
import java.awt.Toolkit; 

import io.github.givimad.whisperjni.WhisperJNI;
import io.github.givimad.whisperjni.WhisperFullParams;
import io.github.givimad.whisperjni.WhisperContext;

// ===== CONFIG =====
final int BAUD_RATE = 9600;
final int W = 450;
final int H = 550;

Serial porta;
String status = "Conecte a ESP32";
String ultimaFala = "";
boolean ouvindo = false;
boolean gravandoAudio = false; 
float nivelAudio = 0.0;

// Controle de Texto
boolean digitando = false;
String textoInput = "";

// Controle de Microfone
Mixer.Info[] mixers;
int mixerIndex = -1;
AudioFormat formatoAudio;
TargetDataLine microfone; 

// Whisper
WhisperJNI whisper;
WhisperContext ctx;
ByteArrayOutputStream bufferAudio; 

// Cores
color creme, laranja, quente, brasa, ambar, claro, escuro, cinzaBotao;
PFont fontePixel;

// Layout
final float MIC_X = 60;
final float MIC_Y = 320;
final float MIC_W = W - 120;
final float MIC_H = 12;

final float BTN_MAIN_X = W / 2;
final float BTN_MAIN_Y = 390;
final float BTN_MAIN_W = 220;
final float BTN_MAIN_H = 36;

final float BTN_SUB_Y = 460;
final float BTN_SUB_W = 110;
final float BTN_SUB_H = 30;

void setup() {
  size(450, 550);
  surface.setTitle("JORG-E");

  creme = color(234, 215, 176);
  laranja = color(232, 100, 27);
  quente = color(255, 122, 26);
  brasa = color(255, 74, 20);
  ambar = color(255, 161, 51);
  claro = color(255, 208, 138);
  escuro = color(23, 13, 6);
  cinzaBotao = color(90, 60, 40);

  fontePixel = createFont("Monospaced", 16, false);

  conectarSerial();
  iniciarWhisper();
  buscarMicrofones();
}

void conectarSerial() {
  String[] portas = Serial.list();
  if (portas.length == 0) {
    porta = null;
    status = "Nenhuma porta encontrada";
    return;
  }
  try {
    porta = new Serial(this, portas[0], BAUD_RATE);
    status = "ESP32 conectada";
  } catch (Exception e) {
    porta = null;
    status = "Erro: feche o Monitor do Arduino";
  }
}

void buscarMicrofones() {
  mixers = AudioSystem.getMixerInfo();
  for (int i = 0; i < mixers.length; i++) {
    Mixer mixer = AudioSystem.getMixer(mixers[i]);
    if (mixer.isLineSupported(new DataLine.Info(TargetDataLine.class, new AudioFormat(16000, 16, 1, true, false)))) {
      if (abrirMicrofone(i)) {
        mixerIndex = i;
        break;
      }
    }
  }
}

boolean abrirMicrofone(int index) {
  try {
    if (microfone != null && microfone.isOpen()) {
      microfone.close();
    }
    formatoAudio = new AudioFormat(16000, 16, 1, true, false);
    DataLine.Info info = new DataLine.Info(TargetDataLine.class, formatoAudio);
    microfone = (TargetDataLine) AudioSystem.getMixer(mixers[index]).getLine(info);
    microfone.open(formatoAudio);
    microfone.start();
    println("🎤 Microfone alterado para: " + mixers[index].getName());
    return true;
  } catch (Exception e) {
    println("Erro ao abrir microfone: " + e.getMessage());
    return false;
  }
}

void trocarMicrofone() {
  if (mixers == null || mixers.length == 0) return;
  
  int tentativas = 0;
  int proximoIndex = (mixerIndex + 1) % mixers.length;
  
  while (tentativas < mixers.length) {
    Mixer mixer = AudioSystem.getMixer(mixers[proximoIndex]);
    if (mixer.isLineSupported(new DataLine.Info(TargetDataLine.class, new AudioFormat(16000, 16, 1, true, false)))) {
       if (abrirMicrofone(proximoIndex)) {
         mixerIndex = proximoIndex;
         return;
       }
    }
    proximoIndex = (proximoIndex + 1) % mixers.length;
    tentativas++;
  }
  status = "Nenhum outro mic encontrado";
}

void iniciarWhisper() {
  try {
    println("Carregando modelo Whisper. Aguarde...");
    WhisperJNI.loadLibrary();
    WhisperJNI.setLibraryLogger(null); 
    whisper = new WhisperJNI();

    Path caminhoModelo = Path.of(sketchPath("data/ggml-small-q5_1.bin"));
    
    if (!new File(caminhoModelo.toString()).exists()) {
      println("❌ Modelo não encontrado em: " + caminhoModelo);
      status = "Erro: modelo não encontrado";
      return;
    }
    ctx = whisper.init(caminhoModelo);
    println("✅ Whisper small inicializado.");

  } catch (Exception e) {
    println("❌ Erro Whisper: " + e.getMessage());
    status = "Erro no Whisper";
  }
}

void draw() {
  background(20, 12, 7);

  if (ouvindo) {
    nivelAudio = (sin(frameCount * 0.25) + 1) * 0.5;
  } else {
    nivelAudio = 0;
  }

  // Moldura
  noStroke();
  fill(creme);
  rect(0, 0, width, height, 40);
  noFill();
  stroke(laranja);
  strokeWeight(3);
  rect(8, 8, width - 16, height - 16, 33);
  noStroke();
  fill(escuro);
  rect(20, 20, width - 40, height - 40, 22);

  // Textos
  textFont(fontePixel);
  textAlign(CENTER, CENTER);

  // Status da ESP32
  fill(porta != null ? ambar : brasa);
  textSize(12);
  text(porta != null ? "● ESP32" : "○ OFFLINE", W / 2, 50);

  // Exibição de texto ou transcrição
  fill(claro);
  textSize(18);
  if (digitando) {
    text("> " + textoInput + (frameCount / 20 % 2 == 0 ? "|" : ""), W / 2, 180);
  } else {
    text("\"" + encurtar(ultimaFala) + "\"", W / 2, 180);
  }

  // Status do sistema
  fill(ambar);
  textSize(14);
  text(status, W / 2, 240);

  // Barra do microfone
  noStroke();
  fill(color(122, 74, 31));
  rect(MIC_X, MIC_Y, MIC_W, MIC_H, 6);
  fill(ouvindo ? ambar : color(255, 160, 50, 100));
  rect(MIC_X, MIC_Y, MIC_W * nivelAudio, MIC_H, 6);
  fill(ouvindo ? ambar : color(122, 74, 31));
  ellipse(MIC_X - 20, MIC_Y + MIC_H / 2, 14, 14);

  // ===== BOTÕES =====
  
  // Botão Principal
  rectMode(CENTER);
  if (digitando) fill(ambar);
  else fill(ouvindo ? brasa : (sobreBotao(BTN_MAIN_X, BTN_MAIN_Y, BTN_MAIN_W, BTN_MAIN_H) ? color(255, 154, 64) : quente));
  rect(BTN_MAIN_X, BTN_MAIN_Y, BTN_MAIN_W, BTN_MAIN_H, 18);
  fill(escuro);
  textSize(14);
  text(digitando ? "ENVIAR" : (ouvindo ? "PARAR" : "FALAR"), BTN_MAIN_X, BTN_MAIN_Y);

  // Botão Texto (Esquerda)
  float tx = W/2 - BTN_SUB_W - 10;
  fill(digitando ? brasa : cinzaBotao);
  rect(tx, BTN_SUB_Y, BTN_SUB_W, BTN_SUB_H, 10);
  fill(claro);
  textSize(11);
  text("TEXTO", tx, BTN_SUB_Y);

  // Botão Microfone (Centro)
  float mx = W/2;
  fill(cinzaBotao);
  rect(mx, BTN_SUB_Y, BTN_SUB_W, BTN_SUB_H, 10);
  fill(claro);
  text("TROCAR MIC", mx, BTN_SUB_Y);

  // Botão Conectar ESP (Direita)
  float ex = W/2 + BTN_SUB_W + 10;
  fill(porta != null ? cinzaBotao : brasa);
  rect(ex, BTN_SUB_Y, BTN_SUB_W, BTN_SUB_H, 10);
  fill(claro);
  text("CONECTAR ESP", ex, BTN_SUB_Y);

  // Cursor
  cursor((sobreBotao(BTN_MAIN_X, BTN_MAIN_Y, BTN_MAIN_W, BTN_MAIN_H) || 
          sobreBotao(tx, BTN_SUB_Y, BTN_SUB_W, BTN_SUB_H) || 
          sobreBotao(mx, BTN_SUB_Y, BTN_SUB_W, BTN_SUB_H) || 
          sobreBotao(ex, BTN_SUB_Y, BTN_SUB_W, BTN_SUB_H)) ? HAND : ARROW);
          
  rectMode(CORNER);
}

void mousePressed() {
  float tx = W/2 - BTN_SUB_W - 10;
  float mx = W/2;
  float ex = W/2 + BTN_SUB_W + 10;

  if (sobreBotao(BTN_MAIN_X, BTN_MAIN_Y, BTN_MAIN_W, BTN_MAIN_H)) {
    if (digitando) {
      if (textoInput.length() > 0) {
        ultimaFala = textoInput;
        processarTexto(textoInput);
        textoInput = "";
      }
      digitando = false;
    } else if (!ouvindo) {
      iniciarGravacao();
    } else {
      pararGravacao();
    }
  }
  else if (sobreBotao(tx, BTN_SUB_Y, BTN_SUB_W, BTN_SUB_H)) {
    digitando = !digitando;
    ouvindo = false;
    gravandoAudio = false; 
    if (digitando) {
      textoInput = "";
      status = "Digite e aperte Enter";
    } else {
      status = "Cancelado";
    }
  }
  else if (sobreBotao(mx, BTN_SUB_Y, BTN_SUB_W, BTN_SUB_H)) {
    trocarMicrofone();
  }
  else if (sobreBotao(ex, BTN_SUB_Y, BTN_SUB_W, BTN_SUB_H)) {
    conectarSerial();
  }
}

void keyPressed() {
  if (digitando) {
    if (key == ENTER || key == RETURN) {
      if (textoInput.length() > 0) {
        ultimaFala = textoInput;
        processarTexto(textoInput);
        textoInput = "";
      }
      digitando = false;
    } else if (key == BACKSPACE) {
      if (textoInput.length() > 0) {
        textoInput = textoInput.substring(0, textoInput.length() - 1);
      }
    } else if (key == ESC) {
      digitando = false;
      textoInput = "";
      status = "Cancelado";
      key = 0;
    } else if (key >= 32 && key <= 126 && textoInput.length() < 30) {
      textoInput += key;
    }
  }
}

// ===== CONTROLE DE GRAVAÇÃO =====

void iniciarGravacao() {
  if (ctx == null || microfone == null) {
    status = "Whisper/Mic não prontos";
    return;
  }
  ouvindo = true;
  gravandoAudio = true;
  status = "Gravando...";
  bufferAudio = new ByteArrayOutputStream();
  thread("loopGravacao");
}

void pararGravacao() {
  ouvindo = false;
  gravandoAudio = false; 
  status = "Transcrevendo...";
  thread("transcreverAudio");
}

void loopGravacao() {
  byte[] pedaco = new byte[4096];
  while (gravandoAudio) {
    int bytesLidos = microfone.read(pedaco, 0, pedaco.length);
    if (bytesLidos > 0) {
      bufferAudio.write(pedaco, 0, bytesLidos);
    }
  }
}

byte[] cortarSilencio(byte[] audioBytes) {
  int limiar = 250; 
  int inicio = 0;
  int fim = audioBytes.length;

  for (int i = 0; i < audioBytes.length - 1; i += 2) {
    short amostra = (short) ((audioBytes[i + 1] << 8) | (audioBytes[i] & 0xFF));
    if (abs(amostra) > limiar) { inicio = i; break; }
  }

  for (int i = audioBytes.length - 2; i >= 0; i -= 2) {
    short amostra = (short) ((audioBytes[i + 1] << 8) | (audioBytes[i] & 0xFF));
    if (abs(amostra) > limiar) { fim = i + 2; break; }
  }

  if (fim <= inicio) {
    println("⚠️ Filtro falhou. Usando áudio bruto.");
    return audioBytes; 
  }

  byte[] resultado = new byte[fim - inicio];
  System.arraycopy(audioBytes, inicio, resultado, 0, resultado.length);
  return resultado;
}

void transcreverAudio() {
  try {
    byte[] audioBruto = bufferAudio.toByteArray();
    byte[] audioBytes = cortarSilencio(audioBruto);
    
    if (audioBytes.length < 8000) { 
      status = "Áudio muito curto";
      return;
    }

    float[] samples = new float[audioBytes.length / 2];
    for (int i = 0; i < samples.length; i++) {
      short amostra = (short) ((audioBytes[i * 2 + 1] << 8) | (audioBytes[i * 2] & 0xFF));
      samples[i] = amostra / 32768.0f;
    }

    WhisperFullParams params = new WhisperFullParams();
    params.language = "pt";

    int resultado = whisper.full(ctx, params, samples, samples.length);
    if (resultado != 0) {
      status = "Erro na transcrição";
      return;
    }

    int numSegmentos = whisper.fullNSegments(ctx);
    StringBuilder textoCompleto = new StringBuilder();
    for (int i = 0; i < numSegmentos; i++) {
      textoCompleto.append(whisper.fullGetSegmentText(ctx, i));
    }

    String textoFinal = textoCompleto.toString().trim();

    if (textoFinal.length() > 0) {
      ultimaFala = textoFinal;
      processarTexto(textoFinal);
    } else {
      status = "Nada foi ouvido";
    }

  } catch (Exception e) {
    println("Erro na transcrição: " + e.getMessage());
    status = "Erro no áudio";
  }
}

// ===== LÓGICA DE COMANDOS =====
void processarTexto(String t) {
  String limpo = normalizar(t);
  status = "Comando: " + limpo;

  boolean reconhecido = false;

  boolean acaoLigar = limpo.contains("ligar") || limpo.contains("liga") || 
                      limpo.contains("acender") || limpo.contains("acende");
                      
  boolean acaoDesligar = limpo.contains("desligar") || limpo.contains("desliga") || 
                         limpo.contains("apagar") || limpo.contains("apaga");

  boolean objetoLed = limpo.contains("led") || limpo.contains("luz") || 
                      limpo.contains("lampada") || limpo.contains("lâmpada");

  if (limpo.contains("raiva") || limpo.contains("bravo")) {
    enviar('R');
    status = "Raiva";
    reconhecido = true;
  } 
  else if (limpo.contains("triste")) {
    enviar('T');
    status = "Triste";
    reconhecido = true;
  } 
  else if (limpo.contains("feliz") || limpo.contains("alegre")) {
    enviar('F');
    status = "Feliz";
    reconhecido = true;
  } 
  else if (acaoLigar && objetoLed) {
    enviar('L');
    status = "Luz/LED Ligado";
    reconhecido = true;
  } 
  else if (acaoDesligar && objetoLed) {
    enviar('D');
    status = "Luz/LED Desligado";
    reconhecido = true;
  } 
  else if (acaoLigar) {
    enviar('L');
    status = "LED Ligado";
    reconhecido = true;
  } 
  else if (acaoDesligar) {
    enviar('D');
    status = "LED Desligado";
    reconhecido = true;
  } 
  else {
    status = "Comando não reconhecido";
  }

  if (reconhecido) {
    Toolkit.getDefaultToolkit().beep();
  }
}

boolean enviar(char c) {
  if (porta == null) return false;
  try {
    porta.write(c);
    return true;
  } catch (Exception e) {
    return false;
  }
}

String normalizar(String t) {
  String s = java.text.Normalizer.normalize(t, java.text.Normalizer.Form.NFD);
  s = s.replaceAll("[\\p{InCombiningDiacriticalMarks}]", "");
  return s.toLowerCase().trim();
}

String encurtar(String t) {
  if (t == null) return "";
  return t.length() > 24 ? t.substring(0, 21) + "…" : t;
}

boolean sobreBotao(float x, float y, float w, float h) {
  return abs(mouseX - x) < w / 2 && abs(mouseY - y) < h / 2;
}
