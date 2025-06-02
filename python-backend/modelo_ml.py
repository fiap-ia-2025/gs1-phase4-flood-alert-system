# --- modelo_ml.py ---

import sqlite3
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.tree import DecisionTreeClassifier
from sklearn.metrics import accuracy_score

modelo_global = None
features_globais = []


def carregar_dados_treinamento(db_path='projeto.db'):
    conn = sqlite3.connect(db_path)
    try:
        df = pd.read_sql_query("SELECT * FROM sensores", conn)
    finally:
        conn.close()
    
    if df.empty:
        return pd.DataFrame() # Retorna um DataFrame vazio se não houver dados

    df['risco'] = df.apply(lambda row: definir_risco_real(row), axis=1)
    return df


def definir_risco_real(row):
    """
    Define o risco 'real' com base nos dados, CALIBRADO com os novos
    parâmetros oficiais da APAC e INMET para Recife.
    """
    nivel_agua = row['nivel_agua_cm']
    umidade_solo = row['umidade_solo_percent']
    chuva_instantanea = row['chuva_mm_h']

    # --- Critérios de PERIGO (as regras mais importantes) ---
    # Cota de Inundação oficial do Rio Capibaribe (estação a montante)
    if nivel_agua >= 400:
        return 'Perigo'
    # Chuva Violenta/Extrema, padrão INMET (>50mm/h)
    elif chuva_instantanea > 50:
        return 'Perigo'
        
    # --- Critérios de ALERTA ---
    # Cota de Alerta oficial do Rio Capibaribe
    elif nivel_agua >= 300:
        return 'Alerta'
    # Chuva Forte (25 a 50mm/h) com solo já bastante úmido (>85%)
    elif chuva_instantanea > 25 and umidade_solo > 85:
        return 'Alerta'
    # Chuva Moderada (acima de 5mm/h) com solo já saturado (>95%)
    elif chuva_instantanea > 5 and umidade_solo > 95:
        return 'Alerta'
        
    # --- Condições NORMAIS ---
    else:
        return 'Normal'
    

def treinar_modelo_interno(df):
    features = ['nivel_agua_cm', 'chuva_mm_h', 'umidade_solo_percent', 'temperatura_ar_c', 'umidade_ar_percent']
    X = df[features]
    y = df['risco']
    
    # Separa os dados: 80% para treino, 20% para teste
    # stratify=y garante que a proporção de 'Normal', 'Alerta', 'Perigo' seja a mesma nos dois conjuntos
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42, stratify=y)
    
    print(f"Dados divididos em {len(X_train)} para treino e {len(X_test)} para teste.")

    modelo = DecisionTreeClassifier(random_state=42)
    modelo.fit(X_train, y_train) # Treina APENAS com os dados de treino
    
    # Avalia o modelo com os dados de teste que ele nunca viu
    y_pred = modelo.predict(X_test)
    acuracia = accuracy_score(y_test, y_pred)
    
    print(f"\nModelo de Árvore de Decisão treinado com sucesso.")
    print(f"Acurácia do modelo nos dados de teste: {acuracia:.2%}") # Imprime a acurácia formatada
    
    return modelo, features


# --- Função de Previsão Final (com a lógica de 'lazy loading') ---
def prever_risco(dados_sensor: dict) -> str:
    """
    Recebe os dados do sensor e usa o modelo global para prever o risco.
    Se o modelo ainda não foi treinado, ele o treina na primeira chamada.
    """
    global modelo_global, features_globais

    # "Lazy Loading": treina o modelo apenas se ele ainda não foi treinado.
    if modelo_global is None:
        print("Modelo ainda não treinado. Iniciando treinamento agora...")
        dataframe_treinamento = carregar_dados_treinamento()
        
        if dataframe_treinamento.empty:
            print("AVISO: O banco de dados está vazio. Não é possível treinar o modelo.")
            return "Indeterminado" # Retorna um status neutro se não houver dados
            
        modelo_global, features_globais = treinar_modelo_interno(dataframe_treinamento)
        print("Modelo pronto para fazer previsões.")

    # Com o modelo garantidamente treinado, faz a previsão.
    df_input = pd.DataFrame([dados_sensor], columns=features_globais)
    previsao = modelo_global.predict(df_input)
    
    return previsao[0]