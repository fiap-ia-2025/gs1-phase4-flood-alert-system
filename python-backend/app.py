# --- app.py (Aplicação Principal Streamlit - Versão Recife) ---

import streamlit as st
import pandas as pd
import sqlite3
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from logica_negocio import analisar_cenario_atual # Importa a função do Integrante 4

# --- CONFIGURAÇÕES DA PÁGINA E DO BANCO ---
st.set_page_config(
    page_title="AquaSentinela - Alerta Enchentes Recife", # AJUSTADO
    page_icon="🌊",
    layout="wide"
)

NOME_BANCO = 'projeto.db'

# --- FUNÇÕES DE ACESSO AO BANCO DE DADOS (permanecem as mesmas) ---

def buscar_dados_historicos():
    """Busca todos os registros da tabela de sensores para os gráficos."""
    try:
        conn = sqlite3.connect(NOME_BANCO, check_same_thread=False)
        df = pd.read_sql_query("SELECT * FROM sensores ORDER BY timestamp ASC", conn, parse_dates=['timestamp'])
        conn.close()
        return df
    except Exception as e:
        st.error(f"Erro ao buscar dados históricos: {e}")
        return pd.DataFrame()

def buscar_ultimo_dado():
    """Busca o registro mais recente da tabela de sensores para o status atual."""
    try:
        conn = sqlite3.connect(NOME_BANCO, check_same_thread=False)
        df = pd.read_sql_query("SELECT * FROM sensores ORDER BY timestamp DESC LIMIT 1", conn)
        conn.close()
        if not df.empty:
            return df.to_dict('records')[0]
        return None
    except Exception as e:
        st.error(f"Erro ao buscar último dado: {e}")
        return None

# --- CONSTRUÇÃO DA INTERFACE ---

st.title("🌊 AquaSentinela - Monitoramento de Enchentes em Recife") # AJUSTADO
st.markdown("""
Bem-vindo ao sistema de alerta precoce para inundações, com foco na cidade do **Recife** e nos desafios impostos 
pelo **Rio Capibaribe** e pelo regime de chuvas da região.
""") # AJUSTADO

dados_historicos = buscar_dados_historicos()
ultimo_dado = buscar_ultimo_dado()

if ultimo_dado:
    resultado_alerta = analisar_cenario_atual(ultimo_dado) # A função já foi calibrada
    status = resultado_alerta['status']
    cor = resultado_alerta['cor']
    mensagem = resultado_alerta['mensagem'] # As mensagens já foram personalizadas para Recife

    st.header(f"Status Atual para Recife: {status}", divider=cor)
    st.markdown(f"**<span style='color:{cor};'>{mensagem}</span>**", unsafe_allow_html=True)

    st.subheader("Leituras Simuladas Atuais (Recife)") # AJUSTADO
    col1, col2, col3, col4 = st.columns(4)
    col1.metric("Nível Rio Capibaribe (Simulado)", f"{ultimo_dado['nivel_agua_cm']} cm") # AJUSTADO
    col2.metric("Chuva Atual (Simulada)", f"{ultimo_dado['chuva_mm_h']} mm/h") # AJUSTADO
    col3.metric("Umidade do Solo (Simulada)", f"{ultimo_dado['umidade_solo_percent']} %")
    col4.metric("Temperatura do Ar", f"{ultimo_dado['temperatura_ar_c']} °C")
else:
    st.warning("Aguardando dados da simulação para Recife. Inicie o `receptor_mqtt.py` e a simulação no Wokwi.") # AJUSTADO

