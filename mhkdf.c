#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int lfsr8_state = 0b10011110;
int SBOX[16] = { 0xE,0xD,0xB,0x0,0x2,0x1,0x4,0xF,
                 0x7,0xA,0x8,0x5,0x9,0xC,0x3,0x6};

int mod(int a, int b) {
    int result = a % b;
    return (result < 0) ? result + b : result;
}

unsigned char* padding(unsigned char* password, int r) {
    int size = strlen(password);
    int rest = size % r;
    unsigned char* message = (unsigned char*) malloc((size+rest)*sizeof(unsigned char));
    memcpy(message, password, size);
    if(rest) {
        message[size] = 0x80;
    }
    return message;
}

int lfsr8_step() {
    int bit7 = (lfsr8_state >> 7) & 1;
    int bit3 = (lfsr8_state >> 3) & 1;
    int bit2 = (lfsr8_state >> 2) & 1;
    int bit1 = (lfsr8_state >> 1) & 1;
    int feedback = bit7 ^ bit3 ^ bit2 ^ bit1;
    lfsr8_state = ((lfsr8_state << 1) | feedback) & 0xFF;
    return lfsr8_state;
}

void lCounter_b(unsigned char *state, int b_size, int r) {
    int counter = lfsr8_step();
    for (int i = 0; i < r; i++) {
        int bit = (counter >> i) & 1;
        state[i/8] ^= (bit << (i % 8));
    }
    for (int i = 0; i < r; i++) {
        int bit = (counter >> (r - 1 - i)) & 1;
        int pos = b_size - r + i;
        state[pos/8] ^= (bit << (pos % 8));
    }
}

static void sBoxLayer(unsigned char *state, int b_size) {
    int nibbles = b_size / 4;
    for (int i = 0; i < nibbles; i++) {
        int pos = i * 4;
        int nib = 0;
        for (int j = 0; j < 4; j++) {
            int bit = (state[(pos+j)/8] >> ((pos+j) % 8)) & 1;
            nib |= (bit << j);
        }
        int out = SBOX[nib];
        for (int j = 0; j < 4; j++) {
            int bit = (out >> j) & 1;
            int posj = pos + j;
            state[posj/8] = (state[posj/8] & ~(1 << (posj%8))) | (bit << (posj%8));
        }
    }
}

void pLayer(unsigned char *state, int b_size) {
    int last = b_size - 1;
    int mul = b_size / 4;
    unsigned char out[40] = {0};
    for (int j = 0; j < b_size; j++) {
        int dst = (j == last) ? last : (j * mul) % (b_size - 1);
        int bit = (state[j/8] >> (j%8)) & 1;
        out[dst/8] |= (bit << (dst%8));
    }
    memcpy(state, out, (b_size+7)/8);
}

void PRESENT_TYPE_permutation(unsigned char *b, int b_size, int r, int R) {
    for(int i = 0; i < R; i++) {
        lCounter_b(b, b_size, r);
        sBoxLayer(b, b_size);
        pLayer(b, b_size);
    }
}

void absorbing(unsigned char *b, int b_size, unsigned char* message, int r, int R) {
    for(int i = 0; i < strlen(message); i+=r){
        for(int j = 0; j < r; j++){
            b[j] ^= message[i+j];
        }
        PRESENT_TYPE_permutation(b, b_size, r, R);
    }
}

void squeezing(unsigned long** B, unsigned char* b, int b_size, int r, int R) {
    int p1 = 0, p2 = 0;
    unsigned long packed;
    while(true) {
        for(int i = 0; i < r; i++) {
            packed <<= 8;
            packed |= b[i];
            p2++;
            if(p2 % 8 == 0) {
                B[0][p1] = packed;
                p1++;
                p2 = 0;
                packed = 0;
            }
        }
        if(p1 == 16) {
            break;
        }
        PRESENT_TYPE_permutation(b, b_size, r, R);
    }
}

