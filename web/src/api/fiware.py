"""Cliente FIWARE restrito a IPv4 públicos, portas e caminhos conhecidos."""
from ipaddress import IPv4Address
from urllib.parse import quote

import requests
from fastapi import HTTPException

# Escopo FIWARE compartilhado pelas chamadas ao IoT Agent, Orion e STH-Comet.
HEADERS = {"fiware-service": "smart", "fiware-servicepath": "/"}
# Prazos em segundos: conexão TCP e espera de leitura da resposta.
TIMEOUT = (3, 6)


# Aceita apenas IPv4 global unicast para limitar os destinos acessíveis pelo proxy da API.
def validar_ip(ip: str) -> str:
    try:
        address = IPv4Address(ip)
        if not address.is_global or address.is_multicast:
            raise ValueError
        return str(address)
    except ValueError:
        raise HTTPException(422, "Informe um IPv4 público válido da AWS.") from None


# Valida um segmento de URL e preserva os dois-pontos usados nos identificadores URN.
def segmento(value: str) -> str:
    if not value or len(value) > 256 or any(c in value for c in "/\\?#\r\n") or value in (".", ".."):
        raise HTTPException(422, "Identificador inválido.")
    return quote(value, safe=":")


# Executa chamadas apenas nas portas e famílias de rotas FIWARE previstas pela aplicação.
def requisitar(ip, port, method, path, *, params=None, body=None, vazio_404=False):
    ip = validar_ip(ip)
    if port not in (4041, 1026, 8666) or not path.startswith(("/iot/", "/v2/", "/STH/")):
        raise HTTPException(422, "Destino não permitido.")
    try:
        # Ignora proxies do ambiente e não segue redirecionamentos externos.
        with requests.Session() as session:
            session.trust_env = False
            response = session.request(method, f"http://{ip}:{port}{path}", headers=HEADERS,
                                       params=params, json=body, timeout=TIMEOUT, allow_redirects=False)
        # Algumas consultas tratam ausência de entidade ou histórico como resultado vazio.
        if response.status_code == 404 and vazio_404:
            return None
        # Preserva erros esperados de entrada e converte as demais falhas do serviço em erro de gateway.
        if not 200 <= response.status_code < 300:
            status = response.status_code if response.status_code in (400, 404, 409, 422) else 502
            raise HTTPException(status, f"FIWARE (porta {port}) respondeu com erro {response.status_code}.")
        if not response.content:
            return {}
        return response.json()
    # Expõe uma mensagem de disponibilidade sem repassar detalhes internos da requisição.
    except requests.RequestException:
        raise HTTPException(502, f"FIWARE indisponível na porta {port}. Verifique o servidor e o grupo de segurança da AWS.") from None
    except ValueError:
        raise HTTPException(502, "O FIWARE retornou uma resposta inválida.") from None
