#include "api.h"
#include "custom_params.h"
#include "fips202.h"
#include <stdio.h>
#include <string.h>

#ifdef __linux__
#include <string.h>  /* explicit_bzero */
#elif defined(_WIN32)
#include <windows.h>
#define explicit_bzero SecureZeroMemory
#else
static void explicit_bzero(void *s, size_t n) {
    volatile unsigned char *p = s;
    while (n--) *p++ = 0;
}
#endif

/* Role-based deterministic derivation */
static void derive_role_message(const unsigned char *master_msg, uint32_t role, unsigned char out_msg[32]) {
    if (role == 0) {
        memcpy(out_msg, master_msg, 32);
        return;
    }
    unsigned char input[32 + 64] = {0};
    memcpy(input, master_msg, 32);
    const char *domain = "SPHINCS-BTC-ROLE-v1";
    size_t dlen = strlen(domain);
    memcpy(input + 32, domain, dlen);
    memcpy(input + 32 + dlen, &role, 4);
    shake256(out_msg, 32, input, 32 + dlen + 4);
}

int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s --seed <32-byte-hex-or-string> --out <raw105.bin> [--role <N>] [--save-master-key]\n", argv[0]);
        fprintf(stderr, "  --save-master-key  : Save full SPHINCS+ secret key to master_sk_roleN.bin\n");
        return 1;
    }

    unsigned char sk[CRYPTO_SECRETKEYBYTES];
    unsigned char pk[CRYPTO_PUBLICKEYBYTES];
    unsigned char sig[CRYPTO_BYTES];
    unsigned char msg[32] = {0};

    const char *seed_str = NULL;
    const char *out_file = NULL;
    uint32_t role = 0;
    int save_master_key = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--seed") == 0 && i+1 < argc) seed_str = argv[++i];
        else if (strcmp(argv[i], "--out") == 0 && i+1 < argc) out_file = argv[++i];
        else if (strcmp(argv[i], "--role") == 0 && i+1 < argc) role = (uint32_t)atoi(argv[++i]);
        else if (strcmp(argv[i], "--save-master-key") == 0) save_master_key = 1;
    }
    if (!seed_str || !out_file) {
        fprintf(stderr, "Error: missing --seed or --out\n");
        return 1;
    }

    /* Parse seed */
    if (strlen(seed_str) == 64) {
        for (int i = 0; i < 32; i++) sscanf(seed_str + 2*i, "%2hhx", &msg[i]);
    } else {
        strncpy((char*)msg, seed_str, 32);
    }

    unsigned char effective_msg[32];
    derive_role_message(msg, role, effective_msg);

    crypto_sign_keypair(pk, sk);

    /* === NEW: Save full master secret key if requested === */
    if (save_master_key) {
        char sk_filename[256];
        snprintf(sk_filename, sizeof(sk_filename), "master_sk_role%u.bin", role);
        FILE *fsk = fopen(sk_filename, "wb");
        if (fsk) {
            fwrite(sk, 1, CRYPTO_SECRETKEYBYTES, fsk);
            fclose(fsk);
            printf("✓ Full SPHINCS+ master secret key saved to: %s\n", sk_filename);
        }
    }

    unsigned long long sig_len;
    crypto_sign(sig, &sig_len, effective_msg, 32, sk);

    FILE *f = fopen(out_file, "wb");
    if (!f) {
        perror("fopen");
        explicit_bzero(sk, sizeof(sk));
        explicit_bzero(sig, sizeof(sig));
        return 1;
    }
    fwrite(sig, 1, SPX_SLICE_BYTES, f);
    fclose(f);

    explicit_bzero(sk, sizeof(sk));
    explicit_bzero(sig, sizeof(sig));
    explicit_bzero(msg, sizeof(msg));
    explicit_bzero(effective_msg, sizeof(effective_msg));

    printf("✓ Pass 1 complete (Role %u): %d-byte raw buffer written to %s\n", role, SPX_SLICE_BYTES, out_file);
    return 0;
}
