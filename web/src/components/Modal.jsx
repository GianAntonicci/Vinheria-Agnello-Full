// Janela baseada em dialog nativo, com controle de foco e bloqueio do fechamento durante operações.
import { useEffect, useRef } from 'react'
import Icon from './Icon'
export default function Modal({
  title,
  onClose,
  children,
  wide = false,
  busy = false,
}) {
  const ref = useRef(null)
  useEffect(() => {
    const dialog = ref.current
    // Guarda o elemento de origem para devolver o foco quando o modal for desmontado.
    const previous = document.activeElement
    dialog.showModal()
    return () => {
      dialog.close()
      previous?.focus()
    }
  }, [])
  return (
    <dialog
      ref={ref}
      className={`modal ${wide ? 'wide' : ''}`}
      onCancel={(e) => {
        if (busy) e.preventDefault()
        else onClose()
      }}
      aria-labelledby="modal-title"
    >
      <header>
        <h2 id="modal-title">{title}</h2>
        <button
          className="icon-button"
          aria-label="Fechar"
          onClick={onClose}
          disabled={busy}
        >
          <Icon name="close" />
        </button>
      </header>
      {children}
    </dialog>
  )
}
