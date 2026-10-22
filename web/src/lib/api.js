const API = import.meta.env.VITE_API_URL ?? ''
export async function api(path, params = {}, options = {}) {
  const query = new URLSearchParams(
    Object.entries(params).filter(([, v]) => v !== undefined),
  )
  const response = await fetch(`${API}/api/${path}?${query}`, {
    ...options,
    headers: { 'Content-Type': 'application/json', ...options.headers },
    signal: options.signal
      ? AbortSignal.any([options.signal, AbortSignal.timeout(55000)])
      : AbortSignal.timeout(55000),
  })
  const data = await response.json().catch(() => ({}))
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
export function mensagemErro(error) {
  return error.name === 'TimeoutError'
    ? 'O servidor demorou para responder. Tente novamente.'
    : error.message || 'Não foi possível acessar o servidor.'
}
