#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

MODE="${1:-dry-run}"
TAG="${TAG:-seed_students_v1}"
YEAR_PREFIX="${YEAR_PREFIX:-222022}"
GRAD_YEAR="${GRAD_YEAR:-2026}"
DIST="${DIST:-cycle}"
MIN_PER_MAJOR="${MIN_PER_MAJOR:-6}"
MAX_PER_MAJOR="${MAX_PER_MAJOR:-12}"
FIXED_PER_MAJOR="${FIXED_PER_MAJOR:-8}"
MAJOR_SEGMENT_MODE="${MAJOR_SEGMENT_MODE:-tail3}"
MAJOR_SEGMENT_MAP_CSV="${MAJOR_SEGMENT_MAP_CSV:-}"

CMD=(
  "${ROOT_DIR}/tools/student_seed/import_students_seed.py"
  "--db-host" "${DB_HOST:-127.0.0.1}"
  "--db-port" "${DB_PORT:-3306}"
  "--db-user" "${DB_USER:-root}"
  "--db-pass" "${DB_PASS:-123456}"
  "--db-name" "${DB_NAME:-graduate_match}"
  "--year-prefix" "${YEAR_PREFIX}"
  "--graduation-year" "${GRAD_YEAR}"
  "--distribution" "${DIST}"
  "--min-per-major" "${MIN_PER_MAJOR}"
  "--max-per-major" "${MAX_PER_MAJOR}"
  "--fixed-per-major" "${FIXED_PER_MAJOR}"
  "--major-segment-mode" "${MAJOR_SEGMENT_MODE}"
  "--output-dir" "${ROOT_DIR}/import_samples"
  "--tag" "${TAG}"
)

if [[ -n "${MAJOR_SEGMENT_MAP_CSV}" ]]; then
  CMD+=("--major-segment-map-csv" "${MAJOR_SEGMENT_MAP_CSV}")
fi

case "${MODE}" in
  dry-run)
    ;;
  apply)
    CMD+=("--apply")
    ;;
  *)
    echo "Usage: $0 [dry-run|apply]" >&2
    exit 1
    ;;
esac

"${CMD[@]}"
