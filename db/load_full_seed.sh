#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DB_USER="${DB_USER:-root}"
DB_PASS="${DB_PASS:-123456}"
DB_HOST="${DB_HOST:-127.0.0.1}"
DB_PORT="${DB_PORT:-3306}"

mysql_cmd() {
  mysql -h"${DB_HOST}" -P"${DB_PORT}" -u"${DB_USER}" -p"${DB_PASS}" "$@"
}

echo "[1/3] 执行 schema.sql"
mysql_cmd < "${ROOT_DIR}/db/schema.sql"

echo "[2/3] 执行 seed_full.sql"
mysql_cmd < "${ROOT_DIR}/db/seed_full.sql"

echo "[3/3] 执行 verify_full_seed.sql"
mysql_cmd < "${ROOT_DIR}/db/verify_full_seed.sql"

echo "完成：完整样本数据已导入并完成验收查询。"
