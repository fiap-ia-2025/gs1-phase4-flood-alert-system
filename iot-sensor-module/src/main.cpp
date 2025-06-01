#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// 🟢 DEFINIÇÕES DOS PINOS DOS SENSORES
#define TRIG_PIN 5
#define ECHO_PIN 18
#define MQ2_PIN 36       // VP - simula sensor de chuva
#define NTC_PIN 39       // VN - simula sensor de umidade do solo
#define DHT_PIN 4
#define DHT_TYPE DHT22

// 🟢 INICIALIZAÇÃO DO SENSOR DHT
DHT dht(DHT_PIN, DHT_TYPE);

// 🟡 VARIÁVEIS DE WIFI E MQTT
const char* ssid = "Wokwi-GUEST";            // nome do Wi-Fi simulado
const char* password = "";                  // senha (emulado)
const char* MQTT_BROKER = "broker.hivemq.com";  // Endereço do servidor MQTT
const int   MQTT_PORT   = 1883;                 // Porta padrão para conexões sem TLS (não segura)
const char* MQTT_TOPIC  = "fiap/gs/inundacao";  // Tópico onde vamos publicar os dados
String clientId = "esp32-client-" + String(random(0xffff), HEX);

WiFiClient espClient;
PubSubClient client(espClient);

// 🟠 FUNÇÃO PARA CONECTAR AO WI-FI
void setup_wifi() {
  delay(10);
  Serial.println("🌐 Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("✅ Wi-Fi conectado com sucesso");
  Serial.print("🔗 Endereço IP: ");
  Serial.println(WiFi.localIP());
}

// 🟠 FUNÇÃO PARA RECONEXÃO AO BROKER MQTT CASO DESCONECTE
void reconnect() {
  while (!client.connected()) {
    Serial.print("🔄 Tentando conectar ao MQTT...");
    if (client.connect(clientId.c_str())) {
      Serial.println("✅ Conectado ao MQTT");
    } else {
      Serial.print("❌ Falha, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5s...");
      delay(5000);
      Serial.print("Estado do client: ");
      Serial.println(client.state());
    }
  }
}

// 🟢 FUNÇÃO PRINCIPAL DE CONFIGURAÇÃO

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  setup_wifi();              // conecta ao Wi-Fi
  client.setServer(MQTT_BROKER, MQTT_PORT); // configura o servidor MQTT
}

// 🟢 LÓGICA DO PROGRAMA
void loop() {

  // GARANTIA DE CONEXÃO
    if (!client.connected()) {
    reconnect(); // garante reconexão com o broker
  }
  client.loop(); // mantém conexão ativa

  Serial.println("📡 Lendo sensores...");

  // 🟦 Sensor Ultrassônico – nível da água (cm)
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance_cm = duration * 0.034 / 2;
  float water_level_cm = 400 - distance_cm; // simulando nível da água "subindo"

  // 🌧️ MQ2 – chuva simulada: de 0 a 100 mm/h
  int mq2Raw = analogRead(MQ2_PIN);
  float rain_mm_hour = map(mq2Raw, 0, 4095, 0, 100); // normalizado como pluviômetro real

  // 🌱 NTC – umidade do solo simulada: 0% (seco) a 100% (molhado)
  int ntcRaw = analogRead(NTC_PIN);
  float soil_humidity_pct = map(ntcRaw, 4095, 0, 0, 100); // inverso: maior valor = mais seco

  // 🌡️ DHT22 – temperatura e umidade do ar
  float temp_c = dht.readTemperature();
  float humidity_air_pct = dht.readHumidity();

  if (isnan(temp_c) || isnan(humidity_air_pct)) {
    Serial.println("⚠️ Falha ao ler o DHT22");
    return;
  }

  // 🔷 Monta o JSON para envio ao broker (e impressão)
  String payload = "{";
  payload += "\"water_level_cm\":" + String(water_level_cm, 2) + ",";
  payload += "\"rain_mm_hour\":" + String(rain_mm_hour, 2) + ",";
  payload += "\"soil_humidity_pct\":" + String(soil_humidity_pct, 2) + ",";
  payload += "\"temp_c\":" + String(temp_c, 2) + ",";
  payload += "\"humidity_air_pct\":" + String(humidity_air_pct, 2);
  payload += "}";

  // ✅ ENVIO MQTT
  client.publish(MQTT_TOPIC, payload.c_str());

  // 💻 Impressão no Serial para debug 
  Serial.println("🔁 Mostrando o envio para MQTT:");
  Serial.println(payload);
  Serial.println("---------------------------------------------------");
  delay(2000); // Aguarda 2s antes de repetir
}
