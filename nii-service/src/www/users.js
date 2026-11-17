document.addEventListener('DOMContentLoaded', () => {
    fetch("users.html", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: new URLSearchParams({ "command": "sel_me", "app": "../../build/s21_cli.app", "app_arg": "w" }),
    })
        .then(function (r) {
            return r.text();
        })
        .then(function (text) {
            let user = JSON.parse(text)[0];
            if (user != undefined) {
                document.querySelector('.username').innerText = user.name;
                document.querySelector('.userlevel').innerText = user.level;
            }
        })
        .catch(function (err) {
            console.log("Ошибка: " + err);
        });
});

document.addEventListener("DOMContentLoaded", function () {
    var forms = document.querySelectorAll('form');
    if (!forms.length) return;

    var output = document.getElementById("data");

    for (var i = 0; i < forms.length; ++i) {
        let form = forms[i];
        form.addEventListener("submit", function (e) {
            e.preventDefault();

            fetch("users.html", {
                method: "POST",
                headers: { "Content-Type": "application/x-www-form-urlencoded" },
                body: new URLSearchParams(new FormData(form)),
            })
                .then(function (r) {
                    return r.text();
                })
                .then(function (text) {
                    if (!output) return;
                    output.innerHTML = toTable(text);
                })
                .catch(function (err) {
                    if (output) output.textContent = "Ошибка: " + err;
                });
        });

    }
});

function toTable(text) {
    var records = JSON.parse(text);
    var html =
        "<table><tr><th>Имя</th><th>Доступ</th></tr>";

    for (var i = 0; i < records.length; i++) {
        var r = records[i];
        if (r.message) {
            html +=
                "<tr><td colspan=\"2\">" + escapeHtml(r.message) + "</td></tr>";
        } else {
            html +=
                "<tr><td>" +
                escapeHtml(r.name) +
                "</td><td>" +
                escapeHtml(r.level) +
                "</td><tr>";
        }
    }

    return html + "</table>";
}

function escapeHtml(text) {
    return String(text)
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;");
}
