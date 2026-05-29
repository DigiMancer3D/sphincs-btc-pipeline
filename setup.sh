#!/bin/bash
set -e

echo "=== Setting up SPHINCS-BTC Pipeline ==="

# Clone official SPHINCS+ reference if not present
if [ ! -d "ref-source" ]; then
    echo "Cloning official SPHINCS+ reference..."
    git clone https://github.com/sphincs/sphincsplus.git ref-source
fi

# Prepare prog1
echo "Setting up prog1..."
rm -rf prog1/*
cp -r ref-source/ref/* prog1/
cp -f custom_params.h prog1/ 2>/dev/null || true   # in case it's in root

# Prepare prog2
echo "Setting up prog2..."
rm -rf prog2/*
cp -r ref-source/ref/* prog2/
cp -f custom_params2.h prog2/ 2>/dev/null || true

echo "Setup complete!"
echo "You can now build with:"
echo "   cd prog1 && make"
echo "   cd prog2 && make"
