#include <WiFi.h>
#include <PubSubClient.h>

//--- Conexão WiFi e MQTT ---
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* MQTT_BROKER = "broker.hivemq.com";
const int   MQTT_PORT   = 1883;
const char* MQTT_TOPIC  = "fiap/gs/inundacao";
String clientId = "esp32-client-" + String(random(0xffff), HEX);

WiFiClient espClient;
PubSubClient client(espClient);

//--- Parâmetros da Simulação ---
enum FaseSimulacao { 
    FASE_1_NORMAL, 
    FASE_2_AUMENTO, 
    FASE_3_ALERTA, 
    FASE_4_CRITICO, 
    FASE_5_RECEDENDO // Nova fase
};
FaseSimulacao faseAtual = FASE_1_NORMAL;
unsigned long tempoUltimaMudancaDeFase = 0;
const unsigned long DURACAO_FASE_MS = 45000; // 45 segundos por fase

struct DadosSensores {
  float water_level_cm;
  float rain_mm_hour;
  float soil_humidity_pct;
  float temp_c;
  float humidity_air_pct;
};

//--- Variáveis de Estado da Simulação ---
float chuva_simulada = 0.0;
float nivel_agua_simulado = 0.0;
float umidade_solo_simulada = 0.0;

//--- Função Auxiliar ---
float random_float(float min, float max) {
  return min + (float)random(0, 1000) / 1000.0 * (max - min);
}

void resetarSimulacao() {
  Serial.println("\n\n>> REINICIANDO CICLO DE SIMULACAO <<\n");
  chuva_simulada = 5.0; // Chuva leve no início do ciclo
  nivel_agua_simulado = 180.0; // Nível inicial seguro
  umidade_solo_simulada = 65.0; // Umidade do solo normal
}

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

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(MQTT_BROKER, MQTT_PORT);
  resetarSimulacao();
  tempoUltimaMudancaDeFase = millis();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (millis() - tempoUltimaMudancaDeFase > DURACAO_FASE_MS) {
    faseAtual = (FaseSimulacao)((faseAtual + 1) % 5); // Ajustado para 5 fases
    tempoUltimaMudancaDeFase = millis();
    // Resetar simulação apenas ao retornar para FASE_1_NORMAL vindo da FASE_5_RECEDENDO
    if (faseAtual == FASE_1_NORMAL) { 
      resetarSimulacao();
    }
  }

  DadosSensores dados;

  switch (faseAtual) {
    case FASE_1_NORMAL: // Período de normalidade ou chuva leve
      dados.rain_mm_hour = random_float(0.0, 10.0);
      dados.water_level_cm = nivel_agua_simulado; // Usa o valor progressivo, que foi resetado
      dados.soil_humidity_pct = umidade_solo_simulada; // Usa o valor progressivo
      dados.temp_c = random_float(24.0, 29.0);
      dados.humidity_air_pct = random_float(80.0, 88.0);
      break;

    case FASE_2_AUMENTO: // Início e pico da tempestade
      chuva_simulada = random_float(50.0, 75.0); // Chuva violenta
      nivel_agua_simulado += random_float(8.0, 12.0);
      if (nivel_agua_simulado > 300) nivel_agua_simulado = 300; // Atinge a cota de alerta
      umidade_solo_simulada += random_float(2.0, 4.0);
      if (umidade_solo_simulada > 90) umidade_solo_simulada = 90;
      
      dados.rain_mm_hour = chuva_simulada;
      dados.water_level_cm = nivel_agua_simulado;
      dados.soil_humidity_pct = umidade_solo_simulada;
      dados.temp_c = random_float(23.0, 26.0);
      dados.humidity_air_pct = random_float(88.0, 95.0);
      break;

    case FASE_3_ALERTA: // Chuva forte contínua em solo saturado
      chuva_simulada = random_float(25.0, 50.0); // Chuva forte
      nivel_agua_simulado += random_float(10.0, 15.0);
      if (nivel_agua_simulado > 420) nivel_agua_simulado = 420; // Ultrapassa a cota de inundação
      umidade_solo_simulada += random_float(0.5, 1.5);
      if (umidade_solo_simulada > 95) umidade_solo_simulada = 95;

      dados.rain_mm_hour = chuva_simulada;
      dados.water_level_cm = nivel_agua_simulado;
      dados.soil_humidity_pct = umidade_solo_simulada;
      dados.temp_c = random_float(22.0, 25.0);
      dados.humidity_air_pct = random_float(92.0, 98.0);
      break;

    case FASE_4_CRITICO: // Diminuição da chuva, mas com o rio ainda no pico
      chuva_simulada = random_float(5.0, 25.0); // Chuva diminui
      nivel_agua_simulado += random_float(1.0, 5.0); 
      if (nivel_agua_simulado > 450) nivel_agua_simulado = 450; // Pico máximo do nível do rio
      
      dados.rain_mm_hour = chuva_simulada;
      dados.water_level_cm = nivel_agua_simulado;
      dados.soil_humidity_pct = random_float(95.0, 100.0); 
      dados.temp_c = random_float(22.0, 25.0);
      dados.humidity_air_pct = random_float(90.0, 96.0);
      break;

    case FASE_5_RECEDENDO: // Nível do rio e umidade do solo diminuem gradualmente
      chuva_simulada = random_float(0.0, 5.0); // Chuva muito fraca ou ausente
      nivel_agua_simulado -= random_float(4.0, 8.0); 
      if (nivel_agua_simulado < 180) nivel_agua_simulado = 180; // Retorna ao nível base
      umidade_solo_simulada -= random_float(1.0, 2.0);
      if (umidade_solo_simulada < 65) umidade_solo_simulada = 65; // Retorna à umidade base

      dados.rain_mm_hour = chuva_simulada;
      dados.water_level_cm = nivel_agua_simulado;
      dados.soil_humidity_pct = umidade_solo_simulada;
      dados.temp_c = random_float(23.0, 27.0); // Temperatura pode subir um pouco
      dados.humidity_air_pct = random_float(85.0, 92.0); // Umidade do ar pode baixar um pouco
      break;
  }
  
  String payload = "{";
  payload += "\"water_level_cm\":" + String(dados.water_level_cm, 2) + ",";
  payload += "\"rain_mm_hour\":" + String(dados.rain_mm_hour, 2) + ",";
  payload += "\"soil_humidity_pct\":" + String(dados.soil_humidity_pct, 2) + ",";
  payload += "\"temp_c\":" + String(dados.temp_c, 2) + ",";
  payload += "\"humidity_air_pct\":" + String(dados.humidity_air_pct, 2);
  payload += "}";

  client.publish(MQTT_TOPIC, payload.c_str());

  Serial.println("Dados enviados via MQTT:");
  Serial.println(payload);
  Serial.println("-------------------------------------");
  delay(2000); 
}