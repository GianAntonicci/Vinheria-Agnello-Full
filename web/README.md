# Vinheria Agnello

Dashboard em português com React, Vite, Tailwind CSS v4 e Recharts, mais API FastAPI. Monitora temperatura, umidade, luminosidade e LED de dispositivos FIWARE. O navegador acessa sempre a API do projeto.

## Desenvolvimento local

Requisitos: Node.js 22.12 ou superior e Python 3.12 ou superior. A partir da raiz do repositório:

```sh
python3 -m venv web/.venv
source web/.venv/bin/activate
python -m pip install -r web/requirements.txt
cd web
npm ci
npm run dev
```

Abra `http://localhost:5173`. O comando inicia Vite e Uvicorn; mantenha o ambiente Python ativado. O Vite encaminha `/api` para `http://127.0.0.1:8000`, sem configuração adicional de CORS. Para rodar separadamente, use `npm run dev:web` e `npm run dev:api` em terminais com o ambiente Python ativado.

Informe o IPv4 público da EC2 ou abra `http://localhost:5173/?ip=54.236.175.117`. A aplicação testa o Orion antes de abrir o painel. O IP e o dispositivo selecionado ficam salvos no navegador. Sair e Trocar servidor apagam a conexão salva e o IP da URL. O tema permanece salvo.

A URL da API é relativa (`/api`). Deixe `VITE_API_URL` ausente no Vercel e no desenvolvimento padrão. Essa variável é opcional para uma API hospedada separadamente, que precisaria de sua própria configuração de CORS.

## Deploy no Vercel

1. Importe o repositório em um único projeto.
2. Configure **Root Directory: `web`**, **Framework Preset: Vite**, **Build Command: `npm run build`**, **Output Directory: `dist`**, **Install Command: `npm ci`**.
3. `vercel.json` encaminha `/api/*` à função ASGI `api/index.py`, que importa `src/api/app.py`. As demais rotas entregam o React. O `requirements.txt` de `web/` instala as dependências sem matplotlib. `.python-version` define Python 3.12.
4. Depois de revisar e commitar manualmente as alterações, faça o deploy pelo painel. Este desenvolvimento não publica nem altera branches.
5. O endereço para o QR code do ESP32 é `https://seu-projeto.vercel.app/?ip=IP_PUBLICO_DA_EC2`.

