#!/usr/bin/env python3
"""
ThreatOps CTF checker.

Protocol:
  checker.py check <host>
  checker.py put   <host> <flag_id> <flag> <vuln>
  checker.py get   <host> <flag_id> <flag> <vuln>

Success contract required by the local checker harness:
  - stdout contains one empty line
  - stderr contains one empty line
  - exit code is 101

On failure:
  - stdout stays silent
  - stderr contains the diagnostic message
"""

import hashlib
import json
import random
import string
import sys
import time
import traceback
import urllib.error
import urllib.request
from typing import Optional
from urllib.parse import urlencode

OK = 101
CORRUPT = 102
MUMBLE = 103
DOWN = 104
CHECKER_ERROR = 110

TIMEOUT = 10


def _exit(code: int, msg: str = "") -> None:
    if code == OK:
        print("", file=sys.stdout)
        print("", file=sys.stderr)
    elif msg:
        label = {
            OK: "OK",
            CORRUPT: "CORRUPT",
            MUMBLE: "MUMBLE",
            DOWN: "DOWN",
            CHECKER_ERROR: "CHECKER_ERROR",
        }.get(code, str(code))
        print(f"[{label}] {msg}", file=sys.stderr)
    sys.exit(code)


def _rand(n: int = 8) -> str:
    return "".join(random.choices(string.ascii_lowercase + string.digits, k=n))


def _base(host: str) -> str:
    value = host.strip()
    if not value.startswith("http"):
        value = f"http://{value}"
    if ":" not in value.split("//", 1)[-1]:
        value = f"{value}:8888"
    return value


def _request(
    url: str,
    *,
    method: str = "GET",
    data: Optional[bytes] = None,
    headers: Optional[dict[str, str]] = None,
) -> tuple[int, bytes]:
    req = urllib.request.Request(url, data=data, headers=headers or {}, method=method)
    try:
        with urllib.request.urlopen(req, timeout=TIMEOUT) as response:
            return response.status, response.read()
    except urllib.error.HTTPError as exc:
        return exc.code, exc.read()
    except Exception as exc:
        _exit(DOWN, f"{method} {url}: {exc}")


def _get(url: str, token: Optional[str] = None) -> tuple[int, bytes]:
    headers: dict[str, str] = {}
    return _request(url, headers=headers)


def _post(url: str, data: dict, token: Optional[str] = None) -> tuple[int, dict]:
    headers = {"Content-Type": "application/x-www-form-urlencoded"}
    status, body = _request(
        url,
        method="POST",
        data=urlencode(data).encode(),
        headers=headers,
    )
    try:
        return status, json.loads(body or b"{}")
    except Exception:
        return status, {}


def checker_psw_by_host(host: str) -> str:
    pswds = {
        "localhost": "checker",
        "10.62.1.10": "WH9VnS0Df07+5V-ybU6L",
        "10.62.2.10": "VQV5xmhQvqGN+C4cQYM4",
        "10.62.3.10": "zEwrgX%P@1Su_CBNcMut",
        "10.62.4.10": "@LxwuuwDvCNxA3kNbhUY",
        "10.62.5.10": "VpPvXE99R-%%-dzGhVw2",
        "10.62.6.10": "x6jbPbdwNzl-4TLWXhYH",
        "10.62.7.10": "SPaofzV2NpVxYw2uAP_A",
        "10.62.8.10": "8HtJJ+n02rimG_yZ@kVX",
        "10.62.9.10": "lcgcQEbU@tHtmP6JoQ6+",
        "10.62.10.10": "STIL@DyChthYs@bKN=k7",
        "10.62.11.10": "yRuG4b5h_rl=-2T_Xm2M",
        "10.62.12.10": "dFI@9yYSTb%qWkLjoPgV",
        "10.62.13.10": "%Bv7XBuwCVauJFMESnvI",
        "10.62.14.10": "kpM6Lq_Bd4VsNB-4Eyd9",
        "10.62.15.10": "9V44mASA0xuLLpu_km%2",
        "10.62.16.10": "x8Us_iECzYU=F1PZ_J@9",
        "10.62.17.10": "44%slo4JpKbDc0hO_pCt",
        "10.62.18.10": "fW2vy%qoyNKfT19AWTu@",
        "10.62.19.10": "6VUFp@csDpZr2UIxIY8Q",
        "10.62.20.10": "=k0Jqibb%rlyqOM+VebO",
        "10.62.21.10": "cfEegD%hfWQGKKf35e3K",
        "10.62.22.10": "9haCAs+T6jhgx9L7WynE",
        "10.62.23.10": "A_s8Az@BeBAW_-zPE9Q3",
        "10.62.24.10": "SDcQrX=db7+3k1FQV6ye",
        "10.62.25.10": "gqq-Jgu2DbFCV=Py-=OE",
        "10.62.26.10": "wkwP88OxXpHvUvr@j4Ii",
        "10.62.30.10": "v4rCWW2+Imq9lEmjP-xN",
    }
    return pswds.get(host)


