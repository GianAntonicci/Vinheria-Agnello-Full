import { useCallback, useEffect, useState } from 'react'
import { api, mensagemErro } from '../lib/api'
import DeviceTable from '../components/DeviceTable'
import DeviceForm from '../components/DeviceForm'
import Modal from '../components/Modal'
import Icon from '../components/Icon'
import { PADRAO } from '../config/sensores'
export default function Devices({ ip, device, selectDevice }) {
  const [list, setList] = useState({ devices: [], loading: true, error: '' })
  const [modal, setModal] = useState(null)
  const [busy, setBusy] = useState(false)
  const [error, setError] = useState('')
  const [notice, setNotice] = useState(null)
  const [removeEntity, setRemoveEntity] = useState(false)
  const [detail, setDetail] = useState({ loading: true })
  const load = useCallback(
    (signal) => {
      return api('dispositivos', { ip }, { signal })
        .then((data) => {
          if (!signal?.aborted)
            setList({ devices: data.devices || [], loading: false, error: '' })
        })
        .catch((error) => {
          if (!signal?.aborted)
            setList({ devices: [], loading: false, error: mensagemErro(error) })
        })
    },
    [ip],
  )
  function reload() {
    setList((old) => ({ ...old, loading: true, error: '' }))
    load()
  }
  useEffect(() => {
    const c = new AbortController()
    load(c.signal)
    return () => c.abort()
  }, [load])
  useEffect(() => {
    if (modal?.type !== 'detail') return
    const c = new AbortController(),
      d = modal.device
    api(
      'entidade',
      { ip, entity: d.entity_name, entity_type: d.entity_type },
      { signal: c.signal },
    )
      .then((data) => {
        if (!c.signal.aborted) setDetail({ data, loading: false })
      })
      .catch((err) => {
        if (!c.signal.aborted)
          setDetail({ loading: false, error: mensagemErro(err) })
      })
    return () => c.abort()
  }, [modal, ip])
  function open(type, d) {
    setModal({ type, device: d })
    setError('')
    setRemoveEntity(false)
    setDetail({ loading: true })
  }
  async function save(payload) {
    setBusy(true)
    setError('')
    try {
      const editing = modal.type === 'edit'
      const result = await api(
        editing
          ? `dispositivos/${encodeURIComponent(modal.device.device_id)}`
          : 'dispositivos',
        { ip },
        {
          method: editing ? 'PUT' : 'POST',
          body: JSON.stringify(editing ? payload.device : payload),
        },
      )
      if (editing && device.device_id === payload.device.device_id)
        selectDevice(payload.device)
      setNotice({
        warning: result.warnings?.length > 0,
        text: result.warnings?.length
          ? result.warnings.join(' ')
          : editing
            ? 'Dispositivo atualizado.'
            : 'Dispositivo cadastrado com sucesso.',
      })
      setModal(null)
      reload()
    } catch (err) {
      setError(mensagemErro(err))
    } finally {
      setBusy(false)
    }
  }
  async function remove() {
    setBusy(true)
    setError('')
    try {
      const result = await api(
        `dispositivos/${encodeURIComponent(modal.device.device_id)}`,
        { ip, remove_entity: removeEntity },
        { method: 'DELETE' },
      )
      if (modal.device.device_id === device.device_id) selectDevice(PADRAO)
      setNotice({
        warning: result.warnings?.length > 0,
        text: [result.mensagem, ...(result.warnings || [])].join(' '),
      })
      setModal(null)
      reload()
    } catch (err) {
      setError(mensagemErro(err))
    } finally {
      setBusy(false)
    }
  }
  return (
    <>
      <div className="page-subheading">
        <p>Cadastre, configure e acompanhe seus dispositivos IoT.</p>
        <button className="button primary" onClick={() => open('create')}>
          <Icon name="plus" size={18} />
          Novo dispositivo
        </button>
      </div>
      {notice && (
        <p
          className={`notice ${notice.warning ? 'warning' : 'success'}`}
          role="status"
        >
          {notice.text}
          <button
            className="icon-button"
            aria-label="Dispensar mensagem"
            onClick={() => setNotice(null)}
          >
            <Icon name="close" size={16} />
          </button>
        </p>
      )}
      <section className="card devices-card">
        <div className="section-heading">
          <div>
            <span className="eyebrow">INVENTÁRIO DA ADEGA</span>
            <h2>
              Dispositivos provisionados{' '}
              <span className="count">{list.devices.length}</span>
            </h2>
          </div>
          <button
            className="button secondary"
            disabled={list.loading}
            onClick={reload}
          >
            <Icon name="refresh" size={16} />
            Atualizar
          </button>
        </div>
        <DeviceTable
          {...list}
          selected={device}
          onSelect={(d) => {
            selectDevice(d)
            setNotice({
              text: `${d.device_id} selecionado para exibição no dashboard.`,
            })
          }}
          onDetail={(d) => open('detail', d)}
          onEdit={(d) => open('edit', d)}
          onDelete={(d) => open('delete', d)}
        />
        <p className="card-footnote">
          “Provisionado” confirma o cadastro no IoT Agent; não indica que o
          dispositivo está online.
        </p>
      </section>
      {modal && (
        <Modal
          title={
            modal.type === 'create'
              ? 'Novo dispositivo'
              : modal.type === 'edit'
                ? `Editar ${modal.device.device_id}`
                : modal.type === 'delete'
                  ? 'Excluir dispositivo'
                  : `Detalhes de ${modal.device.device_id}`
          }
          onClose={() => setModal(null)}
          busy={busy}
          wide={['create', 'edit', 'detail'].includes(modal.type)}
        >
          {error && (
            <p className="notice error" role="alert">
              {error}
            </p>
          )}
          {['create', 'edit'].includes(modal.type) ? (
            <DeviceForm device={modal.device} onSubmit={save} busy={busy} />
          ) : modal.type === 'delete' ? (
            <div>
              <p>
                Deseja excluir <strong>{modal.device.device_id}</strong> do IoT
                Agent? Esta ação não pode ser desfeita.
              </p>
              <label className="checkbox-label delete-option">
                <input
                  type="checkbox"
                  checked={removeEntity}
                  onChange={(e) => setRemoveEntity(e.target.checked)}
                  disabled={busy}
                />
                Remover também a entidade {modal.device.entity_name} do Orion
              </label>
              <p className="muted">
                O histórico, registros e assinaturas existentes não são
                excluídos por esta ação.
              </p>
              <div className="modal-actions">
                <button
                  className="button secondary"
                  onClick={() => setModal(null)}
                  disabled={busy}
                >
                  Cancelar
                </button>
                <button
                  className="button destructive"
                  onClick={remove}
                  disabled={busy}
                >
                  {busy ? 'Excluindo...' : 'Excluir dispositivo'}
                </button>
              </div>
            </div>
          ) : (
            <div>
              {detail.loading ? (
                <p className="empty-state">Carregando valores da entidade...</p>
              ) : detail.error ? (
                <p className="notice error" role="alert">
                  {detail.error}
                </p>
              ) : (
                <>
                  <p className="muted">
                    {modal.device.entity_name} · {modal.device.entity_type}
                  </p>
                  <div className="table-scroll">
                    <table>
                      <thead>
                        <tr>
                          <th>Atributo</th>
                          <th>Tipo</th>
                          <th>Valor atual</th>
                        </tr>
                      </thead>
                      <tbody>
                        {Object.entries(detail.data || {})
                          .filter(
                            ([, value]) => value && typeof value === 'object',
                          )
                          .map(([key, value]) => (
                            <tr key={key}>
                              <td>{key}</td>
                              <td>{value.type || '—'}</td>
                              <td className="detail-value">
                                {typeof value.value === 'object'
                                  ? JSON.stringify(value.value)
                                  : String(value.value ?? '—')}
                              </td>
                            </tr>
                          ))}
                      </tbody>
                    </table>
                    {Object.keys(detail.data || {}).length <= 2 && (
                      <p className="empty-state">
                        A entidade ainda não possui atributos.
                      </p>
                    )}
                  </div>
                </>
              )}
            </div>
          )}
        </Modal>
      )}
    </>
  )
}
