all:
	gcc -o prog mhkdfSpongent.c -lssl -lcrypto
	./prog

