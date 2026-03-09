# Job Sync Tool (MVP)

This tool provides a minimal offline sync pipeline for job data:

1. Read cached raw JSON data (`cache/<source_key>/latest.json`).
2. Extract and normalize job records to a stable schema.
3. Write normalized output (`output/<source_key>.json`).
4. Record one run report (`runs/<timestamp>.json`).

## Directory Layout (v0.2)

- `fetchers/`: reserved for source-specific crawling adapters
- `parsers/`: reserved for source payload parsers
- `normalizers/`: reserved for schema mapping rules
- `cache/`: raw cache files (`latest.json` + timestamp snapshots)
- `output/`: normalized output for DB upsert
- `runs/`: run reports and cron logs

## Quick Start

```bash
cd /home/roamer/graduation_proj_0.1/tools/job_sync
python3 job_sync.py --source-key zhaopin_C_530
```

If `cache/<source_key>/latest.json` is missing, the tool will try to use
the newest timestamped JSON file under that cache directory.

## Optional Arguments

```bash
python3 job_sync.py \
  --source-key zhaopin_C_530 \
  --cache-file ./cache/zhaopin_C_530/latest.json \
  --output-file ./output/zhilian_cpp_beijing.json \
  --limit 200
```

## Normalized Fields

Each job item in output includes:

- `external_id`
- `title`
- `company_name`
- `location_province`
- `location_city`
- `salary_range`
- `min_degree`
- `experience`
- `responsibilities_text`
- `source`
- `source_key`
- `source_url`
- `published_at`
- `content_hash`
- `raw_index`

## Notes

- This MVP is cache-based and does not fetch from websites directly yet.
- It is safe for repeated runs; deduplication is done by external id (or hash fallback).
- The run report can be used later to map into database sync batch logs.

## Scheduled Run Example

Use wrapper script:

```bash
cd /home/roamer/graduation_proj_0.1/tools/job_sync
./run_sync.sh zhaopin_C_530 500
```

Cron template is provided at:

- `tools/job_sync/job_sync.cron.example`
