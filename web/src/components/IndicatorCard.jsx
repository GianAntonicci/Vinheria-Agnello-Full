// Exibe a leitura atual, a variação desde a consulta anterior e o horário de um sensor.
import Icon from './Icon'
import StatusBadge from './StatusBadge'
import ReadingTime from './ReadingTime'
import { formatarValor, nivelPorValor, numeroSensor } from '../config/sensores'
export default function IndicatorCard({
  sensor,
  value,
  previous,
  dark,
  loading,
  status,
  timestamp,
  referenceTime,
}) {
  value = numeroSensor(value)
  previous = numeroSensor(previous)
  // Só calcula a variação quando as duas leituras são válidas.
  const delta = value !== null && previous !== null ? value - previous : null
  // A tendência considera a gravidade da faixa, pois aumentar ou diminuir o valor nem sempre melhora o ambiente.
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
      <div className="indicator-status">
        <StatusBadge status={status} />
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
      <ReadingTime timestamp={timestamp} referenceTime={referenceTime} />
    </article>
  )
}
