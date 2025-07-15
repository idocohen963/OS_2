# Root Makefile for task2

SUBDIRS = q1 q2 q3 q4 q5 q6

all:
	@for dir in $(SUBDIRS); do \
		echo "Building $$dir..."; \
		$(MAKE) -C $$dir all || exit 1; \
	done
	@echo "All builds completed successfully"

clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done


.PHONY: all clean 