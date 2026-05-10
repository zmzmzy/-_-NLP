#!/usr/bin/env python3
"""
Generate and optionally import student seed data.

Design goals:
- Deterministic generation by existing colleges/majors in DB.
- Student number format: <year_prefix><college_seg3><major_seg3><seq3>
- Each major gets [min_per_major, max_per_major] students (default 6-12).
- Idempotent import via ON DUPLICATE KEY UPDATE (no duplicate-key hard failure).
- Outputs CSV + rollback SQL + verification CSV.
"""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import hashlib
import os
import pathlib
import subprocess
import sys
from dataclasses import dataclass
from typing import Dict, List, Tuple


@dataclass
class MajorRow:
    major_id: int
    major_code: str
    major_name: str
    college_id: int
    college_code: str
    college_name: str


@dataclass
class StudentRow:
    student_no: str
    full_name: str
    gender: str
    college_id: int
    major_id: int
    graduation_year: int
    email: str
    phone: str
    resume_text: str
    technical_projects_text: str
    capability_summary_text: str
    college_code: str
    college_name: str
    major_code: str
    major_name: str
    major_segment: str
    seq: int


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate and import student seed data")
    parser.add_argument("--db-host", default=os.getenv("DB_HOST", "127.0.0.1"))
    parser.add_argument("--db-port", type=int, default=int(os.getenv("DB_PORT", "3306")))
    parser.add_argument("--db-user", default=os.getenv("DB_USER", "root"))
    parser.add_argument("--db-pass", default=os.getenv("DB_PASS", "123456"))
    parser.add_argument("--db-name", default=os.getenv("DB_NAME", "graduate_match"))

    parser.add_argument("--year-prefix", default="222022", help="6-digit prefix, e.g. 222022")
    parser.add_argument("--graduation-year", type=int, default=2026)
    parser.add_argument("--min-per-major", type=int, default=6)
    parser.add_argument("--max-per-major", type=int, default=12)
    parser.add_argument(
        "--distribution",
        choices=["cycle", "fixed"],
        default="cycle",
        help="cycle => 6..12 round-robin by major order; fixed => same count for all majors",
    )
    parser.add_argument("--fixed-per-major", type=int, default=8)
    parser.add_argument(
        "--major-segment-mode",
        choices=["tail3", "seq3"],
        default="tail3",
        help="tail3 uses last 3 digits of major_code when possible; seq3 uses ordered 001..",
    )
    parser.add_argument(
        "--major-segment-map-csv",
        default="",
        help="optional CSV: major_code,major_segment",
    )

    parser.add_argument(
        "--output-dir",
        default="import_samples",
        help="directory for generated csv/sql reports",
    )
    parser.add_argument("--tag", default="seed_students_v1")
    parser.add_argument("--apply", action="store_true", help="write generated rows into DB")

    return parser.parse_args()


def mysql_cmd_base(args: argparse.Namespace) -> List[str]:
    return [
        "mysql",
        f"-h{args.db_host}",
        f"-P{args.db_port}",
        f"-u{args.db_user}",
        f"-p{args.db_pass}",
        "-N",
        "-B",
    ]


def run_mysql_query(args: argparse.Namespace, sql: str) -> List[str]:
    cmd = mysql_cmd_base(args) + ["-e", sql]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        raise RuntimeError(f"mysql query failed: {proc.stderr.strip()}")
    out = proc.stdout.strip()
    if not out:
        return []
    return out.splitlines()


def run_mysql_script(args: argparse.Namespace, sql: str) -> None:
    cmd = [
        "mysql",
        f"-h{args.db_host}",
        f"-P{args.db_port}",
        f"-u{args.db_user}",
        f"-p{args.db_pass}",
    ]
    proc = subprocess.run(cmd, input=sql, text=True, capture_output=True)
    if proc.returncode != 0:
        raise RuntimeError(f"mysql script failed: {proc.stderr.strip()}")


def load_majors(args: argparse.Namespace) -> List[MajorRow]:
    sql = (
        f"USE {args.db_name}; "
        "SELECT m.id, m.code, m.name, c.id, c.code, c.name "
        "FROM majors m INNER JOIN colleges c ON c.id = m.college_id "
        "ORDER BY c.code, m.code, m.id;"
    )
    lines = run_mysql_query(args, sql)
    rows: List[MajorRow] = []
    for line in lines:
        parts = line.split("\t")
        if len(parts) != 6:
            continue
        rows.append(
            MajorRow(
                major_id=int(parts[0]),
                major_code=parts[1] or "",
                major_name=parts[2] or "",
                college_id=int(parts[3]),
                college_code=parts[4] or "",
                college_name=parts[5] or "",
            )
        )
    return rows


