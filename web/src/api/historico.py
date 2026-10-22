"""Histórico STH-Comet, sem dependências de renderização no servidor."""
from math import isfinite
from fastapi import HTTPException

from .fiware import requisitar, segmento


def obter_dados(ip, atributo, lastN=30, entidade="urn:ngsi-ld:Lamp:001", tipo="Lamp"):
    path = (f"/STH/v1/contextEntities/type/{segmento(tipo)}/id/{segmento(entidade)}"
            f"/attributes/{segmento(atributo)}")
    data = requisitar(ip, 8666, "GET", path, params={"lastN": lastN}, vazio_404=True)
    if data is None:
        return []
    try:
        responses = data.get("contextResponses", [])
        if not responses:
            return []
        element = responses[0]
        status = int(element.get("statusCode", {}).get("code", 200))
        if status == 404:
            return []
        if status >= 400:
            raise ValueError
        attrs = element["contextElement"]["attributes"]
        values = attrs[0].get("values", []) if attrs else []
        result = []
        for value in values:
            number = float(value["attrValue"])
            if isfinite(number):
                result.append({"horario": value["recvTime"], "valor": number})
        return sorted(result, key=lambda row: row["horario"])
    except (KeyError, TypeError, ValueError, IndexError):
        raise HTTPException(502, "Histórico inválido retornado pelo STH-Comet.") from None


def obter_dados_luminosidade(ip, lastN=30, **kwargs):
    return obter_dados(ip, "luminosity", lastN, **kwargs)


def obter_dados_temperatura(ip, lastN=30, **kwargs):
    return obter_dados(ip, "temperature", lastN, **kwargs)


def obter_dados_umidade(ip, lastN=30, **kwargs):
    return obter_dados(ip, "humidity", lastN, **kwargs)
