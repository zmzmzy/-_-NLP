#!/usr/bin/env python3
"""Generate and apply defense employment simulation data.

Targets:
- employment rate ~= 70%
- alignment rate ~= 45%

It seeds three linked tables in `graduate_match`:
- employment_submissions
- employment_records
- match_results

And keeps a reusable audit trail in `import_samples/`:
- plan csv
- apply sql
- verify sql
"""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import math
import pathlib
import subprocess
import sys
from dataclasses import dataclass


@dataclass
class Student:
    sid: int
    student_no: str
    major_id: int


@dataclass
class Major:
    mid: int
    code: str
    name: str


@dataclass
class Job:
    jid: int
    title: str
    company_id: int


def round_half_up(value: float) -> int:
    return int(math.floor(value + 0.5))


class MySQL:
    def __init__(self, host: str, port: int, user: str, password: str, database: str):
        self.base_cmd = [
            "mysql",
            f"-h{host}",
            f"-P{port}",
            f"-u{user}",
            f"-p{password}",
            "-N",
            "-B",
            database,
        ]

    def query(self, sql: str) -> list[list[str]]:
        cmd = self.base_cmd + ["-e", sql]
        out = subprocess.check_output(cmd, text=True)
        rows: list[list[str]] = []
        for line in out.splitlines():
            if not line.strip():
                continue
            rows.append(line.rstrip("\n").split("\t"))
        return rows

    def exec_sql_file(self, sql_path: pathlib.Path) -> None:
        cmd = self.base_cmd.copy()
        with sql_path.open("r", encoding="utf-8") as handle:
            subprocess.check_call(cmd, stdin=handle)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Seed employment data around 70/45 targets")
    parser.add_argument("mode", nargs="?", choices=["dry-run", "apply"], default="dry-run")
    parser.add_argument("--db-host", default="127.0.0.1")
    parser.add_argument("--db-port", type=int, default=3306)
    parser.add_argument("--db-user", default="root")
    parser.add_argument("--db-pass", default="123456")
    parser.add_argument("--db-name", default="graduate_match")
    parser.add_argument("--employment-rate", type=float, default=0.70)
    parser.add_argument("--alignment-rate", type=float, default=0.45)
    return parser.parse_args()


def load_students(db: MySQL) -> list[Student]:
    sql = (
        "SELECT id, student_no, major_id "
        "FROM students "
        "ORDER BY major_id ASC, student_no ASC, id ASC"
    )
    rows = db.query(sql)
    return [Student(int(r[0]), r[1], int(r[2])) for r in rows]


def load_majors(db: MySQL) -> dict[int, Major]:
    rows = db.query("SELECT id, code, name FROM majors ORDER BY id")
    return {int(r[0]): Major(int(r[0]), r[1], r[2]) for r in rows}


def load_jobs(db: MySQL) -> list[Job]:
    rows = db.query("SELECT id, title, company_id FROM jobs ORDER BY id")
    return [Job(int(r[0]), r[1], int(r[2])) for r in rows]


def find_job_by_keyword(jobs: list[Job], keyword: str) -> Job | None:
    for job in jobs:
        if keyword in job.title:
            return job
    return None


def build_major_aligned_job_map(majors: dict[int, Major], jobs: list[Job]) -> dict[int, Job]:
    keyword_by_major_code = {
        "085401": "C++后端开发工程师",
        "085402": "网络安全工程师",
        "085403": "机器学习工程师",
        "085404": "数据治理工程师",
        "080901": "嵌入式开发工程师",
        "080902": "5G网络优化工程师",
        "080301": "工业控制工程师",
        "080302": "嵌入式开发工程师",
        "081001": "BIM工程师",
        "081002": "环境监测工程师",
        "120201": "运营分析师",
        "120202": "市场运营专员",
    }

    job_map: dict[int, Job] = {}
    fallback = jobs[0]
    for mid, major in majors.items():
        keyword = keyword_by_major_code.get(major.code)
        picked = find_job_by_keyword(jobs, keyword) if keyword else None
        if picked is None:
            picked = fallback
        job_map[mid] = picked
    return job_map