def digits_only(text: str) -> str:
    return "".join(ch for ch in text if ch.isdigit())


def normalize_3digits(text: str, fallback_seed: str) -> str:
    d = digits_only(text)
    if len(d) >= 3:
        return d[-3:]
    if d:
        return d.zfill(3)
    h = hashlib.sha1(fallback_seed.encode("utf-8")).hexdigest()
    return str(int(h[:6], 16) % 1000).zfill(3)


def load_major_segment_map(path: str) -> Dict[str, str]:
    mapping: Dict[str, str] = {}
    if not path:
        return mapping
    p = pathlib.Path(path)
    if not p.exists():
        raise FileNotFoundError(f"major segment map csv not found: {path}")
    with p.open("r", encoding="utf-8", newline="") as f:
        reader = csv.DictReader(f)
        if "major_code" not in reader.fieldnames or "major_segment" not in reader.fieldnames:
            raise ValueError("major-segment-map csv requires headers: major_code,major_segment")
        for row in reader:
            code = (row.get("major_code") or "").strip()
            seg = digits_only((row.get("major_segment") or "").strip())
            if not code or len(seg) != 3:
                continue
            mapping[code] = seg
    return mapping


def target_count(args: argparse.Namespace, major_index: int) -> int:
    if args.distribution == "fixed":
        return max(args.min_per_major, min(args.max_per_major, args.fixed_per_major))
    span = args.max_per_major - args.min_per_major + 1
    return args.min_per_major + (major_index % span)


def name_generator() -> Tuple[List[str], List[str], List[str]]:
    surnames = [
        "张", "王", "李", "赵", "刘", "陈", "杨", "黄", "周", "吴",
        "徐", "孙", "胡", "朱", "高", "林", "何", "郭", "马", "罗",
        "梁", "宋", "郑", "谢", "韩", "唐", "冯", "于", "董", "萧",
    ]
    a = [
        "晨", "宇", "思", "子", "嘉", "景", "奕", "安", "若", "知",
        "星", "文", "可", "亦", "承", "彦", "雨", "明", "博", "天",
    ]
    b = [
        "轩", "涵", "宁", "辰", "昊", "彤", "瑶", "泽", "航", "颖",
        "然", "悦", "霖", "哲", "源", "琪", "一", "凡", "阳", "睿",
    ]
    return surnames, a, b


def build_resume_text(major_name: str, college_name: str, idx: int) -> str:
    return (
        f"{major_name}在读学生，来自{college_name}。在校阶段系统学习专业核心课程，"
        f"持续参与课程项目与团队协作，能够完成需求梳理、方案设计、编码实现、测试验证和文档沉淀。"
        f"注重工程规范与过程复盘，具备良好的沟通表达能力和执行能力，目标岗位倾向技术研发与工程实施。"
        f"个人编号{idx}。"
    )


def build_project_text(major_name: str, idx: int) -> str:
    return (
        f"项目经历：参与{major_name}综合实践项目，负责模块开发、接口联调与异常排查。"
        f"在项目中使用版本管理与任务协同工具，完成迭代交付与阶段汇报。"
        f"可独立完成基础数据处理、日志分析和性能优化任务，并能根据反馈持续改进实现方案。"
        f"项目序号{idx}。"
    )


def build_capability_text(major_name: str, idx: int) -> str:
    return (
        f"能力总结：围绕{major_name}方向形成了问题拆解、信息检索、方案对比和执行落地能力。"
        f"具备团队协作意识、质量意识与责任意识，能在时间约束下推进任务闭环。"
        f"重视结果复盘和持续学习，能够将课堂知识迁移到真实业务场景。"
        f"能力编号{idx}。"
    )


