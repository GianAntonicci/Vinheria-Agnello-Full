// Mostra a leitura como barra proporcional acompanhada das faixas de normalidade, alerta e criticidade.
import StatusBadge from './StatusBadge'
import Icon from './Icon'
import { formatarValor, numeroSensor } from '../config/sensores'
export default function LevelBar({ sensor, value, status }) {
  value = numeroSensor(value)
  const valid = value !== null
  const points = [0, ...sensor.limites, sensor.max]
  // Luz tem apenas limites superiores; os outros sensores têm faixas dos dois lados da região normal.
  const classes =
    sensor.limites.length === 2
      ? ['normal', 'alerta', 'critico']
      : ['critico', 'alerta', 'normal', 'alerta', 'critico']
  return (
    <div className="level-row">
      <div className="level-heading">
        <span className="level-name">
          <Icon name={sensor.icon} size={18} />
          {sensor.nome}
        </span>
        <span className="level-value">
          {formatarValor(value)} <small>{sensor.unidade}</small>
        </span>
        <StatusBadge status={status} />
      </div>
      <div
        className="level-track"
        role="meter"
        aria-label={sensor.nome}
        aria-valuemin={0}
        aria-valuemax={sensor.max}
        aria-valuenow={
          valid ? Math.max(0, Math.min(sensor.max, value)) : undefined
        }
        aria-valuetext={valid ? `${value} ${sensor.unidade}` : 'Sem dados'}
      >
        <div
          className="level-fill"
          style={{
            width: valid
              ? `${Math.max(0, Math.min(100, (value / sensor.max) * 100))}%`
              : 0,
          }}
        />
        {valid && (
          <span
            className="level-marker"
            style={{
              left: `${Math.max(0, Math.min(100, (value / sensor.max) * 100))}%`,
            }}
          />
        )}
      </div>
      <div className="range-bands">
        {classes.map((c, i) => (
          <span
            key={i}
            className={c}
            style={{
              width: `${((points[i + 1] - points[i]) / sensor.max) * 100}%`,
            }}
          />
        ))}
      </div>
      <div className="range-labels">
        <span>0</span>
        {sensor.limites.map((n) => (
          <span key={n} style={{ left: `${(n / sensor.max) * 100}%` }}>
            {n}
          </span>
        ))}
        <span>
          {sensor.max} {sensor.unidade}
        </span>
      </div>
    </div>
  )
}
