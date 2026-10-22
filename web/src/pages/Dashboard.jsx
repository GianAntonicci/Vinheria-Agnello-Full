import { useCallback, useState } from 'react'
import { api } from '../lib/api'
import usePolling from '../hooks/usePolling'
import {
  SENSORES,
  PESO,
  avaliarSensores,
  formatarValor,
  estadoGeral,
  ESTADOS,
} from '../config/sensores'
import IndicatorCard from '../components/IndicatorCard'
import LevelBar from '../components/LevelBar'
import StatusBadge from '../components/StatusBadge'
import GraficoSensor from '../components/GraficoSensor'
import LedCard from '../components/LedCard'
import Icon from '../components/Icon'
import ReadingTime from '../components/ReadingTime'
export default function Dashboard({ ip, device }) {
  const [points, setPoints] = useState(30)
  const entity = device.entity_name,
    entity_type = device.entity_type
  const currentFetcher = useCallback(
    (signal) => api('atual', { ip, entity, entity_type }, { signal }),
    [ip, entity, entity_type],
  )
  const historyFetcher = useCallback(
    (signal) =>
      api('historico', { ip, entity, entity_type, lastN: points }, { signal }),
    [ip, entity, entity_type, points],
  )
  const current = usePolling(currentFetcher, 5000)
  const history = usePolling(historyFetcher, 10000)
  const values = current.data?.values || {}
  const assessment = avaliarSensores(values.alert, values)
  const statuses = current.error
    ? Object.fromEntries(SENSORES.map((s) => [s.key, 'desconhecido']))
    : assessment.estados
  const general = estadoGeral(statuses)
  const previousAssessment = avaliarSensores(
    current.previous?.values?.alert,
    current.previous?.values,
  )
  const previousGeneral = estadoGeral(previousAssessment.estados)
  const stateDelta =
    current.previous &&
    general !== 'desconhecido' &&
    previousGeneral !== 'desconhecido'
      ? PESO[general] - PESO[previousGeneral]
      : null
  return (
    <>
      <div className="page-subheading">
        <p>Uma visão completa do ambiente dos seus vinhos.</p>
        <span className="live-label">
          <span className={`status-dot ${current.error ? 'offline' : ''}`} />
          {current.error
            ? 'Conexão interrompida'
            : current.loading
              ? 'Conectando'
              : 'Atualização a cada 5 s'}
        </span>
      </div>
      {current.error && (
        <div className="notice error" role="alert">
          <Icon name="alert" />
          <span>
            {current.error}{' '}
            {current.data &&
              'Os valores exibidos são da última consulta bem-sucedida.'}
          </span>
          <button className="text-button" onClick={current.refresh}>
            Tentar novamente
          </button>
        </div>
      )}
      <section className="indicators" aria-label="Indicadores atuais">
        {SENSORES.map((sensor, i) => (
          <IndicatorCard
            key={sensor.key}
            sensor={sensor}
            value={assessment.sensores[sensor.key].valor}
            previous={current.previous?.values[sensor.key]}
            status={statuses[sensor.key]}
            timestamp={current.data?.timestamps?.[sensor.key]}
            referenceTime={current.data?.consultado_em}
            dark={i === 0}
            loading={current.loading}
          />
        ))}
        <article className="indicator">
          <div className="indicator-title">
            <span>Estado geral</span>
            <Icon name="shield" />
          </div>
          <div className="general-state">
            <StatusBadge status={general} />
          </div>
          <div className="indicator-footer">
            {stateDelta !== null ? (
              <span
                className={`trend ${stateDelta > 0 ? 'text-error' : stateDelta < 0 ? 'text-success' : ''}`}
              >
                <Icon name={stateDelta > 0 ? 'up' : 'down'} size={14} />
                {stateDelta > 0
                  ? 'Piorou'
                  : stateDelta < 0
                    ? 'Melhorou'
                    : 'Estável'}{' '}
                desde a última leitura
              </span>
            ) : (
              <span>Conforme as leituras e o alerta</span>
            )}
          </div>
        </article>
      </section>
      {!current.error && current.data && assessment.divergencias.length > 0 && (
        <div className="notice warning classification-notice" role="status">
          <Icon name="alert" size={16} />
          <div>
            <p>
              Divergência com o alerta do dispositivo. Exibindo o pior estado:{' '}
              {assessment.divergencias
                .map(
                  (sensor) =>
                    `${sensor.nome} (${ESTADOS[assessment.sensores[sensor.key].calculado]} pela leitura; ${ESTADOS[assessment.sensores[sensor.key].informado]} pelo dispositivo)`,
                )
                .join('; ')}
              .
            </p>
            <ReadingTime
              timestamp={current.data?.timestamps?.alert}
              referenceTime={current.data?.consultado_em}
              label="Última atualização do alerta no Orion"
            />
          </div>
        </div>
      )}
      {!current.error && current.data && !assessment.alertaDisponivel && (
        <p className="classification-note" role="status">
          {assessment.alertaInvalido
            ? 'Alerta do dispositivo com formato inválido.'
            : 'Alerta do dispositivo indisponível.'}{' '}
          Os estados são calculados pelas leituras válidas.
        </p>
      )}
      <section className="environment-grid">
        <article className="card levels-card">
          <div className="section-heading">
            <div>
              <span className="eyebrow">CONDIÇÕES DE CONSERVAÇÃO</span>
              <h2>O ambiente, agora</h2>
            </div>
            <span className="circle-icon">
              <Icon name="shield" />
            </span>
          </div>
          <div className="legend">
            {['normal', 'alerta', 'critico'].map((s) => (
              <span key={s}>
                <i className={s} />
                {ESTADOS[s]}
              </span>
            ))}
          </div>
          {SENSORES.map((sensor) => (
            <LevelBar
              key={sensor.key}
              sensor={sensor}
              value={assessment.sensores[sensor.key].valor}
              status={statuses[sensor.key]}
            />
          ))}
          <p className="card-footnote">
            <Icon name="clock" size={15} />
            {current.data
              ? `Última consulta: ${new Date(current.data.consultado_em).toLocaleTimeString('pt-BR')}`
              : 'Aguardando a primeira consulta ao servidor'}
          </p>
        </article>
        <LedCard
          values={values}
          params={{ ip, entity, entity_type }}
          refresh={current.refresh}
          unavailable={current.loading || !!current.error}
        />
      </section>
      <div className="history-heading">
        <div>
          <span className="eyebrow">CADA LEITURA CONTA</span>
          <h2>Evolução dos sensores</h2>
          <p className="muted">Histórico atualizado a cada 10 segundos.</p>
        </div>
        <div
          className="segmented"
          role="group"
          aria-label="Quantidade de pontos"
        >
          {[30, 60, 100].map((n) => (
            <button
              key={n}
              aria-pressed={points === n}
              className={points === n ? 'selected' : ''}
              onClick={() => setPoints(n)}
            >
              {n} pontos
            </button>
          ))}
        </div>
      </div>
      <section className="charts-grid" key={points}>
        {SENSORES.map((sensor) => (
          <GraficoSensor
            key={sensor.key}
            sensor={sensor}
            dados={history.data?.series[sensor.key]}
            loading={history.loading}
            erro={history.error || history.data?.errors[sensor.key]}
          />
        ))}
      </section>
      <article className="card sensor-table">
        <div className="section-heading">
          <div>
            <span className="eyebrow">RESUMO DO DISPOSITIVO</span>
            <h2>Leituras e faixas ideais</h2>
          </div>
          <span className="device-chip">{device.device_id}</span>
        </div>
        <div className="table-scroll">
          <table>
            <thead>
              <tr>
                <th>Sensor</th>
                <th>Leitura atual</th>
                <th>Faixa normal</th>
                <th>Estado</th>
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
                  <td>
                    {formatarValor(assessment.sensores[s.key].valor)}{' '}
                    {s.unidade}
                  </td>
                  <td>{s.normal}</td>
                  <td>
                    <StatusBadge status={statuses[s.key]} />
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </article>
    </>
  )
}
