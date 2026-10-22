import { useCallback, useEffect, useRef, useState } from 'react'
import { mensagemErro } from '../lib/api'

export default function usePolling(fetcher, interval) {
  const [result, setResult] = useState({
    data: null,
    previous: null,
    error: '',
    loading: true,
  })
  const reload = useRef(() => {})
  useEffect(() => {
    let active = true,
      timer,
      busy = false
    const controller = new AbortController()
    async function run() {
      if (busy || !active) return
      busy = true
      clearTimeout(timer)
      try {
        const data = await fetcher(controller.signal)
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
      } finally {
        busy = false
        if (active) timer = setTimeout(run, interval)
      }
    }
    reload.current = run
    run()
    return () => {
      active = false
      controller.abort()
      clearTimeout(timer)
    }
  }, [fetcher, interval])
  const refresh = useCallback(() => reload.current(), [])
  return result.fetcher === fetcher
    ? { ...result, refresh }
    : { data: null, previous: null, error: '', loading: true, refresh }
}
