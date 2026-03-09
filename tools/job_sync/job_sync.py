#!/usr/bin/env python3
"""Normalize cached job JSON data into a stable output schema.

This script is intentionally cache-first for v0.2 MVP. It does not perform
network crawling yet; it consumes existing cache files and writes normalized
JSON for later DB upsert.
"""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
from pathlib import Path
from typing import Any, Dict, List, Optional, Sequence, Tuple


JOB_HINT_KEYS = {
    "jobid",
    "positionid",
    "title",
    "jobname",
    "positionname",
    "companyname",
    "salary",
    "salarydesc",
    "jobdesc",
    "city",
    "publishtime",
}


def utc_now() -> dt.datetime:
    return dt.datetime.now(dt.timezone.utc)


def utc_stamp() -> str:
    return utc_now().strftime("%Y%m%dT%H%M%SZ")


def as_text(value: Any) -> str:
    if value is None:
        return ""
    if isinstance(value, str):
        return value.strip()
    if isinstance(value, (int, float, bool)):
        return str(value).strip()
    if isinstance(value, dict):
        for key in ("name", "text", "value", "label"):
            if key in value and value[key] is not None:
                text = as_text(value[key])
                if text:
                    return text
        return json.dumps(value, ensure_ascii=False, separators=(",", ":"))
    if isinstance(value, list):
        parts = [as_text(item) for item in value]
        parts = [p for p in parts if p]
        return " ".join(parts).strip()
    return str(value).strip()


def is_present(value: Any) -> bool:
    return bool(as_text(value))


