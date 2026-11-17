#!/usr/bin/env python3
"""
Checker for Предприятие 3826 service.
Verifies basic functionality: registration, login, request creation, request retrieval.

Usage:
    python3 checker.py check  <host>
    python3 checker.py put    <host> <flag_id> <flag> <vuln>
    python3 checker.py get    <host> <flag_id> <flag> <vuln>
"""

import sys
import json
import random
import string
import requests

PORT = 3000
TIMEOUT = 10


def rnd_string(n=12):
    return ''.join(random.choices(string.ascii_lowercase + string.digits, k=n))


def base_url(host):
    return f"http://{host}:{PORT}"


def api(host, method, path, session=None, **kwargs):
    s = session or requests.Session()
    url = f"{base_url(host)}{path}"
    kwargs.setdefault('timeout', TIMEOUT)
    resp = getattr(s, method)(url, **kwargs)
    return resp


# ──────────────────────────── helpers ────────────────────────────

def do_register(host, session, username, password):
    r = api(host, 'post', '/api/register', session=session,
            json={'username': username, 'password': password})
    return r.status_code == 200 and r.json().get('success')


def do_login(host, session, username, password):
    r = api(host, 'post', '/api/login', session=session,
            json={'username': username, 'password': password})
    return r.status_code == 200 and r.json().get('success')


def do_create_request(host, session, content):
    r = api(host, 'post', '/api/requests', session=session,
            json={'content': content})
    return r.status_code == 200 and r.json().get('success')


def do_get_my_requests(host, session):
    r = api(host, 'get', '/api/requests', session=session)
    if r.status_code != 200:
        return None
    return r.json().get('requests', [])


def do_get_me(host, session):
    r = api(host, 'get', '/api/me', session=session)
    if r.status_code != 200:
        return None
    return r.json()


def do_logout(host, session):
    r = api(host, 'post', '/api/logout', session=session)
    return r.status_code == 200


# ──────────────────────────── actions ────────────────────────────

def check(host):
    """Verify that the service is alive and core endpoints work."""
    s = requests.Session()

    # 1. Main page is reachable
    r = api(host, 'get', '/', session=s)
    assert r.status_code == 200, f"GET / returned {r.status_code}"

    # 2. /api/me returns unauthenticated
    me = do_get_me(host, s)
    assert me is not None, "/api/me unreachable"
    assert me.get('authenticated') is False, "/api/me should be unauthenticated"

    # 3. Register + login round-trip
    username = f"check_{rnd_string(8)}"
    password = rnd_string(16)

    ok = do_register(host, s, username, password)
    assert ok, "Registration failed"

    me = do_get_me(host, s)
    assert me and me.get('authenticated') and me.get('username') == username, \
        "Session not established after register"

    # 4. Logout
    do_logout(host, s)
    me = do_get_me(host, s)
    assert me and me.get('authenticated') is False, "Logout did not clear session"

    # 5. Re-login
    ok = do_login(host, s, username, password)
    assert ok, "Login failed after register"

    # 6. Create a request and verify it appears
    content = f"check_content_{rnd_string(10)}"
    ok = do_create_request(host, s, content)
    assert ok, "Request creation failed"

    reqs = do_get_my_requests(host, s)
    assert reqs is not None, "Could not fetch requests"
    found = any(r.get('content') == content for r in reqs)
    assert found, "Created request not found in list"

    print("OK")
    sys.exit(101)


def put(host, flag_id, flag, vuln):
    """Store flag: register user, create request with flag as content."""
    s = requests.Session()

    username = f"u_{rnd_string(10)}"
    password = rnd_string(16)

    ok = do_register(host, s, username, password)
    if not ok:
        print("Registration failed", file=sys.stderr)
        sys.exit(104)

    ok = do_create_request(host, s, flag)
    if not ok:
        print("Request creation failed", file=sys.stderr)
        sys.exit(104)

    # flag_id — данные для последующего get: username:password
    new_flag_id = json.dumps({'u': username, 'p': password})
    print(new_flag_id)
    sys.exit(101)


def get(host, flag_id, flag, vuln):
    """Retrieve flag: login as user, find the flag in their requests."""
    s = requests.Session()

    try:
        creds = json.loads(flag_id)
        username = creds['u']
        password = creds['p']
    except (json.JSONDecodeError, KeyError):
        print("Invalid flag_id format", file=sys.stderr)
        sys.exit(104)

    ok = do_login(host, s, username, password)
    if not ok:
        print("Login failed", file=sys.stderr)
        sys.exit(104)

    reqs = do_get_my_requests(host, s)
    if reqs is None:
        print("Could not fetch requests", file=sys.stderr)
        sys.exit(104)

    found = any(r.get('content') == flag for r in reqs)
    if not found:
        print("Flag not found in requests", file=sys.stderr)
        sys.exit(102)

    print("OK")
    sys.exit(101)


# ──────────────────────────── main ────────────────────────────

EXIT_CODES = {
    101: 'OK',
    102: 'CORRUPT',
    103: 'MUMBLE',
    104: 'DOWN',
}

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <check|put|get> <host> [flag_id] [flag] [vuln]")
        sys.exit(1)

    action = sys.argv[1].lower()
    host = sys.argv[2]

    try:
        if action == 'check':
            check(host)
        elif action == 'put':
            flag_id = sys.argv[3] if len(sys.argv) > 3 else ''
            flag = sys.argv[4] if len(sys.argv) > 4 else ''
            vuln = sys.argv[5] if len(sys.argv) > 5 else '1'
            put(host, flag_id, flag, vuln)
        elif action == 'get':
            flag_id = sys.argv[3]
            flag = sys.argv[4]
            vuln = sys.argv[5] if len(sys.argv) > 5 else '1'
            get(host, flag_id, flag, vuln)
        else:
            print(f"Unknown action: {action}", file=sys.stderr)
            sys.exit(1)
    except requests.exceptions.ConnectionError:
        print("Connection error", file=sys.stderr)
        sys.exit(104)
    except AssertionError as e:
        print(f"Check failed: {e}", file=sys.stderr)
        sys.exit(103)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(104)
