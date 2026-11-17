# Используем легкий и стабильный образ Debian
FROM debian:bookworm-slim

# Объявляем аргумент (по умолчанию 1000, если не передан)
ARG USER_ID=1000
ARG GROUP_ID=1000

# Устанавливаем компилятор GCC, Make и стандартные библиотеки C
RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc \
    make \
    libc6-dev \
    && rm -rf /var/lib/apt/lists/*

# Создаём группу и пользователя с переданными UID/GID
RUN groupadd -g ${GROUP_ID} appuser || true && \
    useradd -m -u ${USER_ID} -g ${GROUP_ID} appuser || true

# Создаём рабочие папки и отдаём их пользователю
RUN mkdir -p /app /data /build && \
    chown -R appuser:appuser /app /data /build

# Переключаемся на пользователя
USER appuser

# Создаем рабочую директорию внутри контейнера
WORKDIR /app

# Копируем исходники (они принадлежат appuser)
COPY --chown=appuser:appuser ./src .

# Указываем, что контейнер будет использовать порт 8080
EXPOSE 8080

# Огранизаторы устанавливают уникальный пароль чекера для каждой команды (конкурсантам не менять!)
ENV CHECKER_PSW=checker

# Компиляция и запуск произойдут одновременно при старте контейнера
CMD make start
