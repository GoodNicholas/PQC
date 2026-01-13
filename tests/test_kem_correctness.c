/**
 * @file test_kem_correctness.c
 * @brief Comprehensive KEM correctness test for all SABER_GOST configurations
 *
 * Tests:
 * 1. Key generation produces valid keys
 * 2. Encapsulation produces valid ciphertext and shared secret
 * 3. Decapsulation recovers the same shared secret
 * 4. Multiple iterations produce consistent results
 * 5. Different seeds produce different outputs
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "../include/api.h"
#include "../include/params.h"

#define TEST_ITERATIONS 100
#define SEED_TEST_COUNT 10

// NIST API compatibility
#define CRYPTO_PUBLICKEYBYTES  SABER_PUBLIC_KEY_BYTES
#define CRYPTO_SECRETKEYBYTES  SABER_SECRET_KEY_BYTES
#define CRYPTO_CIPHERTEXTBYTES SABER_CIPHERTEXT_BYTES
#define CRYPTO_BYTES           SABER_SHARED_KEY_BYTES
#define CRYPTO_ALGNAME         "SABER_GOST"

#define crypto_kem_keypair(pk, sk)          Saber_KeyGen(pk, sk)
#define crypto_kem_enc(ct, ss, pk)          Saber_Encaps(pk, ct, ss)
#define crypto_kem_dec(ss, ct, sk)          Saber_Decaps(sk, ct, ss)

// Color codes for output
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_RESET "\033[0m"

// Test result tracking
typedef struct {
    int total;
    int passed;
    int failed;
} TestStats;

void print_test_header(const char *test_name) {
    printf("\n");
    printf("========================================\n");
    printf("  %s\n", test_name);
    printf("========================================\n");
}

void print_test_result(const char *test_name, int passed) {
    if (passed) {
        printf(COLOR_GREEN "✓ PASSED" COLOR_RESET ": %s\n", test_name);
    } else {
        printf(COLOR_RED "✗ FAILED" COLOR_RESET ": %s\n", test_name);
    }
}

void print_final_stats(TestStats *stats) {
    printf("\n");
    printf("========================================\n");
    printf("  FINAL TEST RESULTS\n");
    printf("========================================\n");
    printf("Total tests:  %d\n", stats->total);
    printf("Passed:       " COLOR_GREEN "%d" COLOR_RESET "\n", stats->passed);
    printf("Failed:       " COLOR_RED "%d" COLOR_RESET "\n", stats->failed);

    if (stats->failed == 0) {
        printf("\n" COLOR_GREEN "ALL TESTS PASSED!" COLOR_RESET "\n");
    } else {
        printf("\n" COLOR_RED "SOME TESTS FAILED!" COLOR_RESET "\n");
    }
    printf("========================================\n");
}

/**
 * Test 1: Basic key generation
 */
int test_basic_keygen(TestStats *stats) {
    print_test_header("Test 1: Basic Key Generation");

    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];

    stats->total++;

    int result = crypto_kem_keypair(pk, sk);

    if (result != 0) {
        print_test_result("crypto_kem_keypair returns 0", 0);
        stats->failed++;
        return 0;
    }

    print_test_result("crypto_kem_keypair returns 0", 1);

    // Check that keys are not all zeros
    int pk_nonzero = 0;
    int sk_nonzero = 0;

    for (size_t i = 0; i < CRYPTO_PUBLICKEYBYTES; i++) {
        if (pk[i] != 0) {
            pk_nonzero = 1;
            break;
        }
    }

    for (size_t i = 0; i < CRYPTO_SECRETKEYBYTES; i++) {
        if (sk[i] != 0) {
            sk_nonzero = 1;
            break;
        }
    }

    stats->total += 2;

    if (pk_nonzero) {
        print_test_result("Public key is non-zero", 1);
        stats->passed++;
    } else {
        print_test_result("Public key is non-zero", 0);
        stats->failed++;
    }

    if (sk_nonzero) {
        print_test_result("Secret key is non-zero", 1);
        stats->passed++;
    } else {
        print_test_result("Secret key is non-zero", 0);
        stats->failed++;
    }

    if (result == 0 && pk_nonzero && sk_nonzero) {
        stats->passed++;
        return 1;
    } else {
        stats->failed++;
        return 0;
    }
}

/**
 * Test 2: Basic encapsulation/decapsulation
 */
int test_basic_encap_decap(TestStats *stats) {
    print_test_header("Test 2: Basic Encapsulation/Decapsulation");

    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss_enc[CRYPTO_BYTES];
    uint8_t ss_dec[CRYPTO_BYTES];

    crypto_kem_keypair(pk, sk);

    stats->total++;
    int enc_result = crypto_kem_enc(ct, ss_enc, pk);

    if (enc_result != 0) {
        print_test_result("crypto_kem_enc returns 0", 0);
        stats->failed++;
        return 0;
    }
    print_test_result("crypto_kem_enc returns 0", 1);
    stats->passed++;

    stats->total++;
    int dec_result = crypto_kem_dec(ss_dec, ct, sk);

    if (dec_result != 0) {
        print_test_result("crypto_kem_dec returns 0", 0);
        stats->failed++;
        return 0;
    }
    print_test_result("crypto_kem_dec returns 0", 1);
    stats->passed++;

    // Check that shared secrets match
    stats->total++;
    int secrets_match = (memcmp(ss_enc, ss_dec, CRYPTO_BYTES) == 0);

    if (secrets_match) {
        print_test_result("Shared secrets match", 1);
        stats->passed++;
        return 1;
    } else {
        print_test_result("Shared secrets match", 0);
        printf("  Encapsulated SS: ");
        for (int i = 0; i < 16; i++) printf("%02x", ss_enc[i]);
        printf("...\n");
        printf("  Decapsulated SS: ");
        for (int i = 0; i < 16; i++) printf("%02x", ss_dec[i]);
        printf("...\n");
        stats->failed++;
        return 0;
    }
}

