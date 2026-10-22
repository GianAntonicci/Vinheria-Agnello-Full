// Serializa as consultas e preserva atualizações solicitadas durante uma leitura.
export function criarPolling(
  consultar,
  intervalo,
  agendar = setTimeout,
  cancelar = clearTimeout,
) {
  let ativo = true
  let ocupado = false
  let pendente = false
  let timer
  const controller = new AbortController()

  async function atualizar() {
    if (!ativo) return
    if (ocupado) {
      pendente = true
      return
    }
    ocupado = true
    cancelar(timer)
    try {
      await consultar(controller.signal)
    } finally {
      ocupado = false
      if (ativo) {
        timer = agendar(atualizar, pendente ? 0 : intervalo)
        pendente = false
      }
    }
  }

  function encerrar() {
    ativo = false
    controller.abort()
    cancelar(timer)
  }

  return { atualizar, encerrar }
}