def check(host: str) -> None:
    base = _base(host)

    status, data = _post(
        f"{base}/login.html",
        {
            "app": "../../build/s21_cli.app",
            "app_arg": "w",
            "login": "checker",
            "password": checker_psw_by_host(host),
            "command": "exit",
        },
    )
    _get(f"{base}/logout.html")
    if (
        status == 200
        and isinstance(data, dict)
        and data.get("message") == "Enter your query:"
    ):
        _exit(OK, "")
    elif (
        status == 200
        and isinstance(data, dict)
        and data.get("message") == "Error: Login or password incorrect!"
    ):
        _exit(MUMBLE, f"check: HTTP {status} bad password")
    else:
        _exit(MUMBLE, f"check: HTTP {status} bad JSON")


def put(host: str, flag_id: str, flag: str, vuln: int) -> None:
    del flag_id

    base = _base(host)

    if vuln == 1:  # CWE-78
        pswd = hashlib.sha256(flag.encode()).hexdigest()
        status, data = _post(
            f"{base}/",
            {
                "app": "../../build/s21_cli.app",
                "app_arg": "w",
                "login": "checker",
                "password": checker_psw_by_host(host),
                "arg1": flag,
                "arg2": "4",
                "arg3": f"{pswd[12:40]}_kno",
                "command": "add_user",
            },
        )
        _get(f"{base}/logout.html")
        if (
            status == 200
            and isinstance(data, list)
            and data[0].get("message") == "User added"
        ):
            _exit(OK, "")
        elif status == 200 and isinstance(data, list):
            _exit(MUMBLE, f"put vuln=1: HTTP {status} {data[0].get('message', '')}")
        else:
            _exit(MUMBLE, f"put vuln=1: HTTP {status} bad JSON")

    elif vuln == 2:  # CWE-121
        status, data = _post(
            f"{base}/",
            {
                "app": "../../build/s21_cli.app",
                "app_arg": "w",
                "login": "checker",
                "password": checker_psw_by_host(host),
                "arg1": flag,
                "arg2": "4",
                "arg3": _rand(12),
                "command": "insert",
            },
        )
        _get(f"{base}/logout.html")
        if (
            status == 200
            and isinstance(data, list)
            and data[0].get("message") == "Data inserted"
        ):
            _exit(OK, "")
        elif status == 200 and isinstance(data, list):
            _exit(MUMBLE, f"put vuln=2: HTTP {status} {data[0].get('message', '')}")
        else:
            _exit(MUMBLE, f"put vuln=2: HTTP {status} bad JSON")
    else:
        _exit(CHECKER_ERROR, f"put: unknown vuln={vuln}")


def get(host: str, flag_id: str, flag: str, vuln: int) -> None:
    del flag_id

    base = _base(host)

    if vuln == 1:  # CWE-78
        pswd = hashlib.sha256(flag.encode()).hexdigest()
        status, data = _post(
            f"{base}/",
            {
                "app": "../../build/s21_cli.app",
                "app_arg": "w",
                "login": flag,
                "password": f"{pswd[12:40]}_kno",
                "command": "exit",
            },
        )
        _get(f"{base}/logout.html")
        if (
            status == 200
            and isinstance(data, dict)
            and data.get("message") == "Enter your query:"
        ):
            _exit(OK, "")
        else:
            _exit(CORRUPT, f"get vuln=1: HTTP {status} flag not found")

    elif vuln == 2:  # CWE-121
        status, data = _post(
            f"{base}/",
            {
                "app": "../../build/s21_cli.app",
                "app_arg": "w",
                "login": "checker",
                "password": checker_psw_by_host(host),
                "arg1": "name",
                "arg2": flag,
                "command": "select",
            },
        )
        _get(f"{base}/logout.html")
        if status == 200 and isinstance(data, list) and data[0].get("name") == flag:
            _exit(OK, "")
        elif status == 200 and isinstance(data, list):
            _exit(CORRUPT, f"put vuln=2: HTTP {status} {data[0].get('message', '')}")
        else:
            _exit(CORRUPT, f"put vuln=2: HTTP {status} flag not found")
    else:
        _exit(CHECKER_ERROR, f"get: unknown vuln={vuln}")


def main() -> None:
    args = sys.argv[1:]
    if len(args) < 2:
        print(
            "Usage:\n"
            "  checker.py check <host>\n"
            "  checker.py put   <host> <flag_id> <flag> <vuln>\n"
            "  checker.py get   <host> <flag_id> <flag> <vuln>\n",
            file=sys.stderr,
        )
        sys.exit(CHECKER_ERROR)

    action = args[0].lower()
    host = args[1]

    try:
        if action == "check":
            check(host)
        elif action == "put":
            if len(args) < 5:
                _exit(CHECKER_ERROR, "put requires: host flag_id flag vuln")
            put(host, args[2], args[3], int(args[4]))
        elif action == "get":
            if len(args) < 5:
                _exit(CHECKER_ERROR, "get requires: host flag_id flag vuln")
            get(host, args[2], args[3], int(args[4]))
        else:
            _exit(CHECKER_ERROR, f"unknown action: {action!r}")
    except SystemExit:
        raise
    except Exception:
        print(traceback.format_exc(), file=sys.stderr)
        _exit(CHECKER_ERROR, "unhandled exception in checker")


if __name__ == "__main__":
    main()
