import { test } from 'node:test'
import assert from 'node:assert/strict'
import {
  SENSORES,
  avaliarSensores,
  nivelPorValor,
  estadoGeral,
  interpretarAlerta,
  numeroSensor,
  formatarValor,
} from './sensores.js'

const normais = { temperature: 13, humidity: 60, luminosity: 25 }
const casos = [
  [
    [-10, 2],
    [7.9, 2],
    [8, 1],
    [9.9, 1],
    [10, 0],
    [13, 0],
    [16, 0],
    [16.1, 1],
    [18, 1],
    [18.1, 2],
    [27.4, 2],
  ],
  [
    [0, 2],
    [39.9, 2],
    [40, 1],
    [49.9, 1],
    [50, 0],
    [60, 0],
    [75, 0],
    [75.1, 1],
    [85, 1],
    [85.1, 2],
    [100, 2],
  ],
  [
    [-5, 0],
    [0, 0],
    [30, 0],
    [30.1, 1],
    [52, 1],
    [60, 1],
    [60.1, 2],
    [100, 2],
  ],
]

SENSORES.forEach((sensor, i) => {
  test(`${sensor.nome}: bordas inclusivas, valores fora das faixas e strings numéricas`, () => {
    for (const [valor, nivel] of casos[i]) {
      for (const entrada of [valor, String(valor), ` ${valor} `]) {
        assert.equal(
          nivelPorValor(sensor, entrada),
          nivel,
          `${sensor.key}: ${JSON.stringify(entrada)}`,
        )
        const resultado = avaliarSensores('ok', {
          ...normais,
          [sensor.key]: entrada,
        })
        assert.equal(
          resultado.sensores[sensor.key].status,
          ['normal', 'alerta', 'critico'][nivel],
        )
        assert.equal(resultado.sensores[sensor.key].valor, valor)
      }
    }
  })
})

test('reproduz a resposta real do Orion: 27,4 °C crítico e 52 % de luz em alerta', () => {
  const r = avaliarSensores('temp:critico', {
    temperature: 27.4,
    humidity: 59.9,
    luminosity: 52,
  })
  assert.deepEqual(r.estados, {
    temperature: 'critico',
    humidity: 'normal',
    luminosity: 'alerta',
  })
  assert.equal(estadoGeral(r.estados), 'critico')
  assert.deepEqual(
    r.divergencias.map((s) => s.key),
    ['luminosity'],
  )
  assert.equal(r.sensores.luminosity.informado, 'normal')
})

test('alert=ok nunca reduz uma leitura crítica para normal', () => {
  const r = avaliarSensores('ok', {
    temperature: '27.4',
    humidity: '90',
    luminosity: '70',
  })
  assert.deepEqual(Object.values(r.estados), ['critico', 'critico', 'critico'])
  assert.equal(r.divergencias.length, 3)
})

test('preserva o pior entre valor e alerta, nos dois sentidos', () => {
  const r = avaliarSensores('temp:critico,umid:alerta,luz:critico', normais)
  assert.deepEqual(r.estados, {
    temperature: 'critico',
    humidity: 'alerta',
    luminosity: 'critico',
  })
  assert.equal(r.divergencias.length, 3)
  assert.equal(estadoGeral(r.estados), 'critico')
})

test('não sinaliza divergência quando leituras e alerta concordam', () => {
  const r = avaliarSensores('temp:critico,umid:alerta,luz:critico', {
    temperature: 22,
    humidity: 80,
    luminosity: 90,
  })
  assert.equal(r.divergencias.length, 0)
  assert.equal(r.alertaDisponivel, true)
  assert.equal(avaliarSensores('ok', normais).divergencias.length, 0)
})

test('alerta ausente ou vazio usa as leituras, sem inventar alerta normal', () => {
  for (const alert of [undefined, null, '', '  ']) {
    const r = avaliarSensores(alert, { ...normais, temperature: 27 })
    assert.equal(r.estados.temperature, 'critico')
    assert.equal(r.sensores.temperature.informado, 'desconhecido')
    assert.equal(r.alertaDisponivel, false)
    assert.equal(r.divergencias.length, 0)
    assert.equal(avaliarSensores(alert, {}).estados.temperature, 'desconhecido')
  }
})

test('dados ausentes ou inválidos são Sem dados mesmo com alerta crítico', () => {
  const invalidos = [
    undefined,
    null,
    '',
    ' ',
    'NaN',
    'Infinity',
    '23.5 °C',
    '0x10',
    '1,5',
    true,
    false,
    NaN,
    Infinity,
    -Infinity,
    {},
    [],
  ]
  for (const valor of invalidos) {
    for (const alert of [
      'ok',
      'temp:critico;umid:critico;luz:critico',
      undefined,
    ]) {
      const r = avaliarSensores(alert, {
        temperature: valor,
        humidity: valor,
        luminosity: valor,
      })
      assert.deepEqual(Object.values(r.estados), [
        'desconhecido',
        'desconhecido',
        'desconhecido',
      ])
      assert.equal(r.divergencias.length, 0)
      assert.equal(nivelPorValor(SENSORES[0], valor), null)
      assert.equal(formatarValor(valor), '—')
    }
  }
})

