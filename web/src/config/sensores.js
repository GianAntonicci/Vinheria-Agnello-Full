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
export const PESO = { normal: 0, alerta: 1, critico: 2, desconhecido: 3 }
export function estadosDoAlerta(alert, values = {}) {
  const result = Object.fromEntries(
    SENSORES.map((s) => [s.key, 'desconhecido']),
  )
  if (typeof alert !== 'string' || !alert.trim()) return result
  const parts = alert.split(';')
  if (
    alert !== 'ok' &&
    parts.some((p) => !/^(temp|umid|luz):(alerta|critico)$/.test(p))
  )
    return result
  for (const sensor of SENSORES) {
    if (
      typeof values[sensor.key] === 'number' &&
      Number.isFinite(values[sensor.key])
    )
      result[sensor.key] = 'normal'
    for (const part of parts) {
      const [key, level] = part.split(':')
      if (
        key === sensor.alertKey &&
        (result[sensor.key] === 'desconhecido' ||
          PESO[level] > PESO[result[sensor.key]])
      )
        result[sensor.key] = level
    }
  }
  return result
}
export function nivelPorValor(sensor, value) {
  const l = sensor.limites
  if (l.length === 2) return value <= l[0] ? 0 : value <= l[1] ? 1 : 2
  return value < l[0] || value > l[3] ? 2 : value < l[1] || value > l[2] ? 1 : 0
}
export function formatarValor(value) {
  return typeof value === 'number' && Number.isFinite(value)
    ? value.toLocaleString('pt-BR', { maximumFractionDigits: 1 })
    : '—'
}

export function estadoGeral(statuses) {
  const levels = Object.values(statuses)
  return (
    ['critico', 'alerta', 'desconhecido', 'normal'].find((level) =>
      levels.includes(level),
    ) || 'desconhecido'
  )
}
