// Adapta o polling ao ciclo de vida do React, expondo dados, comparação anterior, erro e atualização manual.
import { useCallback, useEffect, useRef, useState } from 'react'
import { mensagemErro } from '../lib/api'
import { criarPolling } from '../lib/polling'

export default function usePolling(fetcher, interval) {
  const [result, setResult] = useState({
    data: null,
    previous: null,
    error: '',
    loading: true,
  })
  // A referência permite que refresh mantenha a mesma identidade mesmo após recriar o polling.
  const reload = useRef(() => {})
  // Recria as consultas quando a origem ou o intervalo muda e cancela a instância anterior.
  useEffect(() => {
    let active = true
    const polling = criarPolling(async (signal) => {
      try {
        const data = await fetcher(signal)
        if (active)
          setResult((old) => ({
            data,
            // Só compara leituras da mesma origem, evitando misturar dispositivos.
            previous: old.fetcher === fetcher ? old.data : null,
            error: '',
            loading: false,
            fetcher,
          }))
      } catch (error) {
        if (active)
          setResult((old) => ({
            data: old.fetcher === fetcher ? old.data : null,
            previous: old.previous,
            error: mensagemErro(error),
            loading: false,
            fetcher,
          }))
      }
    }, interval)
    reload.current = polling.atualizar
    polling.atualizar()
    // Impede respostas tardias de alterarem estado depois de desmontar ou trocar a consulta.
    return () => {
      active = false
      polling.encerrar()
    }
  }, [fetcher, interval])
  const refresh = useCallback(() => reload.current(), [])
  // Durante a troca de origem, oculta dados da consulta anterior até chegar a nova resposta.
  return result.fetcher === fetcher
    ? { ...result, refresh }
    : { data: null, previous: null, error: '', loading: true, refresh }
}
