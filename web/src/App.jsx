import GraficoSensor from './components/GraficoSensor.jsx'

const App = () => {
  return (
    <main className="min-h-screen bg-gray-100 p-6">
      <h1 className="mb-6 text-2xl font-bold text-gray-800">Vinheria Agnello</h1>

      <div className="grid gap-6 lg:grid-cols-3">
        <GraficoSensor rota="luminosidade" titulo="Luminosidade" unidade="%" cor="orange" />
        <GraficoSensor rota="temperatura" titulo="Temperatura" unidade="°C" cor="red" />
        <GraficoSensor rota="umidade" titulo="Umidade" unidade="%" cor="blue" />
      </div>
    </main>
  )
}

export default App
