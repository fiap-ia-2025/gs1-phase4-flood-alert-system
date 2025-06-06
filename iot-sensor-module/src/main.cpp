#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

//--- Configurações dos sensores e botão ---
#define TRIG_PIN 5          // Pino do trigger do sensor ultrassônico
#define ECHO_PIN 18         // Pino do echo do sensor ultrassônico
#define MQ2_PIN 36          // Pino analógico do sensor de chuva (simulado)
#define NTC_PIN 39          // Pino analógico do sensor de umidade do solo (simulado)
#define DHT_PIN 4           // Pino do sensor DHT22
#define DHT_TYPE DHT22      // Tipo do sensor DHT
#define SIM_BTN_PIN 15      // Pino do botão para alternar modo simulação/real

DHT dht(DHT_PIN, DHT_TYPE);

//--- Configurações de WiFi e MQTT ---
const char* ssid = "Wokwi-GUEST";                 // Nome da rede Wi-Fi
const char* password = "";                        // Senha da rede Wi-Fi (emulado)
const char* MQTT_BROKER = "broker.hivemq.com";    // Endereço do broker MQTT
const int   MQTT_PORT   = 1883;                   // Porta padrão MQTT
const char* MQTT_TOPIC  = "fiap/gs/inundacao";    // Tópico para envio dos dados
String clientId = "esp32-client-" + String(random(0xffff), HEX); // ID único do cliente MQTT

WiFiClient espClient;
PubSubClient client(espClient);

//--- Enum para as fases da simulação de enchente ---
enum FaseSimulacao { 
    FASE_1_NORMAL, 
    FASE_2_AUMENTO, 
    FASE_3_ALERTA, 
    FASE_4_CRITICO, 
    FASE_5_RECEDENDO // Nova fase: água recuando
};
FaseSimulacao faseAtual = FASE_1_NORMAL;          // Começa na fase normal
unsigned long tempoUltimaMudancaDeFase = 0;       // Marca o tempo da última troca de fase
const unsigned long DURACAO_FASE_MS = 45000;      // Duração de cada fase (45 segundos)

//--- Struct para agrupar os dados dos sensores ---
struct DadosSensores {
  float water_level_cm;       // Nível da água em cm
  float rain_mm_hour;         // Chuva em mm/h
  float soil_humidity_pct;    // Umidade do solo em %
  float temp_c;               // Temperatura em °C
  float humidity_air_pct;     // Umidade do ar em %
};

//--- Variáveis de estado para simulação progressiva ---
float chuva_simulada = 0.0;
float nivel_agua_simulado = 0.0;
float umidade_solo_simulada = 0.0;

//--- Controle de modo simulação ---
bool modoSimulacao = false;           // true = simulação, false = sensores reais
bool btnState = false;                // Estado atual do botão
bool lastBtnState = false;            // Último estado lido do botão
unsigned long lastDebounceTime = 0;   // Para debounce do botão
const unsigned long debounceDelay = 50; // Delay para debounce

//--- Função auxiliar para gerar float aleatório em um intervalo ---
float random_float(float min, float max) {
  return min + (float)random(0, 1000) / 1000.0 * (max - min);
}

//--- Reinicia os valores simulados para o início do ciclo ---
void resetarSimulacao() {
  Serial.println("\n\n>> REINICIANDO CICLO DE SIMULACAO <<\n");
  chuva_simulada = 5.0;           // Chuva leve no início
  nivel_agua_simulado = 180.0;    // Nível seguro do rio
  umidade_solo_simulada = 65.0;   // Solo em condição normal
}

//--- Conecta ao Wi-Fi ---
void setup_wifi() {
  delay(10);
  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado.");
}

//--- Garante conexão com o broker MQTT ---
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado.");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5s...");
      delay(5000);
    }
  }
}

//--- Checa o botão físico para alternar entre modo real e simulação ---
void checarBotaoSimulacao() {
  int leitura = digitalRead(SIM_BTN_PIN);
  if (leitura != lastBtnState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (leitura != btnState) {
      btnState = leitura;
      if (btnState == LOW) { // Botão pressionado (com pull-up)
        modoSimulacao = !modoSimulacao;
        if (modoSimulacao) {
          Serial.println("🔄 MODO SIMULACAO ATIVADO!");
          resetarSimulacao();
          faseAtual = FASE_1_NORMAL;
          tempoUltimaMudancaDeFase = millis();
        } else {
          Serial.println("🔄 MODO REAL (SENSORES) ATIVADO!");
        }
        delay(300); // Evita múltiplos toggles rápidos
      }
    }
  }
  lastBtnState = leitura;
}

//--- Setup inicial do ESP32 ---
void setup() {
  Serial.begin(115200);                   // Inicializa comunicação serial
  dht.begin();                            // Inicializa sensor DHT22

  pinMode(TRIG_PIN, OUTPUT);              // Configura pino do trigger do ultrassônico
  pinMode(ECHO_PIN, INPUT);               // Configura pino do echo do ultrassônico
  pinMode(SIM_BTN_PIN, INPUT_PULLUP);     // Botão com resistor de pull-up

  setup_wifi();                           // Conecta ao Wi-Fi
  client.setServer(MQTT_BROKER, MQTT_PORT); // Configura broker MQTT
  resetarSimulacao();                     // Inicializa variáveis da simulação
  tempoUltimaMudancaDeFase = millis();    // Marca o tempo inicial
}

