#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include "SABER_params.h"
#include "kem.h"

// SaberX2 parallel functions
extern int crypto_kem_keypair2x(
    unsigned char *pk0, unsigned char *sk0,
    unsigned char *pk1, unsigned char *sk1);

extern int crypto_kem_enc2x(
    unsigned char *ct0, unsigned char *ss0, const unsigned char *pk0,
    unsigned char *ct1, unsigned char *ss1, const unsigned char *pk1);

extern int crypto_kem_dec2x(
    unsigned char *ss0, const unsigned char *ct0, const unsigned char *sk0,
    unsigned char *ss1, const unsigned char *ct1, const unsigned char *sk1);

// Sequential SABER functions
extern int crypto_kem_keypair(unsigned char *pk, unsigned char *sk);
extern int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
extern int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);

#define N_TESTS 5000

// Timer utilities
static inline uint64_t rdtsc() {
    uint64_t val;
    asm volatile("mrs %0, cntvct_el0" : "=r" (val));
    return val;
}

int test_correctness() {
    unsigned char pk0[SABER_PUBLICKEYBYTES];
    unsigned char sk0[SABER_SECRETKEYBYTES];
    unsigned char pk1[SABER_PUBLICKEYBYTES];
    unsigned char sk1[SABER_SECRETKEYBYTES];

    unsigned char ct0[SABER_BYTES_CCA_DEC];
    unsigned char ct1[SABER_BYTES_CCA_DEC];

    unsigned char ss_enc0[SABER_KEYBYTES];
    unsigned char ss_enc1[SABER_KEYBYTES];

    unsigned char ss_dec0[SABER_KEYBYTES];
    unsigned char ss_dec1[SABER_KEYBYTES];

    printf("Testing SaberX2 correctness...\n");

    // Test KeyGen
    crypto_kem_keypair2x(pk0, sk0, pk1, sk1);
    printf("  KeyGen: Generated 2 keypairs\n");

    // Test Encaps
    crypto_kem_enc2x(ct0, ss_enc0, pk0, ct1, ss_enc1, pk1);
    printf("  Encaps: Generated 2 ciphertexts\n");

    // Test Decaps
    crypto_kem_dec2x(ss_dec0, ct0, sk0, ss_dec1, ct1, sk1);
    printf("  Decaps: Decrypted 2 ciphertexts\n");

    // Verify shared secrets match
    int correct0 = (memcmp(ss_enc0, ss_dec0, SABER_KEYBYTES) == 0);
    int correct1 = (memcmp(ss_enc1, ss_dec1, SABER_KEYBYTES) == 0);

    printf("  Instance 0: %s\n", correct0 ? "PASSED" : "FAILED");
    printf("  Instance 1: %s\n", correct1 ? "PASSED" : "FAILED");

    return correct0 && correct1;
}

void benchmark_sequential() {
    unsigned char pk[SABER_PUBLICKEYBYTES];
    unsigned char sk[SABER_SECRETKEYBYTES];
    unsigned char ct[SABER_BYTES_CCA_DEC];
    unsigned char ss_enc[SABER_KEYBYTES];
    unsigned char ss_dec[SABER_KEYBYTES];

    uint64_t cycles_keygen = 0;
    uint64_t cycles_encaps = 0;
    uint64_t cycles_decaps = 0;

    printf("\nBenchmarking Sequential SABER (N=%d)...\n", N_TESTS);

    for (int i = 0; i < N_TESTS; i++) {
        uint64_t t0, t1;

        // KeyGen
        t0 = rdtsc();
        crypto_kem_keypair(pk, sk);
        t1 = rdtsc();
        cycles_keygen += (t1 - t0);

        // Encaps
        t0 = rdtsc();
        crypto_kem_enc(ct, ss_enc, pk);
        t1 = rdtsc();
        cycles_encaps += (t1 - t0);

        // Decaps
        t0 = rdtsc();
        crypto_kem_dec(ss_dec, ct, sk);
        t1 = rdtsc();
        cycles_decaps += (t1 - t0);
    }

    printf("  KeyGen: %llu cycles (avg per operation)\n", cycles_keygen / N_TESTS);
    printf("  Encaps: %llu cycles (avg per operation)\n", cycles_encaps / N_TESTS);
    printf("  Decaps: %llu cycles (avg per operation)\n", cycles_decaps / N_TESTS);
    printf("  Total:  %llu cycles (avg per KEM)\n",
           (cycles_keygen + cycles_encaps + cycles_decaps) / N_TESTS);
}

