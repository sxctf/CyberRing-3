#!/usr/bin/env python3
"""
ForcAD-native checker for the "museumorphosis" service.

Полностью нативный ForcAD/checklib-чекер (без gornilo).

  * коды выхода из checklib.Status (OK=101, CORRUPT=102, MUMBLE=103,
    DOWN=104, ERROR=110);
  * PUT публикует публичный flag_id (museum_id/exhibit_id) в stdout как
    attack-data, а приватное состояние (нужное GET-у: токен и пароль музея) —
    в stderr;
  * рекомендуемый тип таски в конфиге: `checker_type: pfr` (public flag data
    returned) — тогда GET получает приватное состояние из stderr. Без `pfr`
    ForcAD отдаёт в GET публичный flag_id из stdout, поэтому пароль музея
    выводится из флага (см. `_password_from_flag`) и GET работает в обоих
    режимах.

CLI (драйвит ForcAD): checker.py <action> <host> [<flag_id> <flag> <vuln>]
"""
import base64
import hashlib
import hmac
import json
import os
import random
import sys
import traceback
from uuid import uuid4

import requests
from checklib import BaseChecker, Status, cquit, rnd_string

PORT = int(os.getenv("SERVICE_PORT", os.getenv("CHECKER_PORT", "6969")))

# Секрет чекера: пароль музея выводится из флага детерминированно, поэтому GET
# может перелогиниться, даже если получил только публичный flag_id (museum_id +
# exhibit_id) — т.е. когда таска в ForcAD настроена без `checker_type: pfr`.
# Секрет живёт только на чексистеме, атакующим командам он недоступен.
_SECRET = os.getenv("CHECKER_SECRET", "museumorphosis/v1/8f2c1e0a").encode()

# Данные для генерации реалистичных музеев/экспонатов.
_DATA_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data.json")
try:
    with open(_DATA_PATH, encoding="utf-8") as _f:
        DATASET = json.load(_f)
except Exception:
    DATASET = [{
        "museum": "Fallback Museum",
        "exhibitName": "Fallback Exhibit",
        "exhibitDescription": "Fallback description.",
    }]


def _id_from_token(token: str) -> str:
    """Достаём id из JWT без проверки подписи (payload = 2-я часть)."""
    try:
        payload_b64 = token.split(".")[1]
        payload_b64 += "=" * (-len(payload_b64) % 4)  # padding
        payload = json.loads(base64.urlsafe_b64decode(payload_b64))
        return payload.get("id", "")
    except Exception:
        return ""


def _password_from_flag(flag: str) -> str:
    """Детерминированный пароль музея, восстановимый в GET из одного флага."""
    return hmac.new(_SECRET, flag.encode(), hashlib.sha256).hexdigest()


