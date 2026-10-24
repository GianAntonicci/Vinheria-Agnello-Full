// Cliente HTTP comum às páginas: serializa parâmetros e traduz falhas da API.
const API = import.meta.env.VITE_API_URL ?? ''
// Aceita opções de fetch, inclusive método, corpo e sinal de cancelamento.
export async function api(path, params = {}, options = {}) {
  const query = new URLSearchParams(
    Object.entries(params).filter(([, v]) => v !== undefined),
  )
  const response = await fetch(`${API}/api/${path}?${query}`, {
    ...options,
    headers: { 'Content-Type': 'application/json', ...options.headers },
    // Combina o cancelamento do chamador com o prazo máximo de 55 segundos.
    signal: options.signal
      ? AbortSignal.any([options.signal, AbortSignal.timeout(55000)])
      : AbortSignal.timeout(55000),
  })
  const data = await response.json().catch(() => ({}))
  // Mensagens de validação estruturadas recebem uma descrição legível para o formulário.
  if (!response.ok) {
    const detail = data.detail
    throw new Error(
      typeof detail === 'string'
        ? detail
        : Array.isArray(detail)
          ? 'Dados inválidos. Revise os campos e evite nomes repetidos.'
          : `Não foi possível concluir a solicitação (${response.status}).`,
    )
  }
  return data
}
// Distingue expiração do prazo de outros erros de acesso ao servidor.
export function mensagemErro(error) {
  return error.name === 'TimeoutError'
    ? 'O servidor demorou para responder. Tente novamente.'
    : error.message || 'Não foi possível acessar o servidor.'
}
