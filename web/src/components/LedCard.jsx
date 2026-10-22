import { useState } from 'react'
import { CORES } from '../config/sensores'
import { api, mensagemErro } from '../lib/api'
import Icon from './Icon'
export default function LedCard({ values, params, refresh, unavailable }) {
  const [busy, setBusy] = useState('')
  const [feedback, setFeedback] = useState(null)
  const [requestedMode, setRequestedMode] = useState(null)
  const [requestedColor, setRequestedColor] = useState(null)
  const confirmedMode = values?.ledMode
  // Descarta a intenção assim que uma leitura confirma o modo solicitado.
  if (requestedMode && confirmedMode === requestedMode) setRequestedMode(null)
  const mode = requestedMode ?? confirmedMode
  const manual = mode === 'manual'
  const modeKnown = ['manual', 'auto'].includes(mode)
  if (requestedColor && (values?.state === requestedColor || mode === 'auto')) {
    setRequestedColor(null)
  }
  const selectedColor = manual ? requestedColor ?? values?.state : values?.state
  const color = CORES.find((c) => c.comando === selectedColor)
  const awaitingColor = manual && requestedColor && requestedColor !== values?.state
  const awaitingConfirmation = requestedMode && requestedMode !== confirmedMode
  async function send(command) {
    setBusy(command)
    setFeedback(null)
    try {
      const result = await api('comandos', params, {
        method: 'POST',
        body: JSON.stringify({ command }),
      })
      if (command === 'manual' || command === 'auto') {
        setRequestedMode(command)
        if (command === 'auto') setRequestedColor(null)
      } else {
        setRequestedColor(command)
      }
      setFeedback({ ok: true, text: result.mensagem })
      refresh()
    } catch (error) {
      setFeedback({ ok: false, text: mensagemErro(error) })
    } finally {
      setBusy('')
    }
  }
  return (
    <article className="card led-card">
      <div className="section-heading">
        <div>
          <span className="eyebrow">CONTROLE DO AMBIENTE</span>
          <h2>Iluminação do LED</h2>
        </div>
        <span className="circle-icon">
          <Icon name="light" />
        </span>
      </div>
      <div className="led-current">
        <span
          className="led-orb"
          style={{
            background: color?.hex ?? 'var(--surface-alt)',
            boxShadow:
              color && color.comando !== 'off'
                ? `0 0 0 9px ${color.hex}15, 0 0 24px ${color.hex}35`
                : undefined,
          }}
        />
        <div>
          <small>{awaitingColor ? 'Cor solicitada' : 'Cor atual'}</small>
          <strong>{color?.nome ?? 'Sem dados'}</strong>
        </div>
        <span className="led-mode">
          {awaitingConfirmation
            ? `${manual ? 'Manual' : 'Automático'} solicitado`
            : modeKnown
              ? manual
                ? 'Manual'
                : 'Automático'
              : 'Aguardando'}
        </span>
      </div>
      <div className="mode-switch" role="group" aria-label="Modo do LED">
        {[
          ['auto', 'Automático'],
          ['manual', 'Manual'],
        ].map(([key, label]) => (
          <button
            key={key}
            aria-pressed={mode === key}
            disabled={!!busy || unavailable}
            className={mode === key ? 'selected' : ''}
            onClick={() => send(key)}
          >
            {label}
          </button>
        ))}
      </div>
      <p className="muted led-help">
        {awaitingConfirmation
          ? `Comando de modo ${manual ? 'manual' : 'automático'} enviado. Aguardando confirmação do dispositivo.${manual ? ' Você já pode enviar uma cor.' : ''}`
          : !modeKnown
            ? 'O dispositivo ainda não informou o modo. Selecione Manual para enviar uma cor.'
            : manual
              ? 'Escolha uma cor para enviar ao dispositivo.'
              : 'A cor é controlada automaticamente pelos sensores.'}
      </p>
      <div className="color-grid">
        {CORES.map((c) => (
          <button
            key={c.comando}
            disabled={!manual || !!busy || unavailable}
            className={selectedColor === c.comando ? 'chosen' : ''}
            onClick={() => send(c.comando)}
            aria-label={`LED ${c.nome.toLowerCase()}`}
            aria-pressed={selectedColor === c.comando}
          >
            <span className="color-dot" style={{ background: c.hex }} />
            {c.nome}
          </button>
        ))}
      </div>
      <div className="led-feedback" aria-live="polite">
        {busy ? (
          <span>Enviando comando...</span>
        ) : feedback ? (
          <span className={feedback.ok ? 'text-success' : 'text-error'}>
            {feedback.text}
          </span>
        ) : (
          <span>O estado é confirmado pela leitura do ESP32.</span>
        )}
      </div>
    </article>
  )
}
