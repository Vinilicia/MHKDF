#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <openssl/evp.h>

int mod(int a, int b) {
    int result = a % b;
    return (result < 0) ? result + b : result;
}

void hashFunction(unsigned char *input, size_t input_len, unsigned char *hash) {
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    const EVP_MD *md = EVP_sha256();

    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, input, input_len);
    EVP_DigestFinal_ex(mdctx, hash, NULL);
    EVP_MD_CTX_free(mdctx);
}

void G(unsigned long *a, unsigned long *b, unsigned long *c, unsigned long *d) {
    *a = (*a + *b + 2 * (*a) * (*b)) & 0xffffffffffffffff;
    *d = *d ^ *a;
    *d = (*d >> 32 | *d << 32) & 0xffffffffffffffff;
    *c = (*c + *d + 2 * (*c) * (*d)) & 0xffffffffffffffff;
    *b ^= *c;
    *b = (*b >> 24 | *b << 40) & 0xffffffffffffffff;
    *a = (*a + *b + 2 * (*a) * (*b)) & 0xffffffffffffffff;
    *d ^= *a;
    *d = (*d >> 16 | *d << 48) & 0xffffffffffffffff;
    *c = (*c + *d + 2 * (*c) * (*d)) & 0xffffffffffffffff;
    *b ^= *c;
    *b = (*b >> 63 | *b << 1) & 0xffffffffffffffff;
}

void blake2bPermutation(unsigned long *v) {
    G(&v[0], &v[4], &v[8], &v[12]);
    G(&v[1], &v[5], &v[9], &v[13]);
    G(&v[2], &v[6], &v[10], &v[14]);
    G(&v[3], &v[7], &v[11], &v[15]);
    G(&v[0], &v[5], &v[10], &v[15]);
    G(&v[1], &v[6], &v[11], &v[12]);
    G(&v[2], &v[7], &v[8], &v[13]);
    G(&v[3], &v[4], &v[9], &v[14]);
}

unsigned long** allocateMatrix(int k, int n) {
    unsigned long** B = (unsigned long**)malloc(k * sizeof(unsigned long*));
    for (int i = 0; i < k; i++) {
        B[i] = (unsigned long*)malloc(n * sizeof(unsigned long));
    }
    return B;
}

unsigned long* allocateKey(int key_size){
    if(key_size % 8 != 0){
        exit(1);
    }
    int n = key_size / 8;
    unsigned long* key = (unsigned long*)malloc(n*sizeof(unsigned long));
    return key;
}

void freeMatrix(unsigned long** B, int k) {
    for (int i = 0; i < k; i++) {
        free(B[i]);
    }
    free(B);
}

void printMatrix(unsigned long** B, int k, int n) {
    printf("B:\n");
    for (int i = 0; i < k; i++) {
        for (int j = 0; j < n; j++) {
            printf("%lx ", B[i][j]);
        }
        printf("\n");
    }
}

void printKey(unsigned long* key, int key_size){
    printf("Key: ");
    for (int i = 0; i < key_size; i++) {
        printf("%lx", key[i]);
    }
    printf("\n");
}

