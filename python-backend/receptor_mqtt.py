# --- receptor_mqtt.py ---
import paho.mqtt.client as mqtt
import sqlite3
import datetime
import os
import json

# --- CONFIGURAÇÕES ---
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPIC = "fiap/gs/inundacao"
NOME_BANCO = 'projeto.db'
NOME_SENSOR = 'sensor_principal_01'

# --- NOVOS CALLBACKS DE DIAGNÓSTICO ---
def on_connect(client, userdata, flags, reason_code, properties): # Assinatura para Paho MQTT v1.6+
    if reason_code == 0:
        print(f"Conectado ao Broker MQTT com sucesso! (Código: {reason_code})")
        client.subscribe(MQTT_TOPIC)
    else:
        print(f"Falha ao conectar ao Broker, código de retorno: {reason_code}")

def on_subscribe(client, userdata, mid, reason_codes, properties): # Assinatura para Paho MQTT v1.6+
    print(f"Inscrito com sucesso no tópico '{MQTT_TOPIC}' (MID: {mid}, QoS concedido: {reason_codes[0]})")

def on_message(client, userdata, message):
    # imprimir o payload bruto
    print(f"--- FUNÇÃO ON_MESSAGE CHAMADA ---") 
    payload_str = message.payload.decode("utf-8")
    print(f"Payload Bruto Recebido: {payload_str}")
    
    try:
        dados_json = json.loads(payload_str)
        
        nivel_agua = float(dados_json['water_level_cm'])
        chuva_mm = float(dados_json['rain_mm_hour'])
        umidade_solo = float(dados_json['soil_humidity_pct'])
        temperatura = float(dados_json['temp_c'])
        umidade_ar = float(dados_json['humidity_air_pct'])
        
        timestamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        
        conn = sqlite3.connect(NOME_BANCO)
        cursor = conn.cursor()
        
        cursor.execute("""
            INSERT INTO sensores (timestamp, sensor_id, nivel_agua_cm, chuva_mm_h, umidade_solo_percent, temperatura_ar_c, umidade_ar_percent)
            VALUES (?, ?, ?, ?, ?, ?, ?);
        """, (timestamp, NOME_SENSOR, nivel_agua, chuva_mm, umidade_solo, temperatura, umidade_ar))
        
        conn.commit()
        conn.close()
        
        print(" -> Dado salvo no banco de dados com sucesso!")

    except Exception as e:
        print(f" -> Erro ao processar ou salvar a mensagem: {e}")

# --- Configuração do Cliente MQTT ---
if not os.path.exists(NOME_BANCO):
    print(f"ERRO: Banco de dados '{NOME_BANCO}' não encontrado. Execute o script 'inicializar_banco.py' primeiro.")
    exit()

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, "receptor_sqlite_debug") # Usando API v2 e ClientID diferente
client.on_connect = on_connect # Atribui o callback de conexão
client.on_subscribe = on_subscribe # Atribui o callback de inscrição
client.on_message = on_message # Atribui o callback de mensagem

print(f"Tentando conectar ao broker MQTT: {MQTT_BROKER}...")
client.connect(MQTT_BROKER, MQTT_PORT, 60)

print("Iniciando loop principal do MQTT...")
client.loop_forever()