if not dados_historicos.empty:
    st.header("Análise Histórica da Simulação (Recife)", divider='gray') # AJUSTADO
    
    col_graf1, col_graf2 = st.columns(2)

    with col_graf1:
        st.subheader("Nível do Rio (Simulado) vs. Chuva (Simulada)") # AJUSTADO
        fig1, ax1 = plt.subplots(figsize=(10, 4))
        ax1.plot(dados_historicos['timestamp'], dados_historicos['nivel_agua_cm'], label='Nível Rio (cm)', color='blue', marker='o') # AJUSTADO
        ax2 = ax1.twinx()
        ax2.bar(dados_historicos['timestamp'], dados_historicos['chuva_mm_h'], label='Chuva (mm/h)', color='lightblue', alpha=0.6)
        
        ax1.set_ylabel('Nível Rio (cm)') # AJUSTADO
        ax2.set_ylabel('Chuva (mm/h)')
        fig1.legend(loc="upper left", bbox_to_anchor=(0.1,0.9))
        ax1.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M'))
        # Adicionando linhas de referência para cotas de alerta e inundação (EXEMPLO)
        ax1.axhline(y=230, color='orange', linestyle='--', label='Cota de Alerta (230cm)')
        ax1.axhline(y=270, color='red', linestyle='--', label='Cota de Inundação (270cm)')
        fig1.legend(loc="upper right", bbox_to_anchor=(1.0,0.9)) # Ajustar legenda se necessário
        st.pyplot(fig1)

    with col_graf2:
        st.subheader("Umidade do Solo e do Ar (Simuladas)") # AJUSTADO
        fig2, ax = plt.subplots(figsize=(10, 4))
        ax.plot(dados_historicos['timestamp'], dados_historicos['umidade_solo_percent'], label='Umidade do Solo (%)', color='green', marker='o')
        ax.plot(dados_historicos['timestamp'], dados_historicos['umidade_ar_percent'], label='Umidade do Ar (%)', color='orange', linestyle='--')
        
        ax.set_ylabel('Umidade (%)')
        ax.legend()
        ax.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M'))
        st.pyplot(fig2)

    with st.expander("Ver dados brutos da simulação"):
        st.dataframe(dados_historicos)

# --- CONTEÚDO DO INTEGRANTE 2 - AGORA PREENCHIDO COM BASE NA PESQUISA ---
st.header("Contexto: O Rio Capibaribe e as Enchentes em Recife", divider='gray') # AJUSTADO
st.markdown(f"""
Recife, a capital de Pernambuco, é uma cidade historicamente marcada por sua relação com as águas, 
sendo cortada por diversos rios, com destaque para o **Rio Capibaribe**. Este rio, com cerca de 22 km de extensão dentro da cidade, 
desempenha um papel crucial na dinâmica urbana, mas também representa um desafio constante devido ao risco de inundações, 
especialmente durante os períodos de chuvas intensas.

Eventos de chuva concentrada podem elevar rapidamente o nível do Rio Capibaribe. A Agência Pernambucana de Águas e Clima (APAC) 
monitora continuamente o rio e emite **avisos hidrológicos** quando são atingidas as **cotas de alerta e inundação**. 
Por exemplo, notícias recentes indicam que o Rio Capibaribe atingiu a cota de alerta em São Lourenço da Mata, Camaragibe e Recife, 
colocando essas áreas em atenção para o risco de inundação.

A Defesa Civil do Recife entra em ação quando esses alertas são emitidos, iniciando medidas de mitigação e, se necessário, 
a evacuação de áreas de risco. Em eventos passados, como o de abril/maio de 2024, o Recife registrou mais de 
**100 mm de chuva em 24 horas**, levando o Rio Capibaribe à cota de alerta. Em outras ocasiões, volumes como 69mm em 12 horas também foram significativos. 
Esses eventos frequentemente resultam em diversos pontos de alagamento em avenidas importantes como Av. Recife, Av. Caxangá e Av. Mascarenhas de Moraes, 
e afetam severamente comunidades ribeirinhas e bairros vulneráveis como Ibura, Várzea, Pina, Coqueiral e Areias.

O sistema AquaSentinela busca simular o monitoramento de variáveis como o nível da água (inspirado nas cotas do Capibaribe) 
e a intensidade da chuva para fornecer alertas antecipados, auxiliando na prevenção e na tomada de decisão.
""")
# Se você tiver uma imagem específica de uma enchente em Recife, pode adicioná-la aqui:
# st.image("caminho_para_sua_imagem.jpg", caption="Enchente no Rio Capibaribe, Recife - [Data ou Fonte]")

st.caption("AquaSentinela - Global Solution FIAP 2025.1")