void fillBuffer(unsigned long** B, int k, int n, unsigned char *password) {
    unsigned char hash[32];
    unsigned char aux_hash[32];
	int c, l;
    //  B[0][0] ... B[0][15] ←  H(P || S)
    hashFunction(password, strlen(password), hash);
    B[0][0] = ((hash[0] << 8 | hash[1]) << 8 | hash[2]) << 8 | hash[3]; B[0][0] = (((B[0][0] << 8 | hash[4]) << 8 | hash[5]) << 8 | hash[6]) << 8 | hash[7];
    B[0][1] = ((hash[8] << 8 | hash[9]) << 8 | hash[10]) << 8 | hash[11]; B[0][1] = (((B[0][1] << 8 | hash[12]) << 8 | hash[13]) << 8 | hash[14]) << 8 | hash[15];
    B[0][2] = ((hash[16] << 8 | hash[17]) << 8 | hash[18]) << 8 | hash[19]; B[0][2] = (((B[0][2] << 8 | hash[20]) << 8 | hash[21]) << 8 | hash[22]) << 8 | hash[23];
    B[0][3] = ((hash[24] << 8 | hash[25]) << 8 | hash[26]) << 8 | hash[27]; B[0][3] = (((B[0][3] << 8 | hash[28]) << 8 | hash[29]) << 8 | hash[30]) << 8 | hash[31];
    hashFunction(hash, 32, aux_hash);
    B[0][4] = ((aux_hash[0] << 8 | aux_hash[1]) << 8 | aux_hash[2]) << 8 | aux_hash[3]; B[0][4] = (((B[0][4] << 8 | aux_hash[4]) << 8 | aux_hash[5]) << 8 | aux_hash[6]) << 8 | aux_hash[7];
    B[0][5] = ((aux_hash[8] << 8 | aux_hash[9]) << 8 | aux_hash[10]) << 8 | aux_hash[11]; B[0][5] = (((B[0][5] << 8 | aux_hash[12]) << 8 | aux_hash[13]) << 8 | aux_hash[14]) << 8 | aux_hash[15];
    B[0][6] = ((aux_hash[16] << 8 | aux_hash[17]) << 8 | aux_hash[18]) << 8 | aux_hash[19]; B[0][6] = (((B[0][6] << 8 | aux_hash[20]) << 8 | aux_hash[21]) << 8 | aux_hash[22]) << 8 | aux_hash[23];
    B[0][7] = ((aux_hash[24] << 8 | aux_hash[25]) << 8 | aux_hash[26]) << 8 | aux_hash[27]; B[0][7] = (((B[0][7] << 8 | aux_hash[28]) << 8 | aux_hash[29]) << 8 | aux_hash[30]) << 8 | aux_hash[31];	
    hashFunction(aux_hash, 32, hash);
    B[0][8] = ((hash[0] << 8 | hash[1]) << 8 | hash[2]) << 8 | hash[3]; B[0][8] = (((B[0][8] << 8 | hash[4]) << 8 | hash[5]) << 8 | hash[6]) << 8 | hash[7];
    B[0][9] = ((hash[8] << 8 | hash[9]) << 8 | hash[10]) << 8 | hash[11]; B[0][9] = (((B[0][9] << 8 | hash[12]) << 8 | hash[13]) << 8 | hash[14]) << 8 | hash[15];
    B[0][10] = ((hash[16] << 8 | hash[17]) << 8 | hash[18]) << 8 | hash[19]; B[0][10] = (((B[0][10] << 8 | hash[20]) << 8 | hash[21]) << 8 | hash[22]) << 8 | hash[23];
    B[0][11] = ((hash[24] << 8 | hash[25]) << 8 | hash[26]) << 8 | hash[27]; B[0][11] = (((B[0][11] << 8 | hash[28]) << 8 | hash[29]) << 8 | hash[30]) << 8 | hash[31];
    hashFunction(hash, 32, aux_hash);
    B[0][12] = ((aux_hash[0] << 8 | aux_hash[1]) << 8 | aux_hash[2]) << 8 | aux_hash[3]; B[0][12] = (((B[0][12] << 8 | aux_hash[4]) << 8 | aux_hash[5]) << 8 | aux_hash[6]) << 8 | aux_hash[7];
    B[0][13] = ((aux_hash[8] << 8 | aux_hash[9]) << 8 | aux_hash[10]) << 8 | aux_hash[11]; B[0][13] = (((B[0][13] << 8 | aux_hash[12]) << 8 | aux_hash[13]) << 8 | aux_hash[14]) << 8 | aux_hash[15];
    B[0][14] = ((aux_hash[16] << 8 | aux_hash[17]) << 8 | aux_hash[18]) << 8 | aux_hash[19]; B[0][14] = (((B[0][14] << 8 | aux_hash[20]) << 8 | aux_hash[21]) << 8 | aux_hash[22]) << 8 | aux_hash[23];
    B[0][15] = ((aux_hash[24] << 8 | aux_hash[25]) << 8 | aux_hash[26]) << 8 | aux_hash[27]; B[0][15] = (((B[0][15] << 8 | aux_hash[28]) << 8 | aux_hash[29]) << 8 | aux_hash[30]) << 8 | aux_hash[31];
	unsigned long *v = (unsigned long*) malloc(16*sizeof(unsigned long));

    // Preencher restante da matriz
    

    // Primeira linha
    int cont;
    for(int j = 0; j < n/16 - 1; j++){
    	cont = 0;
        for(int h = 16*j; h < 16*j+16; h++){
            v[cont] = B[0][h];
            cont++;
        }
            blake2bPermutation(v);
        cont = 0;
        for(int h = 16*j; h < 16*j+16; h++){
            B[0][h+16] = v[cont];
            cont++;
        }
    }
    
    for(int i = 1; i < k; i++){
        // B[i][0...15] <-- H(B[i-1][n-16...n-1]
        cont = 0;
        l = B[i-1][n-1] % i;
        c = B[i-1][n-1] % n;
        for(int h = c; h < c+16; h++){
            v[cont] = B[l][h%n];
            cont++;
        }
            blake2bPermutation(v);
        for(int h = 0; h < 16; h++){
            B[i][h] = v[h];
        }
            
        // B[i][h...h+15] <-- H(B[i][h+16...h+31]
        for(int j = 1; j < n/16; j++){
            cont = 0;
            l = B[i][16*j-1] % i;
            c = B[i][16*j-1] % n;
            for(int h = c; h < c+16; h++){
                v[cont] = B[l][h%n];
                cont++;
            }
            blake2bPermutation(v);
            cont = 0;
            for(int h = j*16; h < j*16+16; h++){
                B[i][h%n] = v[cont];
                cont++;
            }
        }
   }
   free(v);
}

