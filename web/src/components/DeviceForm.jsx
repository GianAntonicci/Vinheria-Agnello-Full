import { useState } from 'react'
import { PADRAO } from '../config/sensores'
import Icon from './Icon'
const types = ['Float', 'Integer', 'Text', 'Boolean', 'Number']
export default function DeviceForm({ device, onSubmit, busy }) {
  const [form, setForm] = useState(() =>
    structuredClone(
      device
        ? {
            ...PADRAO,
            ...Object.fromEntries(
              Object.keys(PADRAO).map((k) => [k, device[k] ?? PADRAO[k]]),
            ),
            attributes: (device.attributes || []).map(
              ({ object_id, name, type }) => ({ object_id, name, type }),
            ),
            commands: (device.commands || []).map(({ name }) => ({
              name,
              type: 'command',
            })),
          }
        : PADRAO,
    ),
  )
  const [register, setRegister] = useState(true)
  const [subscribe, setSubscribe] = useState(true)
  function update(key, value) {
    setForm((old) => ({ ...old, [key]: value }))
  }
  function row(key, index, field, value) {
    update(
      key,
      form[key].map((item, i) =>
        i === index ? { ...item, [field]: value } : item,
      ),
    )
  }
  function remove(key, index) {
    update(
      key,
      form[key].filter((_, i) => index !== i),
    )
  }
  return (
    <form
      onSubmit={(event) => {
        event.preventDefault()
        onSubmit({
          device: form,
          register_commands: register,
          subscribe_history: subscribe,
        })
      }}
      className="device-form"
    >
      <fieldset disabled={busy}>
        <div className="form-grid">
          {[
            ['device_id', 'ID do dispositivo'],
            ['entity_name', 'Nome da entidade'],
            ['entity_type', 'Tipo da entidade'],
          ].map(([key, label]) => (
            <label key={key}>
              {label}
              <input
                required
                maxLength={key === 'entity_name' ? 256 : 128}
                value={form[key]}
                onChange={(e) => update(key, e.target.value)}
                disabled={!!device}
              />
            </label>
          ))}
          <label>
            Protocolo
            <input value={form.protocol} disabled />
          </label>
          <label>
            Transporte
            <input value={form.transport} disabled />
          </label>
          <label>
            Chave do grupo de serviço
            <input value="TEF" disabled />
          </label>
        </div>
        {device && (
          <p className="muted">
            A edição altera atributos e comandos. A identidade permanece fixa.
          </p>
        )}
        <div className="form-section-heading">
          <h3>Atributos</h3>
          <button
            type="button"
            className="text-button"
            onClick={() =>
              update('attributes', [
                ...form.attributes,
                { object_id: '', name: '', type: 'Float' },
              ])
            }
          >
            <Icon name="plus" size={16} />
            Adicionar atributo
          </button>
        </div>
        <div className="dynamic-rows">
          {form.attributes.length === 0 && (
            <p className="empty-inline">
              Nenhum atributo. Adicione o primeiro.
            </p>
          )}
          {form.attributes.map((a, i) => (
            <div className="attribute-row" key={i}>
              <label>
                Object ID
                <input
                  required
                  maxLength={64}
                  value={a.object_id}
                  onChange={(e) =>
                    row('attributes', i, 'object_id', e.target.value)
                  }
                />
              </label>
              <label>
                Nome
                <input
                  required
                  pattern="[A-Za-z][A-Za-z0-9_]*"
                  value={a.name}
                  onChange={(e) => row('attributes', i, 'name', e.target.value)}
                />
              </label>
              <label>
                Tipo
                <select
                  value={a.type}
                  onChange={(e) => row('attributes', i, 'type', e.target.value)}
                >
                  {types.map((t) => (
                    <option key={t}>{t}</option>
                  ))}
                </select>
              </label>
              <button
                type="button"
                className="icon-button"
                aria-label={`Remover atributo ${a.name || i + 1}`}
                onClick={() => remove('attributes', i)}
              >
                <Icon name="trash" size={18} />
              </button>
            </div>
          ))}
        </div>
        <div className="form-section-heading">
          <h3>Comandos</h3>
          <button
            type="button"
            className="text-button"
            onClick={() =>
              update('commands', [
                ...form.commands,
                { name: '', type: 'command' },
              ])
            }
          >
            <Icon name="plus" size={16} />
            Adicionar comando
          </button>
        </div>
        <div className="command-rows">
          {form.commands.length === 0 && (
            <p className="empty-inline">Nenhum comando configurado.</p>
          )}
          {form.commands.map((c, i) => (
            <div className="command-row" key={i}>
              <label className="sr-only" htmlFor={`command-${i}`}>
                Comando {i + 1}
              </label>
              <input
                id={`command-${i}`}
                required
                pattern="[A-Za-z][A-Za-z0-9_]*"
                value={c.name}
                onChange={(e) => row('commands', i, 'name', e.target.value)}
              />
              <button
                type="button"
                className="icon-button"
                aria-label={`Remover comando ${c.name || i + 1}`}
                onClick={() => remove('commands', i)}
              >
                <Icon name="close" size={16} />
              </button>
            </div>
          ))}
        </div>
        {!device && (
          <div className="provision-options">
            <label className="checkbox-label">
              <input
                type="checkbox"
                checked={register}
                onChange={(e) => setRegister(e.target.checked)}
              />
              Criar registro dos comandos no Orion
            </label>
            <label className="checkbox-label">
              <input
                type="checkbox"
                checked={subscribe}
                onChange={(e) => setSubscribe(e.target.checked)}
              />
              Criar assinaturas do STH-Comet para atributos numéricos
            </label>
            <p className="muted">
              O grupo de serviço MQTT com chave TEF deve estar provisionado no
              IoT Agent.
            </p>
          </div>
        )}
        <button className="button primary form-submit" disabled={busy}>
          {busy
            ? 'Salvando...'
            : device
              ? 'Salvar alterações'
              : 'Cadastrar dispositivo'}
          <Icon name="check" size={18} />
        </button>
      </fieldset>
    </form>
  )
}
