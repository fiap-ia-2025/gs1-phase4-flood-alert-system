# --- logica_negocio.py ---

import sqlite3
import pandas as pd 
import datetime
from modelo_ml import prever_risco 

NOME_BANCO = 'projeto.db'

def calcular_chuva_acumulada_24h(timestamp_atual_str: str) -> float:
    """
    Calcula a soma da chuva (chuva_mm_h) nas últimas 24 horas
    a partir do timestamp_atual fornecido.
    """
    try:
        conn = sqlite3.connect(NOME_BANCO)
        cursor = conn.cursor()

        # Converte o timestamp_atual_str (string) para um objeto datetime
        # O formato no banco é 'YYYY-MM-DD HH:MM:SS'
        timestamp_atual_dt = datetime.datetime.strptime(timestamp_atual_str, '%Y-%m-%d %H:%M:%S')
        
        # Calcula o timestamp de 24 horas atrás
        timestamp_24h_atras_dt = timestamp_atual_dt - datetime.timedelta(hours=24)
        timestamp_24h_atras_str = timestamp_24h_atras_dt.strftime('%Y-%m-%d %H:%M:%S')

        # Consulta SQL para somar a chuva no intervalo
        query = """
        SELECT SUM(chuva_mm_h) 
        FROM sensores 
        WHERE timestamp BETWEEN ? AND ?;
        """
        cursor.execute(query, (timestamp_24h_atras_str, timestamp_atual_str))
        resultado = cursor.fetchone()
        
        conn.close()

        if resultado and resultado[0] is not None:
            return float(resultado[0])
        return 0.0 # Retorna 0 se não houver dados ou a soma for nula

    except Exception as e:
        print(f"Erro ao calcular chuva acumulada: {e}")
        return 0.0


def analisar_cenario_atual(dados_sensor: dict) -> dict:
    """
    Analisa os dados atuais, consulta o modelo de ML, 
    verifica a chuva acumulada e aplica regras de negócio FINAIS para Recife.
    """
    risco_ml = prever_risco(dados_sensor)
    nivel_agua = dados_sensor.get('nivel_agua_cm', 0)
    timestamp_atual = dados_sensor.get('timestamp', None)
    
    status_final = risco_ml

    # --- REGRAS DE SOBRESCRITA (OVERRIDES) COM BASE NA PESQUISA PARA RECIFE ---
    
    # 1. Regra da Chuva Acumulada em 24h
    chuva_acumulada_24h = 0.0
    if timestamp_atual:
        chuva_acumulada_24h = calcular_chuva_acumulada_24h(timestamp_atual)
        if chuva_acumulada_24h >= 100: # Limiar crítico de chuva acumulada
            status_final = 'Perigo'
            mensagem = f"PERIGO: Chuva acumulada de {chuva_acumulada_24h:.1f}mm nas últimas 24h em Recife! Risco altíssimo de transbordamento e alagamentos."
            return {'status': status_final, 'cor': 'red', 'mensagem': mensagem}
    
   # 2. Regra da Cota de Inundação (Nível do Rio Capibaribe)
    if nivel_agua >= 400: # 
        status_final = 'Perigo'
        # Mensagem também ajustada para refletir o novo valor
        mensagem = f"PERIGO: Nível do Rio Capibaribe em {nivel_agua:.1f}cm, atingiu ou ultrapassou a cota de inundação (400cm). Evacue áreas de risco!"
        return {'status': status_final, 'cor': 'red', 'mensagem': mensagem}
        
    # 3. Regra da Cota de Alerta (Nível do Rio Capibaribe)
    if nivel_agua >= 300: 
        status_final = 'Alerta'

    # --- MENSAGENS FINAIS PERSONALIZADAS PARA CADA STATUS ---
    if status_final == 'Perigo':
        return {'status': 'Perigo', 'cor': 'red', 'mensagem': f"PERIGO: Condições críticas detectadas. Nível do rio em {nivel_agua:.1f}cm (Cota de Inundação: 400cm)."}
    elif status_final == 'Alerta':
        # Mensagem também ajustada para refletir o novo valor
        return {'status': 'Alerta', 'cor': 'orange', 'mensagem': f"ALERTA: Nível do Rio Capibaribe em {nivel_agua:.1f}cm (Cota de Alerta: 300cm). Monitore as condições."}
    else: # Normal
        return {'status': 'Normal', 'cor': 'green', 'mensagem': f"NORMAL: Condições estáveis (Nível rio: {nivel_agua:.1f}cm)."}



