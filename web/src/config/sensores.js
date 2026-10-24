// Referências compartilhadas de sensores, provisionamento e classificação dos estados do ambiente.
// Limites equivalentes aos do firmware: quatro para temperatura/umidade e dois para luz.
export const SENSORES = [
  {
    key: 'temperature',
    alertKey: 'temp',
    nome: 'Temperatura',
    unidade: '°C',
    icon: 'thermometer',
    max: 40,
    limites: [8, 10, 16, 18],
    faixas: ['8', '10', '16', '18'],
    normal: '10 a 16 °C',
    alerta: '8 a menos de 10 °C e acima de 16 a 18 °C',
    critico: 'Abaixo de 8 °C ou acima de 18 °C',
  },
  {
    key: 'humidity',
    alertKey: 'umid',
    nome: 'Umidade',
    unidade: '%',
    icon: 'droplet',
    max: 100,
    limites: [40, 50, 75, 85],
    faixas: ['40', '50', '75', '85'],
    normal: '50 a 75 %',
    alerta: '40 a menos de 50 % e acima de 75 a 85 %',
    critico: 'Abaixo de 40 % ou acima de 85 %',
  },
  {
    key: 'luminosity',
    alertKey: 'luz',
    nome: 'Luminosidade',
    unidade: '%',
    icon: 'sun',
    max: 100,
    limites: [30, 60],
    faixas: ['30', '60'],
    normal: 'Até 30 %',
    alerta: 'Acima de 30 a 60 %',
    critico: 'Acima de 60 %',
  },
]
// Nomes de comandos aceitos pelo firmware e cores correspondentes na interface.
export const CORES = [
  { comando: 'red', nome: 'Vermelho', hex: '#e54c4c' },
  { comando: 'green', nome: 'Verde', hex: '#32a46b' },
  { comando: 'blue', nome: 'Azul', hex: '#397de5' },
  { comando: 'yellow', nome: 'Amarelo', hex: '#efd34f' },
  { comando: 'cyan', nome: 'Ciano', hex: '#4ecdd6' },
  { comando: 'magenta', nome: 'Magenta', hex: '#d95fca' },
  { comando: 'white', nome: 'Branco', hex: '#ffffff' },
  { comando: 'off', nome: 'Desligado', hex: '#303332' },
]
// Modelo de provisionamento: mapeia as chaves UltraLight para os atributos NGSI do Orion.
export const PADRAO = {
  device_id: 'lamp001',
  entity_name: 'urn:ngsi-ld:Lamp:001',
  entity_type: 'Lamp',
  protocol: 'PDI-IoTA-UltraLight',
  transport: 'MQTT',
  attributes: [
    { object_id: 't', name: 'temperature', type: 'Float' },
    { object_id: 'h', name: 'humidity', type: 'Float' },
    { object_id: 'l', name: 'luminosity', type: 'Integer' },
    { object_id: 's', name: 'state', type: 'Text' },
    { object_id: 'm', name: 'ledMode', type: 'Text' },
    { object_id: 'a', name: 'alert', type: 'Text' },
  ],
  commands: [
    ...CORES.map((c) => ({ name: c.comando, type: 'command' })),
    { name: 'auto', type: 'command' },
    { name: 'manual', type: 'command' },
  ],
}
export const ESTADOS = {
  normal: 'Normal',
  alerta: 'Alerta',
  critico: 'Crítico',
  desconhecido: 'Sem dados',
}
export const PESO = { normal: 0, alerta: 1, critico: 2 }
const NIVEIS = ['normal', 'alerta', 'critico']
// A interface destaca leituras com mais de 30 segundos em relação à consulta da API.
export const LEITURA_DESATUALIZADA_MS = 30000

// Aceita apenas números finitos e strings numéricas completas; vazios não viram zero.
export function numeroSensor(value) {
  if (typeof value === 'string') {
    value = value.trim()
    if (!/^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:e[+-]?\d+)?$/i.test(value))
      return null
    value = Number(value)
  }
  return typeof value === 'number' && Number.isFinite(value) ? value : null
}

