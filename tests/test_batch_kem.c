/**
 * @file test_batch_kem.c
 * @brief Test suite for batched KEM operations
 *
 * Verifies correctness and measures performance of 2x parallel batching.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "api.h"
#include "batch/batch_kem.h"

#define ITERATIONS 1000
#define TEST_BATCH_SIZE 2

// ANSI color codes for output
#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define YELLOW "\033[0;33m"
#define RESET "\033[0m"

void print_header() {
    printf("\n");
    printf("=================================================\n");
    printf("       SABER Batching Test Suite\n");
    printf("=================================================\n\n");
}

/**
 * Test correctness of batched operations
 */
int test_batch_correctness() {
    printf("Testing Batch Correctness\n");
    printf("-------------------------------------------------\n");

    // Arrays for batched operations
    uint8_t pk_batch[TEST_BATCH_SIZE][CRYPTO_PUBLICKEYBYTES];
    uint8_t sk_batch[TEST_BATCH_SIZE][CRYPTO_SECRETKEYBYTES];
    uint8_t ct_batch[TEST_BATCH_SIZE][CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss_enc_batch[TEST_BATCH_SIZE][CRYPTO_BYTES];
    uint8_t ss_dec_batch[TEST_BATCH_SIZE][CRYPTO_BYTES];

    // Arrays for sequential operations (for comparison)
    uint8_t pk_seq[TEST_BATCH_SIZE][CRYPTO_PUBLICKEYBYTES];
    uint8_t sk_seq[TEST_BATCH_SIZE][CRYPTO_SECRETKEYBYTES];
    uint8_t ct_seq[TEST_BATCH_SIZE][CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss_enc_seq[TEST_BATCH_SIZE][CRYPTO_BYTES];
    uint8_t ss_dec_seq[TEST_BATCH_SIZE][CRYPTO_BYTES];

    // Test 1: Batch key generation
    printf("1. Testing batch keygen... ");
    if (saber_batch_keygen(pk_batch, sk_batch, TEST_BATCH_SIZE) != 0) {
        printf(RED "FAILED" RESET " (batch keygen error)\n");
        return -1;
    }

    // Generate sequential keypairs for comparison
    for (int i = 0; i < TEST_BATCH_SIZE; i++) {
        if (crypto_kem_keypair(pk_seq[i], sk_seq[i]) != 0) {
            printf(RED "FAILED" RESET " (seq keygen error)\n");
            return -1;
        }
    }
    printf(GREEN "OK" RESET "\n");

    // Test 2: Batch encapsulation
    printf("2. Testing batch encaps... ");
    if (saber_batch_encaps(ct_batch, ss_enc_batch,
                          (const uint8_t (*)[CRYPTO_PUBLICKEYBYTES])pk_batch,
                          TEST_BATCH_SIZE) != 0) {
        printf(RED "FAILED" RESET " (batch encaps error)\n");
        return -1;
    }

    // Sequential encapsulation
    for (int i = 0; i < TEST_BATCH_SIZE; i++) {
        if (crypto_kem_enc(ct_seq[i], ss_enc_seq[i], pk_seq[i]) != 0) {
            printf(RED "FAILED" RESET " (seq encaps error)\n");
            return -1;
        }
    }
    printf(GREEN "OK" RESET "\n");

    // Test 3: Batch decapsulation
    printf("3. Testing batch decaps... ");
    if (saber_batch_decaps(ss_dec_batch,
                          (const uint8_t (*)[CRYPTO_CIPHERTEXTBYTES])ct_batch,
                          (const uint8_t (*)[CRYPTO_SECRETKEYBYTES])sk_batch,
                          TEST_BATCH_SIZE) != 0) {
        printf(RED "FAILED" RESET " (batch decaps error)\n");
        return -1;
    }

    // Sequential decapsulation
    for (int i = 0; i < TEST_BATCH_SIZE; i++) {
        if (crypto_kem_dec(ss_dec_seq[i], ct_seq[i], sk_seq[i]) != 0) {
            printf(RED "FAILED" RESET " (seq decaps error)\n");
            return -1;
        }
    }
    printf(GREEN "OK" RESET "\n");

    // Test 4: Verify shared secrets match
    printf("4. Verifying shared secrets... ");
    int all_match = 1;
    for (int i = 0; i < TEST_BATCH_SIZE; i++) {
        // Check batch enc/dec match
        if (memcmp(ss_enc_batch[i], ss_dec_batch[i], CRYPTO_BYTES) != 0) {
            printf(RED "FAILED" RESET " (batch ss[%d] mismatch)\n", i);
            all_match = 0;
        }
        // Check sequential enc/dec match
        if (memcmp(ss_enc_seq[i], ss_dec_seq[i], CRYPTO_BYTES) != 0) {
            printf(RED "FAILED" RESET " (seq ss[%d] mismatch)\n", i);
            all_match = 0;
        }
    }

    if (all_match) {
        printf(GREEN "MATCH" RESET "\n");
    } else {
        return -1;
    }

    // Test 5: Cross-compatibility (batch encaps, sequential decaps)
    printf("5. Testing cross-compatibility... ");
    uint8_t ss_cross[CRYPTO_BYTES];

    // Use batch-generated keypair and ciphertext with sequential decaps
    if (crypto_kem_dec(ss_cross, ct_batch[0], sk_batch[0]) != 0) {
        printf(RED "FAILED" RESET " (cross decaps error)\n");
        return -1;
    }

    if (memcmp(ss_cross, ss_enc_batch[0], CRYPTO_BYTES) != 0) {
        printf(RED "FAILED" RESET " (cross compatibility mismatch)\n");
        return -1;
    }

    printf(GREEN "OK" RESET "\n");

    return 0;
}

/**
 * Benchmark batched vs sequential performance
 */
void benchmark_batch_performance() {
    printf("\nPerformance Benchmarks\n");
    printf("-------------------------------------------------\n");
    printf("Iterations: %d\n\n", ITERATIONS);

    // Arrays for operations
    uint8_t pk[TEST_BATCH_SIZE][CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[TEST_BATCH_SIZE][CRYPTO_SECRETKEYBYTES];
    uint8_t ct[TEST_BATCH_SIZE][CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss_enc[TEST_BATCH_SIZE][CRYPTO_BYTES];
    uint8_t ss_dec[TEST_BATCH_SIZE][CRYPTO_BYTES];

    clock_t start, end;
    double time_seq, time_batch;

    // Initialize batching system
    saber_batch_init();

    // Benchmark key generation
    printf("Key Generation:\n");

    // Sequential
    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (int i = 0; i < TEST_BATCH_SIZE; i++) {
            crypto_kem_keypair(pk[i], sk[i]);
        }
    }
    end = clock();
    time_seq = ((double)(end - start)) / CLOCKS_PER_SEC;

    // Batched
    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        saber_batch_keygen(pk, sk, TEST_BATCH_SIZE);
    }
    end = clock();
    time_batch = ((double)(end - start)) / CLOCKS_PER_SEC;

    printf("  Sequential (2 ops): %.3f ms/iter\n", time_seq * 1000 / ITERATIONS);
    printf("  Batched (2x parallel): %.3f ms/iter\n", time_batch * 1000 / ITERATIONS);
    printf("  " YELLOW "Speedup: %.2fx" RESET "\n\n", time_seq / time_batch);

    // Benchmark encapsulation
    printf("Encapsulation:\n");

    // Generate keypairs for testing
    saber_batch_keygen(pk, sk, TEST_BATCH_SIZE);

    // Sequential
    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (int i = 0; i < TEST_BATCH_SIZE; i++) {
            crypto_kem_enc(ct[i], ss_enc[i], pk[i]);
        }
    }
    end = clock();
    time_seq = ((double)(end - start)) / CLOCKS_PER_SEC;

    // Batched
    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        saber_batch_encaps(ct, ss_enc, (const uint8_t (*)[CRYPTO_PUBLICKEYBYTES])pk, TEST_BATCH_SIZE);
    }
    end = clock();
    time_batch = ((double)(end - start)) / CLOCKS_PER_SEC;

    printf("  Sequential (2 ops): %.3f ms/iter\n", time_seq * 1000 / ITERATIONS);
    printf("  Batched (2x parallel): %.3f ms/iter\n", time_batch * 1000 / ITERATIONS);
    printf("  " YELLOW "Speedup: %.2fx" RESET "\n\n", time_seq / time_batch);

    // Benchmark decapsulation
    printf("Decapsulation:\n");

    // Sequential
    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (int i = 0; i < TEST_BATCH_SIZE; i++) {
            crypto_kem_dec(ss_dec[i], ct[i], sk[i]);
        }
    }
    end = clock();
    time_seq = ((double)(end - start)) / CLOCKS_PER_SEC;

    // Batched
    start = clock();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        saber_batch_decaps(ss_dec,
                          (const uint8_t (*)[CRYPTO_CIPHERTEXTBYTES])ct,
                          (const uint8_t (*)[CRYPTO_SECRETKEYBYTES])sk,
                          TEST_BATCH_SIZE);
    }
    end = clock();
    time_batch = ((double)(end - start)) / CLOCKS_PER_SEC;

    printf("  Sequential (2 ops): %.3f ms/iter\n", time_seq * 1000 / ITERATIONS);
    printf("  Batched (2x parallel): %.3f ms/iter\n", time_batch * 1000 / ITERATIONS);
    printf("  " YELLOW "Speedup: %.2fx" RESET "\n\n", time_seq / time_batch);

    // Cleanup
    saber_batch_cleanup();
}

