import Icon from './Icon'
import StatusBadge from './StatusBadge'
export default function DeviceTable({
  devices,
  loading,
  error,
  selected,
  onSelect,
  onEdit,
  onDelete,
  onDetail,
}) {
  if (loading)
    return <div className="empty-state">Carregando dispositivos...</div>
  if (error)
    return (
      <div className="empty-state">
        <Icon name="alert" size={30} />
        <h3>Dispositivos indisponíveis</h3>
        <p>{error}</p>
      </div>
    )
  if (!devices.length)
    return (
      <div className="empty-state">
        <Icon name="devices" size={32} />
        <h3>Nenhum dispositivo cadastrado</h3>
        <p>Cadastre o primeiro dispositivo para começar o monitoramento.</p>
      </div>
    )
  return (
    <div className="table-scroll">
      <table className="device-table">
        <thead>
          <tr>
            <th>Dispositivo / Entidade</th>
            <th>Tipo</th>
            <th>Protocolo / Transporte</th>
            <th>Atributos</th>
            <th>Comandos</th>
            <th>Status</th>
            <th>Ações</th>
          </tr>
        </thead>
        <tbody>
          {devices.map((d) => (
            <tr key={d.device_id}>
              <td>
                <strong>{d.device_id}</strong>
                <small className="entity-id">{d.entity_name}</small>
              </td>
              <td>{d.entity_type}</td>
              <td>
                <span>{d.protocol || '—'}</span>
                <small className="entity-id">{d.transport || '—'}</small>
              </td>
              <td>{d.attributes?.length ?? 0}</td>
              <td>{d.commands?.length ?? 0}</td>
              <td>
                <StatusBadge status="normal" label="Provisionado" />
              </td>
              <td>
                <div className="table-actions">
                  <button
                    className={`button tiny ${selected.device_id === d.device_id ? 'secondary' : 'primary'}`}
                    onClick={() => onSelect(d)}
                    disabled={selected.device_id === d.device_id}
                  >
                    {selected.device_id === d.device_id
                      ? 'Em exibição'
                      : 'Exibir'}
                  </button>
                  <button
                    className="icon-button"
                    title="Ver detalhes"
                    aria-label={`Ver detalhes de ${d.device_id}`}
                    onClick={() => onDetail(d)}
                  >
                    <Icon name="eye" size={17} />
                  </button>
                  <button
                    className="icon-button"
                    title="Editar"
                    aria-label={`Editar ${d.device_id}`}
                    onClick={() => onEdit(d)}
                  >
                    <Icon name="edit" size={17} />
                  </button>
                  <button
                    className="icon-button danger"
                    title="Excluir"
                    aria-label={`Excluir ${d.device_id}`}
                    onClick={() => onDelete(d)}
                  >
                    <Icon name="trash" size={17} />
                  </button>
                </div>
              </td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  )
}