def allocate_employed(students: list[Student], employment_rate: float) -> tuple[set[int], dict[int, int], dict[int, list[Student]]]:
    grouped: dict[int, list[Student]] = {}
    for s in students:
        grouped.setdefault(s.major_id, []).append(s)

    employed_ids: set[int] = set()
    employed_count_by_major: dict[int, int] = {}
    approved_students_by_major: dict[int, list[Student]] = {}

    for major_id, lst in grouped.items():
        target = round_half_up(len(lst) * employment_rate)
        employed_count_by_major[major_id] = target
        approved = lst[:target]
        approved_students_by_major[major_id] = approved
        for s in approved:
            employed_ids.add(s.sid)

    return employed_ids, employed_count_by_major, approved_students_by_major


def allocate_aligned_targets(
    employed_count_by_major: dict[int, int],
    alignment_rate: float,
    aligned_total_target: int,
) -> dict[int, int]:
    floor_sum = 0
    raw_rows: list[tuple[int, float, int, float]] = []
    # major_id, raw, floor, frac
    for mid, employed_n in employed_count_by_major.items():
        raw = employed_n * alignment_rate
        floor_n = int(math.floor(raw))
        frac = raw - floor_n
        raw_rows.append((mid, raw, floor_n, frac))
        floor_sum += floor_n

    need = aligned_total_target - floor_sum
    raw_rows.sort(key=lambda x: (x[3], employed_count_by_major[x[0]], -x[0]), reverse=True)

    aligned_by_major: dict[int, int] = {mid: floor_n for mid, _raw, floor_n, _frac in raw_rows}
    idx = 0
    while need > 0 and idx < len(raw_rows):
        mid = raw_rows[idx][0]
        aligned_by_major[mid] += 1
        idx += 1
        need -= 1

    return aligned_by_major


def pick_unaligned_job(aligned_job_id: int, jobs: list[Job], seed: int) -> Job:
    n = len(jobs)
    start = seed % n
    for off in range(n):
        job = jobs[(start + off) % n]
        if job.jid != aligned_job_id:
            return job
    return jobs[0]


def score_tuple(is_aligned: bool, idx: int) -> tuple[float, float, float, float]:
    if is_aligned:
        final = 72.0 + float((idx * 7) % 21)
        semantic = min(98.0, final + 3.0 + float(idx % 4))
        skill = min(98.0, final + 1.5 + float(idx % 3))
        constraint = 1.0
    else:
        final = 42.0 + float((idx * 5) % 27)
        semantic = min(84.0, final + 4.0 + float(idx % 3))
        skill = max(24.0, final - 6.0 + float(idx % 4))
        constraint = 0.9 if (idx % 3 == 0) else 1.0
    return (round(semantic, 3), round(skill, 3), round(constraint, 3), round(final, 3))


def employment_type_by_index(idx: int) -> str:
    mod = idx % 10
    if mod < 6:
        return "signed"
    if mod < 9:
        return "offer"
    return "internship"


def sql_escape(raw: str) -> str:
    return raw.replace("\\", "\\\\").replace("'", "''")