// Decodifica alertas atuais com vírgula e legados com ponto e vírgula.
export function interpretarAlerta(alert) {
  const estados = Object.fromEntries(
    SENSORES.map((s) => [s.key, 'desconhecido']),
  )
  if (typeof alert !== 'string' || !alert.trim()) {
    return { estados, disponivel: false, invalido: false }
  }
  // Normaliza caixa, acentos e espaços antes de validar cada identificação de sensor.
  const normalized = alert
    .trim()
    .toLowerCase()
    .normalize('NFD')
    .replace(/[\u0300-\u036f]/g, '')
  const parts = normalized
    .split(/[,;]/)
    .map((part) => part.trim())
    .filter(Boolean)
  if (!parts.length) return { estados, disponivel: false, invalido: false }
  if (parts.length === 1 && parts[0] === 'ok') {
    return {
      estados: Object.fromEntries(SENSORES.map((s) => [s.key, 'normal'])),
      disponivel: true,
      invalido: false,
    }
  }
  let invalido = false
  for (const part of parts) {
    const match = /^(temp|umid|luz)\s*:\s*(alerta|critico)$/.exec(part.trim())
    if (!match) {
      invalido = true
      continue
    }
    const sensor = SENSORES.find((s) => s.alertKey === match[1])
    const level = match[2]
    if (
      estados[sensor.key] === 'desconhecido' ||
      PESO[level] > PESO[estados[sensor.key]]
    )
      estados[sensor.key] = level
  }
  // Só considera normais os sensores omitidos quando o conjunto inteiro é válido.
  if (!invalido) {
    for (const sensor of SENSORES)
      if (estados[sensor.key] === 'desconhecido') estados[sensor.key] = 'normal'
  }
  return { estados, disponivel: !invalido, invalido }
}

// Retorna 0, 1 ou 2 para normal, alerta ou crítico; null indica leitura inválida.
export function nivelPorValor(sensor, value) {
  value = numeroSensor(value)
  if (value === null) return null
  const l = sensor.limites
  if (l.length === 2) return value > l[1] ? 2 : value > l[0] ? 1 : 0
  if (value < l[0] || value > l[3]) return 2
  return value < l[1] || value > l[2] ? 1 : 0
}

// Cruza os limites locais com o alerta recebido, preservando a maior gravidade e as divergências.
export function avaliarSensores(alert, values = {}) {
  const informado = interpretarAlerta(alert)
  const sensores = Object.fromEntries(
    SENSORES.map((sensor) => {
      const valor = numeroSensor(values?.[sensor.key])
      const nivel = nivelPorValor(sensor, valor)
      const calculado = nivel === null ? 'desconhecido' : NIVEIS[nivel]
      const dispositivo = informado.estados[sensor.key]
      // Ausência de leitura nunca é confirmada como normal, mesmo com alert=ok.
      const status =
        nivel === null
          ? 'desconhecido'
          : PESO[dispositivo] > nivel
            ? dispositivo
            : calculado
      const divergente =
        nivel !== null &&
        dispositivo !== 'desconhecido' &&
        calculado !== dispositivo
      return [
        sensor.key,
        { valor, calculado, informado: dispositivo, status, divergente },
      ]
    }),
  )
  return {
    sensores,
    estados: Object.fromEntries(
      SENSORES.map((s) => [s.key, sensores[s.key].status]),
    ),
    divergencias: SENSORES.filter((s) => sensores[s.key].divergente),
    alertaDisponivel: informado.disponivel,
    alertaInvalido: informado.invalido,
  }
}

// Formata com uma casa decimal em português; valores ausentes usam um travessão.
export function formatarValor(value) {
  const numero = numeroSensor(value)
  return numero === null
    ? '—'
    : numero.toLocaleString('pt-BR', { maximumFractionDigits: 1 })
}

// Prioriza crítico e alerta; dados incompletos têm precedência sobre normalidade.
export function estadoGeral(statuses) {
  const levels = Object.values(statuses)
  return (
    ['critico', 'alerta', 'desconhecido', 'normal'].find((level) =>
      levels.includes(level),
    ) || 'desconhecido'
  )
}
