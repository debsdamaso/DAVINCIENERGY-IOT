#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "EmonLib.h" // Biblioteca Emon para simulação de sensores de energia

#define SCREEN_WIDTH 128 // Largura do OLED, em pixels
#define SCREEN_HEIGHT 64 // Altura do OLED, em pixels

// Cria um objeto de display OLED conectado ao I2C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// Credenciais do Wi-Fi
const char* ssid = "Wokwi-GUEST"; // Nome da rede Wi-Fi
const char* password = "";        // Senha da rede Wi-Fi

// Configuração do ThingSpeak
const char* host = "api.thingspeak.com";
const String apiKey = "PR0FLYIR0LWWX7M3"; // Sua chave API de escrita do ThingSpeak

// Pinos dos sensores
const int voltagePin = 34; // Potenciômetro simula a tensão
const int currentPin = 35; // Potenciômetro simula a corrente

// Variáveis
float voltage, current, power;
float consumoPadrao = 200.0;      // Consumo padrão do INMETRO em watts
float eficienciaInformada = 0.85; // Eficiência energética informada pelo fabricante (85%)
float vidaUtilInicial = 100.0;    // Vida útil inicial em percentual
const float potenciaMaxima = 400.0; // Limite máximo para potência simulada (W)

// Função para gerar valores aleatórios
float gerarValorAleatorio(float base, float variacao) {
  return base + random(-variacao * 10, variacao * 10) / 100.0;
}

// Função para enviar dados ao ThingSpeak
void enviarDadosThingSpeak(float voltage, float current, float power, float eficiencia) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;

    if (client.connect(host, 80)) {
      // Criação da URL para envio
      String url = "/update?api_key=" + apiKey +
                   "&field1=" + String(voltage, 2) +
                   "&field2=" + String(current, 2) +
                   "&field3=" + String(power, 2) +
                   "&field4=" + String(eficiencia, 2);

      // Envio da solicitação HTTP
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n" +
                   "Connection: close\r\n\r\n");
      delay(100); // Tempo para garantir o envio
    }
  } else {
    Serial.println("Erro: Não conectado ao Wi-Fi.");
  }
}

// Função para exibir no OLED
void exibirNoOLED(float voltage, float current, float power, float eficiencia) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Tensao: ");
  display.println(voltage, 2);
  display.print("V");

  display.setCursor(0, 16);
  display.print("Corrente: ");
  display.println(current, 2);
  display.print("A");

  display.setCursor(0, 32);
  display.print("Potencia: ");
  display.println(power, 2);
  display.print("W");

  display.setCursor(0, 48);
  display.print("Eficiencia: ");
  display.println(eficiencia, 2);
  display.print("%");

  display.display(); // Atualiza o conteúdo do OLED
}

// Função para calcular a eficiência real do aparelho
float calcularEficienciaReal(float consumoReal, float consumoPadrao) {
  return consumoPadrao / consumoReal;
}

// Função para classificar a eficiência energética com base no INMETRO
String classificarEficiencia(float eficienciaReal) {
  if (eficienciaReal >= 0.9) {
    return "A (Muito eficiente)";
  } else if (eficienciaReal >= 0.8) {
    return "B (Eficiente)";
  } else if (eficienciaReal >= 0.7) {
    return "C (Moderado)";
  } else if (eficienciaReal >= 0.6) {
    return "D (Baixa eficiência)";
  } else {
    return "E (Ineficiente)";
  }
}

// Função para avaliar a conformidade da eficiência
void avaliarConformidade(float eficienciaReal, float eficienciaInformada, String classificacao) {
  Serial.print("Eficiência informada pelo fabricante: ");
  Serial.print(eficienciaInformada * 100);
  Serial.println(" %");

  Serial.print("Eficiência medida: ");
  Serial.print(eficienciaReal * 100);
  Serial.println(" %");

  Serial.print("Classificação do INMETRO: ");
  Serial.println(classificacao);

  if (eficienciaReal >= eficienciaInformada) {
    Serial.println("O aparelho está em conformidade com o valor informado.");
  } else {
    Serial.println("Alerta: O aparelho não está em conformidade com o valor informado!");
    Serial.println("Possível defeito ou desgaste identificado.");
  }
}

// Função para calcular e exibir a vida útil
void calcularVidaUtil(float eficienciaReal, float eficienciaInformada) {
  if (eficienciaReal < eficienciaInformada) {
    // Reduz a vida útil se a eficiência for inferior à informada
    vidaUtilInicial -= (eficienciaInformada - eficienciaReal) * 100;
    if (vidaUtilInicial < 0) {
      vidaUtilInicial = 0;
    }
  }

  Serial.print("Vida útil atual do aparelho: ");
  Serial.print(vidaUtilInicial);
  Serial.println(" %");

  if (vidaUtilInicial <= 20) {
    Serial.println("Alerta: O aparelho está próximo do fim de sua vida útil.");
  } else if (vidaUtilInicial == 0) {
    Serial.println("O aparelho atingiu o fim de sua vida útil.");
  }
}

void setup() {
  // Inicializa a comunicação serial
  Serial.begin(115200);

  // Inicializa o OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha na inicialização do OLED"));
    for (;;); // Entra em loop infinito caso o OLED não inicie
  }
  display.clearDisplay();
  display.setTextColor(WHITE);

  // Conecta ao Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) { // Aguarda até que esteja conectado
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado ao WiFi");
}

void sendDataToThingSpeak() {
  // Gera valores aleatórios de tensão e corrente
  voltage = gerarValorAleatorio(220.0, 10.0);  // ±10V de variação
  current = gerarValorAleatorio(1.0, 2.0);    // ±2A de variação

  // Calcula a potência usando P = VI
  power = voltage * current;

  // Limita a potência ao valor máximo
  if (power > potenciaMaxima) {
    power = potenciaMaxima - random(0, 50);
  }

  // Calcula a eficiência real
  float eficienciaReal = calcularEficienciaReal(power, consumoPadrao);

  // Classifica a eficiência com base no INMETRO
  String classificacao = classificarEficiencia(eficienciaReal);

  // Avalia conformidade e possíveis defeitos
  avaliarConformidade(eficienciaReal, eficienciaInformada, classificacao);

  // Calcula e exibe a vida útil do aparelho
  calcularVidaUtil(eficienciaReal, eficienciaInformada);

  // Exibe os dados no OLED
  exibirNoOLED(voltage, current, power, eficienciaReal * 100);

  // Envia os dados ao ThingSpeak
  enviarDadosThingSpeak(voltage, current, power, eficienciaReal * 100);
}

void loop() {
  sendDataToThingSpeak();
  delay(15000);
}
