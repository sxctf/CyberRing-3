#!/usr/bin/env python3.11
import random
from string import ascii_letters

import requests.exceptions
from gornilo import Checker, PutRequest, GetRequest, CheckRequest, Verdict

from api import Api
from consts import users, ideas

import logging

PORT = 18888

checker = Checker()


def get_username() -> str:
    return random.choice(users) + get_random_string(5)


def get_idea() -> tuple[str, str, str]:
    idea = random.choice(ideas)
    category = idea["category"]
    title = category + get_random_string(6)
    text = idea["text"]
    return title, category, text


def get_random_string(n: int) -> str:
    return ''.join(random.choice(ascii_letters) for _ in range(n))


def _rating_entries_ok(data) -> bool:
    if not isinstance(data, list):
        return False
    for entry in data:
        if not isinstance(entry, dict):
            return False
        for key in ("username", "ideas_count", "last_published_at", "last_title"):
            if key not in entry:
                return False
                                                                              
                                                                         
        if "text" in entry:
            return False
    return True


@checker.define_check
def check(req: CheckRequest) -> Verdict:
    try:
        api = Api(req.hostname, PORT)

        resp = api.front()
        if resp.status_code != 200:
            return Verdict.MUMBLE("something went wrong with frontend")

        username, password = get_username(), get_random_string(20)

        resp = api.register(username, password)
        if resp.status_code != 200:
            return Verdict.MUMBLE("something went wrong with register")

        token = resp.json().get("token", "")
        if token == "":
            return Verdict.MUMBLE("no token on register handler")

        resp = api.register(username, get_random_string(20))
        if resp.status_code != 400:
            return Verdict.MUMBLE("something went wrong with register user that already exist")

        resp = api.login(username, password)
        if resp.status_code != 200:
            return Verdict.MUMBLE("something went wrong with login")

        token = resp.json().get("token", "")
        if token == "":
            return Verdict.MUMBLE("no token on login handler")

        resp = api.login(get_random_string(20), password)
        if resp.status_code != 400:
            return Verdict.MUMBLE("something went wrong with login user that not exists")

        resp = api.login(username, get_random_string(20))
        if resp.status_code != 403:
            return Verdict.MUMBLE("something went wrong with login with invalid password")

        status = get_random_string(5)
        title, category, text = get_idea()

        resp = api.create_idea(token, status, title, category, text)
        if resp.status_code != 400:
            return Verdict.MUMBLE("something went wrong with create idea with invalid status")

        status = "private"

        resp = api.create_idea(token, status, title, category, text)
        if resp.status_code != 201:
            return Verdict.MUMBLE("something went wrong with create idea")

        resp = api.create_idea(token, status, title, category, text)
        if resp.status_code != 400:
            return Verdict.MUMBLE("something went wrong with create idea that already exists")

        resp = api.get_idea(token, username, title)
        if resp.status_code != 200:
            return Verdict.MUMBLE("something went wrong with get idea")

        data = resp.json()
        if "status" not in data or "title" not in data or "text" not in data:
            return Verdict.MUMBLE("missing fields when get idea")

        resp = api.get_idea(token, username, get_random_string(20))
        if resp.status_code != 404:
            return Verdict.MUMBLE("something went wrong with get non-existence idea")

        title2, category2, text2 = get_idea()

        status = "public"

        resp = api.create_idea(token, status, title2, category2, text2)
        if resp.status_code != 201:
            return Verdict.MUMBLE("something went wrong with create another idea")

        resp = api.get_idea(token, username, title2)
        if resp.status_code != 200:
            return Verdict.MUMBLE("something went wrong with get idea")

        data = resp.json()
        if "status" not in data or "title" not in data or "text" not in data:
            return Verdict.MUMBLE("missing fields when get idea")

        resp = api.list_ideas(token)
        if resp.status_code != 200:
            return Verdict.MUMBLE("something went wrong when get ideas list")

        data = resp.json().get("data", [])
        for idea in data:
            if not ((idea.get("title", "") == title and idea.get("text", "") == text)
                    or (idea.get("title", "") == title2 and idea.get("text", "") == text2)):
                return Verdict.MUMBLE("invalid data in ideas list")

                                                                              
                             
        resp = api.rating(token, sort="count", page=0)
        if resp.status_code != 200:
            return Verdict.MUMBLE("something went wrong with rating (count)")
        if not _rating_entries_ok(resp.json().get("data", [])):
            return Verdict.MUMBLE("invalid data in rating (count)")

        resp = api.rating(token, sort="recent", page=0)
        if resp.status_code != 200:
            return Verdict.MUMBLE("something went wrong with rating (recent)")
        if not _rating_entries_ok(resp.json().get("data", [])):
            return Verdict.MUMBLE("invalid data in rating (recent)")

        username2, password2 = get_username(), get_random_string(20)

        resp = api.register(username2, password2)
        if resp.status_code != 200:
            return Verdict.MUMBLE("something went wrong with register")

        token2 = resp.json().get("token", "")
        if token2 == "":
            return Verdict.MUMBLE("no token on register handler")

        status = "private"

        resp = api.create_idea(token2, status, title, category, text)
        if resp.status_code != 201:
            return Verdict.MUMBLE("something went wrong with create idea (user 2)")

        resp = api.create_idea(token2, status, title2, category2, text2)
        if resp.status_code != 201:
            return Verdict.MUMBLE("something went wrong with create idea (user 2)")

        resp = api.friend_request(token, username2)
        if resp.status_code != 200:
            return Verdict.MUMBLE("something went wrong with friend request")

        resp = api.friend_request(token, get_random_string(20))
        if resp.status_code != 400:
            return Verdict.MUMBLE("something went wrong with friend request when user is not exists")

        resp = api.requests_list(token2)
        if resp.status_code != 200:
            return Verdict.MUMBLE("something went wrong with requests list")

        data = resp.json().get("data", [])
        if len(data) < 1 or data[0] != username:
            return Verdict.MUMBLE("invalid data in requests list")

        resp = api.accept_request(token2, username)
        if resp.status_code != 200:
            return Verdict.MUMBLE("something went wrong with accept request")

        resp = api.accept_request(token2, get_random_string(20))
        if resp.status_code != 400:
            return Verdict.MUMBLE("something went wrong with accept request when user not exists")

        resp = api.friends_list(token)
        if resp.status_code != 200:
            return Verdict.MUMBLE("something went wrong with friends list")

        data = resp.json().get("data", [])
        if len(data) < 1 or data[0] != username2:
            return Verdict.MUMBLE("invalid data in friends list")

        resp = api.friends_list(token2)
        if resp.status_code != 200:
            return Verdict.MUMBLE("something went wrong with friends list")

        data = resp.json().get("data", [])
        if len(data) < 1 or data[0] != username:
            return Verdict.MUMBLE("invalid data in friends list")

        resp = api.get_idea(token2, username, title)
        if resp.status_code != 200:
            return Verdict.MUMBLE("something went wrong with get idea (from user 2)")

        data = resp.json()
        if "status" not in data or "title" not in data or "text" not in data \
                or data["title"] != title or data["text"] != text:
            return Verdict.MUMBLE("missing fields when get idea")

    except requests.JSONDecodeError:
        return Verdict.MUMBLE("invalid json")
    except requests.ConnectionError:
        return Verdict.DOWN("connection error")
    except requests.Timeout:
        return Verdict.DOWN("timeout error")
    except requests.RequestException:
        return Verdict.MUMBLE("http error")
    return Verdict.OK()


