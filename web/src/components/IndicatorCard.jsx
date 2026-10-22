import Icon from './Icon'
import { formatarValor, nivelPorValor } from '../config/sensores'
export default function IndicatorCard({
  sensor,
  value,
  previous,
  dark,
  loading,
}) {
  const valid = typeof value === 'number' && Number.isFinite(value)
  const delta = valid && typeof previous === 'number' ? value - previous : null
  const good =
    delta !== null &&
    nivelPorValor(sensor, value) <= nivelPorValor(sensor, previous)
  return (
    <article className={`indicator ${dark ? 'dark-card' : ''}`}>
      <div className="indicator-title">
        <span>{sensor.nome}</span>
        <Icon name={sensor.icon} />
      </div>
      <div className="indicator-number">
        {loading ? <span className="skeleton" /> : formatarValor(value)}
        <span>{sensor.unidade}</span>
      </div>
      <div className="indicator-footer">
        {delta !== null ? (
          <>
            <span
              className={`trend ${delta === 0 ? '' : good ? 'good' : 'bad'}`}
            >
              <Icon name={delta < 0 ? 'down' : 'up'} size={14} />
              {delta > 0 ? '+' : ''}
              {formatarValor(delta)} {sensor.unidade}
            </span>
            <span>desde a última leitura</span>
          </>
        ) : (
          <span>
            {loading ? 'Buscando leitura' : 'Aguardando próxima leitura'}
          </span>
        )}
      </div>
    </article>
  )
}
