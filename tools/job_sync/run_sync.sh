#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
SOURCE_KEY="${1:-}"
LIMIT="${2:-0}"

if [[ -z "$SOURCE_KEY" ]]; then
  echo "usage: $0 <source_key> [limit]" >&2
  exit 1
fi

cd "$ROOT_DIR"
if [[ "$LIMIT" =~ ^[0-9]+$ ]] && [[ "$LIMIT" -gt 0 ]]; then
  python3 job_sync.py --source-key "$SOURCE_KEY" --limit "$LIMIT"
else
  python3 job_sync.py --source-key "$SOURCE_KEY"
fi
