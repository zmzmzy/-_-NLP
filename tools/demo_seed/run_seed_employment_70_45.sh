#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${ROOT_DIR}"

MODE="${1:-dry-run}"

DB_HOST="${DB_HOST:-127.0.0.1}"
DB_PORT="${DB_PORT:-3306}"
DB_USER="${DB_USER:-root}"
DB_PASS="${DB_PASS:-123456}"
DB_NAME="${DB_NAME:-graduate_match}"

python3 tools/demo_seed/seed_employment_70_45.py "${MODE}" \
  --db-host "${DB_HOST}" \
  --db-port "${DB_PORT}" \
  --db-user "${DB_USER}" \
  --db-pass "${DB_PASS}" \
  --db-name "${DB_NAME}"