void updateState(unsigned long** B, unsigned long* key, int k, int n, int key_size){
    int l1, l2, c1, c2;
    l1 = B[k-1][n-1] % k;
	c1 = B[k-1][n-1] % n;
    int gap = n/4;
    int d1, d2;
    int m = k*n/16/key_size;
    unsigned long key_part = 0;
    unsigned long *v = (unsigned long*) malloc(16*sizeof(unsigned long));
    for(int i = 0; i < k*n/16; i++){
        switch(B[l1][c1]%4){
            case 0:
                d1 = -1; d2 = -1;
                break;
            case 1:
                d1 = -1; d2 = 1;
                break;
            case 2:
                d1 = 1; d2 = 1;
                break;
            case 3:
                d1 = 1; d2 = -1;
                break;
        }
        l2 = B[mod(l1+15*d1,k)][mod(c1+15*d2,n)] % k;
        c2 = (c1 < n/2) ? c1 + gap + B[mod(l1+15*d1,k)][mod(c1+15*d2,n)] % gap : c1 - gap - B[mod(l1+15*d1,k)][mod(c1+15*d2,n)] % gap;
        for(int h = 0; h < 16; h++){
	    	v[h] = B[mod(l1+h*d1,k)][mod(c1+h*d2,n)];
	    }
        //printf("B[%d][%d] --> ", l1, c1);
        blake2bPermutation(v);
	    for(int h = 0; h < 16; h++){
	    	B[mod(l2+h*d1,k)][mod(c2+h*d2,n)] = v[h];
	    }
        //printf("B[%d][%d]\n", l2, c2);
        B[mod(l1+15*d1,k)][mod(c1+15*d2,n)] ^= B[mod(l2+15*d1,k)][mod(c2+15*d2,n)];
        key_part ^= (B[l1][c1] ^ B[l2][c2]);
        if(i%m == 0 && i != 0){
            key[i/m - 1] = key_part;
            key_part = 0;
        }
        l1 = B[mod(l2+15*d1,k)][mod(c2+15*d2,n)] % k;
	    c1 = B[mod(l2+15*d1,k)][mod(c2+15*d2,n)] % n; 
    }
    key[key_size-1] = key_part;
    free(v);
}

int main() {
    clock_t start, end;
    double cpu_time_used;

    start = clock();
    int k = 160, n = 160;
    // 11584      16384       20064
    int key_size = 32;

    unsigned long** B = allocateMatrix(k, n);
    unsigned long* key = allocateKey(key_size);
    unsigned char *password = "senha_muito_forte";
    fillBuffer(B, k, n, password);
    updateState(B, key, k, n, key_size/8);
    //printMatrix(B, k, n);
    printKey(key, key_size/8);
    
    freeMatrix(B, k);
    free(key);

    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

    printf("Tempo de execucao: %f segundos\n", cpu_time_used);
    return 0;
}
