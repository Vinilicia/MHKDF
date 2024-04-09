all:
	gcc -o prog fillBuffer3.c -lssl -lcrypto
	./prog
