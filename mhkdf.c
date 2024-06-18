#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include <openssl/evp.h>
#include <string.h>

int mod(int a, int b) {
    int result = a % b;
    return (result < 0) ? result + b : result;
}

void hashFunction(unsigned char *input, unsigned char *hash) {
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    const EVP_MD *md = EVP_sha256();

    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, input, strlen(input));
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

void fillBuffer(unsigned long** B, int k, int n, unsigned char *password) {
    unsigned char hash[32];
    unsigned char aux_hash[32];
	int c, l;
    //  B[0][0] ... B[0][15] ←  H(P || S)
    hashFunction(password, hash);
    for (int i = 0; i < 4; i++) {
        unsigned long part = 0;
        for (int j = 8*i; j < 8*i + 8; j++) {
	    if(hash[j] <= 0xf){
		part = (part << 4) | hash[j];
	    	continue;
	    }
            part = (part << 8) | hash[j];
        }
        B[0][i] = part;
    }	
    hashFunction(hash, aux_hash);
    for (int i = 0; i < 4; i++){
        unsigned long part = 0;
        for (int j = 8*i; j < 8*i + 8; j++) {
	    if(aux_hash[j] <= 0xf){
		part = (part << 4) | aux_hash[j];
	    	continue;
	    }
            part = (part << 8) | aux_hash[j];
        }
        B[0][i+4] = part;
    }	
    hashFunction(aux_hash, hash);
    for (int i = 0; i < 4; i++){
        unsigned long part = 0;
        for (int j = 8*i; j < 8*i + 8; j++) {
	    if(hash[j] <= 0xf){
		part = (part << 4) | hash[j];
	    	continue;
	    }
            part = (part << 8) | hash[j];
        }
        B[0][i+8] = part;
    }	
    hashFunction(hash, aux_hash);
    for (int i = 0; i < 4; i++){
        unsigned long part = 0;
        for (int j = 8*i; j < 8*i + 8; j++) {
	    if(aux_hash[j] <= 0xf){
		part = (part << 4) | aux_hash[j];
	    	continue;
	    }
            part = (part << 8) | aux_hash[j];
        }
        B[0][i+12] = part;
    }	
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

void updateState(unsigned long** B, int k, int n){
    int l1, l2, c1, c2;
    l1 = B[k-1][n-1] % k;
	c1 = B[k-1][n-1] % n;
    int gap = n/4;
    int d1, d2;
    unsigned long *v = (unsigned long*) malloc(16*sizeof(unsigned long));
    for(int i = 0; i < k*n/16; i++){
        l2 = B[l1][c1] % k;
        c2 = (c1 < n/2) ? c1 + gap + B[l1][c1] % gap : c1 - gap - B[l1][c1] % gap;
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
        for(int h = 0; h < 16; h++){
	    	v[h] = B[mod(l1+h*d1,k)][mod(c1+h*d2,n)];
            //printf("B[%d][%d],", mod(l1+h*d1,k), mod(c1+h*d2,n));
	    }
        //printf(" -->\n");
        blake2bPermutation(v);
	    for(int h = 0; h < 16; h++){
	    	B[mod(l2+h*d1,k)][mod(c2+h*d2,n)] = v[h];
            //printf("B[%d][%d],", mod(l2+h*d1,k), mod(c2+h*d2,n));
	    }
        //printf("\n\n");
        l1 = B[mod(l2+15*d1,k)][mod(c2+15*d2,n)] % k;
	    c1 = B[mod(l2+15*d1,k)][mod(c2+15*d2,n)] % n;
    }
    free(v);
}

int main() {
    clock_t start, end;
    double cpu_time_used;

    start = clock();
    int k = 160, n = 160;
    // 11584

    unsigned long** B = allocateMatrix(k, n);

    unsigned char *password = "senha_muito_forte";
    fillBuffer(B, k, n, password);
    updateState(B, k, n);
    //printMatrix(B, k, n);
    
    freeMatrix(B, k);
    
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

    printf("Tempo de execucao: %f segundos\n", cpu_time_used);
    return 0;
}
