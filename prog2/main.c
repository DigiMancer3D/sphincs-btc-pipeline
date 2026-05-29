#include "api.h"
#include "custom_params2.h"
#include "fips202.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __linux__
#include <string.h>
#elif defined(_WIN32)
#include <windows.h>
#define explicit_bzero SecureZeroMemory
#else
static void explicit_bzero(void *s, size_t n) {
    volatile unsigned char *p = s; while (n--) *p++ = 0;
}
#endif

/* Base58 alphabet */
static const char *b58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

/* === REAL DOUBLE SHA256 (Bitcoin standard) === */
typedef struct {
    uint8_t data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[8];
} SHA256_CTX;

#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

static const uint32_t k[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static void sha256_transform(SHA256_CTX *ctx, const uint8_t data[]) {
    uint32_t a,b,c,d,e,f,g,h,i,j,t1,t2,m[64];
    for (i=0,j=0; i < 16; ++i, j += 4)
        m[i] = (data[j] << 24) | (data[j+1] << 16) | (data[j+2] << 8) | (data[j+3]);
    for ( ; i < 64; ++i)
        m[i] = SIG1(m[i-2]) + m[i-7] + SIG0(m[i-15]) + m[i-16];
    a = ctx->state[0]; b = ctx->state[1]; c = ctx->state[2]; d = ctx->state[3];
    e = ctx->state[4]; f = ctx->state[5]; g = ctx->state[6]; h = ctx->state[7];
    for (i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e,f,g) + k[i] + m[i];
        t2 = EP0(a) + MAJ(a,b,c);
        h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }
    ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
    ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
}

static void sha256_init(SHA256_CTX *ctx) {
    ctx->datalen = 0; ctx->bitlen = 0;
    ctx->state[0] = 0x6a09e667; ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372; ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f; ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab; ctx->state[7] = 0x5be0cd19;
}