test('parser aceita espaços, caixa e acentos, com os identificadores temp/umid/luz', () => {
  const r = interpretarAlerta(' TEMP : CRÍTICO ; UMID: ALERTA ; LUZ : CRITICO ')
  assert.deepEqual(r.estados, {
    temperature: 'critico',
    humidity: 'alerta',
    luminosity: 'critico',
  })
  assert.equal(r.disponivel, true)
  assert.equal(interpretarAlerta(' OK ').estados.temperature, 'normal')
})

test('tokens inválidos não apagam alertas válidos nem implicam normalidade', () => {
  const r = avaliarSensores('temp:critico;humidity:critical', normais)
  assert.equal(r.estados.temperature, 'critico')
  assert.equal(r.sensores.humidity.informado, 'desconhecido')
  assert.equal(r.estados.humidity, 'normal')
  assert.equal(r.alertaInvalido, true)
  assert.equal(
    avaliarSensores('invalido', { temperature: 27 }).estados.temperature,
    'critico',
  )
  assert.equal(
    interpretarAlerta('temp:critico;temp:alerta').estados.temperature,
    'critico',
  )
})

test('estado geral escolhe o pior nível e não chama dados incompletos de normais', () => {
  assert.equal(
    estadoGeral({
      temperature: 'normal',
      humidity: 'normal',
      luminosity: 'normal',
    }),
    'normal',
  )
  assert.equal(
    estadoGeral({
      temperature: 'normal',
      humidity: 'alerta',
      luminosity: 'normal',
    }),
    'alerta',
  )
  assert.equal(
    estadoGeral({
      temperature: 'critico',
      humidity: 'desconhecido',
      luminosity: 'alerta',
    }),
    'critico',
  )
  assert.equal(
    estadoGeral({ temperature: 'normal', humidity: 'desconhecido' }),
    'desconhecido',
  )
  assert.equal(estadoGeral({}), 'desconhecido')
})

test('conversão numérica preserva zero e formato científico sem coerção de vazios', () => {
  assert.equal(numeroSensor('0'), 0)
  assert.equal(numeroSensor('2.35e1'), 23.5)
  assert.equal(formatarValor('23.5'), '23,5')
})

test('parser: ok é normal, inclusive com espaços e partes vazias', () => {
  for (const alert of ['ok', ' OK ', ', , ok ; ;']) {
    const r = interpretarAlerta(alert)
    assert.deepEqual(r.estados, {
      temperature: 'normal',
      humidity: 'normal',
      luminosity: 'normal',
    })
    assert.equal(r.disponivel, true)
    assert.equal(r.invalido, false)
  }
})

test('parser: um alerta identifica o sensor sem alterar os demais', () => {
  const r = interpretarAlerta('temp:critico')
  assert.deepEqual(r.estados, {
    temperature: 'critico',
    humidity: 'normal',
    luminosity: 'normal',
  })
  assert.equal(r.disponivel, true)
})

for (const separador of [',', ';']) {
  test(`parser: múltiplos alertas com ${separador === ',' ? 'vírgula' : 'ponto e vírgula legado'}`, () => {
    const r = interpretarAlerta(
      ['temp:critico', 'umid:alerta', 'luz:critico'].join(separador),
    )
    assert.deepEqual(r.estados, {
      temperature: 'critico',
      humidity: 'alerta',
      luminosity: 'critico',
    })
    assert.equal(r.disponivel, true)
    assert.equal(r.invalido, false)
  })
}

test('parser: ignora espaços, partes vazias e aceita separadores misturados', () => {
  const r = interpretarAlerta(
    ' , ; TEMP : CRÍTICO ,, ; UMID : ALERTA ; , LUZ : CRITICO ,; ',
  )
  assert.deepEqual(r.estados, {
    temperature: 'critico',
    humidity: 'alerta',
    luminosity: 'critico',
  })
  assert.equal(r.disponivel, true)
  assert.equal(r.invalido, false)
})

test('parser: vazio, ausente ou somente separadores significa Sem dados', () => {
  for (const alert of [undefined, null, '', '   ', ',', ';;', ' , ; , ']) {
    const r = interpretarAlerta(alert)
    assert.deepEqual(Object.values(r.estados), [
      'desconhecido',
      'desconhecido',
      'desconhecido',
    ])
    assert.equal(r.disponivel, false)
    assert.equal(r.invalido, false)
    assert.equal(estadoGeral(avaliarSensores(alert).estados), 'desconhecido')
  }
})

test('ambos os formatos preservam o pior estado e o aviso de divergência', () => {
  const novo = avaliarSensores('temp:critico,luz:alerta', normais)
  const legado = avaliarSensores('temp:critico;luz:alerta', normais)
  assert.deepEqual(novo, legado)
  assert.deepEqual(
    novo.divergencias.map((s) => s.key),
    ['temperature', 'luminosity'],
  )
  assert.equal(estadoGeral(novo.estados), 'critico')
})
