all:
	gcc -o prog fillBuffer.c -lssl -lcrypto
	./prog
