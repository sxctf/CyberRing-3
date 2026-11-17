#!/bin/sh

./docker_stop.sh

# Создаём папку на хосте от текущего пользователя ДО запуска контейнера
umask 0022
mkdir -p $(pwd)/../data

# Сборка образа с вашим UID/GID
docker build \
  --build-arg USER_ID=$(id -u) \
  --build-arg GROUP_ID=$(id -g) \
  -t nii_service .

# Запуск контейнера
docker run \
    -v $(pwd)/../data:/../data \
    -d -p 8888:8080 \
    --name nii_service \
    --restart unless-stopped \
    nii_service
