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
export const LEITURA_DESATUALIZADA_MS = 30000

export function numeroSensor(value) {
  if (typeof value === 'string') {
    value = value.trim()
    if (!/^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:e[+-]?\d+)?$/i.test(value))
      return null
    value = Number(value)
  }
  return typeof value === 'number' && Number.isFinite(value) ? value : null
}

export function interpretarAlerta(alert) {
  const estados = Object.fromEntries(
    SENSORES.map((s) => [s.key, 'desconhecido']),
  )
  if (typeof alert !== 'string' || !alert.trim()) {
    return { estados, disponivel: false, invalido: false }
  }
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
  if (!invalido) {
    for (const sensor of SENSORES)
      if (estados[sensor.key] === 'desconhecido') estados[sensor.key] = 'normal'
  }
  return { estados, disponivel: !invalido, invalido }
}

export function nivelPorValor(sensor, value) {
  value = numeroSensor(value)
  if (value === null) return null
  const l = sensor.limites
  if (l.length === 2) return value > l[1] ? 2 : value > l[0] ? 1 : 0
  if (value < l[0] || value > l[3]) return 2
  return value < l[1] || value > l[2] ? 1 : 0
}

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

export function formatarValor(value) {
  const numero = numeroSensor(value)
  return numero === null
    ? '—'
    : numero.toLocaleString('pt-BR', { maximumFractionDigits: 1 })
}

export function estadoGeral(statuses) {
  const levels = Object.values(statuses)
  return (
    ['critico', 'alerta', 'desconhecido', 'normal'].find((level) =>
      levels.includes(level),
    ) || 'desconhecido'
  )
}