Referência: [funções Python em projetos de outros frameworks](https://vercel.com/docs/functions/runtimes/python/api-directory).

A EC2 precisa disponibilizar HTTP nas portas **1026**, **4041** e **8666** para as chamadas do servidor Vercel. O navegador usa apenas HTTPS do projeto, evitando mixed content. Mosquitto e MQTT continuam sob responsabilidade da infraestrutura e do firmware.

## API e comportamento

Todas as rotas de integração exigem `ip`. Leituras e comandos aceitam `entity` e `entity_type`, com padrão `urn:ngsi-ld:Lamp:001` / `Lamp`.

| Rota | Função |
| --- | --- |
| `GET /api/conexao` | Testa a resposta do Orion |
| `GET /api/atual` | Agrupa seis atributos, polling de 5 s |
| `GET /api/historico?lastN=30` | Agrupa três históricos, polling de 10 s, 1–100 pontos |
| `POST /api/comandos` | Envia, por exemplo, `{ "command": "manual" }` |
| `GET /api/dispositivos` | Lista dispositivos provisionados |
| `POST /api/dispositivos` | Cadastra dispositivo, com registro e assinaturas opcionais |
| `PUT /api/dispositivos/{device_id}` | Edita atributos e comandos |
| `DELETE /api/dispositivos/{device_id}?remove_entity=true` | Exclui dispositivo e opcionalmente entidade |
| `GET /api/entidade` | Obtém os atributos atuais para os detalhes |

Documentação interativa: `/api/docs`. Não há WebSocket. São duas chamadas iniciais no dashboard e uma chamada por ciclo para cada grupo; o servidor consulta os três históricos em paralelo. O polling evita sobreposição e é cancelado ao sair da página. Agrupar chamadas reduz invocações, mas o consumo depende do número e duração dos acessos; não garante permanecer na franquia gratuita.

Os estados são calculados pelas leituras numéricas e comparados com `alert` (exemplo: `temp:critico,luz:alerta`). O formato atual usa vírgula entre alertas; o parser também aceita ponto e vírgula de dados legados, inclusive separadores misturados, ignorando espaços e partes vazias. Um valor vazio ou composto apenas por separadores significa alerta indisponível (“Sem dados”), nunca alerta normal. O painel exibe o pior dos dois e avisa quando divergem. Strings numéricas são convertidas, mas vazios, booleanos e valores inválidos resultam em “Sem dados”, mesmo com alerta normal ou crítico. Com `alert` ausente ou inválido, leituras válidas continuam sendo classificadas pelas faixas e o painel informa a indisponibilidade do alerta. O estado geral considera o pior nível; dados incompletos não são chamados de normais. Na falha de conexão, a última leitura permanece visível com o erro e os estados ficam “Sem dados”. Limites inclusivos e faixas estão centralizados em `src/config/sensores.js`; a luminosidade não tem limite inferior.

A API preserva `timestamps` por atributo, usando `metadata.TimeInstant` ou o `dateModified` individual, sem substituir pelo horário de atualização da entidade. Os cards mostram data e hora da leitura e marcam como desatualizada após 30 segundos. Sem timestamp, mostram “Horário da leitura indisponível”. `consultado_em` continua indicando apenas a consulta ao servidor e serve como referência para calcular a idade do dado. O aviso de divergência também informa o horário do último alerta. Esses avisos continuam válidos após a troca de separador: descrevem a divergência e a idade do dado recebido, sem atribuir uma causa ao atraso.

As cores do LED ficam habilitadas quando `ledMode` informa `manual` ou após o Orion aceitar o comando `manual` enviado pelo painel. Enquanto a telemetria não confirma a mudança, o painel exibe “Manual solicitado”. A API encaminha os comandos como no Postman, sem bloquear cores com base numa leitura anterior de `ledMode`; o firmware só aceita cores no modo manual. Sucesso no envio confirma a aceitação pelo Orion. Cor e modo físicos continuam sendo confirmados pelas leituras do ESP32.

O formulário usa `lamp001`, MQTT e UltraLight como modelo. O grupo de serviço TEF precisa existir. Registro de comandos e assinaturas numéricas seguem a coleção Postman existente. Se uma etapa adicional falhar, a API devolve avisos de sucesso parcial; não cadastre novamente um dispositivo já criado.

A edição preserva ID, nome e tipo da entidade, conforme a [API do IoT Agent](https://iotagent-node-lib.readthedocs.io/en/latest/api.html#modify-device-put-iot-devices-deviceid). A exclusão não remove registros, assinaturas nem histórico; a opção Orion remove apenas a entidade. O selo “Provisionado” confirma cadastro, sem afirmar disponibilidade física.

## Validação e segurança

```sh
npm run lint
npm run build
python -m pip install -r requirements-dev.txt
python -m unittest discover -s src/api/tests -v
node --test src/config/sensores.test.js src/lib/polling.test.js
```

Os testes simulam o FIWARE e cobrem destino, headers, timeout, respostas agrupadas, histórico parcial, comandos, cadastro, edição e exclusão parcial. A validação com EC2, ESP32 e roteamento real do Vercel exige infraestrutura ativa.

A API aceita apenas IPv4 público: rejeita IP privado, loopback, link-local, metadados da nuvem, multicast, URLs e IPv6. Portas e rotas FIWARE são fixas e identificadores são validados e codificados. Redirecionamentos e proxies do ambiente estão desabilitados. Todas as chamadas usam `fiware-service: smart` e `fiware-servicepath: /`, timeout de conexão de 3 s e de leitura de 6 s. Falhas de rede retornam 502, conflitos 409 e entradas inválidas 422.

O acesso por IP é uma escolha de navegação, sem autenticação: quem conhece o IP e acessa o projeto pode operar os dispositivos.
