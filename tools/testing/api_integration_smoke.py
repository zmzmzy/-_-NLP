#!/usr/bin/env python3
"""API integration smoke test for graduation_proj_0.1.

No third-party dependencies required.
"""

from __future__ import annotations

import json
import subprocess
import sys
import time
import urllib.error
import urllib.request
from dataclasses import dataclass
from typing import Any, Dict, Optional, Tuple

BASE_URL = "http://127.0.0.1:5555/api"
HTTP_TIMEOUT_SECONDS = 20


@dataclass
class TestStats:
    passed: int = 0
    failed: int = 0

    def ok(self, name: str, code: int) -> None:
        self.passed += 1
        print(f"[PASS] {name} (HTTP {code})")

    def bad(self, name: str, code: int, expected: int, payload: str) -> None:
        self.failed += 1
        print(f"[FAIL] {name} (HTTP {code}, expected {expected})")
        if payload:
            print(payload[:600])


def _build_opener() -> urllib.request.OpenerDirector:
    # Force local direct connection to avoid accidental proxy hijacking.
    return urllib.request.build_opener(urllib.request.ProxyHandler({}))


def request_json(
    opener: urllib.request.OpenerDirector,
    method: str,
    path: str,
    body: Optional[Dict[str, Any]] = None,
    token: Optional[str] = None,
) -> Tuple[int, str, Optional[Dict[str, Any]]]:
    headers = {"Content-Type": "application/json"}
    if token:
        headers["Authorization"] = f"Bearer {token}"
    payload = None if body is None else json.dumps(body, ensure_ascii=False).encode("utf-8")
    req = urllib.request.Request(BASE_URL + path, data=payload, method=method, headers=headers)

    try:
        with opener.open(req, timeout=HTTP_TIMEOUT_SECONDS) as resp:
            text = resp.read().decode("utf-8", errors="replace")
            code = resp.getcode()
    except urllib.error.HTTPError as exc:
        text = exc.read().decode("utf-8", errors="replace")
        code = exc.code
    except Exception as exc:  # pragma: no cover - smoke script
        return 0, str(exc), None

    parsed: Optional[Dict[str, Any]]
    try:
        parsed = json.loads(text) if text else None
    except json.JSONDecodeError:
        parsed = None
    return code, text, parsed


def expect(
    stats: TestStats,
    opener: urllib.request.OpenerDirector,
    name: str,
    method: str,
    path: str,
    expected: int,
    body: Optional[Dict[str, Any]] = None,
    token: Optional[str] = None,
) -> Tuple[int, Optional[Dict[str, Any]]]:
    code, raw, parsed = request_json(opener, method, path, body=body, token=token)
    if code == expected:
        stats.ok(name, code)
    else:
        stats.bad(name, code, expected, raw)
    return code, parsed


def promote_admin(username: str) -> None:
    cmd = [
        "mysql",
        "-uroot",
        "-p123456",
        "graduate_match",
        "-e",
        f"UPDATE users SET role='admin' WHERE username='{username}';",
    ]
    subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)


def fetch_seed_student() -> Tuple[str, str]:
    row = subprocess.check_output(
        [
            "mysql",
            "-N",
            "-uroot",
            "-p123456",
            "graduate_match",
            "-e",
            "SELECT s.student_no, s.full_name "
            "FROM students s "
            "LEFT JOIN users u ON u.linked_student_id = s.id "
            "WHERE u.id IS NULL "
            "ORDER BY s.id LIMIT 1;",
        ],
        stderr=subprocess.DEVNULL,
    ).decode("utf-8", errors="replace").strip()
    if not row or "\t" not in row:
        raise RuntimeError("failed to fetch seed student")
    student_no, full_name = row.split("\t", 1)
    return student_no, full_name


