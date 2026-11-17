document.addEventListener("DOMContentLoaded", function () {
    var forms = document.querySelectorAll('form');
    if (!forms.length) return;

    var output = document.getElementById("data");

    for (var i = 0; i < forms.length; ++i) {
        let form = forms[i];
        form.addEventListener("submit", function (e) {
            e.preventDefault();

            fetch("data.html", {
                method: "POST",
                headers: { "Content-Type": "application/x-www-form-urlencoded" },
                body: new URLSearchParams(new FormData(form)),
            })
                .then(function (r) {
                    return r.text();
                })
                .then(function (text) {
                    if (!output) return;
                    let msg = JSON.parse(text.slice(0, text.indexOf("}") + 1)).message;
                    if (msg === "Enter your query:") {
                        location.href = "/data.html";
                    } else {
                        output.innerHTML = msg;
                    }
                })
                .catch(function (err) {
                    if (output) output.textContent = "Ошибка: " + err;
                });
        });

    }
});