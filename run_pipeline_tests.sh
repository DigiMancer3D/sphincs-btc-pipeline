#!/bin/bash
set -e

BASE_DIR=~/sphincs-btc-pipeline
PROG1="$BASE_DIR/prog1/prog1"
PROG2="$BASE_DIR/prog2/prog2"

echo "=== SPHINCS-BTC Pipeline Test Runner ==="
echo "Running multiple seeds with Role 0 + master key saving"
echo "Output will be in $BASE_DIR/results/"
echo

mkdir -p "$BASE_DIR/results"

# Test seeds (you can add more)
SEEDS=(
    "0000000000000000000000000000000000000000000000000000000000000000"
    "1111111111111111111111111111111111111111111111111111111111111111"
    "deadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef"
    "a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2"
)

for i in "${!SEEDS[@]}"; do
    SEED="${SEEDS[$i]}"
    NAME="seed_$((i+1))_role0"
    OUTDIR="$BASE_DIR/results/$NAME"

    echo "[$((i+1))/${#SEEDS[@]}] Processing $NAME ..."

    # Run prog1 with master key saving
    cd "$BASE_DIR/prog1"
    ./prog1 --seed "$SEED" --out "raw105_${NAME}.bin" --role 0 --save-master-key

    # Run prog2
    mkdir -p "$OUTDIR"
    cd "$BASE_DIR/prog2"
    ./prog2 --input "../prog1/raw105_${NAME}.bin" --outdir "$OUTDIR" --save-sphincs-key

    echo "   ✓ Done → $OUTDIR/btc_address_role_based.txt"
    echo
done

echo "All tests complete!"
echo "Master secret keys saved in: $BASE_DIR/prog1/master_sk_role0.bin"
echo "Full results in: $BASE_DIR/results/"
echo
echo "Quick summary of BTC addresses:"
for d in "$BASE_DIR/results"/*; do
    if [ -f "$d/btc_address_role_based.txt" ]; then
        echo "=== $(basename "$d") ==="
        head -n 30 "$d/btc_address_role_based.txt" | tail -n 20 | grep -E "(Address|Payload)"
        echo
    fi
done