def main() -> int:
    args = parse_args()
    db = MySQL(args.db_host, args.db_port, args.db_user, args.db_pass, args.db_name)

    students = load_students(db)
    majors = load_majors(db)
    jobs = load_jobs(db)

    if not students:
        print("[error] no students found")
        return 1
    if not majors:
        print("[error] no majors found")
        return 1
    if not jobs:
        print("[error] no jobs found")
        return 1

    major_job_map = build_major_aligned_job_map(majors, jobs)

    employed_ids, employed_count_by_major, approved_by_major = allocate_employed(
        students, args.employment_rate
    )
    total_students = len(students)
    employed_total_target = len(employed_ids)
    aligned_total_target = round_half_up(employed_total_target * args.alignment_rate)
    aligned_target_by_major = allocate_aligned_targets(
        employed_count_by_major,
        args.alignment_rate,
        aligned_total_target,
    )

    aligned_ids: set[int] = set()
    for major_id, approved_students in approved_by_major.items():
        target = aligned_target_by_major.get(major_id, 0)
        for s in approved_students[:target]:
            aligned_ids.add(s.sid)

    if len(aligned_ids) != aligned_total_target:
        print(
            f"[error] aligned allocation mismatch: expect {aligned_total_target}, got {len(aligned_ids)}"
        )
        return 1

    all_students_sorted = sorted(students, key=lambda s: (s.major_id, s.student_no, s.sid))
    unemployed = [s for s in all_students_sorted if s.sid not in employed_ids]

    pending_target = max(8, round_half_up(total_students * 0.085))
    rejected_target = max(7, round_half_up(total_students * 0.075))
    if pending_target + rejected_target > len(unemployed):
        overflow = pending_target + rejected_target - len(unemployed)
        rejected_target = max(0, rejected_target - overflow)

    pending_students = unemployed[:pending_target]
    rejected_students = unemployed[pending_target : pending_target + rejected_target]

    approved_students = [s for s in all_students_sorted if s.sid in employed_ids]

    plan_rows: list[dict[str, object]] = []
    base_date = dt.date(2026, 2, 20)

    for idx, stu in enumerate(approved_students, start=1):
        aligned = stu.sid in aligned_ids
        aligned_job = major_job_map.get(stu.major_id, jobs[0])
        if aligned:
            picked_job = aligned_job
        else:
            picked_job = pick_unaligned_job(aligned_job.jid, jobs, seed=(stu.sid + idx + stu.major_id))

        semantic, skill, constraint, final = score_tuple(aligned, idx)
        start_date = base_date + dt.timedelta(days=(idx % 75))
        plan_rows.append(
            {
                "student_id": stu.sid,
                "student_no": stu.student_no,
                "major_id": stu.major_id,
                "job_id": picked_job.jid,
                "company_id": picked_job.company_id,
                "scenario": "approved",
                "aligned": 1 if aligned else 0,
                "employment_type": employment_type_by_index(idx),
                "start_date": start_date.isoformat(),
                "semantic_score": semantic,
                "skill_score": skill,
                "constraint_score": constraint,
                "final_score": final,
            }
        )

    for idx, stu in enumerate(pending_students, start=1):
        aligned_job = major_job_map.get(stu.major_id, jobs[0])
        plan_rows.append(
            {
                "student_id": stu.sid,
                "student_no": stu.student_no,
                "major_id": stu.major_id,
                "job_id": aligned_job.jid,
                "company_id": aligned_job.company_id,
                "scenario": "pending",
                "aligned": 0,
                "employment_type": "signed",
                "start_date": (base_date + dt.timedelta(days=(80 + idx))).isoformat(),
                "semantic_score": 0.0,
                "skill_score": 0.0,
                "constraint_score": 1.0,
                "final_score": 0.0,
            }
        )

    for idx, stu in enumerate(rejected_students, start=1):
        aligned_job = major_job_map.get(stu.major_id, jobs[0])
        alt_job = pick_unaligned_job(aligned_job.jid, jobs, seed=(stu.sid + 7 * idx))
        plan_rows.append(
            {
                "student_id": stu.sid,
                "student_no": stu.student_no,
                "major_id": stu.major_id,
                "job_id": alt_job.jid,
                "company_id": alt_job.company_id,
                "scenario": "rejected",
                "aligned": 0,
                "employment_type": "offer",
                "start_date": (base_date + dt.timedelta(days=(100 + idx))).isoformat(),
                "semantic_score": 0.0,
                "skill_score": 0.0,
                "constraint_score": 1.0,
                "final_score": 0.0,
            }
        )

    timestamp = dt.datetime.now().strftime("%Y%m%d_%H%M%S")
    root = pathlib.Path(__file__).resolve().parents[2]
    out_dir = root / "import_samples"
    out_dir.mkdir(parents=True, exist_ok=True)

    plan_csv = out_dir / f"employment_seed_plan_70_45_{timestamp}.csv"
    apply_sql = out_dir / f"employment_seed_apply_70_45_{timestamp}.sql"
    verify_sql = out_dir / f"employment_seed_verify_70_45_{timestamp}.sql"

    fieldnames = [
        "student_id",
        "student_no",
        "major_id",
        "job_id",
        "company_id",
        "scenario",
        "aligned",
        "employment_type",
        "start_date",
        "semantic_score",
        "skill_score",
        "constraint_score",
        "final_score",
    ]

    with plan_csv.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for row in plan_rows:
            writer.writerow(row)

    values_sql_rows = []
    for row in plan_rows:
        values_sql_rows.append(
            "(" + ",".join(
                [
                    str(int(row["student_id"])),
                    f"'{sql_escape(str(row['student_no']))}'",
                    str(int(row["major_id"])),
                    str(int(row["job_id"])),
                    str(int(row["company_id"])),
                    f"'{row['scenario']}'",
                    str(int(row["aligned"])),
                    f"'{row['employment_type']}'",
                    f"'{row['start_date']}'",
                    f"{float(row['semantic_score']):.3f}",
                    f"{float(row['skill_score']):.3f}",
                    f"{float(row['constraint_score']):.3f}",
                    f"{float(row['final_score']):.3f}",
                ]
            )
            + ")"
        )

    apply_lines = [
        "USE graduate_match;",
        "SET NAMES utf8mb4;",
        "START TRANSACTION;",
        "",
        "INSERT INTO users(username,email,password_hash,role,is_active)",
        "VALUES('seed_submitter','seed_submitter@local','sha256:ff96673205dc722320598ebf8f88325b2ac56922d5a2164b5765868274bc0d73','viewer',1)",
        "ON DUPLICATE KEY UPDATE email=VALUES(email), password_hash=VALUES(password_hash), role=VALUES(role), is_active=VALUES(is_active);",
        "",
        "SET @seed_submitter_id := (SELECT id FROM users WHERE username='seed_submitter' LIMIT 1);",
        "SET @admin_id := (SELECT id FROM users WHERE role='admin' ORDER BY id ASC LIMIT 1);",
        "",
        "DELETE FROM employment_submissions;",
        "DELETE FROM employment_records;",
        "DELETE FROM match_results;",
        "",
        "DROP TEMPORARY TABLE IF EXISTS tmp_seed_plan;",
        "CREATE TEMPORARY TABLE tmp_seed_plan (",
        "  student_id BIGINT NOT NULL,",
        "  student_no VARCHAR(64) NOT NULL,",
        "  major_id BIGINT NOT NULL,",
        "  job_id BIGINT NOT NULL,",
        "  company_id BIGINT NOT NULL,",
        "  scenario ENUM('approved','pending','rejected') NOT NULL,",
        "  aligned TINYINT(1) NOT NULL DEFAULT 0,",
        "  employment_type ENUM('signed','offer','internship','pending') NOT NULL DEFAULT 'signed',",
        "  start_date DATE NOT NULL,",
        "  semantic_score DECIMAL(6,3) NOT NULL DEFAULT 0.000,",
        "  skill_score DECIMAL(6,3) NOT NULL DEFAULT 0.000,",
        "  constraint_score DECIMAL(6,3) NOT NULL DEFAULT 1.000,",
        "  final_score DECIMAL(6,3) NOT NULL DEFAULT 0.000,",
        "  PRIMARY KEY(student_id)",
        ") ENGINE=InnoDB;",
        "",
        "INSERT INTO tmp_seed_plan(student_id,student_no,major_id,job_id,company_id,scenario,aligned,employment_type,start_date,semantic_score,skill_score,constraint_score,final_score)",
        "VALUES",
        "  " + ",\n  ".join(values_sql_rows) + ";",
        "",
        "INSERT INTO employment_records(student_id,company_id,job_id,employment_type,employment_status,start_date,end_date,evidence_source,notes,created_at,updated_at)",
        "SELECT p.student_id,p.company_id,p.job_id,p.employment_type,'active',p.start_date,NULL,'seed_70_45_v1',",
        "       CONCAT('seed_approved_',p.student_no,': defense showcase baseline for 70/45 targets'), NOW(), NOW()",
        "FROM tmp_seed_plan p",
        "WHERE p.scenario='approved';",
        "",
        "INSERT INTO employment_submissions(",
        "  submitter_user_id,student_id,company_id,job_id,",
        "  proposed_company_name,proposed_job_title,",
        "  proposed_job_responsibilities_text,proposed_job_required_knowledge_skills_text,proposed_job_required_tools_platform_text,",
        "  employment_type,employment_status,start_date,end_date,evidence_source,notes,status,",
        "  review_comment,reviewed_by_user_id,reviewed_at,approved_employment_record_id,created_at,updated_at",
        ")",
        "SELECT @seed_submitter_id,p.student_id,p.company_id,p.job_id,c.name,j.title,",
        "       'Seeded approved submission for defense showcase process demo.',",
        "       'Skill profile validated in seeded scenario.',",
        "       'Platform profile validated in seeded scenario.',",
        "       p.employment_type,'active',p.start_date,NULL,'seed_70_45_v1',",
        "       CONCAT('submission_approved_',p.student_no),",
        "       'approved','approved by seeded admin flow',@admin_id,NOW(),er.id,NOW(),NOW()",
        "FROM tmp_seed_plan p",
        "JOIN employment_records er ON er.student_id=p.student_id",
        "JOIN companies c ON c.id=p.company_id",
        "JOIN jobs j ON j.id=p.job_id",
        "WHERE p.scenario='approved';",
        "",
        "INSERT INTO employment_submissions(",
        "  submitter_user_id,student_id,company_id,job_id,",
        "  proposed_company_name,proposed_job_title,",
        "  proposed_job_responsibilities_text,proposed_job_required_knowledge_skills_text,proposed_job_required_tools_platform_text,",
        "  employment_type,employment_status,start_date,end_date,evidence_source,notes,status,created_at,updated_at",
        ")",
        "SELECT @seed_submitter_id,p.student_id,p.company_id,p.job_id,c.name,j.title,",
        "       'Seeded pending submission for live admin-review demo.',",
        "       'Pending skill evidence check.',",
        "       'Pending platform evidence check.',",
        "       p.employment_type,'active',p.start_date,NULL,'seed_70_45_v1',",
        "       CONCAT('submission_pending_',p.student_no),'pending',NOW(),NOW()",
        "FROM tmp_seed_plan p",
        "JOIN companies c ON c.id=p.company_id",
        "JOIN jobs j ON j.id=p.job_id",
        "WHERE p.scenario='pending';",
        "",
        "INSERT INTO employment_submissions(",
        "  submitter_user_id,student_id,company_id,job_id,",
        "  proposed_company_name,proposed_job_title,",
        "  proposed_job_responsibilities_text,proposed_job_required_knowledge_skills_text,proposed_job_required_tools_platform_text,",
        "  employment_type,employment_status,start_date,end_date,evidence_source,notes,status,",
        "  review_comment,reviewed_by_user_id,reviewed_at,created_at,updated_at",
        ")",
        "SELECT @seed_submitter_id,p.student_id,p.company_id,p.job_id,c.name,j.title,",
        "       'Seeded rejected submission to demonstrate governance quality checks.',",
        "       'Rejected due to missing required materials.',",
        "       'Rejected due to evidence inconsistency.',",
        "       p.employment_type,'active',p.start_date,NULL,'seed_70_45_v1',",
        "       CONCAT('submission_rejected_',p.student_no),'rejected',",
        "       'rejected in seeded admin review',@admin_id,NOW(),NOW(),NOW()",
        "FROM tmp_seed_plan p",
        "JOIN companies c ON c.id=p.company_id",
        "JOIN jobs j ON j.id=p.job_id",
        "WHERE p.scenario='rejected';",
        "",
        "INSERT INTO match_results(student_id,major_id,job_id,algorithm_version,semantic_score,skill_score,constraint_score,final_score,matched_skills_json,missing_skills_json,constraint_failures_json,generated_at)",
        "SELECT p.student_id,p.major_id,p.job_id,'v0.4-major-student',p.semantic_score,p.skill_score,p.constraint_score,p.final_score,",
        "       JSON_ARRAY(),JSON_ARRAY(),JSON_ARRAY(),NOW()",
        "FROM tmp_seed_plan p",
        "WHERE p.scenario='approved';",
        "",
        "COMMIT;",
    ]

    verify_lines = [
        "USE graduate_match;",
        "SET NAMES utf8mb4;",
        "",
        "WITH latest_emp AS (",
        "  SELECT er.student_id, er.job_id, er.employment_type,",
        "         ROW_NUMBER() OVER (PARTITION BY er.student_id ORDER BY er.created_at DESC, er.id DESC) rn",
        "  FROM employment_records er",
        "),",
        "emp AS (",
        "  SELECT student_id, job_id",
        "  FROM latest_emp",
        "  WHERE rn=1 AND employment_type <> 'pending' AND job_id IS NOT NULL",
        "),",
        "latest_match AS (",
        "  SELECT mr.student_id, mr.job_id, mr.final_score,",
        "         ROW_NUMBER() OVER (PARTITION BY mr.student_id, mr.job_id ORDER BY mr.generated_at DESC, mr.id DESC) rn",
        "  FROM match_results mr",
        ")",
        "SELECT",
        "  (SELECT COUNT(*) FROM students) AS total_students,",
        "  (SELECT COUNT(*) FROM emp) AS employed_students,",
        "  ROUND((SELECT COUNT(*) FROM emp)*100.0/(SELECT COUNT(*) FROM students), 2) AS employment_rate_pct,",
        "  (SELECT COUNT(*) FROM emp e",
        "    LEFT JOIN latest_match lm ON lm.student_id=e.student_id AND lm.job_id=e.job_id AND lm.rn=1",
        "    WHERE lm.final_score >= 70) AS aligned_students,",
        "  ROUND((SELECT COUNT(*) FROM emp e",
        "    LEFT JOIN latest_match lm ON lm.student_id=e.student_id AND lm.job_id=e.job_id AND lm.rn=1",
        "    WHERE lm.final_score >= 70)*100.0/NULLIF((SELECT COUNT(*) FROM emp),0), 2) AS alignment_rate_pct;",
        "",
        "SELECT scenario, COUNT(*) AS cnt FROM (",
        "  SELECT 'approved' AS scenario FROM employment_submissions WHERE status='approved'",
        "  UNION ALL",
        "  SELECT 'pending' AS scenario FROM employment_submissions WHERE status='pending'",
        "  UNION ALL",
        "  SELECT 'rejected' AS scenario FROM employment_submissions WHERE status='rejected'",
        ") t GROUP BY scenario ORDER BY scenario;",
    ]

    apply_sql.write_text("\n".join(apply_lines) + "\n", encoding="utf-8")
    verify_sql.write_text("\n".join(verify_lines) + "\n", encoding="utf-8")

    print(f"[ok] students: {total_students}")
    print(f"[ok] employed target: {employed_total_target}")
    print(f"[ok] aligned target: {aligned_total_target}")
    print(f"[ok] pending target: {len(pending_students)}")
    print(f"[ok] rejected target: {len(rejected_students)}")
    print(f"[ok] plan csv: {plan_csv}")
    print(f"[ok] apply sql: {apply_sql}")
    print(f"[ok] verify sql: {verify_sql}")

    if args.mode == "apply":
        db.exec_sql_file(apply_sql)
        print("[ok] apply done")
        verify_out = db.query("\n".join(verify_lines[2:]))
        if verify_out:
            print("[ok] verify snapshot:")
            for row in verify_out:
                print("\t".join(row))
    else:
        print("[info] dry-run only. use mode=apply to write DB")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
