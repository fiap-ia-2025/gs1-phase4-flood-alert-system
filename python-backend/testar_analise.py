# --- testar_analise.py (Versão para testar a lógica no terminal com dados existentes no banco) ---

import sqlite3
import pandas as pd
import datetime 

# Importa a função principal que queremos testar.
# Ao importar logica_negocio, o modelo_ml.py será importado e
# o modelo de ML será treinado (se ainda não tiver sido) usando os dados
# que JÁ ESTÃO no banco.
from logica_negocio import analisar_cenario_atual

# --- CONFIGURAÇÕES ---
NOME_BANCO = 'projeto.db' 

def testar_analise_com_dados_do_banco():
    """
    Lê os dados existentes no banco (populados pelo receptor_mqtt.py)
    e aplica a lógica de análise para cada registro.
    """
    
    # Conecta e lê os dados do banco 
    conn = sqlite3.connect(NOME_BANCO)

    df = pd.read_sql_query("SELECT * FROM sensores ORDER BY timestamp ASC", conn)
    conn.close()
    
    if df.empty:
        print(f"Não foram encontrados dados no '{NOME_BANCO}' para testar a análise.")
        print("Certifique-se de que o 'receptor_mqtt.py' está rodando e recebendo dados do Wokwi.")
        return

    print(f"\n--- INICIANDO TESTE DA ANÁLISE COM OS DADOS ATUAIS DO BANCO ({len(df)} REGISTROS) ---")
    
   
    for registro_db in df.to_dict('records'):

        resultado_alerta = analisar_cenario_atual(registro_db)
        
        # Coleta alguns dados para um print mais informativo
        timestamp_registro = registro_db['timestamp']
        nivel_agua = registro_db['nivel_agua_cm']
        chuva_h = registro_db['chuva_mm_h'] 
        
        print(f"Analisando (TS: {timestamp_registro}) Nível: {nivel_agua:<5.1f}cm | Chuva/h: {chuva_h:<4.1f}mm -> Status: '{resultado_alerta['status']}'")
        # Se quiser ver a mensagem completa também:
        # print(f"  Mensagem: {resultado_alerta['mensagem']}")


# Bloco principal para executar o teste
if __name__ == "__main__":
    testar_analise_com_dados_do_banco()