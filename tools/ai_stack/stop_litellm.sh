#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
STACK_DIR="${ROOT_DIR}/deploy/litellm"

if ! docker compose version >/dev/null 2>&1; then
  echo "Docker Compose plugin is required (docker compose ...)" >&2
  exit 1
fi

if [[ ! -d "${STACK_DIR}" ]]; then
  echo "LiteLLM stack directory not found: ${STACK_DIR}" >&2
  exit 1
fi

(
  cd "${STACK_DIR}"
  docker compose down
)

echo "[stop_litellm] LiteLLM stack stopped."
