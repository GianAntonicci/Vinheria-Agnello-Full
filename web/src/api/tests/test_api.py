"""Testes sem conexão com uma instância FIWARE real."""
import unittest
from unittest.mock import patch, MagicMock
import requests
from fastapi import HTTPException
from fastapi.testclient import TestClient
from src.api.app import app
from src.api.fiware import requisitar

# Cliente em memória; as chamadas ao FIWARE são substituídas por mocks nos cenários.
client = TestClient(app)
IP = "54.236.175.117"
DEVICE = {"device_id": "lamp001", "entity_name": "urn:ngsi-ld:Lamp:001", "entity_type": "Lamp", "attributes": [{"object_id": "t", "name": "temperature", "type": "Float"}], "commands": [{"name": "red", "type": "command"}]}

# Regressões de validação, integração HTTP e operações de dispositivos sem servidor externo.
class ApiTests(unittest.TestCase):
    # Verifica: rejeita destinos inseguros.
    def test_rejeita_destinos_inseguros(self):
        with patch("src.api.fiware.requests.Session") as session:
            for ip in ("127.0.0.1", "10.0.0.1", "169.254.169.254", "::1", "http://54.236.175.117", "224.0.0.1", "255.255.255.255"):
                self.assertEqual(client.get("/api/conexao", params={"ip": ip}).status_code, 422)
            session.assert_not_called()

    # Verifica: headers timeout sem redirecionamento.
    def test_headers_timeout_sem_redirecionamento(self):
        with patch("src.api.fiware.requests.Session") as factory:
            session = factory.return_value.__enter__.return_value
            session.request.return_value = MagicMock(status_code=200, content=b'{}')
            session.request.return_value.json.return_value = {"ok": True}
            self.assertEqual(requisitar(IP, 1026, "GET", "/v2/entities"), {"ok": True})
            kwargs = session.request.call_args.kwargs
            self.assertEqual(kwargs["headers"], {"fiware-service": "smart", "fiware-servicepath": "/"})
            self.assertFalse(kwargs["allow_redirects"])
            self.assertEqual(kwargs["timeout"], (3, 6))
            self.assertFalse(session.trust_env)

    # Verifica: timeout vira 502.
    def test_timeout_vira_502(self):
        with patch("src.api.fiware.requests.Session") as factory:
            factory.return_value.__enter__.return_value.request.side_effect = requests.Timeout()
            self.assertEqual(client.get("/api/conexao", params={"ip": IP}).status_code, 502)

    @patch("src.api.app.requisitar")
    def test_valores_agrupados_e_cache(self, request):
        request.return_value = {"temperature": {"value": 13}, "alert": {"value": "ok"}}
        response = client.get("/api/atual", params={"ip": IP})
        self.assertEqual(response.json()["values"]["temperature"], 13)
        self.assertIsNone(response.json()["values"]["humidity"])
        self.assertEqual(response.headers["cache-control"], "no-store")
        self.assertEqual(request.call_count, 1)

    @patch("src.api.app.requisitar")
    def test_preserva_horario_individual_sem_usar_modificacao_da_entidade(self, request):
        request.return_value = {
            "temperature": {"value": "27.4", "metadata": {"TimeInstant": {"value": "2026-10-22T20:45:34.509Z"}}},
            "humidity": {"value": 59.9, "metadata": {"dateModified": {"value": "2026-10-22T20:45:34.939Z"}}},
            "luminosity": {"value": 52},
            "alert": {"value": "temp:critico", "metadata": {"TimeInstant": {"value": "2026-10-22T20:40:52.530Z"}}},
            "dateModified": {"value": "2026-10-22T20:45:34.950Z"},
        }
        response = client.get("/api/atual", params={"ip": IP})
        data = response.json()
        self.assertEqual(response.status_code, 200)
        self.assertEqual(data["values"]["temperature"], "27.4")
        self.assertEqual(data["timestamps"]["temperature"], "2026-10-22T20:45:34.509Z")
        self.assertEqual(data["timestamps"]["alert"], "2026-10-22T20:40:52.530Z")
        self.assertEqual(data["timestamps"]["humidity"], "2026-10-22T20:45:34.939Z")
        self.assertIsNone(data["timestamps"]["luminosity"])
        self.assertIsNone(data["timestamps"]["ledMode"])
        self.assertEqual(request.call_args.kwargs["params"]["options"], "dateModified")

    @patch("src.api.app.requisitar")
    def test_alerta_e_repassado_sem_alterar_formato_atual_ou_legado(self, request):
        for alert in ("ok", "temp:critico,luz:alerta", "temp:critico;luz:alerta", "", None):
            request.return_value = {"alert": {"value": alert}}
            response = client.get("/api/atual", params={"ip": IP})
            self.assertEqual(response.status_code, 200)
            self.assertEqual(response.json()["values"]["alert"], alert)

    @patch("src.api.app.obter_dados")
    def test_historico_parcial_e_limite(self, history):
        def fetch(ip, sensor, *args):
            if sensor == "humidity": raise HTTPException(502, "Falha de umidade")
            return [{"horario": "2026-10-22T12:00:00Z", "valor": 13}]
        history.side_effect = fetch
        response = client.get("/api/historico", params={"ip": IP, "lastN": 60})
        self.assertEqual(response.status_code, 200)
        self.assertEqual(len(response.json()["series"]), 3)
        self.assertIn("humidity", response.json()["errors"])
        self.assertEqual(client.get("/api/historico", params={"ip": IP, "lastN": 101}).status_code, 422)

    @patch("src.api.historico.requisitar")
    def test_historico_sth(self, request):
        from src.api.historico import obter_dados
        request.return_value = {"contextResponses": [{"contextElement": {"attributes": [{"values": [{"recvTime": "2026-10-22T12:00:00Z", "attrValue": "14.5"}]}]}}]}
        self.assertEqual(obter_dados(IP, "temperature")[0]["valor"], 14.5)
        request.return_value = None
        self.assertEqual(obter_dados(IP, "temperature"), [])

    @patch("src.api.app.requisitar")
    def test_cor_nao_depende_de_telemetria_de_modo_atrasada(self, request):
        request.return_value = {}
        response = client.post("/api/comandos", params={"ip": IP}, json={"command": "red"})
        self.assertEqual(response.status_code, 200)
        request.assert_called_once()
        self.assertEqual(request.call_args.args[2], "PATCH")
        self.assertEqual(request.call_args.kwargs["body"], {"red": {"type": "command", "value": ""}})
        self.assertEqual(client.post("/api/comandos", params={"ip": IP}, json={"command": "invalid"}).status_code, 422)

    @patch("src.api.app.requisitar")
    def test_cadastro_e_falha_parcial(self, request):
        request.side_effect = [{}, {}, HTTPException(502, "STH indisponível")]
        response = client.post("/api/dispositivos", params={"ip": IP}, json={"device": DEVICE})
        self.assertEqual(response.status_code, 201)
        self.assertEqual(len(response.json()["warnings"]), 1)
        calls = request.call_args_list
        self.assertEqual(calls[0].kwargs["body"]["devices"][0]["device_id"], "lamp001")
        self.assertEqual(calls[1].kwargs["body"]["provider"]["http"]["url"], f"http://{IP}:4041")
        self.assertEqual(calls[2].kwargs["body"]["notification"]["attrsFormat"], "legacy")

    @patch("src.api.app.requisitar")
    def test_edicao_preserva_identidade(self, request):
        request.return_value = DEVICE
        self.assertEqual(client.put("/api/dispositivos/lamp001", params={"ip": IP}, json=DEVICE).status_code, 200)
        self.assertEqual(set(request.call_args.kwargs["body"]), {"attributes", "commands"})
        self.assertEqual(client.put("/api/dispositivos/lamp001", params={"ip": IP}, json={**DEVICE, "entity_name": "outra"}).status_code, 422)

    @patch("src.api.app.requisitar")
    def test_exclusao_parcial(self, request):
        request.side_effect = [DEVICE, {}, HTTPException(502, "Orion indisponível")]
        response = client.delete("/api/dispositivos/lamp001", params={"ip": IP, "remove_entity": True})
        self.assertEqual(response.status_code, 200)
        self.assertIn("entidade permanece", response.json()["warnings"][0])

    # Verifica: rejeita caminhos e nomes repetidos.
    def test_rejeita_caminhos_e_nomes_repetidos(self):
        for payload in ({**DEVICE, "entity_name": "../metadata"}, {**DEVICE, "attributes": DEVICE["attributes"] * 2}):
            self.assertEqual(client.post("/api/dispositivos", params={"ip": IP}, json={"device": payload}).status_code, 422)

if __name__ == "__main__": unittest.main()
