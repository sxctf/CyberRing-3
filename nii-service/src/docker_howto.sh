# Утановить атрибуты на исполнение
chmod +x docker_start.sh
chmod +x docker_stop.sh
chmod +x reset_data.sh

# Запустить контейнер
./docker_start.sh

# Остановить и удалить контейнер
./docker_stop.sh

# Очистить базу данных и логи сервера
./reset_data.sh



####### Разное #######
# Показать все контейнеры (и запущенные, и остановленные из-за ошибок)
docker ps -a

# Посмотреть логи и ошибки контейнера
docker logs nii_service

# Размер докер образа
docker ps -as --filter "name=nii_service"

# bash в контейнере
docker exec -it nii_service bash
