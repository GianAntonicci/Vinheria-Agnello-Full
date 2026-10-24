// Configura a compilação React/Tailwind e o encaminhamento da API no servidor de desenvolvimento.
import react from '@vitejs/plugin-react'
import tailwindcss from '@tailwindcss/vite'
import { defineConfig } from 'vite'

// https://vite.dev/config/
export default defineConfig({
  // Durante npm run dev, mantém as chamadas /api na mesma origem vista pelo navegador.
  server: { proxy: { '/api': 'http://127.0.0.1:8000' } },
  plugins: [react(), tailwindcss()],
})
