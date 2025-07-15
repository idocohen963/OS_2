CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200112L --coverage
LDFLAGS = -lpthread

all: atom_supplier molecule_requester drinks_bar

atom_supplier: atom_supplier.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o atom_supplier atom_supplier.c

molecule_requester: molecule_requester.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o molecule_requester molecule_requester.c

drinks_bar: drinks_bar.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o drinks_bar drinks_bar.c

coverage:
	gcov *.c

coverage-report:
	@echo "=== Q6 Code Coverage Report ===" > coverage_report_q6.txt
	@echo "Generated on: $$(date)" >> coverage_report_q6.txt
	@echo "" >> coverage_report_q6.txt
	@gcov *.c 2>&1 | grep "Lines executed" >> coverage_report_q6.txt 2>/dev/null || echo "No coverage data found - run programs first" >> coverage_report_q6.txt
	@echo "" >> coverage_report_q6.txt
	@echo "Coverage files (.gcov) generated:" >> coverage_report_q6.txt
	@ls *.gcov >> coverage_report_q6.txt 2>/dev/null || echo "No .gcov files found" >> coverage_report_q6.txt
	@echo "" >> coverage_report_q6.txt
	@echo "=== Uncovered Lines (####) ===" >> coverage_report_q6.txt
	@grep "####" *.gcov >> coverage_report_q6.txt 2>/dev/null || echo "All lines covered!" >> coverage_report_q6.txt
	@echo "Coverage report saved to coverage_report_q6.txt"

clean:
	rm -f atom_supplier molecule_requester drinks_bar *.gcno *.gcda *.gcov *.sock
	@pkill drinks_bar 2>/dev/null || true
	@pkill atom_supplier 2>/dev/null || true
	@pkill molecule_requester 2>/dev/null || true

# Clean socket files
clean-sockets:
	rm -f /tmp/*.sock *.sock

.PHONY: all coverage coverage-report clean clean-sockets