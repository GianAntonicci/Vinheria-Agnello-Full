import { useEffect, useState } from 'react'
import { api } from '../lib/api'
import Icon from '../components/Icon'
export default function Login({ onConnect }) {
  const [ip, setIp] = useState(
    () =>
      new URLSearchParams(location.search).get('ip') ||
      localStorage.getItem('agnello-ip') ||
      '',
  )
  const [busy, setBusy] = useState(false)
  const [error, setError] = useState('')
  useEffect(() => {
    const saved =
      new URLSearchParams(location.search).get('ip') ||
      localStorage.getItem('agnello-ip')
    if (!saved) return
    let active = true
    const controller = new AbortController()
    api('conexao', { ip: saved }, { signal: controller.signal })
      .then(() => {
        if (active) onConnect(saved)
      })
      .catch(() => {
        if (active) setError('Servidor não encontrado')
      })
    return () => {
      active = false
      controller.abort()
    }
  }, [onConnect])
  async function submit(event) {
    event.preventDefault()
    setBusy(true)
    setError('')
    try {
      await api('conexao', { ip: ip.trim() })
      onConnect(ip.trim())
    } catch {
      setError('Servidor não encontrado')
    } finally {
      setBusy(false)
    }
  }
  return (
    <main className="login-layout">
      <section className="login-story">
        <div className="brand">
          <span className="brand-icon">
            <Icon name="wine" size={30} />
          </span>
          <span>
            Vinheria<strong>Agnello.</strong>
          </span>
        </div>
        <div className="login-overview">
          <span className="eyebrow">CONTROLE DO AMBIENTE</span>
          <h1>Monitoramento da adega.</h1>
          <p>
            Temperatura, umidade e luminosidade reunidas em um único painel.
          </p>
          <div className="login-stats">
            {[
              ['thermometer', 'Temperatura', 'Condições de armazenamento'],
              ['droplet', 'Umidade', 'Equilíbrio do ambiente'],
              ['sun', 'Luminosidade', 'Exposição à luz'],
            ].map(([icon, label, description]) => (
              <div className="login-sensor" key={icon}>
                <span className="login-sensor-icon">
                  <Icon name={icon} />
                </span>
                <span>
                  <strong>{label}</strong>
                  <small>{description}</small>
                </span>
              </div>
            ))}
          </div>
        </div>
        <small>
          VINHERIA AGNELLO <span aria-hidden="true">/</span> PAINEL DE MONITORAMENTO
        </small>
      </section>
      <section className="login-form-section">
        <form className="login-form" onSubmit={submit}>
          <span className="circle-icon">
            <Icon name="server" size={25} />
          </span>
          <span className="eyebrow">ACESSO AO PAINEL</span>
          <h2>Conecte sua adega.</h2>
          <p className="muted">
            Informe o IP público do seu servidor AWS para acessar o painel de
            monitoramento.
          </p>
          <label>
            IP público do servidor
            <input
              required
              value={ip}
              onChange={(e) => setIp(e.target.value)}
              inputMode="decimal"
              autoComplete="off"
              spellCheck="false"
              pattern="([0-9]{1,3}\.){3}[0-9]{1,3}"
            />
          </label>
          {error && (
            <p className="notice error" role="alert">
              <Icon name="alert" size={18} />
              {error}
            </p>
          )}
          <button className="button primary" disabled={busy}>
            {busy ? 'Conectando...' : 'Entrar no dashboard'}
            <Icon name="arrow" size={18} />
          </button>
          <p className="login-footnote">
            <Icon name="shield" size={16} />A conexão será verificada antes de
            entrar.
          </p>
        </form>
      </section>
    </main>
  )
}
