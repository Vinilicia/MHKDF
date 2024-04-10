#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include <openssl/evp.h>
#include <string.h>


///////////////////////////////////////////////////////////////////////////////////////////////


unsigned int hexStringToHexInt(unsigned char *hexString) {
    unsigned int hexInt;
    sscanf(hexString, "%x", &hexInt);
    return hexInt;
}

unsigned int firstNumbers(unsigned char *hash){ 
    unsigned int num = 0;
    for (int i = 0; i < 4; i++) {
        num = (num << 8) | hash[i];
    }
    return num;
}

unsigned long intConcatenate(unsigned int a, unsigned int b){
    unsigned long concatenatedInt;
    concatenatedInt = (unsigned long)a << 32;
    concatenatedInt += b;
    return concatenatedInt;
}

void hashFunction(unsigned char *input, unsigned char *hash) {
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    const EVP_MD *md = EVP_sha256();

    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, input, strlen(input));
    EVP_DigestFinal_ex(mdctx, hash, NULL);
    EVP_MD_CTX_free(mdctx);
}


///////////////////////////////////////////////////////////////////////////////////////////////

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
    unsigned char* hexString = (char*) malloc(17 * sizeof(char));
    char concatenatedString[8 + strlen(password)];
    unsigned long concatenatedInt;

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

    // Preencher restante da matriz
    

    // Primeira linha
    for(int j = 0; j < n/16 - 1; j++){
	unsigned long *v = (unsigned long*) malloc(16*sizeof(unsigned long));
	for(int h = 16*j; h < 16*j+16; h++){
	    v[h] = B[0][h];
	}
        blake2bPermutation(v);
	for(int h = 16*j; h < 16*j+16; h++){
	    B[0][h+16] = v[h];
	}
    }
    

    for(int i = 1; i < k; i++){
	// B[i][0...15] <-- H(B[i-1][k-16...k-1]
	unsigned long *v = (unsigned long*) malloc(16*sizeof(unsigned long));
	for(int h = k-16; h < k; h++){
	    v[h] = B[i-1][h];
	}
        blake2bPermutation(v);
	for(int h = 0; h < 16; h++){
            B[i][h] = v[h];
	}
        free(v);

	// B[i][0...15] <-- H(B[i-1][k-15...k-1]
	for(int j = 1; j < n/16; j++){
	    unsigned long *v = (unsigned long*) malloc(16*sizeof(unsigned long));
	    for(int h = 16*j; h < 16*j+16; h++){
	    	v[h] = B[i][h];
	    }
            blake2bPermutation(v);
	    for(int h = 16*j; h < 16*j+16; h++){
	    	B[i][h+16] = v[h];
	    }
	    free(v);
	}
    }


	
    
    /*// B[0][0] ← B[0][0] ⊕  H(B[k-1][0] || B[k-1][1]
    concatenatedInt = intConcatenate(B[k-1][0], B[k-1][1]);
    concatenatedInt = blake2bPermutation(concatenatedInt);
    B[0][0] ^= (concatenatedInt >> 32);

    // B[0][j] ← B[0][j] ⊕  H(B[k-1][j+1] || B[0][j−1]
    for (int j = 1; j < n - 1; j++) {
    	concatenatedInt = intConcatenate(B[k-1][j+1], B[0][j-1]);
    	concatenatedInt = blake2bPermutation(concatenatedInt);
        B[0][j] ^= (concatenatedInt >> 32);
    }

    // B[0][n−1] ← B[0][n−1] ⊕  H(B[k-1][n−1] || B[0][n−2]
    concatenatedInt = intConcatenate(B[k-1][n-1], B[0][n-2]);
    concatenatedInt = blake2bPermutation(concatenatedInt);
    B[0][n-1] ^= (concatenatedInt >> 32);*/

    free(hexString);
//    free(hString);
}

int main() {
    clock_t start, end;
    double cpu_time_used;

    start = clock();
    int k = 4, n = 16;

    unsigned long** B = allocateMatrix(k, n);

    unsigned char *password = "senha_muito_forte";
    fillBuffer(B, k, n, password);
    printMatrix(B, k, n);

    freeMatrix(B, k);
    
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

    printf("Tempo de execucao: %f segundos\n", cpu_time_used);
    return 0;
}

