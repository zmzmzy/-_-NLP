#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BACKEND_DIR="${ROOT_DIR}/backend"
BACKEND_BUILD_DIR="${BACKEND_DIR}/build"

log() {
  echo "[run_local_tests] $*"
}

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing command: $1" >&2
    exit 1
  fi
}

require_cmd cmake
require_cmd mysql
require_cmd python3

log "Rebuild backend"
cmake -S "${BACKEND_DIR}" -B "${BACKEND_BUILD_DIR}"
cmake --build "${BACKEND_BUILD_DIR}" -j

log "Reset schema + seed data"
mysql -uroot -p123456 < "${ROOT_DIR}/db/schema.sql"
mysql -uroot -p123456 < "${ROOT_DIR}/db/seed_full.sql"

log "Run database verification report (baseline)"
mysql -uroot -p123456 < "${ROOT_DIR}/db/verify_full_seed.sql"

log "Start backend service"
(
  cd "${BACKEND_BUILD_DIR}"
  ./graduate_match_backend
) >/tmp/gm_test_backend.log 2>&1 &
BACKEND_PID=$!

cleanup() {
  if kill -0 "${BACKEND_PID}" >/dev/null 2>&1; then
    log "Stopping backend (pid=${BACKEND_PID})"
    kill "${BACKEND_PID}" >/dev/null 2>&1 || true
    wait "${BACKEND_PID}" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

log "Wait for health endpoint"
for i in $(seq 1 30); do
  if curl --noproxy '*' -sS "http://127.0.0.1:5555/api/health" >/dev/null 2>&1; then
    break
  fi
  sleep 1
done

if ! curl --noproxy '*' -sS "http://127.0.0.1:5555/api/health" >/dev/null 2>&1; then
  echo "Backend health check failed. Last logs:" >&2
  tail -n 200 /tmp/gm_test_backend.log >&2 || true
  exit 1
fi

log "Run integration smoke test"
python3 "${ROOT_DIR}/tools/testing/api_integration_smoke.py"

log "Run negative-path smoke test"
python3 "${ROOT_DIR}/tools/testing/api_negative_smoke.py"

log "All local tests passed"
