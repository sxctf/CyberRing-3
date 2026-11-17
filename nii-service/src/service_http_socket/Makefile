CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror
LGFLAGS = -lm

CLI_APP = $(BUILD_DIR)/s21_cli.app
DATA_DIR = ../../data
LOG = $(DATA_DIR)/log
USER_IP = $(DATA_DIR)/user_ip
USER_IP_TMP = $(DATA_DIR)/user_ip.tmp

BUILD_DIR = ../../build
APP = $(BUILD_DIR)/s21_web.app

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

.PHONY: all clean rebuild format run gitpush $(CLI_APP)

all: $(APP)

$(CLI_APP):
	@echo "Сборка подпроекта CLI..."
	$(MAKE) -C ../cli  all
	@echo "Подпроект CLI собран успешно"

$(DATA_DIR):
	mkdir -p $(DATA_DIR)

$(LOG):
	touch $(LOG)

$(USER_IP):
	touch $(USER_IP)

$(USER_IP_TMP):
	touch $(USER_IP_TMP)

$(APP): $(OBJ) $(DATA_DIR) $(LOG) $(USER_IP) $(USER_IP_TMP)
	$(CC) $(OBJ) $(LGFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm -f $(OBJ) $(APP)

clean_sub: clean
	$(MAKE) -C ../cli  clean_sub

rebuild: clean all

format:
	-clang-format -n *.[c,h]
	-clang-format -i *.[c,h]

gitpush: clean format
	git add . && \
	git commit -m "$(M)" && \
	git push origin $$(git branch --show-current)

start: $(CLI_APP) $(APP)
	chmod a-w $(DATA_DIR)
	$(APP) ../www

reset_start: clean_sub $(CLI_APP) $(APP)
	chmod a-w $(DATA_DIR)
	$(APP) ../www
