CC=gcc
CFLAGS=-Wall -Wextra -Werror -std=c11
CHECK_LIBS = $(shell pkg-config --libs check)
GCOVFLAG=

TARGET=$(TARGET_BUILD_DIR)/data_base_lib.a
FILE_H=./data_base_lib.h
TARGET_BUILD_DIR=../../build

BUILD_DIR = build
TEST_BUILD_DIR = tests/build

SOURCES = $(wildcard *.c)
OBJECTS = $(addprefix $(BUILD_DIR)/, $(SOURCES:.c=.o))

TEST_SRCS = $(wildcard tests/*.c)
TEST_EXES = $(addprefix $(TEST_BUILD_DIR)/, $(patsubst tests/%.c,%,$(TEST_SRCS)))

#Build
all: $(TARGET)

$(TARGET): $(OBJECTS) | $(TARGET_BUILD_DIR)
	@ar rcs $@ $^

$(TARGET_BUILD_DIR):
	@mkdir -p $@

$(BUILD_DIR)/%.o: %.c $(FILE_H) | $(BUILD_DIR)
	@$(CC) $(CFLAGS) $(GCOVFLAG) -c $< -o $@

#Tests
testwin: $(TARGET) $(TEST_EXES)
	./test/build/insert_levels_test

test: $(TARGET) $(TEST_EXES)
	@error_sum=0; \
	for exe in $(TEST_EXES); do \
		name=$$(basename $$exe); \
		echo "\033[1;34m========================================\033[0m"; \
		echo "\033[1;34mRunning $$name\033[0m"; \
		echo "\033[1;34m========================================\033[0m"; \
		./$$exe; \
		return_code=$$?; \
		error_sum=$$(expr $$error_sum + $$return_code); \
		echo ""; \
	done; \
	if [ $$error_sum -eq 0 ]; then \
		echo "\033[1;32m========================================\033[0m"; \
		echo "\033[1;32m✅ TESTS SUCCEEDED!\033[0m"; \
		echo "\033[1;32m========================================\033[0m"; \
	else \
		echo "\033[1;31m========================================\033[0m"; \
		echo "\033[1;31m❌ ERROR: $$error_sum errors detected!\033[0m"; \
		echo "\033[1;31m========================================\033[0m"; \
	fi

# valgrind --tool=memcheck --leak-check=yes ./$$exe; \

$(TEST_BUILD_DIR)/%: tests/%.c $(FILE_H) | $(TEST_BUILD_DIR)
	@$(CC) $(CFLAGS) $< -L. -l:$(TARGET) $(CHECK_LIBS) $(GCOVFLAG) -o $@

$(BUILD_DIR):
	@mkdir -p $@

$(TEST_BUILD_DIR):
	@mkdir $@


rebuild: clean $(TARGET)

gcov_report: GCOVFLAG=-lgcov -fprofile-arcs -ftest-coverage
gcov_report: clean test
	@lcov --capture --directory ./ --output-file coverage.info \
		--rc branch_coverage=1 > /dev/null
	@lcov --remove coverage.info '*/tests/*' \
		--output-file coverage_filtered.info > /dev/null
	@genhtml --branch-coverage \
		--output-directory coverage_report \
		--title "Checking string functions" \
		coverage_filtered.info > /dev/null
	@echo "Coverage report: ./coverage_report/index.html"

#Clean
clean:
	@rm -f $(TARGET) $(OBJECTS)
	@rm -f *.gcno *.gcda *.gcov
	@rm -f tests/*.gcno tests/*.gcda tests/*.gcov *.info
	@rm -rf coverage_report

	@rm -f $(TARGET)
	@rm -rf $(BUILD_DIR)

retest: clean test

#Clang-format
format:
	clang-format -n *.[c,h] ./**/*.[c,h]
	clang-format -i *.[c,h] ./**/*.[c,h]

.PHONY: all test gcov_report retest rebuild format clean
