
.PHONY: clean format gitpush

clean:
	$(MAKE) -C ./src clean

format:
	$(MAKE) -C ./src format

gitpush: clean format
	git add . && \
	git commit -m "$(M)" && \
	git push origin $(shell git branch --show-current);
