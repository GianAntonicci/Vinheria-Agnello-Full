// Renderiza a série histórica de um sensor com horário local, unidade e destaque para a última amostra.
import {
  Bar,
  BarChart,
  CartesianGrid,
  Cell,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from 'recharts'
import Icon from './Icon'
// Formatação comum aos rótulos do eixo horizontal e à dica de cada barra.
const horario = (value) =>
  new Date(value).toLocaleTimeString('pt-BR', {
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
  })
export default function GraficoSensor({ sensor, dados = [], loading, erro }) {
  return (
    <article className="card chart-card">
      <div className="section-heading">
        <div>
          <span className="eyebrow">HISTÓRICO</span>
          <h3>{sensor.nome}</h3>
        </div>
        <span className="circle-icon">
          <Icon name={sensor.icon} size={18} />
        </span>
      </div>
      {erro && (
        <p className="notice error" role="alert">
          {erro}
        </p>
      )}
      {loading ? (
        <div className="chart-empty skeleton-chart">
          Carregando histórico...
        </div>
      ) : !dados.length ? (
        <div className="chart-empty">
          <Icon name="clock" size={28} />
          <p>
            {erro
              ? 'Histórico indisponível'
              : 'Ainda não há leituras registradas.'}
          </p>
        </div>
      ) : (
        <div className="chart">
          <ResponsiveContainer width="100%" height="100%">
            <BarChart
              data={dados}
              margin={{ top: 14, right: 4, left: -18, bottom: 0 }}
            >
              <CartesianGrid
                vertical={false}
                stroke="var(--line)"
                strokeDasharray="3 5"
              />
              <XAxis
                dataKey="horario"
                tickFormatter={horario}
                minTickGap={60}
                axisLine={false}
                tickLine={false}
                tick={{ fill: 'var(--muted)', fontSize: 10 }}
              />
              <YAxis
                unit={sensor.unidade}
                axisLine={false}
                tickLine={false}
                tick={{ fill: 'var(--muted)', fontSize: 10 }}
              />
              <Tooltip
                cursor={{ fill: 'var(--surface-alt)' }}
                contentStyle={{
                  background: 'var(--surface)',
                  border: '1px solid var(--line)',
                  borderRadius: 14,
                  color: 'var(--text)',
                }}
                labelFormatter={horario}
                formatter={(v) => [
                  `${v.toLocaleString('pt-BR')} ${sensor.unidade}`,
                  sensor.nome,
                ]}
              />
              <Bar
                dataKey="valor"
                radius={[5, 5, 0, 0]}
                maxBarSize={26}
                isAnimationActive={false}
              >
                {dados.map((row, i) => (
                  <Cell
                    key={`${row.horario}-${i}`}
                    fill={
                      i === dados.length - 1
                        ? 'var(--bar-highlight)'
                        : 'var(--bar)'
                    }
                  />
                ))}
              </Bar>
            </BarChart>
          </ResponsiveContainer>
        </div>
      )}
      <div className="chart-footer">
        <span>
          <i />
          Leituras registradas
        </span>
        <span>Última barra em destaque</span>
      </div>
    </article>
  )
}
