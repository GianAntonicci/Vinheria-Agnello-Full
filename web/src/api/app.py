import requests
from fastapi import FastAPI, HTTPException, Query
from fastapi.middleware.cors import CORSMiddleware

from historico import obter_dados_luminosidade, obter_dados_temperatura, obter_dados_umidade

app = FastAPI(title="API Vinheria Agnello")

# Libera o acesso do site React rodando no Vite (npm run dev)
app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:5173", "http://127.0.0.1:5173"],
    allow_methods=["GET"],
    allow_headers=["*"],
)

def formatar(dados):
    return [{'horario': d['recvTime'], 'valor': d['attrValue']} for d in dados]

def buscar(funcao, lastN):
    try:
        return formatar(funcao(lastN))
    except requests.RequestException as erro:
        raise HTTPException(status_code=502, detail=f"Erro ao acessar o FIWARE: {erro}")

@app.get("/api/luminosidade")
def luminosidade(lastN: int = Query(30, ge=1, le=100)):
    return buscar(obter_dados_luminosidade, lastN)

@app.get("/api/temperatura")
def temperatura(lastN: int = Query(30, ge=1, le=100)):
    return buscar(obter_dados_temperatura, lastN)

@app.get("/api/umidade")
def umidade(lastN: int = Query(30, ge=1, le=100)):
    return buscar(obter_dados_umidade, lastN)
