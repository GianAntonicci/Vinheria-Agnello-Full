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
  const reload = useRef(() => {})
  useEffect(() => {
    let active = true
    const polling = criarPolling(async (signal) => {
      try {
        const data = await fetcher(signal)
        if (active)
          setResult((old) => ({
            data,
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
    return () => {
      active = false
      polling.encerrar()
    }
  }, [fetcher, interval])
  const refresh = useCallback(() => reload.current(), [])
  return result.fetcher === fetcher
    ? { ...result, refresh }
    : { data: null, previous: null, error: '', loading: true, refresh }
}
