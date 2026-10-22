import { lazy, Suspense, useCallback, useEffect, useState } from 'react'
import Sidebar from './components/Sidebar'
import Icon from './components/Icon'
import Login from './pages/Login'
const Dashboard = lazy(() => import('./pages/Dashboard'))
import Devices from './pages/Devices'
import Settings from './pages/Settings'
import { PADRAO } from './config/sensores'
const pageNames = {
  dashboard: 'Dashboard',
  dispositivos: 'Dispositivos',
  configuracoes: 'Configurações',
}
export default function App() {
  const [ip, setIp] = useState('')
  const [page, setPage] = useState('dashboard')
  const [menu, setMenu] = useState(false)
  const [theme, setTheme] = useState(
    () => localStorage.getItem('agnello-theme') || 'light',
  )
  const [device, setDevice] = useState(() => {
    try {
      return JSON.parse(localStorage.getItem('agnello-device')) || PADRAO
    } catch {
      return PADRAO
    }
  })
  useEffect(() => {
    document.documentElement.dataset.theme = theme
    localStorage.setItem('agnello-theme', theme)
  }, [theme])
  const connect = useCallback((value) => {
    const previous = localStorage.getItem('agnello-ip')
    if (previous !== value) {
      setDevice(PADRAO)
      localStorage.removeItem('agnello-device')
    }
    localStorage.setItem('agnello-ip', value)
    const url = new URL(location.href)
    url.searchParams.set('ip', value)
    window.history.replaceState({}, '', url)
    setIp(value)
  }, [])
  function logout() {
    localStorage.removeItem('agnello-ip')
    localStorage.removeItem('agnello-device')
    const url = new URL(location.href)
    url.searchParams.delete('ip')
    url.hash = ''
    window.history.replaceState({}, '', url)
    setIp('')
    setPage('dashboard')
    setDevice(PADRAO)
    setMenu(false)
  }
  function selectDevice(value) {
    setDevice(value)
    localStorage.setItem('agnello-device', JSON.stringify(value))
  }
  if (!ip) return <Login onConnect={connect} />
  return (
    <div className="app-layout">
      <Sidebar
        page={page}
        setPage={setPage}
        logout={logout}
        ip={ip}
        open={menu}
        setOpen={setMenu}
      />
      <main className="main-content" id="conteudo">
        <header className="topbar">
          <div className="page-title">
            <button
              className="icon-button mobile-menu"
              aria-label="Abrir menu"
              aria-expanded={menu}
              onClick={() => setMenu(!menu)}
            >
              <Icon name="menu" />
            </button>
            <div>
              <span className="eyebrow">VINHERIA AGNELLO / VISÃO GERAL</span>
              <h1>{pageNames[page]}</h1>
            </div>
          </div>
          <div className="topbar-right">
            <span className="device-chip">
              <Icon name="devices" size={15} />
              {device.device_id}
            </span>
            <span className="topbar-date">
              {new Date().toLocaleDateString('pt-BR', {
                day: 'numeric',
                month: 'long',
              })}
            </span>
            <span className="avatar">VA</span>
          </div>
        </header>
        {page === 'dashboard' ? (
          <Suspense
            fallback={
              <div className="empty-state">Carregando dashboard...</div>
            }
          >
            <Dashboard
              key={`${ip}:${device.entity_name}:${device.entity_type}`}
              ip={ip}
              device={device}
            />
          </Suspense>
        ) : page === 'dispositivos' ? (
          <Devices ip={ip} device={device} selectDevice={selectDevice} />
        ) : (
          <Settings ip={ip} theme={theme} setTheme={setTheme} logout={logout} />
        )}
        <footer className="main-footer">
          <span>Vinheria Agnello</span>
          <span>Precisão para preservar o melhor.</span>
        </footer>
      </main>
    </div>
  )
}
