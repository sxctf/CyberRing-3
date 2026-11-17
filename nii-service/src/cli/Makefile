CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror
LGFLAGS = -lm -L../../build -l:data_base_lib.a

TARGET_LIB=$(BUILD_DIR)/data_base_lib.a
FILE_H=../data_base_lib/data_base_lib.h

DATA_DIR = ../../data
USER_DB = $(DATA_DIR)/user_db
DATA_DB = $(DATA_DIR)/data_db
BUILD_DIR = ../../build
APP = $(BUILD_DIR)/s21_cli.app

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

.PHONY: all clean rebuild format run gitpush $(TARGET_LIB)

all: $(APP)

$(TARGET_LIB):
	@echo "Сборка подпроекта data_base_lib..."
	$(MAKE) -C ../data_base_lib  all
	@echo "Подпроект data_base_lib собран успешно"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(DATA_DIR):
	mkdir -p $(DATA_DIR)

$(USER_DB):
	touch $(USER_DB)

$(DATA_DB):
	touch $(DATA_DB)

$(APP): $(FILE_H) $(TARGET_LIB) $(OBJ) $(BUILD_DIR) $(DATA_DIR) $(USER_DB) $(DATA_DB)
	$(CC) $(OBJ) $(LGFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-chmod a+w $(DATA_DIR)
	-rm -rf $(DATA_DIR)/* $(OBJ) $(APP)

clean_sub: clean
	$(MAKE) -C ../data_base_lib  clean

rebuild: clean all

format:
	-clang-format -i -- *.c *.h */*.c */*.h

gitpush: clean format
	git add . && \
	git commit -m "$(M)" && \
 	git push origin $(shell git branch --show-current);

run: $(APP) | $(DATA_DIR)
	$(APP)

run_w: $(APP) 
	$(APP) w