/**
 * Test edge cases
 */
int test_edge_cases() {
    printf("\nTesting Edge Cases\n");
    printf("-------------------------------------------------\n");

    uint8_t pk[2][CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[2][CRYPTO_SECRETKEYBYTES];

    // Test 1: Single operation (should fall back to sequential)
    printf("1. Single operation fallback... ");
    if (saber_batch_keygen(pk, sk, 1) != 0) {
        printf(RED "FAILED" RESET "\n");
        return -1;
    }
    printf(GREEN "OK" RESET "\n");

    // Test 2: Invalid batch size (0)
    printf("2. Invalid batch size (0)... ");
    if (saber_batch_keygen(pk, sk, 0) == 0) {
        printf(RED "FAILED" RESET " (should reject 0)\n");
        return -1;
    }
    printf(GREEN "OK" RESET " (correctly rejected)\n");

    // Test 3: Invalid batch size (>2)
    printf("3. Invalid batch size (>2)... ");
    if (saber_batch_keygen(pk, sk, 3) == 0) {
        printf(RED "FAILED" RESET " (should reject >2)\n");
        return -1;
    }
    printf(GREEN "OK" RESET " (correctly rejected)\n");

    return 0;
}

int main() {
    print_header();

    // Show configuration
    printf("Configuration: %s\n", saber_batch_get_config());
    printf("Max Batch Size: %d\n", SABER_MAX_BATCH);
    printf("Architecture: ARM64 with NEON\n\n");

    // Initialize batching
    if (saber_batch_init() != 0) {
        printf(RED "ERROR: Failed to initialize batching (NEON required)\n" RESET);
        return 1;
    }

    // Run tests
    printf("-------------------------------------------------\n");
    printf("Running Tests\n");
    printf("-------------------------------------------------\n\n");

    int failed = 0;

    // Test correctness
    if (test_batch_correctness() != 0) {
        printf(RED "✗ Correctness test failed\n" RESET);
        failed++;
    } else {
        printf(GREEN "✓ Correctness test passed\n" RESET);
    }

    // Test edge cases
    if (test_edge_cases() != 0) {
        printf(RED "✗ Edge cases test failed\n" RESET);
        failed++;
    } else {
        printf(GREEN "✓ Edge cases test passed\n" RESET);
    }

    // Benchmark performance
    benchmark_batch_performance();

    // Cleanup
    saber_batch_cleanup();

    // Summary
    printf("=================================================\n");
    if (failed == 0) {
        printf(GREEN "All tests PASSED\n" RESET);
        printf("Batching provides significant speedups!\n");
    } else {
        printf(RED "%d tests FAILED\n" RESET, failed);
    }
    printf("=================================================\n\n");

    return failed;
}