def main() -> int:
    opener = _build_opener()
    stats = TestStats()
    stamp = int(time.time())

    # Preflight
    health_code, _, _ = request_json(opener, "GET", "/health")
    if health_code != 200:
        print("[ERROR] backend is not ready at http://127.0.0.1:5555")
        print("Please start backend first, or run: ./tools/testing/run_local_tests.sh")
        return 1

    # 1) health
    expect(stats, opener, "health", "GET", "/health", 200)

    # 2) admin setup
    admin_user = f"it_admin_{stamp}"
    admin_email = f"{admin_user}@test.local"
    admin_password = "Demo@123"
    _, reg = expect(
        stats,
        opener,
        "register admin candidate",
        "POST",
        "/auth/register",
        201,
        body={"username": admin_user, "email": admin_email, "password": admin_password},
    )
    if not reg:
        print("admin register failed, stop.")
        return 1
    promote_admin(admin_user)

    _, login = expect(
        stats,
        opener,
        "admin login",
        "POST",
        "/auth/login",
        200,
        body={"account": admin_user, "password": admin_password},
    )
    admin_token = (login or {}).get("data", {}).get("token", "")
    if not admin_token:
        print("admin token missing, stop.")
        return 1

    expect(stats, opener, "admin auth/me", "GET", "/auth/me", 200, token=admin_token)
    expect(stats, opener, "colleges list", "GET", "/colleges?page=1&page_size=5", 200, token=admin_token)

    new_college_code = f"ITC{stamp}"
    expect(
        stats,
        opener,
        "create college",
        "POST",
        "/colleges",
        201,
        body={
            "code": new_college_code,
            "name": f"集成测试学院{new_college_code}",
            "description": "integration smoke",
        },
        token=admin_token,
    )

    expect(
        stats,
        opener,
        "match major-job",
        "POST",
        "/match/major-job",
        200,
        body={"major_id": 1, "job_id": 1, "algorithm_version": "v0.2-major-job"},
        token=admin_token,
    )
    expect(
        stats,
        opener,
        "dashboard major-alignment",
        "GET",
        "/dashboard/major-alignment?alignment_threshold=70&min_students=1",
        200,
        token=admin_token,
    )

    # 3) viewer + submission + admin approval flow
    viewer_user = f"it_viewer_{stamp}"
    viewer_email = f"{viewer_user}@test.local"
    viewer_password = "Demo@123"
    expect(
        stats,
        opener,
        "register viewer",
        "POST",
        "/auth/register",
        201,
        body={"username": viewer_user, "email": viewer_email, "password": viewer_password},
    )
    _, viewer_login = expect(
        stats,
        opener,
        "viewer login",
        "POST",
        "/auth/login",
        200,
        body={"account": viewer_user, "password": viewer_password},
    )
    viewer_token = (viewer_login or {}).get("data", {}).get("token", "")
    if not viewer_token:
        print("viewer token missing, stop.")
        return 1

    student_no, full_name = fetch_seed_student()
    expect(
        stats,
        opener,
        "bind student profile",
        "POST",
        "/me/student-profile/bind",
        200,
        body={"student_no": student_no, "full_name": full_name},
        token=viewer_token,
    )

    _, sub = expect(
        stats,
        opener,
        "submit employment",
        "POST",
        "/me/employment-submissions",
        201,
        body={
            "proposed_company_name": f"测试公司{stamp}",
            "proposed_company_main_business": "工业软件",
            "proposed_company_flagship_products": "平台A",
            "proposed_company_domain_area": "工业互联网",
            "proposed_company_application_industries": "制造业",
            "proposed_job_title": "测试岗位",
            "proposed_job_responsibilities_text": "负责系统开发与测试",
            "proposed_job_technical_projects_text": "参与后端服务优化",
            "proposed_job_required_knowledge_skills_text": "C++/数据库",
            "proposed_job_required_tools_platform_text": "Linux/Git",
            "employment_type": "signed",
            "employment_status": "active",
            "evidence_source": "offer",
            "notes": "integration smoke",
        },
        token=viewer_token,
    )
    submission_id = (sub or {}).get("data", {}).get("id")

    expect(
        stats,
        opener,
        "admin list submissions",
        "GET",
        "/admin/employment-submissions?page=1&page_size=5&status=pending",
        200,
        token=admin_token,
    )
    if submission_id:
        expect(
            stats,
            opener,
            "admin approve submission",
            "POST",
            f"/admin/employment-submissions/{submission_id}/approve",
            200,
            body={"review_comment": "integration smoke approved"},
            token=admin_token,
        )
    else:
        stats.failed += 1
        print("[FAIL] admin approve submission (missing submission_id)")

    expect(
        stats,
        opener,
        "system logs list",
        "GET",
        "/system-logs?page=1&page_size=5",
        200,
        token=admin_token,
    )

    print("==== SUMMARY ====")
    print("PASS =", stats.passed)
    print("FAIL =", stats.failed)
    return 0 if stats.failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
