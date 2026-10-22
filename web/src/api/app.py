"""API do dashboard: o navegador acessa apenas esta aplicação."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from typing import Literal

from fastapi import FastAPI, HTTPException, Query
from pydantic import BaseModel, ConfigDict, Field, field_validator, model_validator

from .fiware import requisitar, segmento, validar_ip
from .historico import obter_dados

app = FastAPI(title="API Vinheria Agnello", docs_url="/api/docs", openapi_url="/api/openapi.json")
DEFAULT_ENTITY = "urn:ngsi-ld:Lamp:001"
SENSORS = ("temperature", "humidity", "luminosity")
COMMANDS = ("auto", "manual", "red", "green", "blue", "yellow", "cyan", "magenta", "white", "off")


@app.middleware("http")
async def sem_cache(request, call_next):
    response = await call_next(request)
    response.headers["Cache-Control"] = "no-store"
    return response


class Attribute(BaseModel):
    model_config = ConfigDict(extra="forbid")
    object_id: str = Field(min_length=1, max_length=64)
    name: str = Field(min_length=1, max_length=128, pattern=r"^[A-Za-z][A-Za-z0-9_]*$")
    type: Literal["Float", "Integer", "Text", "Boolean", "Number"]


class Command(BaseModel):
    model_config = ConfigDict(extra="forbid")
    name: str = Field(min_length=1, max_length=128, pattern=r"^[A-Za-z][A-Za-z0-9_]*$")
    type: Literal["command"] = "command"


class Device(BaseModel):
    model_config = ConfigDict(extra="forbid")
    device_id: str = Field(min_length=1, max_length=128)
    entity_name: str = Field(min_length=1, max_length=256)
    entity_type: str = Field(min_length=1, max_length=128)
    protocol: Literal["PDI-IoTA-UltraLight"] = "PDI-IoTA-UltraLight"
    transport: Literal["MQTT"] = "MQTT"
    attributes: list[Attribute] = Field(default_factory=list, max_length=50)
    commands: list[Command] = Field(default_factory=list, max_length=30)

    @field_validator("device_id", "entity_name", "entity_type")
    @classmethod
    def identifier(cls, value):
        segmento(value)
        return value

    @model_validator(mode="after")
    def unique_names(self):
        names = [a.name for a in self.attributes] + [c.name for c in self.commands]
        ids = [a.object_id for a in self.attributes]
        if len(names) != len(set(names)) or len(ids) != len(set(ids)):
            raise ValueError("Nomes e object_id devem ser únicos.")
        return self


class Provision(BaseModel):
    model_config = ConfigDict(extra="forbid")
    device: Device
    register_commands: bool = True
    subscribe_history: bool = True


class CommandRequest(BaseModel):
    command: Literal["auto", "manual", "red", "green", "blue", "yellow", "cyan", "magenta", "white", "off"]


def entity_path(entity):
    return f"/v2/entities/{segmento(entity)}"


@app.get("/api/conexao")
def conexao(ip: str):
    requisitar(ip, 1026, "GET", "/v2/entities", params={"limit": 1})
    return {"conectado": True, "ip": validar_ip(ip)}


@app.get("/api/atual")
def atual(ip: str, entity: str = DEFAULT_ENTITY, entity_type: str = "Lamp"):
    data = requisitar(ip, 1026, "GET", entity_path(entity), params={
        "attrs": ",".join((*SENSORS, "state", "ledMode", "alert")), "type": entity_type})
    values = {key: data.get(key, {}).get("value") for key in (*SENSORS, "state", "ledMode", "alert")}
    return {"values": values, "consultado_em": datetime.now(timezone.utc).isoformat()}


@app.get("/api/historico")
def historico(ip: str, entity: str = DEFAULT_ENTITY, entity_type: str = "Lamp",
              lastN: int = Query(30, ge=1, le=100)):
    validar_ip(ip)
    segmento(entity)
    segmento(entity_type)
    def fetch(sensor):
        try:
            return sensor, obter_dados(ip, sensor, lastN, entity, entity_type), None
        except HTTPException as error:
            return sensor, [], error.detail
    with ThreadPoolExecutor(max_workers=3) as executor:
        results = list(executor.map(fetch, SENSORS))
    errors = {key: error for key, _, error in results if error}
    if len(errors) == 3:
        raise HTTPException(502, "Não foi possível consultar o histórico no STH-Comet.")
    return {"series": {key: rows for key, rows, _ in results}, "errors": errors}


@app.post("/api/comandos")
def comandos(payload: CommandRequest, ip: str, entity: str = DEFAULT_ENTITY, entity_type: str = "Lamp"):
    if payload.command not in ("auto", "manual"):
        current = atual(ip, entity, entity_type)
        if current["values"]["ledMode"] != "manual":
            raise HTTPException(409, "Ative o modo manual e aguarde a confirmação do dispositivo.")
    requisitar(ip, 1026, "PATCH", entity_path(entity) + "/attrs", params={"type": entity_type},
               body={payload.command: {"type": "command", "value": ""}})
    return {"mensagem": "Comando enviado. Aguardando atualização do dispositivo."}


@app.get("/api/dispositivos")
def dispositivos(ip: str):
    # IoT Agent pagina a listagem; busca todos sem uma invocação por página no navegador.
    devices = []
    offset = 0
    while True:
        data = requisitar(ip, 4041, "GET", "/iot/devices", params={"limit": 100, "offset": offset})
        batch = data.get("devices", [])
        devices.extend(batch)
        offset += len(batch)
        if not batch or offset >= data.get("count", offset) or len(batch) < 100:
            break
        if offset >= 2000:
            raise HTTPException(502, "Listagem excede o limite de 2.000 dispositivos.")
    return {"devices": devices}


@app.get("/api/entidade")
def entidade(ip: str, entity: str, entity_type: str = "Lamp"):
    return requisitar(ip, 1026, "GET", entity_path(entity), params={"type": entity_type})


@app.post("/api/dispositivos", status_code=201)
def cadastrar(payload: Provision, ip: str):
    validar_ip(ip)
    device = payload.device
    requisitar(ip, 4041, "POST", "/iot/devices", body={"devices": [device.model_dump()]})
    warnings = []
    entities = [{"id": device.entity_name, "type": device.entity_type}]
    if payload.register_commands and device.commands:
        try:
            requisitar(ip, 1026, "POST", "/v2/registrations", body={
                "description": f"Comandos de {device.device_id}",
                "dataProvided": {"entities": entities, "attrs": [c.name for c in device.commands]},
                "provider": {"http": {"url": f"http://{validar_ip(ip)}:4041"}, "legacyForwarding": True}})
        except HTTPException as error:
            warnings.append(f"Dispositivo cadastrado; registro de comandos pendente: {error.detail}")
    if payload.subscribe_history:
        for attribute in device.attributes:
            if attribute.type not in ("Float", "Integer", "Number"):
                continue
            try:
                requisitar(ip, 1026, "POST", "/v2/subscriptions", body={
                    "description": f"Histórico de {device.device_id}: {attribute.name}",
                    "subject": {"entities": entities, "condition": {"attrs": [attribute.name]}},
                    "notification": {"http": {"url": f"http://{validar_ip(ip)}:8666/notify"},
                                     "attrs": [attribute.name], "attrsFormat": "legacy"}})
            except HTTPException as error:
                warnings.append(f"Assinatura de {attribute.name} pendente: {error.detail}")
    return {"device": device.model_dump(), "warnings": warnings}


@app.put("/api/dispositivos/{device_id}")
def editar(device_id: str, payload: Device, ip: str):
    if device_id != payload.device_id:
        raise HTTPException(422, "O identificador do dispositivo não pode ser alterado.")
    # A identidade permanece fixa; atualiza apenas os campos oferecidos pelo formulário.
    current = requisitar(ip, 4041, "GET", f"/iot/devices/{segmento(device_id)}")
    if payload.entity_name != current.get("entity_name") or payload.entity_type != current.get("entity_type"):
        raise HTTPException(422, "A identidade da entidade não pode ser alterada nesta edição.")
    body = payload.model_dump(include={"attributes", "commands"})
    requisitar(ip, 4041, "PUT", f"/iot/devices/{segmento(device_id)}", body=body)
    return {"mensagem": "Dispositivo atualizado."}


@app.delete("/api/dispositivos/{device_id}")
def excluir(device_id: str, ip: str, remove_entity: bool = False):
    path = f"/iot/devices/{segmento(device_id)}"
    device = requisitar(ip, 4041, "GET", path)
    requisitar(ip, 4041, "DELETE", path)
    warnings = []
    if remove_entity:
        try:
            requisitar(ip, 1026, "DELETE", entity_path(device["entity_name"]),
                       params={"type": device["entity_type"]}, vazio_404=True)
        except HTTPException as error:
            warnings.append(f"Dispositivo excluído, mas a entidade permanece no Orion: {error.detail}")
    return {"mensagem": "Dispositivo excluído.", "warnings": warnings,
            "entity": device.get("entity_name")}