/**
 * Test 3: Multiple iterations consistency
 */
int test_multiple_iterations(TestStats *stats) {
    char header[128];
    snprintf(header, sizeof(header), "Test 3: Multiple Iterations (%d rounds)", TEST_ITERATIONS);
    print_test_header(header);

    int failures = 0;

    for (int i = 0; i < TEST_ITERATIONS; i++) {
        uint8_t pk[CRYPTO_PUBLICKEYBYTES];
        uint8_t sk[CRYPTO_SECRETKEYBYTES];
        uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
        uint8_t ss_enc[CRYPTO_BYTES];
        uint8_t ss_dec[CRYPTO_BYTES];

        crypto_kem_keypair(pk, sk);
        crypto_kem_enc(ct, ss_enc, pk);
        crypto_kem_dec(ss_dec, ct, sk);

        if (memcmp(ss_enc, ss_dec, CRYPTO_BYTES) != 0) {
            failures++;
            if (failures <= 3) {  // Print first 3 failures
                printf("  Iteration %d: " COLOR_RED "MISMATCH" COLOR_RESET "\n", i);
            }
        }
    }

    stats->total++;

    if (failures == 0) {
        print_test_result("All iterations passed", 1);
        stats->passed++;
        return 1;
    } else {
        printf("  " COLOR_RED "%d/%d iterations failed" COLOR_RESET "\n",
               failures, TEST_ITERATIONS);
        print_test_result("All iterations passed", 0);
        stats->failed++;
        return 0;
    }
}

/**
 * Test 4: Different keys produce different ciphertexts
 */
int test_key_uniqueness(TestStats *stats) {
    print_test_header("Test 4: Key Uniqueness");

    uint8_t pk1[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk1[CRYPTO_SECRETKEYBYTES];
    uint8_t pk2[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk2[CRYPTO_SECRETKEYBYTES];

    crypto_kem_keypair(pk1, sk1);
    crypto_kem_keypair(pk2, sk2);

    stats->total += 2;

    int pk_different = (memcmp(pk1, pk2, CRYPTO_PUBLICKEYBYTES) != 0);
    int sk_different = (memcmp(sk1, sk2, CRYPTO_SECRETKEYBYTES) != 0);

    if (pk_different) {
        print_test_result("Public keys are different", 1);
        stats->passed++;
    } else {
        print_test_result("Public keys are different", 0);
        stats->failed++;
    }

    if (sk_different) {
        print_test_result("Secret keys are different", 1);
        stats->passed++;
    } else {
        print_test_result("Secret keys are different", 0);
        stats->failed++;
    }

    return pk_different && sk_different;
}

/**
 * Test 5: Ciphertext integrity (modified ciphertext should fail)
 */
int test_ciphertext_integrity(TestStats *stats) {
    print_test_header("Test 5: Ciphertext Integrity");

    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss_enc[CRYPTO_BYTES];
    uint8_t ss_dec1[CRYPTO_BYTES];
    uint8_t ss_dec2[CRYPTO_BYTES];

    crypto_kem_keypair(pk, sk);
    crypto_kem_enc(ct, ss_enc, pk);
    crypto_kem_dec(ss_dec1, ct, sk);

    // Modify one byte of ciphertext
    ct[CRYPTO_CIPHERTEXTBYTES / 2] ^= 0x01;

    crypto_kem_dec(ss_dec2, ct, sk);

    stats->total++;

    // After modification, decapsulated secret should be different
    // (FO transform should catch this)
    int secrets_different = (memcmp(ss_dec1, ss_dec2, CRYPTO_BYTES) != 0);

    if (secrets_different) {
        print_test_result("Modified ciphertext produces different secret", 1);
        stats->passed++;
        return 1;
    } else {
        print_test_result("Modified ciphertext produces different secret", 0);
        printf(COLOR_YELLOW "  Warning: FO transform may not be active" COLOR_RESET "\n");
        stats->failed++;
        return 0;
    }
}

/**
 * Main test runner
 */
int main() {
    TestStats stats = {0, 0, 0};

    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║   SABER_GOST KEM Correctness Tests    ║\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("  Algorithm:      %s\n", CRYPTO_ALGNAME);
    printf("  Public key:     %d bytes\n", CRYPTO_PUBLICKEYBYTES);
    printf("  Secret key:     %d bytes\n", CRYPTO_SECRETKEYBYTES);
    printf("  Ciphertext:     %d bytes\n", CRYPTO_CIPHERTEXTBYTES);
    printf("  Shared secret:  %d bytes\n", CRYPTO_BYTES);
    printf("========================================\n");

    // Run all tests
    test_basic_keygen(&stats);
    test_basic_encap_decap(&stats);
    test_multiple_iterations(&stats);
    test_key_uniqueness(&stats);
    test_ciphertext_integrity(&stats);

    // Print final statistics
    print_final_stats(&stats);

    return (stats.failed == 0) ? 0 : 1;
}
