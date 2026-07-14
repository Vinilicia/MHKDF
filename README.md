# Secure Memory-Hard Cryptographic Hash Function (Argon2-Inspired)

An advanced, cache miss based cryptographic key derivation and password hashing function written in **C**. Designed as a graduation thesis (TCC) at **Universidade Federal de São João del-Rei (UFSJ)**, this algorithm implements memory-hard compute concepts to defend credential storage against hardware-accelerated (GPU/ASIC) brute-force attacks.

The project achieved a **maximum grade of 10/10** for its implementation, performance analysis, and security design.

---

## Core Architecture & Security Concepts

Traditional hashing algorithms (like MD5 or SHA-256) are extremely fast, making them highly vulnerable to parallelized offline dictionary attacks using GPUs or ASICs. This project addresses this vulnerability using **Memory-Hard Functions (MHFs)**, heavily inspired by the Argon2 design:

*   **Cache-Miss Driven Security:** Utilizes pseudo-random multidimensional matrix indexing (`l1`, `c1`, `l2`, `c2`) dependent on the internal state. This forces predictable cache misses at the CPU level, creating a massive bandwidth bottleneck for highly parallelized cracking hardware (ASICs/GPUs) trying to bypass memory access.
*   **Flexible Permutations:** Implements customized Blake2b mixing permutations across multiple vector sizes (16, 32, 48, and 64 longs) to achieve strong avalanche effects and high diffusion.
*   **Configurable Time and Memory Costs:** Employs a dynamic matrix allocation representing memory cost ($k \times n$ matrix size) alongside custom update rounds to control computation time independently.
*   **Hybrid Hashing:** Integrates OpenSSL's EVP API (SHA-256) for secure initial entropy injection (`H(Password || Salt)`).

---

## Code Features & Low-Level Design

The codebase highlights advanced **C system programming** and security best practices:
*   **Dynamic Resource Allocation:** Strict multi-dimensional memory layout handling (`allocateMatrix` & `freeMatrix`).
*   **Side-Channel & Leak Protection:** Programmed to cleanly deallocate state-holding matrices and vectors, avoiding memory leaks.
*   **Cryptographic Primitives:** Uses the industry-standard `OpenSSL/EVP` for SHA-256 base hashes combined with Custom Blake2b permutation functions for non-linear state permutations.

---

## How It Works (High-Level Execution Flow)

1. **Entropy Initialization (`fillBuffer`):**
   The password input is hashed using SHA-256 to seed the initial matrix cells ($B[0][0 \dots 15]$).
2. **Initial Diffusion:**
   Rows are populated sequentially using a custom 16-word Blake2b permutation.
3. **State Mutation & Mixing (`updateState`):**
   The matrix undergoes intensive pseudo-random non-linear permutations depending on coordinate hopping vectors. Dynamic directional paths ($d1, d2$) dictate memory hops.
4. **Key Derivation (`printKey`):**
   A final secure key of configurable size is derived from XORing the accumulated state matrix.

---

## Getting Started & Execution

### Prerequisites
Make sure you have `gcc` and the `OpenSSL` development libraries installed on your system.

**On Debian/Ubuntu-based systems:**
```
sudo apt-get update
sudo apt-get install build-essential libssl-dev
```

## Quick Start (Build & Run)
To compile the source code (with optimization flags) and execute the program immediately, simply run:

```
make
```

## Technical Specifications (Default Configuration)
- Memory Matrix Size ($k \times n$): $160 \times 160$ (configurable)
- Vector Size ($v$): 16, 32, 48 or 64
- Derived Key Size: 32 Bytes
- Base Primitive: SHA-256 + Custom Blake2b permutations