static void sha256_update(SHA256_CTX *ctx, const uint8_t data[], size_t len) {
    for (size_t i = 0; i < len; ++i) {
        ctx->data[ctx->datalen] = data[i];
        ctx->datalen++;
        if (ctx->datalen == 64) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

static void sha256_final(SHA256_CTX *ctx, uint8_t hash[]) {
    uint32_t i = ctx->datalen;
    if (ctx->datalen < 56) {
        ctx->data[i++] = 0x80;
        while (i < 56) ctx->data[i++] = 0x00;
    } else {
        ctx->data[i++] = 0x80;
        while (i < 64) ctx->data[i++] = 0x00;
        sha256_transform(ctx, ctx->data);
        memset(ctx->data, 0, 56);
    }
    ctx->bitlen += ctx->datalen * 8;
    for (i = 0; i < 8; ++i) {
        ctx->data[63 - i*4]     = (ctx->bitlen >> (i*8)) & 0xff;
        ctx->data[62 - i*4 + 1] = (ctx->bitlen >> (i*8 + 8)) & 0xff;
    }
    sha256_transform(ctx, ctx->data);
    for (i = 0; i < 8; ++i) {
        hash[i*4]     = (ctx->state[i] >> 24) & 0xff;
        hash[i*4 + 1] = (ctx->state[i] >> 16) & 0xff;
        hash[i*4 + 2] = (ctx->state[i] >> 8) & 0xff;
        hash[i*4 + 3] =  ctx->state[i] & 0xff;
    }
}

static void real_sha256(unsigned char *out, const unsigned char *in, size_t inlen) {
    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, in, inlen);
    sha256_final(&ctx, out);
}

static void double_sha256(unsigned char *out, const unsigned char *in, size_t inlen) {
    unsigned char tmp[32];
    real_sha256(tmp, in, inlen);
    real_sha256(out, tmp, 32);
}

/* Double SHAKE256 for shake + pq-checksum variant */
static void double_shake256(unsigned char *out, const unsigned char *in, size_t inlen) {
    unsigned char tmp[32];
    shake256(tmp, 32, in, inlen);
    shake256(out, 32, tmp, 32);
}

/* Correct Bitcoin Base58Check */
static void real_btc_base58(const unsigned char *in25, char *address_out) {
    unsigned char digits[50] = {0};
    size_t digits_len = 0;
    unsigned int carry;

    for (size_t i = 0; i < 25; ++i) {
        carry = in25[i];
        for (size_t j = 0; j < digits_len; ++j) {
            carry += digits[j] * 256U;
            digits[j] = carry % 58;
            carry /= 58;
        }
        while (carry) {
            digits[digits_len++] = carry % 58;
            carry /= 58;
        }
    }

    size_t leading = 0;
    while (leading < 25 && in25[leading] == 0) {
        address_out[leading++] = '1';
    }

    for (size_t i = digits_len; i > 0; --i) {
        address_out[leading++] = b58[digits[i-1]];
    }
    address_out[leading] = '\0';
}

/* Updated: works for BOTH real BTC and shake + pq-checksum */
static void payload_to_btc_address(const unsigned char *payload20, char *address_out, int use_real) {
    unsigned char extended[25];
    unsigned char checksum[32];
    extended[0] = 0x00;
    memcpy(extended + 1, payload20, 20);

    if (use_real) {
        double_sha256(checksum, extended, 21);
    } else {
        double_shake256(checksum, extended, 21);   /* ← double-SHAKE256 (pq-checksum) */
    }
    memcpy(extended + 21, checksum, 4);

    real_btc_base58(extended, address_out);
}

/* Legacy reductions (kept 100% intact) */
static void parallel_xor_collapse(const unsigned char *in105, unsigned char out20[20]) {
    unsigned char temp[20];
    memset(out20, 0, 20);
    for (int i = 0; i < 8; i++) {
        unsigned char counter[4] = {0,0,0,(unsigned char)i};
        unsigned char input[109];
        memcpy(input, in105, 105);
        memcpy(input + 105, counter, 4);
        shake256(temp, 20, input, 109);
        for (int j = 0; j < 20; j++) out20[j] ^= temp[j];
        explicit_bzero(counter, 4);
    }
}

static void iterated_sponge_chain(const unsigned char *in105, unsigned char out20[20]) {
    unsigned char state[20];
    memcpy(state, in105, 20);
    for (int i = 0; i < 6; i++) {
        unsigned char counter[4] = {0,0,0,(unsigned char)i};
        unsigned char input[109];
        memcpy(input, in105, 105);
        memcpy(input + 105, counter, 4);
        shake256(state, 20, input, 109);
        explicit_bzero(counter, 4);
    }
    memcpy(out20, state, 20);
    explicit_bzero(state, 20);
}

/* New recommended reduction (domain-separated + sideways projection) */
static void domain_separated_shake_reduce(const unsigned char *in105, unsigned char out20[20], uint32_t role) {
    unsigned char domain[32] = {0};
    const char *sep = "SPHINCS-BTC-PAYLOAD-v1";
    size_t seplen = strlen(sep);
    memcpy(domain, sep, seplen);
    memcpy(domain + seplen, &role, 4);

    unsigned char extended[105 + 32];
    memcpy(extended, in105, 105);
    memcpy(extended + 105, domain, 32);

    unsigned char tmp[64];
    shake256(tmp, 64, extended, 105 + 32);

    for (int i = 0; i < 20; i++) {
        out20[i] = tmp[i] ^ tmp[i + 32] ^ tmp[i + 44];  /* sideways mixing */
    }
}

/* Full SPHINCS+ signature for proof */
static void generate_full_signature(const unsigned char *msg, unsigned char *sig_out, unsigned long long *sig_len) {
    unsigned char sk[CRYPTO_SECRETKEYBYTES];
    unsigned char pk[CRYPTO_PUBLICKEYBYTES];
    crypto_sign_keypair(pk, sk);
    crypto_sign(sig_out, sig_len, msg, 32, sk);
    explicit_bzero(sk, sizeof(sk));
}

int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s --input <raw105.bin> --outdir <dir> [--save-sphincs-key]\n", argv[0]);
        return 1;
    }

    const char *in_file = NULL;
    const char *out_dir = ".";
    int save_sphincs = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--input") == 0 && i+1 < argc) in_file = argv[++i];
        else if (strcmp(argv[i], "--outdir") == 0 && i+1 < argc) out_dir = argv[++i];
        else if (strcmp(argv[i], "--save-sphincs-key") == 0) save_sphincs = 1;
    }

    unsigned char raw105[105];
    FILE *f = fopen(in_file, "rb");
    if (!f || fread(raw105, 1, 105, f) != 105) {
        fprintf(stderr, "Error reading %s\n", in_file);
        return 1;
    }
    fclose(f);

    /* === MASTER / ROLELESS (legacy reductions) - kept exactly as before === */
    unsigned char payload_parallel[20], payload_sponge[20];
    parallel_xor_collapse(raw105, payload_parallel);
    iterated_sponge_chain(raw105, payload_sponge);

    char legacy_parallel_pq[40] = {0}, legacy_parallel_real[40] = {0};
    char legacy_sponge_pq[40] = {0}, legacy_sponge_real[40] = {0};

    payload_to_btc_address(payload_parallel, legacy_parallel_pq, 0);   /* shake + pq-checksum */
    payload_to_btc_address(payload_parallel, legacy_parallel_real, 1);
    payload_to_btc_address(payload_sponge, legacy_sponge_pq, 0);
    payload_to_btc_address(payload_sponge, legacy_sponge_real, 1);

    char path[256];

    /* Parallel legacy file (updated labels) */
    snprintf(path, sizeof(path), "%s/btc_address_parallel.txt", out_dir);
    f = fopen(path, "w");
    fprintf(f, "=== MASTER / ROLELESS (Legacy Parallel XOR) ===\n");
    fprintf(f, "Address (Parallel XOR - shake + pq-checksum): %s\n", legacy_parallel_pq);
    fprintf(f, "Address (Parallel XOR - real BTC): %s\n", legacy_parallel_real);
    fprintf(f, "Payload hex: ");
    for (int i = 0; i < 20; i++) fprintf(f, "%02x", payload_parallel[i]);
    fprintf(f, "\n");
    fclose(f);

    /* Sponge legacy file (updated labels) */
    snprintf(path, sizeof(path), "%s/btc_address_sponge.txt", out_dir);
    f = fopen(path, "w");
    fprintf(f, "=== MASTER / ROLELESS (Legacy Sponge Chain) ===\n");
    fprintf(f, "Address (Sponge Chain - shake + pq-checksum): %s\n", legacy_sponge_pq);
    fprintf(f, "Address (Sponge Chain - real BTC): %s\n", legacy_sponge_real);
    fprintf(f, "Payload hex: ");
    for (int i = 0; i < 20; i++) fprintf(f, "%02x", payload_sponge[i]);
    fprintf(f, "\n");
    fclose(f);

    /* === ROLE-BASED REDUCTIONS (0-9) using new domain_separated_shake_reduce === */
    snprintf(path, sizeof(path), "%s/btc_address_role_based.txt", out_dir);
    f = fopen(path, "w");
    fprintf(f, "=== ROLE-BASED DERIVATIONS FROM MASTER RAW105 (PQC Hierarchical) ===\n\n");

    for (int role = 0; role <= 9; role++) {
        unsigned char payload[20] = {0};
        char pq_addr[40] = {0}, real_addr[40] = {0};

        if (role == 9) {
            /* Role 9 = simple view check (first 20 bytes of master slice) */
            memcpy(payload, raw105, 20);
            fprintf(f, "ROLE %d (SIMPLE VIEW CHECK - no heavy reduction):\n", role);
        } else {
            domain_separated_shake_reduce(raw105, payload, (uint32_t)role);
            fprintf(f, "ROLE %d (Domain-Separated SHAKE Reduction):\n", role);
        }

        payload_to_btc_address(payload, pq_addr, 0);     /* shake + pq-checksum */
        payload_to_btc_address(payload, real_addr, 1);   /* real BTC */

        fprintf(f, "Address (shake + pq-checksum): %s\n", pq_addr);
        fprintf(f, "Address (real BTC): %s\n", real_addr);
        fprintf(f, "Payload hex: ");
        for (int i = 0; i < 20; i++) fprintf(f, "%02x", payload[i]);
        fprintf(f, "\n\n");
    }
    fclose(f);

    /* Proof files (legacy + master) - kept exactly as before */
    unsigned char full_sig[CRYPTO_BYTES];
    unsigned long long sig_len;

    snprintf(path, sizeof(path), "%s/proof_parallel.txt", out_dir);
    f = fopen(path, "w");
    fprintf(f, "=== SPHINCS+ PROOF - PARALLEL XOR (MASTER) ===\n");
    fprintf(f, "Raw 105-byte buffer hex: ");
    for (int i = 0; i < 105; i++) fprintf(f, "%02x", raw105[i]);
    fprintf(f, "\n");
    generate_full_signature(raw105, full_sig, &sig_len);
    if (save_sphincs) {
        snprintf(path, sizeof(path), "%s/sphincs_signature_parallel.bin", out_dir);
        FILE *sf = fopen(path, "wb");
        fwrite(full_sig, 1, sig_len, sf);
        fclose(sf);
        fprintf(f, "Full SPHINCS+ signature saved to: %s\n", path);
    }
    fprintf(f, "Proof handshake (raw105 XOR first 105 bytes of signature): ");
    for (int i = 0; i < 105; i++) fprintf(f, "%02x", raw105[i] ^ full_sig[i]);
    fprintf(f, "\n");
    fclose(f);

    snprintf(path, sizeof(path), "%s/proof_sponge.txt", out_dir);
    f = fopen(path, "w");
    fprintf(f, "=== SPHINCS+ PROOF - SPONGE CHAIN (MASTER) ===\n");
    fprintf(f, "Raw 105-byte buffer hex: ");
    for (int i = 0; i < 105; i++) fprintf(f, "%02x", raw105[i]);
    fprintf(f, "\n");
    generate_full_signature(raw105, full_sig, &sig_len);
    if (save_sphincs) {
        snprintf(path, sizeof(path), "%s/sphincs_signature_sponge.bin", out_dir);
        FILE *sf = fopen(path, "wb");
        fwrite(full_sig, 1, sig_len, sf);
        fclose(sf);
        fprintf(f, "Full SPHINCS+ signature saved to: %s\n", path);
    }
    fprintf(f, "Proof handshake (raw105 XOR first 105 bytes of signature): ");
    for (int i = 0; i < 105; i++) fprintf(f, "%02x", raw105[i] ^ full_sig[i]);
    fprintf(f, "\n");
    fclose(f);

    printf("✓ Program 2 complete!\n");
    printf("   Master / legacy files written\n");
    printf("   Role-based addresses (0-9) written to btc_address_role_based.txt\n");
    printf("   Proof files written to %s/\n", out_dir);

    explicit_bzero(raw105, sizeof(raw105));
    return 0;
}
