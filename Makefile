all:
	gcc -o prog mhkdfSHA256.c -lssl -lcrypto
	./prog