void spongent256(unsigned long** B, unsigned char* password) {
    #define B_BYTES 34
    unsigned char b[B_BYTES] = {0};
    int r = 2;
    int b_size = 272;
    int R = 140;
    // int n = 32;
    unsigned char *message = padding(password, r);
    absorbing(b, b_size, message, r, R);
    squeezing(B, b, b_size, r, R);
    free(message);
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

void permutation32(unsigned long *v) {
    G(&v[0], &v[8], &v[16], &v[24]);
    G(&v[1], &v[9], &v[17], &v[25]);
    G(&v[2], &v[10], &v[18], &v[26]);
    G(&v[3], &v[11], &v[19], &v[27]);
    G(&v[4], &v[12], &v[20], &v[28]);
    G(&v[5], &v[13], &v[21], &v[29]);
    G(&v[6], &v[14], &v[22], &v[30]);
    G(&v[7], &v[15], &v[23], &v[31]);
}

void permutation48(unsigned long *v) {
    G(&v[0], &v[12], &v[24], &v[36]);
    G(&v[1], &v[13], &v[25], &v[37]);
    G(&v[2], &v[14], &v[26], &v[38]);
    G(&v[3], &v[15], &v[27], &v[39]);
    G(&v[4], &v[16], &v[28], &v[40]);
    G(&v[5], &v[17], &v[29], &v[41]);
    G(&v[6], &v[18], &v[30], &v[42]);
    G(&v[7], &v[19], &v[31], &v[43]);
    G(&v[8], &v[20], &v[32], &v[44]);
    G(&v[9], &v[21], &v[33], &v[45]);
    G(&v[10], &v[22], &v[34], &v[46]);
    G(&v[11], &v[23], &v[35], &v[47]);
}

void permutation64(unsigned long *v) {
    G(&v[0], &v[16], &v[32], &v[48]);
    G(&v[1], &v[17], &v[33], &v[49]);
    G(&v[2], &v[18], &v[34], &v[50]);
    G(&v[3], &v[19], &v[35], &v[51]);
    G(&v[4], &v[20], &v[36], &v[52]);
    G(&v[5], &v[21], &v[37], &v[53]);
    G(&v[6], &v[22], &v[38], &v[54]);
    G(&v[7], &v[23], &v[39], &v[55]);
    G(&v[8], &v[24], &v[40], &v[56]);
    G(&v[9], &v[25], &v[41], &v[57]);
    G(&v[10], &v[26], &v[42], &v[58]);
    G(&v[11], &v[27], &v[43], &v[59]);
    G(&v[12], &v[28], &v[44], &v[60]);
    G(&v[13], &v[29], &v[45], &v[61]);
    G(&v[14], &v[30], &v[46], &v[62]);
    G(&v[15], &v[31], &v[47], &v[63]);
}

unsigned long** allocateMatrix(int k, int n) {
    unsigned long** B = (unsigned long**)malloc(k * sizeof(unsigned long*));
    for (int i = 0; i < k; i++) {
        B[i] = (unsigned long*)malloc(n * sizeof(unsigned long));
    }
    return B;
}

unsigned long* allocateKey(int key_size){
    if(key_size % 16 != 0){
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
    key_size /= 8;
    printf("Key: ");
    for (int i = 0; i < key_size; i++) {
        printf("%lx", key[i]);
    }
    printf("\n");
}

void fillBuffer(unsigned long** B, int k, int n, unsigned char *password) {
    //  B[0][0] ... B[0][15] ←  H(P || S)
    spongent256(B, password);

    // Preencher restante da matriz
	int c, l;
	unsigned long *v = (unsigned long*) malloc(16*sizeof(unsigned long));

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

void updateState(unsigned long** B, unsigned long* key, int k, int n, int vec_size, int key_size){
    key_size /= 8;
    int l1, l2, c1, c2;
    l1 = (B[k-1][n-1] % 0x7ab931c7) % k;
	c1 = (B[k-1][n-1] % 0xf107d359) % n;
    int d1, d2;
    int m = k*n/16/key_size;
    unsigned long key_part = 0;
    unsigned long *v = (unsigned long*) malloc(vec_size*sizeof(unsigned long));
    vec_size--;
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
        l2 = (B[mod(l1+vec_size*d1,k)][mod(c1+vec_size*d2,n)] % 0x3e459107) % k;
        c2 = (B[mod(l1+vec_size*d1,k)][mod(c1+vec_size*d2,n)] % 0xf146295e) % n;
        for(int h = 0; h <= vec_size; h++){
	    	v[h] = B[mod(l1+h*d1,k)][mod(c1+h*d2,n)];
	    }
        switch (vec_size) {
            case 15:
                blake2bPermutation(v);
                break;
            case 31:
                permutation32(v);
                break;
            case 47:
                permutation48(v);
                break;
            case 63:
                permutation64(v);
                break;
        }
	    for(int h = 0; h <= vec_size; h++){
	    	B[mod(l2+h*d1,k)][mod(c2+h*d2,n)] = v[h];
	    }
        B[mod(l1+vec_size*d1,k)][mod(c1+vec_size*d2,n)] ^= B[mod(l2+vec_size*d1,k)][mod(c2+vec_size*d2,n)];
        key_part ^= (B[l1][c1] ^ B[l2][c2]);
        if(i%m == 0 && i != 0){
            key[i/m - 1] = key_part;
            key_part = 0;
        }
        l1 = (B[mod(l2+vec_size*d1,k)][mod(c2+vec_size*d2,n)] % 0x7ab931c7) % k;
	    c1 = (B[mod(l2+vec_size*d1,k)][mod(c2+vec_size*d2,n)] % 0xf107d359) % n;
    }
    key[key_size-1] = key_part;
    free(v);
}

int main() {
    int k = 16, n = 16;
    // 11584      16384       20064
    int key_size = 32;
    int v = 16;

    unsigned long** B = allocateMatrix(k, n);
    unsigned long* key = allocateKey(key_size);
    unsigned char *password = "senha_muito_forte";
    fillBuffer(B, k, n, password);
    updateState(B, key, k, n, v, key_size);
    //printMatrix(B, k, n);
    printKey(key, key_size);

    freeMatrix(B, k);
    free(key);
    return 0;
}