void benchmark_parallel() {
    unsigned char pk0[SABER_PUBLICKEYBYTES];
    unsigned char sk0[SABER_SECRETKEYBYTES];
    unsigned char pk1[SABER_PUBLICKEYBYTES];
    unsigned char sk1[SABER_SECRETKEYBYTES];

    unsigned char ct0[SABER_BYTES_CCA_DEC];
    unsigned char ct1[SABER_BYTES_CCA_DEC];

    unsigned char ss_enc0[SABER_KEYBYTES];
    unsigned char ss_enc1[SABER_KEYBYTES];

    unsigned char ss_dec0[SABER_KEYBYTES];
    unsigned char ss_dec1[SABER_KEYBYTES];

    uint64_t cycles_keygen = 0;
    uint64_t cycles_encaps = 0;
    uint64_t cycles_decaps = 0;

    printf("\nBenchmarking SaberX2 (N=%d, 2 parallel instances)...\n", N_TESTS);

    for (int i = 0; i < N_TESTS; i++) {
        uint64_t t0, t1;

        // KeyGen x2
        t0 = rdtsc();
        crypto_kem_keypair2x(pk0, sk0, pk1, sk1);
        t1 = rdtsc();
        cycles_keygen += (t1 - t0);

        // Encaps x2
        t0 = rdtsc();
        crypto_kem_enc2x(ct0, ss_enc0, pk0, ct1, ss_enc1, pk1);
        t1 = rdtsc();
        cycles_encaps += (t1 - t0);

        // Decaps x2
        t0 = rdtsc();
        crypto_kem_dec2x(ss_dec0, ct0, sk0, ss_dec1, ct1, sk1);
        t1 = rdtsc();
        cycles_decaps += (t1 - t0);
    }

    printf("  KeyGen: %llu cycles (avg for 2 parallel operations)\n", cycles_keygen / N_TESTS);
    printf("  Encaps: %llu cycles (avg for 2 parallel operations)\n", cycles_encaps / N_TESTS);
    printf("  Decaps: %llu cycles (avg for 2 parallel operations)\n", cycles_decaps / N_TESTS);
    printf("  Total:  %llu cycles (avg for 2 parallel KEMs)\n",
           (cycles_keygen + cycles_encaps + cycles_decaps) / N_TESTS);

    // Calculate per-operation average (divide by 2 since we do 2 operations)
    printf("\n  Per-operation averages:\n");
    printf("    KeyGen: %llu cycles\n", cycles_keygen / (N_TESTS * 2));
    printf("    Encaps: %llu cycles\n", cycles_encaps / (N_TESTS * 2));
    printf("    Decaps: %llu cycles\n", cycles_decaps / (N_TESTS * 2));
    printf("    Total:  %llu cycles\n",
           (cycles_keygen + cycles_encaps + cycles_decaps) / (N_TESTS * 2));
}

void print_speedup(uint64_t seq_keygen, uint64_t seq_encaps, uint64_t seq_decaps,
                   uint64_t par_keygen, uint64_t par_encaps, uint64_t par_decaps) {
    printf("\n========== SPEEDUP ANALYSIS ==========\n");

    double speedup_keygen = (double)seq_keygen / par_keygen;
    double speedup_encaps = (double)seq_encaps / par_encaps;
    double speedup_decaps = (double)seq_decaps / par_decaps;
    double speedup_total = (double)(seq_keygen + seq_encaps + seq_decaps) /
                          (par_keygen + par_encaps + par_decaps);

    printf("KeyGen speedup: %.3fx\n", speedup_keygen);
    printf("Encaps speedup: %.3fx\n", speedup_encaps);
    printf("Decaps speedup: %.3fx\n", speedup_decaps);
    printf("Overall speedup: %.3fx\n", speedup_total);

    printf("\nThroughput improvement: %.1f%%\n",
           (speedup_total - 1.0) * 100.0);
}

