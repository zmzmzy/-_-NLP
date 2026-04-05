#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
STACK_DIR="${ROOT_DIR}/deploy/litellm"
AI_ENV_FILE="${AI_ENV_FILE:-${ROOT_DIR}/backend/.env.ai}"

log() {
  echo "[start_litellm] $*"
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

is_placeholder() {
  local value
  value="$(echo -n "${1:-}" | tr '[:upper:]' '[:lower:]')"
  [[ -z "${value}" || "${value}" == replace_with* || "${value}" == changeme* || "${value}" == your_* ]]
}

upsert_env_file() {
  local file="$1"
  local key="$2"
  local value="$3"
  local escaped
  escaped="${value//\\/\\\\}"
  escaped="${escaped//&/\\&}"
  escaped="${escaped//|/\\|}"
  if rg -n "^${key}=" "${file}" >/dev/null 2>&1; then
    sed -i "s|^${key}=.*$|${key}=${escaped}|" "${file}"
  else
    printf '%s=%s\n' "${key}" "${value}" >>"${file}"
  fi
}

read_env_line_value() {
  local file="$1"
  local key="$2"
  if [[ ! -f "${file}" ]]; then
    return 0
  fi
  local line
  line="$(grep -E "^${key}=" "${file}" | tail -n 1 || true)"
  if [[ -z "${line}" ]]; then
    return 0
  fi
  echo "${line#*=}"
}

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing command: $1" >&2
    exit 1
  fi
}

require_cmd docker
require_cmd curl
if ! docker compose version >/dev/null 2>&1; then
  echo "Docker Compose plugin is required (docker compose ...)" >&2
  exit 1
fi

ensure_local_no_proxy

if [[ ! -d "${STACK_DIR}" ]]; then
  echo "LiteLLM stack directory not found: ${STACK_DIR}" >&2
  exit 1
fi

if [[ ! -f "${STACK_DIR}/.env" ]]; then
  if [[ -f "${STACK_DIR}/.env.example" ]]; then
    cp "${STACK_DIR}/.env.example" "${STACK_DIR}/.env"
    log "Created ${STACK_DIR}/.env from example. Please fill in real API keys."
  else
    echo "Missing ${STACK_DIR}/.env and .env.example" >&2
    exit 1
  fi
fi

if [[ -f "${AI_ENV_FILE}" ]]; then
  log "Loading AI env from ${AI_ENV_FILE}"
  # shellcheck disable=SC1090
  set -a
  source "${AI_ENV_FILE}"
  set +a
fi

for key in LITELLM_MASTER_KEY SILICONFLOW_API_KEY OPENAI_API_KEY OPENROUTER_API_KEY GEMINI_API_KEY ANTHROPIC_API_KEY; do
  value="${!key:-}"
  if [[ -n "${value}" ]] && ! is_placeholder "${value}"; then
    upsert_env_file "${STACK_DIR}/.env" "${key}" "${value}"
  fi
done

MASTER_KEY_VALUE="$(read_env_line_value "${STACK_DIR}/.env" "LITELLM_MASTER_KEY")"
SILICON_KEY_VALUE="$(read_env_line_value "${STACK_DIR}/.env" "SILICONFLOW_API_KEY")"
if is_placeholder "${MASTER_KEY_VALUE}"; then
  log "Warning: LITELLM_MASTER_KEY is empty/placeholder."
fi
if is_placeholder "${SILICON_KEY_VALUE}"; then
  log "Warning: SILICONFLOW_API_KEY is empty/placeholder, dashboard-default may fail."
fi

if [[ ! -f "${STACK_DIR}/config.yaml" ]]; then
  if [[ -f "${STACK_DIR}/config.yaml.example" ]]; then
    cp "${STACK_DIR}/config.yaml.example" "${STACK_DIR}/config.yaml"
    log "Created ${STACK_DIR}/config.yaml from example."
  else
    echo "Missing ${STACK_DIR}/config.yaml and config.yaml.example" >&2
    exit 1
  fi
fi

log "Starting LiteLLM gateway via Docker Compose"
(
  cd "${STACK_DIR}"
  docker compose up -d
)

PORT="$(awk -F= '/^LITELLM_PORT=/{print $2}' "${STACK_DIR}/.env" | tail -n 1)"
if [[ -z "${PORT}" ]]; then
  PORT="4000"
fi
WAIT_SECONDS="${LITELLM_WAIT_SECONDS:-120}"
if ! [[ "${WAIT_SECONDS}" =~ ^[0-9]+$ ]]; then
  WAIT_SECONDS=120
fi

log "Waiting for gateway on http://127.0.0.1:${PORT}/"
for ((i=1; i<=WAIT_SECONDS; i++)); do
  if curl --noproxy '*' -sS "http://127.0.0.1:${PORT}/" >/dev/null 2>&1; then
    log "LiteLLM is healthy."
    log "Gateway base URL: http://127.0.0.1:${PORT}"
    if ! is_placeholder "${MASTER_KEY_VALUE}"; then
      status_code="$(curl --noproxy '*' -sS -o /dev/null -w '%{http_code}' \
        -H "Authorization: Bearer ${MASTER_KEY_VALUE}" \
        "http://127.0.0.1:${PORT}/v1/models" || true)"
      if [[ "${status_code}" != "200" ]]; then
        log "Warning: gateway auth check (/v1/models) status=${status_code}"
      fi
    fi
    exit 0
  fi
  sleep 1
done

log "Gateway started but root endpoint did not respond within ${WAIT_SECONDS}s. Check logs:"
log "docker compose -f ${STACK_DIR}/docker-compose.yml logs -f"
exit 1
