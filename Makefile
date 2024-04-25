all:
	gcc -o prog mhkdf.c -lssl -lcrypto
	./prog
