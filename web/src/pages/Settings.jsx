import { SENSORES } from '../config/sensores'
import Icon from '../components/Icon'
export default function Settings({ ip, theme, setTheme, logout }) {
  return (
    <>
      <p className="page-description">
        Seu ambiente de monitoramento, do seu jeito.
      </p>
      <div className="settings-grid">
        <section className="card">
          <div className="section-heading">
            <h2>Aparência</h2>
            <Icon name={theme === 'dark' ? 'moon' : 'sun'} />
          </div>
          <p className="muted">
            Escolha o tema do painel. Sua preferência fica salva neste
            navegador.
          </p>
          <div className="theme-options">
            {[
              ['light', 'sun', 'Claro'],
              ['dark', 'moon', 'Escuro'],
            ].map(([key, icon, label]) => (
              <button
                key={key}
                onClick={() => setTheme(key)}
                className={theme === key ? 'selected' : ''}
                aria-pressed={theme === key}
              >
                <Icon name={icon} size={24} />
                {label}
                {theme === key && <Icon name="check" size={17} />}
              </button>
            ))}
          </div>
        </section>
        <section className="card">
          <div className="section-heading">
            <h2>Conexão com o servidor</h2>
            <Icon name="server" />
          </div>
          <p className="muted">IP público da instância AWS conectada.</p>
          <div className="ip-display">{ip}</div>
          <button className="button secondary" onClick={logout}>
            Trocar servidor
            <Icon name="arrow" size={17} />
          </button>
        </section>
      </div>
      <section className="card">
        <div className="section-heading">
          <div>
            <span className="eyebrow">REFERÊNCIA DO FIRMWARE</span>
            <h2>Faixas de conservação</h2>
          </div>
          <Icon name="shield" />
        </div>
        <p className="muted">
          Valores de referência, disponíveis apenas para consulta.
        </p>
        <div className="table-scroll">
          <table>
            <thead>
              <tr>
                <th>Sensor</th>
                <th>Normal</th>
                <th>Alerta</th>
                <th>Crítico</th>
              </tr>
            </thead>
            <tbody>
              {SENSORES.map((s) => (
                <tr key={s.key}>
                  <td>
                    <span className="table-sensor">
                      <Icon name={s.icon} />
                      {s.nome}
                    </span>
                  </td>
                  <td>{s.normal}</td>
                  <td>{s.alerta}</td>
                  <td>{s.critico}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </section>
    </>
  )
}
