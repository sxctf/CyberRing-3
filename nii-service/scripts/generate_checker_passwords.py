#!/usr/bin/env python3
"""Generate unique CHECKER_PSW values for nii-service team VMs.

Writes:
  - ansible/vars/checker_passwords.yml  (Ansible vars, control node only)
  - ansible/vars/checker_passwords.csv  (operator list, control node only)

Both live under ansible/ and are excluded from the deploy archive sent to VMs.
"""
from __future__ import annotations

import argparse
import secrets
import string
import stat
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
ROOT = SCRIPT_DIR.parent
VARS_DIR = ROOT / "ansible" / "vars"
YML_PATH = VARS_DIR / "checker_passwords.yml"
CSV_PATH = VARS_DIR / "checker_passwords.csv"

# team-1..team-26 and team-30 (no 27-29)
DEFAULT_TEAMS = list(range(1, 27)) + [30]
ALPHABET = string.ascii_letters + string.digits + "@%+=-_"


def random_password(length: int = 20) -> str:
    while True:
        password = "".join(secrets.choice(ALPHABET) for _ in range(length))
        if (
            any(c.islower() for c in password)
            and any(c.isupper() for c in password)
            and any(c.isdigit() for c in password)
            and any(c in "@%+=-_" for c in password)
        ):
            return password


def chmod_0600(path: Path) -> None:
    path.chmod(stat.S_IRUSR | stat.S_IWUSR)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--force", action="store_true", help="Overwrite existing files")
    parser.add_argument("--length", type=int, default=20)
    args = parser.parse_args()

    if (YML_PATH.exists() or CSV_PATH.exists()) and not args.force:
        raise SystemExit(
            f"{YML_PATH.name} / {CSV_PATH.name} already exist. Use --force to overwrite."
        )

    VARS_DIR.mkdir(parents=True, exist_ok=True)
    rows: list[tuple[str, str, str]] = []
    seen: set[str] = set()
    for n in DEFAULT_TEAMS:
        host = f"team-{n}"
        ip = f"10.62.{n}.10"
        while True:
            pwd = random_password(args.length)
            if pwd not in seen:
                seen.add(pwd)
                break
        rows.append((host, ip, pwd))

    yml_lines = [
        "---",
        "# Per-VM CHECKER_PSW for nii-service. Used only on Ansible control node.",
        "# Never copy this file to team VMs (ansible/ is excluded from deploy archive).",
        "checker_passwords:",
    ]
    for host, _ip, pwd in rows:
        yml_lines.append(f'  {host}: "{pwd}"')
    YML_PATH.write_text("\n".join(yml_lines) + "\n", encoding="utf-8")

    csv_lines = ["host,ip,fqdn,checker_psw"]
    for host, ip, pwd in rows:
        fqdn = f"team{host.split('-', 1)[1]}.ctflab.local"
        csv_lines.append(f"{host},{ip},{fqdn},{pwd}")
    CSV_PATH.write_text("\n".join(csv_lines) + "\n", encoding="utf-8")

    chmod_0600(YML_PATH)
    chmod_0600(CSV_PATH)
    print(f"Wrote {YML_PATH}")
    print(f"Wrote {CSV_PATH}")
    print(f"Hosts: {len(rows)}")


if __name__ == "__main__":
    main()
