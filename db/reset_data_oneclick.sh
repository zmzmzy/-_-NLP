#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

DB_HOST="${DB_HOST:-127.0.0.1}"
DB_PORT="${DB_PORT:-3306}"
DB_USER="${DB_USER:-root}"
DB_PASS="${DB_PASS:-123456}"

MODE="defense"
ASSUME_YES=0

usage() {
  cat <<USAGE
用法:
  ./db/reset_data_oneclick.sh --mode <defense|testdata|full|empty> [--yes]

说明:
  defense  先清理答辩包，再导入答辩包，再执行答辩验收
  testdata 先清理测试包，再导入测试包，再执行测试包验收
  full     全库重建: schema + seed_full + verify_full_seed
  empty    仅清空 graduate_match 全部表数据，保留表结构

环境变量(可选):
  DB_HOST DB_PORT DB_USER DB_PASS

示例:
  DB_USER=root DB_PASS=123456 ./db/reset_data_oneclick.sh --mode defense --yes
  ./db/reset_data_oneclick.sh --mode full --yes
USAGE
}

mysql_cmd() {
  mysql -h"${DB_HOST}" -P"${DB_PORT}" -u"${DB_USER}" -p"${DB_PASS}" "$@"
}

run_sql_file() {
  local file="$1"
  if [[ ! -f "${file}" ]]; then
    echo "[reset] SQL 文件不存在: ${file}" >&2
    exit 1
  fi
  mysql_cmd < "${file}"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --mode)
      MODE="${2:-}"
      shift 2
      ;;
    --yes)
      ASSUME_YES=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "未知参数: $1" >&2
      usage
      exit 1
      ;;
  esac
done

if [[ "${MODE}" != "defense" && "${MODE}" != "testdata" && "${MODE}" != "full" && "${MODE}" != "empty" ]]; then
  echo "[reset] 非法 mode: ${MODE}" >&2
  usage
  exit 1
fi

if [[ "${ASSUME_YES}" -ne 1 ]]; then
  echo "[reset] 这是破坏性操作，目标 mode=${MODE}，连接=${DB_HOST}:${DB_PORT}" >&2
  echo "[reset] 请追加 --yes 以确认执行。" >&2
  exit 1
fi

echo "[reset] mode=${MODE} conn=${DB_HOST}:${DB_PORT}"

case "${MODE}" in
  defense)
    echo "[1/3] 清理答辩数据"
    run_sql_file "${ROOT_DIR}/db/defense_showcase_cleanup_v1.sql"

    echo "[2/3] 导入答辩数据"
    run_sql_file "${ROOT_DIR}/db/defense_showcase_seed_v1.sql"

    echo "[3/3] 验收答辩数据"
    run_sql_file "${ROOT_DIR}/db/defense_showcase_verify_v1.sql"
    ;;

  testdata)
    echo "[1/3] 清理测试数据"
    run_sql_file "${ROOT_DIR}/db/testdata_cleanup_v1.sql"

    echo "[2/3] 导入测试数据"
    run_sql_file "${ROOT_DIR}/db/testdata_seed_v1.sql"

    echo "[3/3] 验收测试数据"
    run_sql_file "${ROOT_DIR}/db/testdata_verify_v1.sql"
    ;;

  full)
    echo "[1/3] 重建 schema"
    mysql -h"${DB_HOST}" -P"${DB_PORT}" -u"${DB_USER}" -p"${DB_PASS}" < "${ROOT_DIR}/db/schema.sql"

    echo "[2/3] 导入 full seed"
    mysql -h"${DB_HOST}" -P"${DB_PORT}" -u"${DB_USER}" -p"${DB_PASS}" < "${ROOT_DIR}/db/seed_full.sql"

    echo "[3/3] 执行 full verify"
    mysql -h"${DB_HOST}" -P"${DB_PORT}" -u"${DB_USER}" -p"${DB_PASS}" < "${ROOT_DIR}/db/verify_full_seed.sql"
    ;;

  empty)
    echo "[1/1] 清空全部业务数据（保留表结构）"
    tmp_sql="$(mktemp)"
    {
      echo "USE graduate_match;"
      echo "SET FOREIGN_KEY_CHECKS=0;"
      mysql_cmd -N -B -e "SELECT CONCAT('TRUNCATE TABLE \`', table_name, '\`;')
        FROM information_schema.tables
        WHERE table_schema = 'graduate_match' AND table_type = 'BASE TABLE'
        ORDER BY table_name;"
      echo "SET FOREIGN_KEY_CHECKS=1;"
    } > "${tmp_sql}"
    mysql_cmd < "${tmp_sql}"
    rm -f "${tmp_sql}"
    ;;
esac

echo "[reset] 完成。"
