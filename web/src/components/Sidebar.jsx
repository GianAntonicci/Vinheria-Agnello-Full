import Icon from './Icon'
export default function Sidebar({ page, setPage, logout, ip, open, setOpen }) {
  function navigate(value) {
    setPage(value)
    setOpen(false)
  }
  return (
    <>
      {open && (
        <button
          className="menu-backdrop"
          aria-label="Fechar menu"
          onClick={() => setOpen(false)}
        />
      )}
      <aside className={`sidebar ${open ? 'open' : ''}`}>
        <a
          href="#dashboard"
          className="brand"
          onClick={() => navigate('dashboard')}
        >
          <span className="brand-icon">
            <Icon name="wine" size={29} />
          </span>
          <span>
            Vinheria
            <strong>
              Agnello<span className="brand-period">.</span>
            </strong>
          </span>
        </a>
        <span className="nav-caption">ESPAÇO DE CONTROLE</span>
        <nav aria-label="Navegação principal">
          {[
            ['dashboard', 'home', 'Dashboard'],
            ['dispositivos', 'devices', 'Dispositivos'],
          ].map(([key, icon, label]) => (
            <button
              key={key}
              className={page === key ? 'nav-item active' : 'nav-item'}
              onClick={() => navigate(key)}
              aria-current={page === key ? 'page' : undefined}
            >
              <Icon name={icon} />
              {label}
              {page === key && <span className="nav-mark" />}
            </button>
          ))}
        </nav>
        <footer>
          <div className="server-label">
            <Icon name="server" size={17} />
            <div>
              <small>Servidor conectado</small>
              <span>{ip}</span>
            </div>
          </div>
          <button
            className={`nav-item ${page === 'configuracoes' ? 'active' : ''}`}
            onClick={() => navigate('configuracoes')}
          >
            <Icon name="settings" />
            Configurações
          </button>
          <button className="nav-item" onClick={logout}>
            <Icon name="logout" />
            Sair
          </button>
          <small className="sidebar-copyright">
            VINHERIA AGNELLO · DESDE 1950
          </small>
        </footer>
      </aside>
    </>
  )
}
