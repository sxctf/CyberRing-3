#!/bin/sh

./docker_stop.sh
git stash
git pull
chmod +x docker_start.sh
chmod +x docker_stop.sh
chmod +x reset_data.sh
./docker_start.sh