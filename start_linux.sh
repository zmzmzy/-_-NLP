#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BACKEND_DIR="${ROOT_DIR}/backend"
FRONTEND_DIR="${ROOT_DIR}/frontend"
BACKEND_BUILD_DIR="${BACKEND_DIR}/build"
FRONTEND_PORT="${FRONTEND_PORT:-5173}"
FRONTEND_HOST="${FRONTEND_HOST:-0.0.0.0}"
FRONTEND_USE_HTTPS="${FRONTEND_USE_HTTPS:-1}"
SMTP_ENV_FILE="${SMTP_ENV_FILE:-${BACKEND_DIR}/.env.smtp}"
AI_ENV_FILE="${AI_ENV_FILE:-${BACKEND_DIR}/.env.ai}"
START_LITELLM_GATEWAY="${START_LITELLM_GATEWAY:-0}"

log() {
  echo "[start_linux] $*"
}

ensure_local_no_proxy() {
  local current="${NO_PROXY:-${no_proxy:-}}"
  local required=("127.0.0.1" "localhost" "::1" "0.0.0.0")
  local host
  for host in "${required[@]}"; do
    if [[ ",${current}," != *",${host},"* ]]; then
      current="${current:+${current},}${host}"
    fi
  done
  export NO_PROXY="${current}"
  export no_proxy="${current}"
}

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing command: $1" >&2
    exit 1
  fi
}

if [[ ! -d "${BACKEND_DIR}" || ! -d "${FRONTEND_DIR}" ]]; then
  echo "Invalid project structure. Expecting ./backend and ./frontend under ${ROOT_DIR}" >&2
  exit 1
fi

require_cmd cmake
require_cmd npm

if [[ -f "${SMTP_ENV_FILE}" ]]; then
  log "Loading SMTP env from ${SMTP_ENV_FILE}"
  # shellcheck disable=SC1090
  set -a
  source "${SMTP_ENV_FILE}"
  set +a
fi

if [[ -f "${AI_ENV_FILE}" ]]; then
  log "Loading AI env from ${AI_ENV_FILE}"
  # shellcheck disable=SC1090
  set -a
  source "${AI_ENV_FILE}"
  set +a
fi

ensure_local_no_proxy
log "NO_PROXY prepared for local services"

if [[ -n "${GM_SMTP_HOST:-}" ]]; then
  require_cmd curl
fi

if [[ "${START_LITELLM_GATEWAY}" == "1" || "${START_LITELLM_GATEWAY}" == "true" ]]; then
  if [[ -x "${ROOT_DIR}/tools/ai_stack/start_litellm.sh" ]]; then
    log "Starting LiteLLM gateway before backend"
    "${ROOT_DIR}/tools/ai_stack/start_litellm.sh"
  else
    echo "START_LITELLM_GATEWAY is enabled but tools/ai_stack/start_litellm.sh not found" >&2
    exit 1
  fi
fi

log "Configuring backend"
cmake -S "${BACKEND_DIR}" -B "${BACKEND_BUILD_DIR}"

log "Building backend"
cmake --build "${BACKEND_BUILD_DIR}" -j

if [[ "${FORCE_NPM_INSTALL:-0}" == "1" || ! -d "${FRONTEND_DIR}/node_modules" ]]; then
  if [[ -f "${FRONTEND_DIR}/package-lock.json" ]]; then
    log "Installing frontend dependencies with npm ci"
    npm --prefix "${FRONTEND_DIR}" ci
  else
    log "Installing frontend dependencies with npm install"
    npm --prefix "${FRONTEND_DIR}" install
  fi
else
  log "Skip npm install (node_modules exists). Set FORCE_NPM_INSTALL=1 to force reinstall."
fi

BACKEND_CMD="./graduate_match_backend"
if [[ -x "${BACKEND_BUILD_DIR}/graduate_match_backend" ]]; then
  BACKEND_CMD="./graduate_match_backend"
elif [[ -x "${BACKEND_BUILD_DIR}/Release/graduate_match_backend" ]]; then
  BACKEND_CMD="./Release/graduate_match_backend"
elif [[ -x "${BACKEND_BUILD_DIR}/Debug/graduate_match_backend" ]]; then
  BACKEND_CMD="./Debug/graduate_match_backend"
else
  echo "Backend binary not found under ${BACKEND_BUILD_DIR}" >&2
  exit 1
fi

log "Starting backend (${BACKEND_CMD})"
(
  cd "${BACKEND_BUILD_DIR}"
  ${BACKEND_CMD}
) &
BACKEND_PID=$!

cleanup() {
  if kill -0 "${BACKEND_PID}" >/dev/null 2>&1; then
    log "Stopping backend (pid=${BACKEND_PID})"
    kill "${BACKEND_PID}" >/dev/null 2>&1 || true
    wait "${BACKEND_PID}" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

FRONTEND_SCHEME="https"
FRONTEND_SCRIPT="dev"
if [[ "${FRONTEND_USE_HTTPS}" == "0" || "${FRONTEND_USE_HTTPS}" == "false" ]]; then
  FRONTEND_SCHEME="http"
  FRONTEND_SCRIPT="dev:http"
fi

log "Backend started. Health check: http://127.0.0.1:5555/api/health"
log "Starting frontend on ${FRONTEND_SCHEME}://${FRONTEND_HOST}:${FRONTEND_PORT} (npm script: ${FRONTEND_SCRIPT})"
npm --prefix "${FRONTEND_DIR}" run "${FRONTEND_SCRIPT}" -- --host "${FRONTEND_HOST}" --port "${FRONTEND_PORT}"
