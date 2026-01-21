/**
 * @file test_neon_batch2_correctness.c
 * @brief Comprehensive correctness tests for TRUE NEON batching
 *
 * Tests all levels of batching implementation:
 * 1. Basic polynomial operations
 * 2. Matrix operations
 * 3. CPA operations
 * 4. Full KEM operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "batch/neon_batch2_core.h"
#include "batch/neon_batch2_cpa.h"
#include "batch/neon_batch2_kem.h"
#include "params.h"
#include "rng.h"
#include "poly.h"
#include "core.h"

// Colors for output
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_RESET   "\x1b[0m"

// Test statistics
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_START(name) \
    do { \
        printf(COLOR_BLUE "TEST: " COLOR_RESET "%s ... ", name); \
        fflush(stdout); \
    } while(0)

#define TEST_PASS() \
    do { \
        printf(COLOR_GREEN "PASS" COLOR_RESET "\n"); \
        tests_passed++; \
    } while(0)

#define TEST_FAIL(msg) \
    do { \
        printf(COLOR_RED "FAIL" COLOR_RESET " - %s\n", msg); \
        tests_failed++; \
    } while(0)

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            TEST_FAIL(msg); \
            return -1; \
        } \
    } while(0)

// ============================================================================
// LEVEL 1: BASIC POLYNOMIAL OPERATIONS
// ============================================================================

int test_poly_add(void) {
    TEST_START("Polynomial addition (batch2)");

    uint16_t a0[SABER_N], a1[SABER_N];
    uint16_t b0[SABER_N], b1[SABER_N];
    uint16_t c0_batch[SABER_N], c1_batch[SABER_N];
    uint16_t c0_ref[SABER_N], c1_ref[SABER_N];

    // Initialize with random values
    for (int i = 0; i < SABER_N; i++) {
        a0[i] = rand() % SABER_Q;
        a1[i] = rand() % SABER_Q;
        b0[i] = rand() % SABER_Q;
        b1[i] = rand() % SABER_Q;
    }

    // Compute using batch function
    neon_batch2_poly_add(c0_batch, c1_batch, a0, a1, b0, b1);

    // Compute reference
    for (int i = 0; i < SABER_N; i++) {
        c0_ref[i] = a0[i] + b0[i];
        c1_ref[i] = a1[i] + b1[i];
    }

    // Compare results
    for (int i = 0; i < SABER_N; i++) {
        if (c0_batch[i] != c0_ref[i]) {
            char msg[100];
            snprintf(msg, sizeof(msg), "c0[%d]: got %u, expected %u", i, c0_batch[i], c0_ref[i]);
            TEST_FAIL(msg);
            return -1;
        }
        if (c1_batch[i] != c1_ref[i]) {
            char msg[100];
            snprintf(msg, sizeof(msg), "c1[%d]: got %u, expected %u", i, c1_batch[i], c1_ref[i]);
            TEST_FAIL(msg);
            return -1;
        }
    }

    TEST_PASS();
    return 0;
}

int test_poly_sub(void) {
    TEST_START("Polynomial subtraction (batch2)");

    uint16_t a0[SABER_N], a1[SABER_N];
    uint16_t b0[SABER_N], b1[SABER_N];
    uint16_t c0_batch[SABER_N], c1_batch[SABER_N];
    uint16_t c0_ref[SABER_N], c1_ref[SABER_N];

    // Initialize with random values
    for (int i = 0; i < SABER_N; i++) {
        a0[i] = rand() % SABER_Q;
        a1[i] = rand() % SABER_Q;
        b0[i] = rand() % SABER_Q;
        b1[i] = rand() % SABER_Q;
    }

    // Compute using batch function
    neon_batch2_poly_sub(c0_batch, c1_batch, a0, a1, b0, b1);

    // Compute reference
    for (int i = 0; i < SABER_N; i++) {
        c0_ref[i] = a0[i] - b0[i];
        c1_ref[i] = a1[i] - b1[i];
    }

    // Compare results
    for (int i = 0; i < SABER_N; i++) {
        if (c0_batch[i] != c0_ref[i]) {
            char msg[100];
            snprintf(msg, sizeof(msg), "c0[%d]: got %u, expected %u", i, c0_batch[i], c0_ref[i]);
            TEST_FAIL(msg);
            return -1;
        }
        if (c1_batch[i] != c1_ref[i]) {
            char msg[100];
            snprintf(msg, sizeof(msg), "c1[%d]: got %u, expected %u", i, c1_batch[i], c1_ref[i]);
            TEST_FAIL(msg);
            return -1;
        }
    }

    TEST_PASS();
    return 0;
}

int test_interleave(void) {
    TEST_START("Polynomial interleave/deinterleave");

    uint16_t a0[SABER_N], a1[SABER_N];
    uint16_t interleaved[2 * SABER_N];
    uint16_t a0_out[SABER_N], a1_out[SABER_N];

    // Initialize with known pattern
    for (int i = 0; i < SABER_N; i++) {
        a0[i] = i;
        a1[i] = i + 1000;
    }

    // Interleave
    neon_batch2_interleave(interleaved, a0, a1, SABER_N);

    // Check interleaving pattern
    for (int i = 0; i < SABER_N; i++) {
        if (interleaved[2*i] != a0[i]) {
            char msg[100];
            snprintf(msg, sizeof(msg), "interleaved[%d]: got %u, expected %u",
                     2*i, interleaved[2*i], a0[i]);
            TEST_FAIL(msg);
            return -1;
        }
        if (interleaved[2*i + 1] != a1[i]) {
            char msg[100];
            snprintf(msg, sizeof(msg), "interleaved[%d]: got %u, expected %u",
                     2*i+1, interleaved[2*i+1], a1[i]);
            TEST_FAIL(msg);
            return -1;
        }
    }

    // De-interleave
    neon_batch2_deinterleave(a0_out, a1_out, interleaved, SABER_N);

    // Check de-interleaving
    for (int i = 0; i < SABER_N; i++) {
        if (a0_out[i] != a0[i]) {
            char msg[100];
            snprintf(msg, sizeof(msg), "a0_out[%d]: got %u, expected %u", i, a0_out[i], a0[i]);
            TEST_FAIL(msg);
            return -1;
        }
        if (a1_out[i] != a1[i]) {
            char msg[100];
            snprintf(msg, sizeof(msg), "a1_out[%d]: got %u, expected %u", i, a1_out[i], a1[i]);
            TEST_FAIL(msg);
            return -1;
        }
    }

    TEST_PASS();
    return 0;
}

// ============================================================================
// LEVEL 2: CPA OPERATIONS
// ============================================================================

int test_cpa_keypair(void) {
    TEST_START("CPA keypair generation (batch2)");

    uint8_t pk0[SABER_INDCPA_PUBLICKEYBYTES];
    uint8_t pk1[SABER_INDCPA_PUBLICKEYBYTES];
    uint8_t sk0[SABER_INDCPA_SECRETKEYBYTES];
    uint8_t sk1[SABER_INDCPA_SECRETKEYBYTES];

    // Generate keypairs
    int ret = neon_batch2_indcpa_kem_keypair(pk0, pk1, sk0, sk1);
    ASSERT(ret == 0, "keypair generation failed");

    // Basic sanity checks
    // Check that public keys are different (with very high probability)
    int pk_same = 1;
    for (int i = 0; i < SABER_INDCPA_PUBLICKEYBYTES; i++) {
        if (pk0[i] != pk1[i]) {
            pk_same = 0;
            break;
        }
    }
    ASSERT(!pk_same, "public keys are identical (should be different)");

    // Check that secret keys are different
    int sk_same = 1;
    for (int i = 0; i < SABER_INDCPA_SECRETKEYBYTES; i++) {
        if (sk0[i] != sk1[i]) {
            sk_same = 0;
            break;
        }
    }
    ASSERT(!sk_same, "secret keys are identical (should be different)");

    // Check that seed_A is the same (last 32 bytes of pk should match)
    int seed_same = 1;
    for (int i = 0; i < 32; i++) {
        if (pk0[SABER_POLYVECCOMPRESSEDBYTES + i] != pk1[SABER_POLYVECCOMPRESSEDBYTES + i]) {
            seed_same = 0;
            break;
        }
    }
    ASSERT(seed_same, "seed_A should be the same for both public keys");

    TEST_PASS();
    return 0;
}

int test_cpa_enc_dec(void) {
    TEST_START("CPA encrypt/decrypt (batch2)");

    uint8_t pk0[SABER_INDCPA_PUBLICKEYBYTES];
    uint8_t pk1[SABER_INDCPA_PUBLICKEYBYTES];
    uint8_t sk0[SABER_INDCPA_SECRETKEYBYTES];
    uint8_t sk1[SABER_INDCPA_SECRETKEYBYTES];

    // Generate keypairs
    neon_batch2_indcpa_kem_keypair(pk0, pk1, sk0, sk1);

    // Random messages
    uint8_t m0[SABER_KEYBYTES];
    uint8_t m1[SABER_KEYBYTES];
    randombytes(m0, SABER_KEYBYTES);
    randombytes(m1, SABER_KEYBYTES);

    // Random seeds for encryption
    uint8_t seed0[SABER_NOISE_SEEDBYTES];
    uint8_t seed1[SABER_NOISE_SEEDBYTES];
    randombytes(seed0, SABER_NOISE_SEEDBYTES);
    randombytes(seed1, SABER_NOISE_SEEDBYTES);

    // Encrypt
    uint8_t ct0[SABER_BYTES_CCA_DEC];
    uint8_t ct1[SABER_BYTES_CCA_DEC];
    int ret = neon_batch2_indcpa_kem_enc(ct0, ct1, m0, m1, seed0, seed1, pk0, pk1);
    ASSERT(ret == 0, "encryption failed");

    // Decrypt
    uint8_t m0_dec[SABER_KEYBYTES];
    uint8_t m1_dec[SABER_KEYBYTES];
    ret = neon_batch2_indcpa_kem_dec(m0_dec, m1_dec, ct0, ct1, sk0, sk1);
    ASSERT(ret == 0, "decryption failed");

    // Verify messages match
    for (int i = 0; i < SABER_KEYBYTES; i++) {
        if (m0[i] != m0_dec[i]) {
            char msg[100];
            snprintf(msg, sizeof(msg), "m0[%d]: got %u, expected %u", i, m0_dec[i], m0[i]);
            TEST_FAIL(msg);
            return -1;
        }
        if (m1[i] != m1_dec[i]) {
            char msg[100];
            snprintf(msg, sizeof(msg), "m1[%d]: got %u, expected %u", i, m1_dec[i], m1[i]);
            TEST_FAIL(msg);
            return -1;
        }
    }

    TEST_PASS();
    return 0;
}

// ============================================================================
// LEVEL 3: FULL KEM OPERATIONS
// ============================================================================

int test_kem_keypair(void) {
    TEST_START("KEM keypair generation (batch2)");

    uint8_t pk0[SABER_PUBLICKEYBYTES];
    uint8_t pk1[SABER_PUBLICKEYBYTES];
    uint8_t sk0[SABER_SECRETKEYBYTES];
    uint8_t sk1[SABER_SECRETKEYBYTES];

    int ret = neon_batch2_crypto_kem_keypair(pk0, pk1, sk0, sk1);
    ASSERT(ret == 0, "keypair generation failed");

    // Check that keys are different
    int pk_same = 1;
    for (int i = 0; i < SABER_PUBLICKEYBYTES; i++) {
        if (pk0[i] != pk1[i]) {
            pk_same = 0;
            break;
        }
    }
    ASSERT(!pk_same, "public keys should be different");

    int sk_same = 1;
    for (int i = 0; i < SABER_SECRETKEYBYTES; i++) {
        if (sk0[i] != sk1[i]) {
            sk_same = 0;
            break;
        }
    }
    ASSERT(!sk_same, "secret keys should be different");

    TEST_PASS();
    return 0;
}

int test_kem_encaps_decaps(void) {
    TEST_START("KEM encapsulate/decapsulate (batch2)");

    uint8_t pk0[SABER_PUBLICKEYBYTES];
    uint8_t pk1[SABER_PUBLICKEYBYTES];
    uint8_t sk0[SABER_SECRETKEYBYTES];
    uint8_t sk1[SABER_SECRETKEYBYTES];

    // Generate keypairs
    neon_batch2_crypto_kem_keypair(pk0, pk1, sk0, sk1);

    // Encapsulate
    uint8_t ct0[SABER_BYTES_CCA_DEC];
    uint8_t ct1[SABER_BYTES_CCA_DEC];
    uint8_t ss0_enc[SABER_KEYBYTES];
    uint8_t ss1_enc[SABER_KEYBYTES];

    int ret = neon_batch2_crypto_kem_enc(ct0, ct1, ss0_enc, ss1_enc, pk0, pk1);
    ASSERT(ret == 0, "encapsulation failed");

    // Decapsulate
    uint8_t ss0_dec[SABER_KEYBYTES];
    uint8_t ss1_dec[SABER_KEYBYTES];

    ret = neon_batch2_crypto_kem_dec(ss0_dec, ss1_dec, ct0, ct1, sk0, sk1);
    ASSERT(ret == 0, "decapsulation failed");

    // Verify shared secrets match
    for (int i = 0; i < SABER_KEYBYTES; i++) {
        if (ss0_enc[i] != ss0_dec[i]) {
            char msg[100];
            snprintf(msg, sizeof(msg), "ss0[%d]: enc=%u, dec=%u", i, ss0_enc[i], ss0_dec[i]);
            TEST_FAIL(msg);
            return -1;
        }
        if (ss1_enc[i] != ss1_dec[i]) {
            char msg[100];
            snprintf(msg, sizeof(msg), "ss1[%d]: enc=%u, dec=%u", i, ss1_enc[i], ss1_dec[i]);
            TEST_FAIL(msg);
            return -1;
        }
    }

    TEST_PASS();
    return 0;
}

int test_kem_implicit_rejection(void) {
    TEST_START("KEM implicit rejection (batch2)");

    uint8_t pk0[SABER_PUBLICKEYBYTES];
    uint8_t pk1[SABER_PUBLICKEYBYTES];
    uint8_t sk0[SABER_SECRETKEYBYTES];
    uint8_t sk1[SABER_SECRETKEYBYTES];

    // Generate keypairs
    neon_batch2_crypto_kem_keypair(pk0, pk1, sk0, sk1);

    // Encapsulate
    uint8_t ct0[SABER_BYTES_CCA_DEC];
    uint8_t ct1[SABER_BYTES_CCA_DEC];
    uint8_t ss0_enc[SABER_KEYBYTES];
    uint8_t ss1_enc[SABER_KEYBYTES];

    neon_batch2_crypto_kem_enc(ct0, ct1, ss0_enc, ss1_enc, pk0, pk1);

    // Corrupt ciphertext
    ct0[0] ^= 1;

    // Decapsulate with corrupted ciphertext
    uint8_t ss0_dec[SABER_KEYBYTES];
    uint8_t ss1_dec[SABER_KEYBYTES];

    int ret = neon_batch2_crypto_kem_dec(ss0_dec, ss1_dec, ct0, ct1, sk0, sk1);
    ASSERT(ret == 0, "decapsulation should not fail (implicit rejection)");

    // Verify that ss0 is DIFFERENT from original (implicit rejection)
    int ss0_same = 1;
    for (int i = 0; i < SABER_KEYBYTES; i++) {
        if (ss0_enc[i] != ss0_dec[i]) {
            ss0_same = 0;
            break;
        }
    }
    ASSERT(!ss0_same, "ss0 should be different after corruption (implicit rejection)");

    // Verify that ss1 is the SAME (not corrupted)
    for (int i = 0; i < SABER_KEYBYTES; i++) {
        if (ss1_enc[i] != ss1_dec[i]) {
            char msg[100];
            snprintf(msg, sizeof(msg), "ss1[%d] changed despite valid ct1", i);
            TEST_FAIL(msg);
            return -1;
        }
    }

    TEST_PASS();
    return 0;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
    printf("\n");
    printf(COLOR_BLUE "========================================\n" COLOR_RESET);
    printf(COLOR_BLUE "  NEON BATCH2 CORRECTNESS TESTS\n" COLOR_RESET);
    printf(COLOR_BLUE "========================================\n" COLOR_RESET);
    printf("\n");

    // Seed RNG
    srand(time(NULL));

    printf(COLOR_YELLOW "Level 1: Basic Operations\n" COLOR_RESET);
    printf("----------------------------------------\n");
    test_poly_add();
    test_poly_sub();
    test_interleave();
    printf("\n");

    printf(COLOR_YELLOW "Level 2: CPA Operations\n" COLOR_RESET);
    printf("----------------------------------------\n");
    test_cpa_keypair();
    test_cpa_enc_dec();
    printf("\n");

    printf(COLOR_YELLOW "Level 3: Full KEM Operations\n" COLOR_RESET);
    printf("----------------------------------------\n");
    test_kem_keypair();
    test_kem_encaps_decaps();
    test_kem_implicit_rejection();
    printf("\n");

    // Summary
    printf(COLOR_BLUE "========================================\n" COLOR_RESET);
    printf(COLOR_BLUE "  TEST SUMMARY\n" COLOR_RESET);
    printf(COLOR_BLUE "========================================\n" COLOR_RESET);
    printf("Total tests:   %d\n", tests_passed + tests_failed);
    printf(COLOR_GREEN "Passed:        %d\n" COLOR_RESET, tests_passed);
    if (tests_failed > 0) {
        printf(COLOR_RED "Failed:        %d\n" COLOR_RESET, tests_failed);
    } else {
        printf("Failed:        %d\n", tests_failed);
    }
    printf("\n");

    if (tests_failed == 0) {
        printf(COLOR_GREEN "✓ ALL TESTS PASSED!\n" COLOR_RESET);
        return 0;
    } else {
        printf(COLOR_RED "✗ SOME TESTS FAILED\n" COLOR_RESET);
        return 1;
    }
}