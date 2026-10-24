// Prepara o ambiente Python local e inicia a API FastAPI com recarga durante o desenvolvimento.
import { spawn, spawnSync } from 'node:child_process'
import { createHash } from 'node:crypto'
import { existsSync } from 'node:fs'
import { readFile, writeFile } from 'node:fs/promises'
import net from 'node:net'
import path from 'node:path'
import process from 'node:process'
import { fileURLToPath } from 'node:url'

const root = fileURLToPath(new URL('../', import.meta.url))
const windows = process.platform === 'win32'
const venv = path.join(root, '.venv')
// Ajusta o executável do ambiente virtual para Windows ou sistemas POSIX.
const python = path.join(venv, windows ? 'Scripts' : 'bin', windows ? 'python.exe' : 'python')
const marker = path.join(venv, '.requirements-dev.sha256')
const versionCheck = 'import sys; print(sys.version_info[:2])'
const installHelp = 'Instale Python 3.12 ou superior em https://www.python.org/downloads/ (no Windows, marque "Add python.exe to PATH") ou, no macOS, use brew install python@3.12. Garanta que python3 ou python esteja no PATH e tente novamente.'
let child
let stoppingSignal

// O Windows não oferece sinais POSIX ao processo filho. Encerre também o
// worker do reloader para não deixar a porta ocupada após fechar o launcher.
for (const signal of ['SIGINT', 'SIGTERM']) {
  process.on(signal, () => {
    if (stoppingSignal) return
    stoppingSignal = signal
    if (!child?.pid) return
    if (windows) {
      const result = spawnSync('taskkill', ['/pid', String(child.pid), '/T', '/F'], {
        stdio: 'ignore',
        shell: false,
      })
      if (result.error || result.status !== 0) child.kill(signal)
    } else {
      child.kill(signal)
    }
  })
}

// Propaga a interrupção para evitar continuar a preparação após um sinal de encerramento.
function checkStopped() {
  if (stoppingSignal) {
    const error = new Error('Inicialização interrompida.')
    error.exitCode = stoppingSignal === 'SIGINT' ? 130 : 143
    throw error
  }
}

// Consulta a versão sem shell e aceita apenas Python 3.12 ou superior da série 3.
function supportsPython(command, args = []) {
  const result = spawnSync(command, [...args, '-c', versionCheck], {
    cwd: root,
    encoding: 'utf8',
    timeout: 10000,
    windowsHide: true,
    shell: false,
  })
  const version = result.stdout?.trim().match(/^\((\d+),\s*(\d+)\)$/)
  return result.status === 0 && version && Number(version[1]) === 3 && Number(version[2]) >= 12
}

// Acompanha o processo filho e converte falhas ou sinais em códigos de saída do launcher.
async function run(command, args) {
  checkStopped()
  await new Promise((resolve, reject) => {
    child = spawn(command, args, { cwd: root, stdio: 'inherit', shell: false })
    child.once('error', (cause) => {
      child = undefined
      reject(new Error(`Não foi possível executar ${command}: ${cause.message}`))
    })
    child.once('close', (code, signal) => {
      child = undefined
      if (code === 0 && !stoppingSignal) return resolve()
      const error = new Error(`Processo encerrado (${signal ?? stoppingSignal ?? `código ${code}`}).`)
      error.exitCode = stoppingSignal === 'SIGINT' || signal === 'SIGINT' ? 130
        : stoppingSignal === 'SIGTERM' || signal === 'SIGTERM' ? 143 : (code ?? 1)
      reject(error)
    })
  })
}

// Testa a disponibilidade da porta antes da preparação e novamente antes de iniciar o servidor.
async function checkPort() {
  checkStopped()
  await new Promise((resolve, reject) => {
    const server = net.createServer()
    server.once('error', (cause) => reject(new Error(cause.code === 'EADDRINUSE'
      ? 'A porta 8000 já está em uso por outro processo. Encerre esse processo e execute npm run dev novamente.'
      : `Não foi possível verificar a porta 8000: ${cause.message}`)))
    server.listen({ host: '127.0.0.1', port: 8000, exclusive: true }, () => server.close(resolve))
  })
}

// O modo --setup apenas prepara as dependências; o modo padrão também inicia o Uvicorn.
async function main() {
  const args = process.argv.slice(2)
  if (args.some((arg) => arg !== '--setup')) throw new Error('Uso: node scripts/dev-api.mjs [--setup]')
  const setupOnly = args.includes('--setup')
  if (!setupOnly) await checkPort()

  // Cria um ambiente virtual apenas se ele ainda não existir e valida o interpretador resultante.
  let created = false
  if (!existsSync(venv)) {
    const candidates = [
      ...(windows ? [['py', ['-3.12']], ['py', ['-3']]] : []),
      ['python3', []],
      ['python', []],
    ]
    const candidate = candidates.find(([command, flags]) => supportsPython(command, flags))
    if (!candidate) throw new Error(`Python 3.12 ou superior não encontrado. ${installHelp}`)
    console.log('Criando ambiente Python em web/.venv...')
    await run(candidate[0], [...candidate[1], '-m', 'venv', venv])
    created = true
  }
  if (!supportsPython(python)) {
    throw new Error(`O ambiente web/.venv está incompleto ou não usa Python 3.12+. Remova ou renomeie esse ambiente e tente novamente. ${installHelp}`)
  }

  // O hash dos dois arquivos de requisitos evita reinstalar dependências que não mudaram.
  const hash = createHash('sha256')
  for (const filename of ['requirements.txt', 'requirements-dev.txt']) {
    hash.update(filename).update('\0').update(await readFile(path.join(root, filename))).update('\0')
  }
  const digest = hash.digest('hex')
  const installed = await readFile(marker, 'utf8').catch((error) => {
    if (error.code !== 'ENOENT') throw error
    return ''
  })
  if (created || installed !== digest) {
    console.log('Instalando dependências da API...')
    await run(python, ['-m', 'pip', 'install', '-r', 'requirements-dev.txt'])
    checkStopped()
    await writeFile(marker, digest, 'utf8')
  } else {
    console.log('Dependências da API já estão atualizadas.')
  }

  if (setupOnly) {
    console.log('Ambiente da API pronto em web/.venv.')
    return
  }
  await checkPort()
  // Restringe a API ao computador local; o Vite encaminha as chamadas /api para esta porta.
  await run(python, ['-m', 'uvicorn', 'src.api.app:app', '--reload', '--reload-dir', 'src/api',
    '--host', '127.0.0.1', '--port', '8000'])
}

main().catch((error) => {
  console.error(`API: ${error.message}`)
  process.exitCode = error.exitCode ?? 1
})