def generate_students(args: argparse.Namespace, majors: List[MajorRow]) -> List[StudentRow]:
    seg_map = load_major_segment_map(args.major_segment_map_csv)
    surnames, a_chars, b_chars = name_generator()

    rows: List[StudentRow] = []
    used_numbers = set()
    used_names = set()
    global_idx = 0

    for mi, m in enumerate(majors):
        cseg = normalize_3digits(m.college_code, f"college:{m.college_id}:{m.college_code}")
        if m.major_code in seg_map:
            mseg = seg_map[m.major_code]
        elif args.major_segment_mode == "seq3":
            mseg = str(mi + 1).zfill(3)
        else:
            mseg = normalize_3digits(m.major_code, f"major:{m.major_id}:{m.major_code}")

        cnt = target_count(args, mi)

        for seq in range(1, cnt + 1):
            global_idx += 1
            s_no = f"{args.year_prefix}{cseg}{mseg}{str(seq).zfill(3)}"
            if s_no in used_numbers:
                raise ValueError(f"duplicate generated student_no: {s_no}")
            used_numbers.add(s_no)

            si = global_idx - 1
            name = (
                surnames[si % len(surnames)]
                + a_chars[(si // len(surnames)) % len(a_chars)]
                + b_chars[(si // (len(surnames) * len(a_chars))) % len(b_chars)]
            )
            if name in used_names:
                name = f"{name}{str(global_idx % 10)}"
            used_names.add(name)

            gender_cycle = ["male", "female", "male", "female", "other", "unknown"]
            gender = gender_cycle[si % len(gender_cycle)]

            email = f"{s_no}@seed.local"
            phone = "13" + str(100000000 + (global_idx % 900000000)).zfill(9)

            rows.append(
                StudentRow(
                    student_no=s_no,
                    full_name=name,
                    gender=gender,
                    college_id=m.college_id,
                    major_id=m.major_id,
                    graduation_year=args.graduation_year,
                    email=email,
                    phone=phone,
                    resume_text=build_resume_text(m.major_name, m.college_name, global_idx),
                    technical_projects_text=build_project_text(m.major_name, global_idx),
                    capability_summary_text=build_capability_text(m.major_name, global_idx),
                    college_code=m.college_code,
                    college_name=m.college_name,
                    major_code=m.major_code,
                    major_name=m.major_name,
                    major_segment=mseg,
                    seq=seq,
                )
            )

    return rows


def validate_students(args: argparse.Namespace, rows: List[StudentRow], majors: List[MajorRow]) -> None:
    if not rows:
        raise ValueError("no students generated")

    # student_no constraints
    seen_no = set()
    for r in rows:
        if r.student_no in seen_no:
            raise ValueError(f"duplicate student_no in memory: {r.student_no}")
        seen_no.add(r.student_no)
        if len(r.student_no) != 15 or not r.student_no.isdigit():
            raise ValueError(f"student_no must be 15 digits: {r.student_no}")

    # per-major count
    count_by_major: Dict[int, int] = {}
    for r in rows:
        count_by_major[r.major_id] = count_by_major.get(r.major_id, 0) + 1

    for m in majors:
        c = count_by_major.get(m.major_id, 0)
        if not (args.min_per_major <= c <= args.max_per_major):
            raise ValueError(
                f"major {m.major_code}/{m.major_name} count {c} outside [{args.min_per_major}, {args.max_per_major}]"
            )


def write_plan_csv(output_dir: pathlib.Path, rows: List[StudentRow], tag: str) -> pathlib.Path:
    ts = dt.datetime.now().strftime("%Y%m%d_%H%M%S")
    path = output_dir / f"students_import_plan_{tag}_{ts}.csv"
    fields = [
        "student_no",
        "full_name",
        "gender",
        "college_id",
        "college_code",
        "college_name",
        "major_id",
        "major_code",
        "major_name",
        "major_segment",
        "seq",
        "graduation_year",
        "email",
        "phone",
        "resume_text",
        "technical_projects_text",
        "capability_summary_text",
    ]
    with path.open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields, quoting=csv.QUOTE_ALL)
        w.writeheader()
        for r in rows:
            w.writerow(
                {
                    "student_no": r.student_no,
                    "full_name": r.full_name,
                    "gender": r.gender,
                    "college_id": r.college_id,
                    "college_code": r.college_code,
                    "college_name": r.college_name,
                    "major_id": r.major_id,
                    "major_code": r.major_code,
                    "major_name": r.major_name,
                    "major_segment": r.major_segment,
                    "seq": r.seq,
                    "graduation_year": r.graduation_year,
                    "email": r.email,
                    "phone": r.phone,
                    "resume_text": r.resume_text,
                    "technical_projects_text": r.technical_projects_text,
                    "capability_summary_text": r.capability_summary_text,
                }
            )
    return path


def sql_quote(text: str) -> str:
    return "'" + text.replace("\\", "\\\\").replace("'", "''") + "'"


def write_sql_files(
    args: argparse.Namespace,
    output_dir: pathlib.Path,
    rows: List[StudentRow],
    tag: str,
) -> Tuple[pathlib.Path, pathlib.Path]:
    ts = dt.datetime.now().strftime("%Y%m%d_%H%M%S")
    apply_path = output_dir / f"students_import_apply_{tag}_{ts}.sql"
    rollback_path = output_dir / f"students_import_rollback_{tag}_{ts}.sql"

    apply_lines: List[str] = [
        f"USE {args.db_name};",
        "SET NAMES utf8mb4;",
        "START TRANSACTION;",
        "INSERT INTO students(student_no, full_name, gender, college_id, major_id, graduation_year, email, phone, resume_text, technical_projects_text, capability_summary_text)",
        "VALUES",
    ]

    values_sql: List[str] = []
    for r in rows:
        values_sql.append(
            "(" + ", ".join(
                [
                    sql_quote(r.student_no),
                    sql_quote(r.full_name),
                    sql_quote(r.gender),
                    str(r.college_id),
                    str(r.major_id),
                    str(r.graduation_year),
                    sql_quote(r.email),
                    sql_quote(r.phone),
                    sql_quote(r.resume_text),
                    sql_quote(r.technical_projects_text),
                    sql_quote(r.capability_summary_text),
                ]
            ) + ")"
        )

    apply_lines.append(",\n".join(values_sql))
    apply_lines.append(
        "ON DUPLICATE KEY UPDATE "
        "full_name = VALUES(full_name), "
        "gender = VALUES(gender), "
        "college_id = VALUES(college_id), "
        "major_id = VALUES(major_id), "
        "graduation_year = VALUES(graduation_year), "
        "email = VALUES(email), "
        "phone = VALUES(phone), "
        "resume_text = VALUES(resume_text), "
        "technical_projects_text = VALUES(technical_projects_text), "
        "capability_summary_text = VALUES(capability_summary_text);"
    )
    apply_lines.append("COMMIT;")

    rollback_lines: List[str] = [
        f"USE {args.db_name};",
        "SET NAMES utf8mb4;",
        "START TRANSACTION;",
        "DELETE FROM students WHERE student_no IN (",
        ",\n".join(sql_quote(r.student_no) for r in rows),
        ");",
        "COMMIT;",
    ]

    apply_path.write_text("\n".join(apply_lines) + "\n", encoding="utf-8")
    rollback_path.write_text("\n".join(rollback_lines) + "\n", encoding="utf-8")
    return apply_path, rollback_path


def verify_after_apply(args: argparse.Namespace, majors: List[MajorRow], output_dir: pathlib.Path, tag: str) -> pathlib.Path:
    ts = dt.datetime.now().strftime("%Y%m%d_%H%M%S")
    path = output_dir / f"students_import_verify_{tag}_{ts}.csv"

    major_ids = ",".join(str(m.major_id) for m in majors)
    sql = (
        f"USE {args.db_name}; "
        "SELECT m.id, m.code, m.name, c.code, c.name, COUNT(s.id) AS student_count "
        "FROM majors m "
        "JOIN colleges c ON c.id = m.college_id "
        "LEFT JOIN students s ON s.major_id = m.id "
        f"WHERE m.id IN ({major_ids}) "
        "GROUP BY m.id, m.code, m.name, c.code, c.name "
        "ORDER BY c.code, m.code;"
    )
    lines = run_mysql_query(args, sql)

    with path.open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["major_id", "major_code", "major_name", "college_code", "college_name", "student_count", "status"])
        for line in lines:
            p = line.split("\t")
            if len(p) != 6:
                continue
            cnt = int(p[5])
            status = "PASS" if args.min_per_major <= cnt <= args.max_per_major else "WARN"
            w.writerow([p[0], p[1], p[2], p[3], p[4], cnt, status])

    return path


def main() -> int:
    args = parse_args()

    if len(args.year_prefix) != 6 or not args.year_prefix.isdigit():
        print("[error] --year-prefix must be 6 digits, e.g. 222022", file=sys.stderr)
        return 2
    if args.min_per_major <= 0 or args.max_per_major <= 0 or args.min_per_major > args.max_per_major:
        print("[error] invalid min/max per major", file=sys.stderr)
        return 2

    output_dir = pathlib.Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    majors = load_majors(args)
    if not majors:
        print("[error] no majors found in DB, please import majors first", file=sys.stderr)
        return 2

    rows = generate_students(args, majors)
    validate_students(args, rows, majors)

    plan_csv = write_plan_csv(output_dir, rows, args.tag)
    apply_sql, rollback_sql = write_sql_files(args, output_dir, rows, args.tag)

    print(f"[ok] majors: {len(majors)}")
    print(f"[ok] generated students: {len(rows)}")
    print(f"[ok] plan csv: {plan_csv}")
    print(f"[ok] apply sql: {apply_sql}")
    print(f"[ok] rollback sql: {rollback_sql}")

    if args.apply:
        sql_text = apply_sql.read_text(encoding="utf-8")
        run_mysql_script(args, sql_text)
        verify_csv = verify_after_apply(args, majors, output_dir, args.tag)
        print(f"[ok] import applied to DB: {args.db_name}")
        print(f"[ok] verify csv: {verify_csv}")
    else:
        print("[info] dry-run only. add --apply to write into DB.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
