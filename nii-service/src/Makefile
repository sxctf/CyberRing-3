BUILD_DIR = ../../build

.PHONY: clean format gitpush start reset_and_start

clean:
	$(MAKE) -C ./service_http_socket clean_sub

format:
	-clang-format -i -- *.c *.h */*.c */*.h

gitpush: clean format
	git add . && \
	git commit -m "$(M)" && \
	git push origin $(shell git branch --show-current);

# production mode
start:
	-chmod a+w ../data
	$(MAKE) -C ./service_http_socket start

# develop mode
reset_and_start: clean start
