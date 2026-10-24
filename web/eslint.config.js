// Define análise estática para a aplicação React, os testes Node e o launcher da API.
import js from '@eslint/js'
import globals from 'globals'
import reactHooks from 'eslint-plugin-react-hooks'
import reactRefresh from 'eslint-plugin-react-refresh'
import { defineConfig, globalIgnores } from 'eslint/config'

export default defineConfig([
  // Exclui a saída compilada e as dependências do ambiente Python.
  globalIgnores(['dist', '.venv']),
  {
    // O launcher roda no Node e utiliza seus objetos globais.
    files: ['scripts/**/*.mjs'],
    extends: [js.configs.recommended],
    languageOptions: { globals: globals.node },
  },
  { files: ['**/*.test.js'], languageOptions: { globals: globals.node } },
  {
    // As regras de hooks e recarga verificam os componentes e módulos da aplicação.
    files: ['**/*.{js,jsx}'],
    extends: [
      js.configs.recommended,
      reactHooks.configs.flat.recommended,
      reactRefresh.configs.vite,
    ],
    languageOptions: {
      globals: globals.browser,
      parserOptions: { ecmaFeatures: { jsx: true } },
    },
  },
])
