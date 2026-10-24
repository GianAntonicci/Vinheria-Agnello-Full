// Controla consultas periódicas sem sobreposição e permite cancelá-las ao sair da tela.
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

  // Uma atualização pedida durante a consulta fica pendente para a próxima execução.
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
        // Conta o intervalo após concluir a consulta; pedidos pendentes são executados logo em seguida.
        timer = agendar(atualizar, pendente ? 0 : intervalo)
        pendente = false
      }
    }
  }

  // Interrompe a requisição em curso e impede novos agendamentos.
  function encerrar() {
    ativo = false
    controller.abort()
    cancelar(timer)
  }

  return { atualizar, encerrar }
}
