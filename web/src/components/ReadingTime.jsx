// Apresenta o instante da leitura no Orion e destaca dados antigos em relação à consulta.
import Icon from './Icon'
import { LEITURA_DESATUALIZADA_MS } from '../config/sensores'

export default function ReadingTime({
  timestamp,
  referenceTime,
  label = 'Última leitura',
}) {
  const date = typeof timestamp === 'string' ? new Date(timestamp) : null
  const valid = date && Number.isFinite(date.getTime())
  // Usa o horário retornado pela API como referência para calcular a idade da leitura.
  const stale =
    valid &&
    Date.parse(referenceTime) - date.getTime() > LEITURA_DESATUALIZADA_MS
  return (
    <div className={`reading-time ${stale ? 'stale' : ''}`}>
      <Icon name="clock" size={12} />
      <span>
        {valid ? (
          <>
            {label}:{' '}
            <time dateTime={date.toISOString()}>
              {date.toLocaleString('pt-BR', {
                day: '2-digit',
                month: '2-digit',
                hour: '2-digit',
                minute: '2-digit',
                second: '2-digit',
              })}
            </time>
            {stale && ' · Desatualizada'}
          </>
        ) : (
          'Horário da leitura indisponível'
        )}
      </span>
    </div>
  )
}
