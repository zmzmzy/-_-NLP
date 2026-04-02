#!/usr/bin/env python3
"""API negative-path smoke tests.

Focus: validate bad-input/permission paths return controlled 4xx/503, not 500.
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
) -> None:
    code, raw, _ = request_json(opener, method, path, body=body, token=token)
    if code == expected:
        stats.ok(name, code)
    else:
        stats.bad(name, code, expected, raw)


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

    # Prepare one admin token for privileged negative checks.
    admin_user = f"neg_admin_{stamp}"
    admin_email = f"{admin_user}@test.local"
    admin_password = "Demo@123"
    request_json(
        opener,
        "POST",
        "/auth/register",
        body={"username": admin_user, "email": admin_email, "password": admin_password},
    )
    promote_admin(admin_user)
    _, _, login = request_json(
        opener,
        "POST",
        "/auth/login",
        body={"account": admin_user, "password": admin_password},
    )
    admin_token = (login or {}).get("data", {}).get("token", "")
    if not admin_token:
        print("[FAIL] admin setup failed in negative test")
        return 1

    expect(
        stats,
        opener,
        "unauthorized protected route",
        "GET",
        "/system-logs?page=1",
        401,
    )
    expect(
        stats,
        opener,
        "invalid major-job payload type",
        "POST",
        "/match/major-job",
        400,
        body={"major_id": "x", "job_id": 1},
        token=admin_token,
    )
    expect(
        stats,
        opener,
        "empty batch approve payload",
        "POST",
        "/admin/employment-submissions/batch-approve",
        400,
        body={"ids": []},
        token=admin_token,
    )
    expect(
        stats,
        opener,
        "delete college with dependencies",
        "DELETE",
        "/colleges/1",
        409,
        token=admin_token,
    )

    # SMTP may be configured or not; for unknown email both 200/503 are acceptable.
    code, raw, _ = request_json(
        opener,
        "POST",
        "/auth/password/forgot",
        body={"email": "nobody@example.invalid"},
    )
    if code in (200, 503):
        stats.ok("forgot password without guaranteed SMTP", code)
    else:
        stats.bad("forgot password without guaranteed SMTP", code, 200, raw)

    print("==== SUMMARY ====")
    print("PASS =", stats.passed)
    print("FAIL =", stats.failed)
    return 0 if stats.failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