@checker.define_put(vuln_num=1, vuln_rate=1)
def put(req: PutRequest) -> Verdict:
    try:
        api = Api(req.hostname, PORT)

        username, password = get_username(), get_random_string(20)

        resp = api.register(username, password)
        if resp.status_code != 200:
            return Verdict.MUMBLE("something went wrong with register")

        token = resp.json().get("token", "")
        if token == "":
            return Verdict.MUMBLE("no token on register handler")

        resp = api.login(username, password)
        if resp.status_code != 200:
            return Verdict.MUMBLE("something went wrong with login")

        token = resp.json().get("token", "")
        if token == "":
            return Verdict.MUMBLE("no token on login handler")

        status = "private"
        title, category, _ = get_idea()

        resp = api.create_idea(token, status, title, category, req.flag)
        if resp.status_code != 201:
            return Verdict.MUMBLE("something went wrong with create idea")

        resp = api.get_idea(token, username, title)
        if resp.status_code != 200:
            return Verdict.MUMBLE("something went wrong with get idea")

        data = resp.json()
        if "status" not in data or "title" not in data or "text" not in data:
            return Verdict.MUMBLE("missing fields when get idea")
    except requests.JSONDecodeError:
        return Verdict.MUMBLE("invalid json")
    except requests.ConnectionError:
        return Verdict.DOWN("connection error")
    except requests.Timeout:
        return Verdict.DOWN("timeout error")
    except requests.RequestException:
        return Verdict.MUMBLE("http error")
    return Verdict.OK_WITH_FLAG_ID(f"{username}+{title}", token)


@checker.define_get(vuln_num=1)
def get(req: GetRequest) -> Verdict:
    try:
        api = Api(req.hostname, PORT)

        token = req.flag_id

        username, title = req.public_flag_id.split('+')[:2]

        resp = api.get_idea(token, username, title)
        if resp.status_code != 200:
            return Verdict.MUMBLE("something went wrong with get idea")

        data = resp.json()
        if "text" not in data or data["text"] != req.flag:
            return Verdict.CORRUPT("wrong text")
    except requests.JSONDecodeError:
        return Verdict.MUMBLE("invalid json")
    except requests.ConnectionError:
        return Verdict.DOWN("connection error")
    except requests.Timeout:
        return Verdict.DOWN("timeout error")
    except requests.RequestException:
        return Verdict.MUMBLE("http error")
    return Verdict.OK()


if __name__ == '__main__':
    if __name__ == '__main__':
        logging.getLogger().setLevel(logging.WARNING)
        logging.getLogger('urllib3').setLevel(logging.WARNING)

        checker.run()
