all:
	gcc -o prog mhkdf.c -g -O2 -lssl -lcrypto
	./prog
#perf stat -e L1-dcache-loads,L1-dcache-load-misses ./prog
#perf stat -e l2_rqsts.demand_data_rd_miss,l2_rqsts.demand_data_rd_hit ./prog
#perf stat -e LLC-loads,LLC-load-misses ./prog
#python modificados_depois_acessados.py
#python matriz_de_frequencias.py
