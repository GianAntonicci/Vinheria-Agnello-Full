import { useState } from 'react'
import { CORES } from '../config/sensores'
import { api, mensagemErro } from '../lib/api'
import Icon from './Icon'
export default function LedCard({ values, params, refresh, unavailable }) {
  const [busy, setBusy] = useState('')
  const [feedback, setFeedback] = useState(null)
  const color = CORES.find((c) => c.comando === values?.state)
  const manual = values?.ledMode === 'manual'
  const modeKnown = ['manual', 'auto'].includes(values?.ledMode)
  async function send(command) {
    setBusy(command)
    setFeedback(null)
    try {
      const result = await api('comandos', params, {
        method: 'POST',
        body: JSON.stringify({ command }),
      })
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
          <small>Cor atual</small>
          <strong>{color?.nome ?? 'Sem dados'}</strong>
        </div>
        <span className="led-mode">
          {modeKnown ? (manual ? 'Manual' : 'Automático') : 'Aguardando'}
        </span>
      </div>
      <div className="mode-switch" role="group" aria-label="Modo do LED">
        {[
          ['auto', 'Automático'],
          ['manual', 'Manual'],
        ].map(([key, label]) => (
          <button
            key={key}
            aria-pressed={values?.ledMode === key}
            disabled={!!busy || unavailable}
            className={values?.ledMode === key ? 'selected' : ''}
            onClick={() => send(key)}
          >
            {label}
          </button>
        ))}
      </div>
      <p className="muted led-help">
        {!modeKnown
          ? 'Aguardando o modo informado pelo dispositivo.'
          : manual
            ? 'Escolha uma cor para enviar ao dispositivo.'
            : 'A cor é controlada automaticamente pelos sensores.'}
      </p>
      <div className="color-grid">
        {CORES.map((c) => (
          <button
            key={c.comando}
            disabled={!manual || !!busy || unavailable}
            className={values?.state === c.comando ? 'chosen' : ''}
            onClick={() => send(c.comando)}
            aria-label={`LED ${c.nome.toLowerCase()}`}
            aria-pressed={values?.state === c.comando}
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
