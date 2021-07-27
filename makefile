qcc:	qcc.c
	gcc qcc.c -o qcc
test:	qcc
	@echo "Got:"
	@./qcc ./tests/quickc_test.qc -p
	@echo "From:"
	@cat tests/quickc_test.qc
	@echo "Wanted:"
	@cat tests/wanted.c
debug:	qcc.c
	gcc -g qcc.c -o qcc-debug
	gdb qcc-debug
