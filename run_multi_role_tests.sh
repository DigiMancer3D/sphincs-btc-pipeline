#!/bin/bash
set -e

BASE_DIR=~/sphincs-btc-pipeline
RESULTS="$BASE_DIR/results"

mkdir -p "$RESULTS"

# ====================== CONFIGURATION ======================
SEEDS=(
    "0000000000000000000000000000000000000000000000000000000000000000"
    "1111111111111111111111111111111111111111111111111111111111111111"
    "deadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef"
)

ROLES=(0 1 2 3 4 5 6 7 8 9)   # Change this to test only specific roles if you want

# ========================================================

echo "=== SPHINCS-BTC Multi-Role Test Runner ==="
echo "Testing ${#SEEDS[@]} seeds × ${#ROLES[@]} roles"
echo "Results → $RESULTS"
echo

for s_idx in "${!SEEDS[@]}"; do
    SEED="${SEEDS[$s_idx]}"
    SEED_NAME="seed_$((s_idx+1))"

    for role in "${ROLES[@]}"; do
        TEST_NAME="${SEED_NAME}_role${role}"
        OUTDIR="$RESULTS/${SEED_NAME}/role${role}"
        RAW_FILE="$BASE_DIR/prog1/raw105_${TEST_NAME}.bin"
        MASTER_KEY_SRC="$BASE_DIR/prog1/master_sk_role${role}.bin"
        MASTER_KEY_DEST="$OUTDIR/master_sk.bin"

        echo "[$((s_idx+1))/${#SEEDS[@]} | Role $role] Processing $TEST_NAME ..."

        # Run prog1 (saves master key)
        cd "$BASE_DIR/prog1"
        ./prog1 --seed "$SEED" --out "raw105_${TEST_NAME}.bin" --role "$role" --save-master-key

        # Move master key into result folder for better organization
        if [ -f "$MASTER_KEY_SRC" ]; then
            mkdir -p "$OUTDIR"
            mv "$MASTER_KEY_SRC" "$MASTER_KEY_DEST"
            echo "   Master key moved to: $MASTER_KEY_DEST"
        fi

        # Run prog2
        mkdir -p "$OUTDIR"
        cd "$BASE_DIR/prog2"
        ./prog2 --input "../prog1/raw105_${TEST_NAME}.bin" --outdir "$OUTDIR" --save-sphincs-key

        # Cleanup temporary raw file
        rm -f "$RAW_FILE"

        echo "   ✓ Completed → $OUTDIR/"
        echo
    done
done

echo "=== ALL TESTS COMPLETE ==="
echo "Master keys are now stored inside each result folder:"
echo "   $RESULTS/seed_X/roleY/master_sk.bin"
echo "All addresses, proofs, and signatures are in: $RESULTS/"
echo "Script finished successfully!"
