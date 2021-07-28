qcc:	qcc.c
	gcc qcc.c -o qcc
test:	qcc
	@rm -f tests/result.c
	@./qcc ./tests/quickc_test.qc -o ./tests/result.c
	@echo "From:"
	@cat tests/quickc_test.qc
	@echo "Wanted:"
	@cat tests/wanted.c
	@echo "Got:"
	@cat tests/result.c
debug:	qcc.c
	gcc -g qcc.c -o qcc-debug
	gdb qcc-debug
