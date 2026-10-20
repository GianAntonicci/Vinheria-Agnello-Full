import { useEffect, useState } from 'react'
import { CartesianGrid, Line, LineChart, ResponsiveContainer, Tooltip, XAxis, YAxis } from 'recharts'

const API_URL = import.meta.env.VITE_API_URL ?? 'http://localhost:8000'

const formatarHorario = (horario) =>
  new Date(horario).toLocaleTimeString('pt-BR')

const GraficoSensor = ({ rota, titulo, unidade, cor, lastN = 30, intervalo = 5000 }) => {
  const [dados, setDados] = useState([])
  const [erro, setErro] = useState(null)

  useEffect(() => {
    const buscarDados = async () => {
      try {
        const resposta = await fetch(`${API_URL}/api/${rota}?lastN=${lastN}`)
        if (!resposta.ok) throw new Error(`Erro ${resposta.status}`)
        setDados(await resposta.json())
        setErro(null)
      } catch (e) {
        setErro(e.message)
      }
    }

    buscarDados()
    const id = setInterval(buscarDados, intervalo)
    return () => clearInterval(id)
  }, [rota, lastN, intervalo])

  const ultimo = dados.at(-1)

  return (
    <div className="rounded-xl bg-white p-4 shadow">
      <div className="mb-2 flex items-baseline justify-between">
        <h2 className="text-lg font-semibold text-gray-800">{titulo}</h2>
        {ultimo && (
          <span className="text-2xl font-bold" style={{ color: cor }}>
            {ultimo.valor} {unidade}
          </span>
        )}
      </div>

      {erro && <p className="text-sm text-red-600">Não foi possível carregar os dados ({erro})</p>}
      {!erro && dados.length === 0 && <p className="text-sm text-gray-500">Carregando...</p>}

      {dados.length > 0 && (
        <ResponsiveContainer width="100%" height={250}>
          <LineChart data={dados}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="horario" tickFormatter={formatarHorario} minTickGap={30} />
            <YAxis domain={['auto', 'auto']} unit={unidade} width={60} />
            <Tooltip
              labelFormatter={formatarHorario}
              formatter={(valor) => [`${valor} ${unidade}`, titulo]}
            />
            <Line type="monotone" dataKey="valor" stroke={cor} strokeWidth={2} dot={false} isAnimationActive={false} />
          </LineChart>
        </ResponsiveContainer>
      )}
    </div>
  )
}

export default GraficoSensor
