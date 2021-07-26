qcc:	qcc.c
	gcc qcc.c -o qcc
	./qcc
debug:	qcc.c
	gcc -g qcc.c -o qcc-debug
	gdb qcc-debug
