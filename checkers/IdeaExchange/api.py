from gornilo.http_clients import requests_with_retries
from requests import Response


class Api:
    def __init__(self, host, port=18888):
        self.base_url = f'http://{host}:{port}'
        self.session = requests_with_retries(status_forcelist=(500, 502), default_timeout=15)

    def front(self):
        return self.session.get(f"{self.base_url}")

    def register(self, username: str, password: str) -> Response:
        return self.session.post(f"{self.base_url}/api/register", data={"username": username, "password": password})

    def login(self, username: str, password: str) -> Response:
        return self.session.post(f"{self.base_url}/api/login", data={"username": username, "password": password})

    def create_idea(self, token: str, status: str, title: str, category: str, text: str) -> Response:
        return self.session.post(f"{self.base_url}/api/ideas",
                                 data={"status": status, "title": title, "category": category, "text": text},
                                 headers={"Authorization": f"Bearer {token}"})

    def get_idea(self, token: str, username: str, title: str) -> Response:
        return self.session.get(f"{self.base_url}/api/ideas", data={"username": username, "title": title},
                                headers={"Authorization": f"Bearer {token}"})

    def list_ideas(self, token: str) -> Response:
        return self.session.get(f"{self.base_url}/api/ideas/list", headers={"Authorization": f"Bearer {token}"})

    def category_ideas(self, token: str, category: str) -> Response:
        return self.session.get(f"{self.base_url}/api/ideas/list/{category}",
                                headers={"Authorization": f"Bearer {token}"})

    def rating(self, token: str, sort: str = "count", page: int = 0) -> Response:
        return self.session.get(f"{self.base_url}/api/ideas/rating", params={"sort": sort, "page": page},
                                headers={"Authorization": f"Bearer {token}"})

    def friends_list(self, token: str) -> Response:
        return self.session.get(f"{self.base_url}/api/friends", headers={"Authorization": f"Bearer {token}"})

    def requests_list(self, token: str) -> Response:
        return self.session.get(f"{self.base_url}/api/friends/requests", headers={"Authorization": f"Bearer {token}"})

    def friend_request(self, token: str, to: str) -> Response:
        return self.session.post(f"{self.base_url}/api/friends/request", data={"to": to},
                                 headers={"Authorization": f"Bearer {token}"})

    def accept_request(self, token: str, fr: str) -> Response:
        return self.session.post(f"{self.base_url}/api/friends/accept", data={"from": fr},
                                 headers={"Authorization": f"Bearer {token}"})
