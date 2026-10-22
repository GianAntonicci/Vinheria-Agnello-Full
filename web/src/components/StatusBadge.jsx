import { ESTADOS } from '../config/sensores'
export default function StatusBadge({ status = 'desconhecido', label }) {
  return (
    <span className={`badge ${status}`}>
      <span className="status-dot" />
      {label || ESTADOS[status]}
    </span>
  )
}
