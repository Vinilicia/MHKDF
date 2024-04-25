#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include <openssl/evp.h>
#include <string.h>

#define N 1024
#define M 0x100000000

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

void fillBuffer(unsigned long** B, unsigned char *password) {
   int i,j,k,c,r,l,t;
   long int W[4] = {0x9a82019df03c376b,
		    0xa45b3e306748fd31,
		    0xd0f4938b00c921ae,
		    0x74f201adf3564fc3};
		    
    unsigned char hash[32];

   // Filling the first row:

   // B[0][0],B[0][1],B[0][2],B[0][3] := Hash(P||S)
   
    hashFunction(password, hash);
    for (i = 0; i < 4; i++) {
        unsigned long part = 0;
        for (j = 8*i; j < 8*i + 8; j++) {
	    if(hash[j] <= 0xf){
		part = (part << 4) | hash[j];
	    	continue;
	    }
            part = (part << 8) | hash[j];
        }
        B[0][i] = part;
    }
    
    for(j = 1; j < N/4; j++){
      c = B[0][4*j-1] % j;
	
      for(k = 0; k < 4; k++)
         B[0][4*j+k] = (B[0][4*c+k] + W[k]) % M;

      	G(&B[0][4*j],&B[0][4*j+1],&B[0][4*j+2],&B[0][4*j+3]);
     }

   // Filling the remaining rows:

   for(i = 1; i < N; i++){
      for(j = 0; j < N; j++){
         l = B[i-1][(4*j) % N] % i;  // Chooses a random row

	 for(k = 0; k < 4; k++)
            B[i][(4*j+k) % N] = (B[l][(4*j+k) % N] + W[k]) % M;

	 G(&B[i][(4*j) % N],&B[i][(4*j+1) % N],&B[i][(4*j+2) % N],&B[i][(4*j+3) % N]);
	 G(&W[0],&W[1],&W[2],&W[3]);

	 // t is an integer such that if j < N/2, then t = 1 and t = 0 otherwise
	 t = (2*j/N + 1) % 2;

	 // c is a random column index such that c > N/2 if j < N/2 and c < N/2 if j > N/2
         c = (B[i][(4*j+1) % N] + B[i][(4*j+3) % N]) % (N/2) + (N/2) * t;

         r = (B[i][(4*j) % N] + B[i][(4*j+2) % N]) % i;

         G(&B[r][(4*c) % N],&B[r][(4*c+1) % N],&B[r][(4*c+2) % N],&B[r][(4*c+3) % N]);
        }
    }
}

int main() {
    clock_t start, end;
    double cpu_time_used;

    start = clock();

    unsigned long** B = allocateMatrix(N, N);

    unsigned char *password = "senha_muito_forte";
    fillBuffer(B, password);
    //printMatrix(B, N, N);

    freeMatrix(B, N);
    
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

    printf("Tempo de execucao: %f segundos\n", cpu_time_used);
    return 0;
}

