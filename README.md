
# SPHINCS-BTC Pipeline

A simple two-program demonstration that turns a 32-byte seed into Bitcoin-style addresses using a heavily customized **SPHINCS+** (post-quantum signature scheme).

This is **example / discussion code**, not production software. It shows one practical way to manipulate SPHINCS+ with minimal changes so it can produce small, usable outputs that resemble Bitcoin keys.

---

## What it does

1. **prog1**, Takes a 32-byte seed and produces a fixed 105-byte slice from a SPHINCS+ signature.
2. **prog2**, Reads that 105-byte slice and turns it into Bitcoin addresses (both standard real-BTC and “shake + pq-checksum” variants).

It also supports **roles** (0–9), with role 0 considered master role and role 9 a view role (essentially).

---

## Setup

```bash
mkdir -p sphincs-btc-pipeline
cd sphincs-btc-pipeline

# Get the official SPHINCS+ reference code
git clone https://github.com/sphincs/sphincsplus.git ref-source

# Create our two working folders
mkdir -p prog1 prog2

# Copy the reference files into both folders
cp -r ref-source/ref/* prog1/
cp -r ref-source/ref/* prog2/
```

Download the following files into the folders listed:

/prog1/ - `custom_params.h` and `prog1/main.c` (as `main.c`)

/prog2/ - `custom_params2.h` and `prog2/main.c` (as `main.c`)


Then build:

```bash
cd ~/sphincs-btc-pipeline/prog1
make clean && make

cd ~/sphincs-btc-pipeline/prog2
make clean && make
```

---

## Brief Overview

### Pass 1, prog1
- Uses very small custom parameters (`custom_params.h`) so the SPHINCS+ signature is tiny.
- Takes your 32-byte seed and signs it.
- Saves only the first **105 bytes** of the signature (`SPX_SLICE_BYTES = 105`).
- This 105-byte slice becomes the input for Pass 2.

### Pass 2, prog2
- Reads the 105-byte slice.
- Reduces it to a 20-byte payload using SHAKE256 (SPHINCS+ native hash).
- Turns that payload into two types of Bitcoin addresses:
  - Real BTC (double SHA256 + standard Base58Check)
  - Shake + pq-checksum (double SHAKE256 + same Base58 format)
- Also generates proof files and optionally saves full SPHINCS+ signatures.

### Why the custom parameters?
Standard SPHINCS+ signatures are much larger. We use tiny parameters (`SPX_N=20`, `SPX_H=1`, `SPX_A=1`, `SPX_K=1`, `SPX_D=1`, `SPX_W=256`) so we can get a manageable 105-byte output that still demonstrates the idea.

**Future upgrade path (more secure):**
After a Bitcoin PQC upgrade, only changes needed would be to the parameters in `custom_params.h` and `custom_params2.h` to the official larger values (e.g. `SPX_H=64`, `SPX_A=14`, etc.). The pipeline would still work, only the signature size and security level would increase.

### Why roles?
Roles create a simple hierarchical system (Role 0 = master, higher roles = derived children). Each role uses a different derivation path, so you can see how addresses change while still being tied back to the same master seed. This is a PQC-native way of doing key derivation without elliptic curves.

---

## Quick Usage

```bash
# Master (Role 0)
cd ~/sphincs-btc-pipeline/prog1
./prog1 --seed YOUR_32_BYTE_SEED_HERE --out raw105_master.bin --role 0 --save-master-key

cd ~/sphincs-btc-pipeline/prog2
./prog2 --input ../prog1/raw105_master.bin --outdir ./output_master --save-sphincs-key
```

All outputs go into nicely organized folders under `results/`.

---

## Useful Scripts

- `run_multi_role_tests.sh`, Runs multiple seeds and all roles automatically (recommended for testing)

---

## Tips

- Keep your 32-byte seed safe, it is the true master key.
- The full SPHINCS+ secret keys (`master_sk.bin`) are saved inside each result folder when you use `--save-master-key`.
- Temporary files are automatically cleaned up.
- This is a **proof-of-concept** to spark discussion about hybrid PQC + Bitcoin systems.
- Do not trust this system with real keys and BTC, this only shows SPHINCS+ can be adjusted with param files.

Feel free to open issues or pull requests, this repo is meant for experimentation and conversation.

---

   
   
