# --- inicializar_banco.py ---
import sqlite3

NOME_BANCO = 'projeto.db'

# Conecta ao banco (cria se não existir)
conn = sqlite3.connect(NOME_BANCO)
cursor = conn.cursor()

# Cria a tabela com a estrutura correta, se ela ainda não existir
cursor.execute("""
CREATE TABLE IF NOT EXISTS sensores (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp TEXT NOT NULL,
    sensor_id TEXT NOT NULL,
    nivel_agua_cm REAL NOT NULL,
    chuva_mm_h REAL NOT NULL,
    umidade_solo_percent REAL NOT NULL,
    temperatura_ar_c REAL NOT NULL,
    umidade_ar_percent REAL NOT NULL
);
""")

conn.commit()
conn.close()

print(f"Banco de dados '{NOME_BANCO}' e tabela 'sensores' inicializados com sucesso!")