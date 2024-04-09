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

void G(int *a, int *b, int *c, int *d) {
    *a = (*a + *b + 2 * (*a) * (*b)) & 0xf;
    *d = *d ^ *a;
    *d = (*d >> 3 | *d << 1) & 0b1111;
    *c = (*c + *d + 2 * (*c) * (*d)) & 0xf;
    *b ^= *c;
    *b = (*b >> 2 | *b << 2) & 0b1111;
    *a = (*a + *b + 2 * (*a) * (*b)) & 0xf;
    *d ^= *a;
    *d = (*d >> 1 | *d << 3) & 0b1111;
    *c = (*c + *d + 2 * (*c) * (*d)) & 0xf;
    *b ^= *c;
    *b = (*b >> 3 | *b << 1) & 0b1111;
}

unsigned int blake2bPermutation(unsigned long input_data) {
    int v[16];

    for (int i = 0; i < 16; i++){
	v[i] = (input_data >> 4 * (15 - i)) & 0xf;
    }

    G(&v[0], &v[1], &v[2], &v[3]);
    G(&v[4], &v[5], &v[6], &v[7]);
    G(&v[0], &v[5], &v[10], &v[15]);
    G(&v[1], &v[6], &v[11], &v[12]);
    G(&v[2], &v[7], &v[8], &v[13]);
    G(&v[3], &v[4], &v[9], &v[14]);
    
    unsigned int result = 0;
    for (int i = 0; i < 8; i++) {
        result |= v[i] << (4 * (7 - i));
    }
    
    return result;
}

unsigned int** allocateMatrix(int k, int n) {
    unsigned int** B = (unsigned int**)malloc(k * sizeof(unsigned int*));
    for (int i = 0; i < k; i++) {
        B[i] = (unsigned int*)malloc(n * sizeof(unsigned int));
    }
    return B;
}

void freeMatrix(unsigned int** B, int k) {
    for (int i = 0; i < k; i++) {
        free(B[i]);
    }
    free(B);
}

void printMatrix(unsigned int** B, int k, int n) {
    printf("B:\n");
    for (int i = 0; i < k; i++) {
        for (int j = 0; j < n; j++) {
            printf("%x ", B[i][j]);
        }
        printf("\n");
    }
}

void fillBuffer(unsigned int** B, int k, int n, unsigned char *password) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned char* hexString = (char*) malloc(17 * sizeof(char));
    char concatenatedString[8 + strlen(password)];
    unsigned long concatenatedInt;

    //  B[0][0] ← H(P || S)
    hashFunction(password, hash);
    B[0][0] = firstNumbers(hash);

    //  B[0][1] ← H(P || S || B[0][0])
    unsigned char* hString = (char*) malloc(9 * sizeof(char));
    sprintf(hString, "%x", B[0][0]);
    hString[8] = '\0';
    sprintf(concatenatedString, "%s%s", hString, password);
    hashFunction(concatenatedString, hash);
    B[0][1] = firstNumbers(hash);

    // B[0][j] ← H(B[0][j−1]|| B[0][j−2])
    for (int j = 2; j < n; j++) {
        concatenatedInt = intConcatenate(B[0][j-1], B[0][j-2]);
        B[0][j] = blake2bPermutation(concatenatedInt);
    }

    int i = 1;
    while(true) {
        // B[i][n−1] ← H(B[i-1][n−2]|| B[i-1][n−1]
        concatenatedInt = intConcatenate(B[i-1][n-2], B[i-1][n-1]);
	B[i][n-1] = blake2bPermutation(concatenatedInt);

        // B[i][j] ← H(B[i-1][j−1] || B[i][j+1]
        for(int j = n - 2; j > 0; j--) {
            concatenatedInt = intConcatenate(B[i-1][j-1], B[i][j+1]);
	    B[i][j] = blake2bPermutation(concatenatedInt);
        }

        // B[i][0] ← H(B[i-1][0] || B[i][1]
        concatenatedInt = intConcatenate(B[i-1][0], B[i][1]);
	B[i][0] = blake2bPermutation(concatenatedInt);

        i++;
        if(i == k)
            break;

        // B[i][0] ← H(B[i-1][0] || B[i-1][1]
        concatenatedInt = intConcatenate(B[i-1][0], B[i-1][1]);
	B[i][0] = blake2bPermutation(concatenatedInt);

        // B[i][j] ← H(B[i-1][j+1] || B[i][j−1]
        for(int j = 1; j < n - 1; j++) {
	   concatenatedInt = intConcatenate(B[i-1][j+1], B[i][j-1]);
	   B[i][j] = blake2bPermutation(concatenatedInt);
        }

        // B[i][n−1] ← H(B[i-1][n−1] || B[i][n−2]
        concatenatedInt = intConcatenate(B[i-1][n-1], B[i][n-2]);
	B[i][n-1] = blake2bPermutation(concatenatedInt);

        if(i == k)
            break;
    }

    // B[0][0] ← B[0][0] ⊕  H(B[k-1][0] || B[k-1][1]
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
    B[0][n-1] ^= (concatenatedInt >> 32);

    free(hexString);
    free(hString);
}

int main() {
    clock_t start, end;
    double cpu_time_used;

    start = clock();
    int k = 16157, n = 16157;

    unsigned int** B = allocateMatrix(k, n);

    unsigned char *password = "senha_muito_forte";
    fillBuffer(B, k, n, password);
    //printMatrix(B, k, n);

    freeMatrix(B, k);
    
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

    printf("Tempo de execucao: %f segundos\n", cpu_time_used);
    return 0;
}

