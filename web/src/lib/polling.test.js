import { test } from 'node:test'
import assert from 'node:assert/strict'
import { criarPolling } from './polling.js'

test('atualização após comando não é perdida durante uma consulta em andamento', async () => {
  const scheduled = []
  let finish,
    calls = 0
  const polling = criarPolling(
    () => {
      calls++
      return new Promise((resolve) => {
        finish = resolve
      })
    },
    5000,
    (run, delay) => {
      scheduled.push({ run, delay })
      return scheduled.length
    },
    () => {},
  )
  const first = polling.atualizar()
  await polling.atualizar()
  await polling.atualizar()
  assert.equal(calls, 1)
  finish()
  await first
  assert.equal(scheduled[0].delay, 0)
  const second = scheduled[0].run()
  assert.equal(calls, 2)
  finish()
  await second
  assert.equal(scheduled[1].delay, 5000)
  polling.encerrar()
})

test('sair da página cancela a leitura e impede novos ciclos', async () => {
  let signal,
    finish,
    schedules = 0
  const polling = criarPolling(
    (value) => {
      signal = value
      return new Promise((resolve) => {
        finish = resolve
      })
    },
    5000,
    () => {
      schedules++
    },
    () => {},
  )
  const pending = polling.atualizar()
  polling.encerrar()
  assert.equal(signal.aborted, true)
  finish()
  await pending
  await polling.atualizar()
  assert.equal(schedules, 0)
})