//--- Loop principal ---
void loop() {
  // Garante conexão com o broker MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  checarBotaoSimulacao(); // Verifica se o botão foi pressionado para alternar modo

  DadosSensores dados; // Struct para armazenar os dados

  if (modoSimulacao) {
    // --- Troca de fase automática SÓ durante a simulação ---
    if (millis() - tempoUltimaMudancaDeFase > DURACAO_FASE_MS) {
      faseAtual = (FaseSimulacao)((faseAtual + 1) % 5); // Avança para a próxima fase (cíclico)
      tempoUltimaMudancaDeFase = millis();
      // Reinicia simulação ao voltar para a fase normal
      if (faseAtual == FASE_1_NORMAL) { 
        resetarSimulacao();
      }
    }

    // --- Simula os dados conforme a fase atual ---
    switch (faseAtual) {
      case FASE_1_NORMAL: // Período de normalidade ou chuva leve
        dados.rain_mm_hour = random_float(0.0, 10.0);
        dados.water_level_cm = nivel_agua_simulado;
        dados.soil_humidity_pct = umidade_solo_simulada;
        dados.temp_c = random_float(24.0, 29.0);
        dados.humidity_air_pct = random_float(80.0, 88.0);
        break;
      case FASE_2_AUMENTO: // Início e pico da tempestade
        chuva_simulada = random_float(50.0, 75.0);
        nivel_agua_simulado += random_float(8.0, 12.0);
        if (nivel_agua_simulado > 300) nivel_agua_simulado = 300;
        umidade_solo_simulada += random_float(2.0, 4.0);
        if (umidade_solo_simulada > 90) umidade_solo_simulada = 90;
        dados.rain_mm_hour = chuva_simulada;
        dados.water_level_cm = nivel_agua_simulado;
        dados.soil_humidity_pct = umidade_solo_simulada;
        dados.temp_c = random_float(23.0, 26.0);
        dados.humidity_air_pct = random_float(88.0, 95.0);
        break;
      case FASE_3_ALERTA: // Chuva forte contínua em solo saturado
        chuva_simulada = random_float(25.0, 50.0);
        nivel_agua_simulado += random_float(10.0, 15.0);
        if (nivel_agua_simulado > 420) nivel_agua_simulado = 420;
        umidade_solo_simulada += random_float(0.5, 1.5);
        if (umidade_solo_simulada > 95) umidade_solo_simulada = 95;
        dados.rain_mm_hour = chuva_simulada;
        dados.water_level_cm = nivel_agua_simulado;
        dados.soil_humidity_pct = umidade_solo_simulada;
        dados.temp_c = random_float(22.0, 25.0);
        dados.humidity_air_pct = random_float(92.0, 98.0);
        break;
      case FASE_4_CRITICO: // Diminuição da chuva, mas rio ainda alto
        chuva_simulada = random_float(5.0, 25.0);
        nivel_agua_simulado += random_float(1.0, 5.0);
        if (nivel_agua_simulado > 450) nivel_agua_simulado = 450;
        dados.rain_mm_hour = chuva_simulada;
        dados.water_level_cm = nivel_agua_simulado;
        dados.soil_humidity_pct = random_float(95.0, 100.0);
        dados.temp_c = random_float(22.0, 25.0);
        dados.humidity_air_pct = random_float(90.0, 96.0);
        break;
      case FASE_5_RECEDENDO: // Nível do rio e solo diminuindo gradualmente
        chuva_simulada = random_float(0.0, 5.0);
        nivel_agua_simulado -= random_float(4.0, 8.0);
        if (nivel_agua_simulado < 180) nivel_agua_simulado = 180;
        umidade_solo_simulada -= random_float(1.0, 2.0);
        if (umidade_solo_simulada < 65) umidade_solo_simulada = 65;
        dados.rain_mm_hour = chuva_simulada;
        dados.water_level_cm = nivel_agua_simulado;
        dados.soil_humidity_pct = umidade_solo_simulada;
        dados.temp_c = random_float(23.0, 27.0);
        dados.humidity_air_pct = random_float(85.0, 92.0);
        break;
    }
  } else {
    // --- LEITURA REAL DOS SENSORES ---
    // Sensor Ultrassônico – nível da água (cm)
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH);
    float distance_cm = duration * 0.034 / 2;
    dados.water_level_cm = 400 - distance_cm; // Ajuste conforme seu reservatório

    // MQ2 – chuva simulada: de 0 a 100 mm/h
    int mq2Raw = analogRead(MQ2_PIN);
    dados.rain_mm_hour = map(mq2Raw, 0, 4095, 0, 100);

    // NTC – umidade do solo simulada: 0% (seco) a 100% (molhado)
    int ntcRaw = analogRead(NTC_PIN);
    dados.soil_humidity_pct = map(ntcRaw, 4095, 0, 0, 100);

    // DHT22 – temperatura e umidade do ar
    dados.temp_c = dht.readTemperature();
    dados.humidity_air_pct = dht.readHumidity();

    if (isnan(dados.temp_c) || isnan(dados.humidity_air_pct)) {
      Serial.println("⚠️ Falha ao ler o DHT22");
      return;
    }
  }

  // --- Monta o JSON para envio ao broker MQTT ---
  String payload = "{";
  payload += "\"water_level_cm\":" + String(dados.water_level_cm, 2) + ",";
  payload += "\"rain_mm_hour\":" + String(dados.rain_mm_hour, 2) + ",";
  payload += "\"soil_humidity_pct\":" + String(dados.soil_humidity_pct, 2) + ",";
  payload += "\"temp_c\":" + String(dados.temp_c, 2) + ",";
  payload += "\"humidity_air_pct\":" + String(dados.humidity_air_pct, 2);
  payload += "}";

  // --- Publica os dados no tópico MQTT ---
  client.publish(MQTT_TOPIC, payload.c_str());

  // --- Mostra no Serial Monitor para debug ---
  Serial.print(modoSimulacao ? "[SIMULACAO] " : "[REAL] ");
  Serial.println(payload);
  Serial.println("-------------------------------------");
  delay(2000); // Aguarda 2 segundos antes de enviar novamente
}