class Checker(BaseChecker):
    vulns: int = 1
    timeout: int = 15
    uses_attack_data: bool = True

    def base(self):
        return f"http://{self.host}:{PORT}/api/"

    # --- helpers -----------------------------------------------------------
    def _post(self, sess, path, json_data=None, token=None, expected=200,
              down_status=Status.MUMBLE, what=None):
        what = what or path
        headers = {"token": token} if token else None
        try:
            r = sess.post(self.base() + path, json=json_data,
                          headers=headers, timeout=self.timeout)
        except (requests.ConnectionError, requests.Timeout):
            self.cquit(Status.DOWN, f"Connection error on {what}",
                       traceback.format_exc())
        if r.status_code != expected:
            self.cquit(down_status, f"Bad status on {what}",
                       f"POST {path} -> {r.status_code}: {r.text[:200]}")
        return r

    def _get(self, sess, path, token=None, expected=200,
             down_status=Status.MUMBLE, what=None):
        what = what or path
        headers = {"token": token} if token else None
        try:
            r = sess.get(self.base() + path, headers=headers, timeout=self.timeout)
        except (requests.ConnectionError, requests.Timeout):
            self.cquit(Status.DOWN, f"Connection error on {what}",
                       traceback.format_exc())
        if r.status_code != expected:
            self.cquit(down_status, f"Bad status on {what}",
                       f"GET {path} -> {r.status_code}: {r.text[:200]}")
        return r

    def _json(self, r, what, down_status=Status.MUMBLE):
        try:
            return r.json()
        except Exception:
            self.cquit(down_status, f"Invalid JSON on {what}",
                       f"{what}: {r.text[:200]}")

    def _register(self, sess, name, password, down_status=Status.MUMBLE):
        r = self._post(sess, "register", {"name": name, "password": password},
                       down_status=down_status, what="/register")
        token = self._json(r, "/register", down_status).get("token", "")
        if not token:
            self.cquit(down_status, "No token on /register",
                       f"/register answer has no token: {r.text[:200]}")
        return token

    def _create_exhibit(self, sess, token, title, description, metadata,
                        down_status=Status.MUMBLE):
        r = self._post(sess, "museum/exhibits",
                       {"title": title, "description": description,
                        "metadata": metadata},
                       token=token, down_status=down_status,
                       what="/museum/exhibits (create)")
        return self._json(r, "/museum/exhibits (create)", down_status)

    @staticmethod
    def _rnd_creds():
        elem = random.choice(DATASET)
        return (elem["museum"], str(uuid4()) + str(uuid4()),
                elem["exhibitName"], elem["exhibitDescription"])

    # --- actions -----------------------------------------------------------
    def check(self):
        sess = self.get_initialized_session()
        museum_name, password, title, description = self._rnd_creds()

        # register
        token = self._register(sess, museum_name, password)
        museum_id = _id_from_token(token)
        if not museum_id:
            self.cquit(Status.MUMBLE, "Bad token from /register",
                       f"can't extract id from token: {token[:64]}")

        # login
        r = self._post(sess, "login", {"id": museum_id, "password": password},
                       what="/login")
        if not self._json(r, "/login").get("token"):
            self.cquit(Status.MUMBLE, "No token on /login",
                       f"/login answer has no token: {r.text[:200]}")

        # get museum
        museum = self._json(self._get(sess, f"museums/{museum_id}",
                                      what="/museums/{id}"), "/museums/{id}")
        if museum.get("id") != museum_id or museum.get("name") != museum_name:
            self.cquit(Status.MUMBLE, "Museum data mismatch",
                       f"expected id={museum_id} name={museum_name!r}, got {museum}")

        # museums list
        museums = self._json(self._get(sess, "museums", what="/museums"), "/museums")
        if not museums.get("museum_id_list"):
            self.cquit(Status.MUMBLE, "Empty museums list",
                       f"/museums -> {museums}")

        # create exhibit
        metadata = rnd_string(16)
        created = self._create_exhibit(sess, token, title, description, metadata)
        exhibit_id = created.get("id", "")
        if not self._exhibit_ok(created, exhibit_id, title, description, metadata):
            self.cquit(Status.MUMBLE, "Created exhibit mismatch",
                       f"got {created}")

        # get exhibit
        got = self._json(self._get(sess, f"museum/exhibits/{exhibit_id}",
                                   token=token, what="/museum/exhibits/{id}"),
                         "/museum/exhibits/{id}")
        if not self._exhibit_ok(got, exhibit_id, title, description, metadata):
            self.cquit(Status.MUMBLE, "Fetched exhibit mismatch",
                       f"got {got}")

        # search / list
        listed = self._json(self._get(sess, "museum/exhibits?search=",
                                      token=token, what="/museum/exhibits?search"),
                            "/museum/exhibits?search")
        if "exhibits" not in listed or not isinstance(listed["exhibits"], list):
            self.cquit(Status.MUMBLE, "Bad exhibits list",
                       f"/museum/exhibits -> {listed}")
        if not any(self._exhibit_ok(e, exhibit_id, title, description, metadata)
                   for e in listed["exhibits"]):
            self.cquit(Status.MUMBLE, "Exhibit not in list",
                       f"exhibit {exhibit_id} not found in list of "
                       f"{len(listed['exhibits'])}")

        self.cquit(Status.OK)

    @staticmethod
    def _exhibit_ok(obj, exhibit_id, title, description, metadata):
        return (obj.get("id") == exhibit_id and
                obj.get("title") == title and
                obj.get("description") == description and
                obj.get("metadata") == metadata)

    def put(self, flag_id, flag, vuln):
        sess = self.get_initialized_session()
        museum_name, _, title, description = self._rnd_creds()
        password = _password_from_flag(flag)

        token = self._register(sess, museum_name, password)
        museum_id = _id_from_token(token)
        if not museum_id:
            self.cquit(Status.MUMBLE, "Bad token from /register",
                       f"can't extract id from token: {token[:64]}")

        # флаг кладём в metadata экспоната
        created = self._create_exhibit(sess, token, title, description, flag)
        exhibit_id = created.get("id", "")
        if not self._exhibit_ok(created, exhibit_id, title, description, flag):
            self.cquit(Status.MUMBLE, "Created exhibit mismatch",
                       f"got {created}")

        # приватное состояние (вернётся в GET как flag_id)
        private = json.dumps({
            "museum_id": museum_id,
            "exhibit_id": exhibit_id,
            "password": password,
            "token": token,
        })
        # публичные attack-data (видны атакующим командам)
        public = json.dumps({"museum_id": museum_id, "exhibit_id": exhibit_id})

        self.cquit(Status.OK, public, private)

    def get(self, flag_id, flag, vuln):
        try:
            state = json.loads(flag_id)
        except Exception:
            self.cquit(Status.ERROR, "Bad flag_id", traceback.format_exc())

        if not isinstance(state, dict) or not state.get("exhibit_id") \
                or not state.get("museum_id"):
            self.cquit(Status.ERROR, "Bad flag_id",
                       f"flag_id has no museum_id/exhibit_id: {flag_id[:200]}")

        sess = self.get_initialized_session()
        token = state.get("token")

        # Если токена нет (таска без `checker_type: pfr` — GET получает только
        # публичный flag_id) либо он протух — логинимся по паролю от флага.
        if not token:
            password = state.get("password") or _password_from_flag(flag)
            r = self._post(sess, "login",
                           {"id": state["museum_id"], "password": password},
                           down_status=Status.CORRUPT, what="/login")
            token = self._json(r, "/login", Status.CORRUPT).get("token")
            if not token:
                self.cquit(Status.CORRUPT, "Re-login failed",
                           f"/login -> {r.text[:200]}")

        r = self._get(sess, f"museum/exhibits/{state['exhibit_id']}",
                      token=token, down_status=Status.CORRUPT,
                      what="/museum/exhibits/{id}")
        exhibit = self._json(r, "/museum/exhibits/{id}", Status.CORRUPT)

        actual = exhibit.get("metadata", "")
        if actual != flag:
            self.cquit(Status.CORRUPT, "Flag is missing!",
                       f"expected {flag[:64]!r}, got {actual[:64]!r}")

        self.cquit(Status.OK)


def _dispatch(checker, action, args):
    # Регистронезависимо: ForcAD зовёт CHECK/PUT/GET, локально удобно check/put/get.
    action = action.upper()
    if action == "CHECK":
        checker.check()
    elif action == "PUT":
        checker.put(*args)          # flag_id flag vuln
    elif action == "GET":
        checker.get(*args)          # flag_id flag vuln
    elif action == "INFO":
        cquit(Status.OK, json.dumps({"vulns": Checker.vulns,
                                     "timeout": Checker.timeout,
                                     "attack_data": Checker.uses_attack_data}))
    else:
        cquit(Status.ERROR, "Checker failed", f"Invalid action: {action}")


if __name__ == "__main__":
    action, host = sys.argv[1], sys.argv[2]
    checker = Checker(host)
    try:
        _dispatch(checker, action, sys.argv[3:])
    except checker.get_check_finished_exception():
        cquit(Status(checker.status), checker.public, checker.private)
    except (requests.exceptions.ConnectionError,
            requests.exceptions.Timeout,
            requests.exceptions.RetryError):
        cquit(Status.DOWN, "Connection error", traceback.format_exc())
    except Exception:
        cquit(Status.ERROR, "Checker error", traceback.format_exc())