def ensure_parent(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def write_json(path: Path, data: Any) -> None:
    ensure_parent(path)
    with path.open("w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)


def nested_get(data: Any, dotted_key: str) -> Any:
    current = data
    for part in dotted_key.split("."):
        if not isinstance(current, dict) or part not in current:
            return None
        current = current[part]
    return current


def pick_value(record: Dict[str, Any], keys: Sequence[str]) -> Any:
    for key in keys:
        value = nested_get(record, key) if "." in key else record.get(key)
        if is_present(value):
            return value
    return None


def collect_candidate_lists(node: Any, out: List[List[Dict[str, Any]]], depth: int = 0) -> None:
    if depth > 7:
        return
    if isinstance(node, list):
        if node and all(isinstance(item, dict) for item in node):
            out.append(node)  # type: ignore[arg-type]
        for item in node[:30]:
            collect_candidate_lists(item, out, depth + 1)
        return
    if isinstance(node, dict):
        for value in node.values():
            collect_candidate_lists(value, out, depth + 1)


def score_list(items: List[Dict[str, Any]]) -> Tuple[int, int]:
    score = 0
    for item in items[:50]:
        keys = {str(k).lower() for k in item.keys()}
        score += len(keys & JOB_HINT_KEYS)
    return (score, len(items))


def extract_raw_jobs(payload: Any) -> List[Dict[str, Any]]:
    if isinstance(payload, list) and all(isinstance(item, dict) for item in payload):
        return payload

    candidates: List[List[Dict[str, Any]]] = []
    collect_candidate_lists(payload, candidates)
    if not candidates:
        return []

    candidates.sort(key=score_list, reverse=True)
    return candidates[0]


def stable_hash(parts: Sequence[str]) -> str:
    digest = hashlib.sha1("|".join(parts).encode("utf-8")).hexdigest()
    return digest


def normalize_source(source_key: str) -> str:
    low = source_key.lower()
    if "zhaopin" in low:
        return "zhaopin"
    if "zhilian" in low:
        return "zhilian"
    if "boss" in low:
        return "boss"
    if "liepin" in low:
        return "liepin"
    return "unknown"


def normalize_jobs(raw_jobs: List[Dict[str, Any]], source_key: str) -> Tuple[List[Dict[str, Any]], int]:
    normalized: List[Dict[str, Any]] = []
    dedupe_keys = set()
    duplicate_count = 0
    source = normalize_source(source_key)

    for idx, raw in enumerate(raw_jobs):
        external_id = as_text(
            pick_value(raw, ["jobId", "positionId", "id", "number", "jobNumber", "positionNo"])
        )
        title = as_text(pick_value(raw, ["title", "jobName", "positionName", "name", "position"]))

        company_name = as_text(
            pick_value(
                raw,
                [
                    "companyName",
                    "company_name",
                    "companyShortName",
                    "companyFullName",
                    "company.name",
                    "company.shortName",
                    "company.fullName",
                    "company",
                ],
            )
        )

        location_province = as_text(
            pick_value(raw, ["province", "provinceName", "workProvince", "jobProvince"])
        )
        location_city = as_text(pick_value(raw, ["city", "cityName", "workCity", "jobCity"]))
        salary_range = as_text(
            pick_value(raw, ["salary", "salaryDesc", "salaryRange", "salaryText", "monthlySalary"])
        )
        min_degree = as_text(pick_value(raw, ["degree", "eduLevel", "education", "educationLevel"]))
        experience = as_text(pick_value(raw, ["experience", "workExp", "experienceDesc"]))
        responsibilities_text = as_text(
            pick_value(raw, ["description", "jobDesc", "detail", "responsibility", "content"])
        )
        source_url = as_text(
            pick_value(raw, ["url", "jobUrl", "positionURL", "positionUrl", "detailUrl"])
        )
        published_at = as_text(
            pick_value(raw, ["publishTime", "publishDate", "publishedAt", "updateTime", "refreshTime"])
        )

        content_hash = stable_hash(
            [title, company_name, location_province, location_city, salary_range, responsibilities_text]
        )

        dedupe_key = external_id or content_hash
        if dedupe_key in dedupe_keys:
            duplicate_count += 1
            continue
        dedupe_keys.add(dedupe_key)

        if not external_id:
            external_id = f"hash-{content_hash[:16]}"

        item = {
            "external_id": external_id,
            "title": title,
            "company_name": company_name,
            "location_province": location_province,
            "location_city": location_city,
            "salary_range": salary_range,
            "min_degree": min_degree,
            "experience": experience,
            "responsibilities_text": responsibilities_text,
            "source": source,
            "source_key": source_key,
            "source_url": source_url,
            "published_at": published_at,
            "content_hash": content_hash,
            "raw_index": idx,
        }
        normalized.append(item)

    return normalized, duplicate_count


def resolve_cache_file(root: Path, source_key: str, cache_file_arg: Optional[str]) -> Path:
    if cache_file_arg:
        cache_file = Path(cache_file_arg).expanduser().resolve()
        if cache_file.is_file():
            return cache_file
        raise FileNotFoundError(f"cache file not found: {cache_file}")

    cache_dir = root / "cache" / source_key
    latest = cache_dir / "latest.json"
    if latest.is_file():
        return latest

    if cache_dir.is_dir():
        candidates = sorted(cache_dir.glob("*.json"))
        if candidates:
            return candidates[-1]
    raise FileNotFoundError(
        f"no cache found for source_key={source_key}, expected {latest}"
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Normalize cached jobs into stable schema")
    parser.add_argument("--source-key", required=True, help="Cache key, e.g. zhaopin_C_530")
    parser.add_argument("--cache-file", default="", help="Optional explicit cache JSON file")
    parser.add_argument("--output-file", default="", help="Optional explicit output JSON file")
    parser.add_argument("--limit", type=int, default=0, help="Trim output jobs to first N (0=all)")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = Path(__file__).resolve().parent
    source_key = args.source_key.strip()
    if not source_key:
        raise ValueError("--source-key cannot be empty")

    started_at = utc_now()
    run_id = utc_stamp()
    runs_dir = root / "runs"
    run_file = runs_dir / f"{run_id}.json"
    latest_run_file = runs_dir / "latest.json"

    cache_file = resolve_cache_file(root, source_key, args.cache_file or None)
    output_file = (
        Path(args.output_file).expanduser().resolve()
        if args.output_file
        else (root / "output" / f"{source_key}.json")
    )

    payload = load_json(cache_file)
    raw_jobs = extract_raw_jobs(payload)
    normalized, duplicate_count = normalize_jobs(raw_jobs, source_key)

    if args.limit > 0:
        normalized = normalized[: args.limit]

    generated_at = utc_now().isoformat()
    output_data = {
        "generated_at": generated_at,
        "source_key": source_key,
        "source": normalize_source(source_key),
        "input_file": str(cache_file),
        "total_raw_jobs": len(raw_jobs),
        "total_normalized_jobs": len(normalized),
        "duplicate_count": duplicate_count,
        "jobs": normalized,
    }
    write_json(output_file, output_data)

    finished_at = utc_now()
    run_report = {
        "run_id": run_id,
        "status": "success",
        "source_key": source_key,
        "input_file": str(cache_file),
        "output_file": str(output_file),
        "started_at": started_at.isoformat(),
        "finished_at": finished_at.isoformat(),
        "duration_ms": int((finished_at - started_at).total_seconds() * 1000),
        "total_raw_jobs": len(raw_jobs),
        "total_normalized_jobs": len(normalized),
        "duplicate_count": duplicate_count,
    }
    write_json(run_file, run_report)
    write_json(latest_run_file, run_report)

    print(f"[job_sync] source_key={source_key}")
    print(f"[job_sync] input={cache_file}")
    print(f"[job_sync] output={output_file}")
    print(
        f"[job_sync] raw={len(raw_jobs)} normalized={len(normalized)} duplicate={duplicate_count}"
    )
    print(f"[job_sync] run_report={run_file}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
