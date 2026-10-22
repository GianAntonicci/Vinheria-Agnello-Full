import { test } from 'node:test'
import assert from 'node:assert/strict'
import { SENSORES, estadosDoAlerta, nivelPorValor, estadoGeral } from './sensores.js'
test('o alerta informa os níveis sem inferir normalidade de dados ausentes', () => {
  const values = { temperature: 13, humidity: 60, luminosity: 25 }
  assert.deepEqual(estadosDoAlerta('temp:critico;luz:alerta', values), {
    temperature: 'critico',
    humidity: 'normal',
    luminosity: 'alerta',
  })
  assert.equal(estadosDoAlerta(undefined, values).temperature, 'desconhecido')
  assert.equal(estadosDoAlerta('ok', {}).temperature, 'desconhecido')
  assert.equal(estadosDoAlerta('temp:critico', {}).temperature, 'critico')
  assert.equal(estadoGeral({ temperature: 'critico', humidity: 'desconhecido' }), 'critico')
  assert.equal(estadosDoAlerta('invalido', values).humidity, 'desconhecido')
})
test('limites inclusivos iguais aos do firmware', () => {
  assert.deepEqual(
    [7.9, 8, 10, 16, 18, 18.1].map((v) => nivelPorValor(SENSORES[0], v)),
    [2, 1, 0, 0, 1, 2],
  )
  assert.deepEqual(
    [39, 40, 50, 75, 85, 86].map((v) => nivelPorValor(SENSORES[1], v)),
    [2, 1, 0, 0, 1, 2],
  )
  assert.deepEqual(
    [30, 31, 60, 61].map((v) => nivelPorValor(SENSORES[2], v)),
    [0, 1, 1, 2],
  )
})