int main() {
    printf("===========================================\n");
    printf("  SaberX2 Complete Implementation Test\n");
    printf("  With Parallel Matrix Multiplication\n");
    printf("===========================================\n\n");

    // Test correctness first
    if (!test_correctness()) {
        printf("\n[ERROR] Correctness test failed!\n");
        return 1;
    }

    printf("\n[SUCCESS] Correctness test passed!\n");

    // Store sequential results
    unsigned char pk[SABER_PUBLICKEYBYTES];
    unsigned char sk[SABER_SECRETKEYBYTES];
    unsigned char ct[SABER_BYTES_CCA_DEC];
    unsigned char ss_enc[SABER_KEYBYTES];
    unsigned char ss_dec[SABER_KEYBYTES];

    uint64_t seq_keygen = 0, seq_encaps = 0, seq_decaps = 0;

    printf("\nBenchmarking Sequential SABER (N=%d)...\n", N_TESTS);
    for (int i = 0; i < N_TESTS; i++) {
        uint64_t t0, t1;
        t0 = rdtsc();
        crypto_kem_keypair(pk, sk);
        t1 = rdtsc();
        seq_keygen += (t1 - t0);

        t0 = rdtsc();
        crypto_kem_enc(ct, ss_enc, pk);
        t1 = rdtsc();
        seq_encaps += (t1 - t0);

        t0 = rdtsc();
        crypto_kem_dec(ss_dec, ct, sk);
        t1 = rdtsc();
        seq_decaps += (t1 - t0);
    }

    seq_keygen /= N_TESTS;
    seq_encaps /= N_TESTS;
    seq_decaps /= N_TESTS;

    printf("  KeyGen: %llu cycles\n", seq_keygen);
    printf("  Encaps: %llu cycles\n", seq_encaps);
    printf("  Decaps: %llu cycles\n", seq_decaps);

    // Store parallel results
    unsigned char pk0[SABER_PUBLICKEYBYTES];
    unsigned char sk0[SABER_SECRETKEYBYTES];
    unsigned char pk1[SABER_PUBLICKEYBYTES];
    unsigned char sk1[SABER_SECRETKEYBYTES];
    unsigned char ct0[SABER_BYTES_CCA_DEC];
    unsigned char ct1[SABER_BYTES_CCA_DEC];
    unsigned char ss_enc0[SABER_KEYBYTES];
    unsigned char ss_enc1[SABER_KEYBYTES];
    unsigned char ss_dec0[SABER_KEYBYTES];
    unsigned char ss_dec1[SABER_KEYBYTES];

    uint64_t par_keygen = 0, par_encaps = 0, par_decaps = 0;

    printf("\nBenchmarking SaberX2 (N=%d, 2 parallel)...\n", N_TESTS);
    for (int i = 0; i < N_TESTS; i++) {
        uint64_t t0, t1;
        t0 = rdtsc();
        crypto_kem_keypair2x(pk0, sk0, pk1, sk1);
        t1 = rdtsc();
        par_keygen += (t1 - t0);

        t0 = rdtsc();
        crypto_kem_enc2x(ct0, ss_enc0, pk0, ct1, ss_enc1, pk1);
        t1 = rdtsc();
        par_encaps += (t1 - t0);

        t0 = rdtsc();
        crypto_kem_dec2x(ss_dec0, ct0, sk0, ss_dec1, ct1, sk1);
        t1 = rdtsc();
        par_decaps += (t1 - t0);
    }

    par_keygen /= (N_TESTS * 2);  // Per-operation average
    par_encaps /= (N_TESTS * 2);
    par_decaps /= (N_TESTS * 2);

    printf("  KeyGen: %llu cycles (per operation)\n", par_keygen);
    printf("  Encaps: %llu cycles (per operation)\n", par_encaps);
    printf("  Decaps: %llu cycles (per operation)\n", par_decaps);

    print_speedup(seq_keygen, seq_encaps, seq_decaps,
                  par_keygen, par_encaps, par_decaps);

    return 0;
}
