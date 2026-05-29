#!/bin/bash
set -e

echo "=== SPHINCS-BTC Pipeline Setup ==="

# 1. Download official SPHINCS+ reference if not present
if [ ! -d "ref-source" ]; then
    echo "→ Cloning official SPHINCS+ reference..."
    git clone https://github.com/sphincs/sphincsplus.git ref-source
else
    echo "→ ref-source already exists."
fi

# 2. Setup prog1
echo "→ Setting up prog1..."
rm -rf prog1/*
cp -r ref-source/ref/* prog1/

# Overwrite with our custom files
cp -f custom_params.h prog1/ 2>/dev/null || true
cp -f prog1/main.c prog1/ 2>/dev/null || true
cp -f prog1/Makefile prog1/ 2>/dev/null || true
cp -f prog1/params.h prog1/ 2>/dev/null || true

# 3. Setup prog2
echo "→ Setting up prog2..."
rm -rf prog2/*
cp -r ref-source/ref/* prog2/

# Overwrite with our custom files
cp -f custom_params2.h prog2/ 2>/dev/null || true
cp -f prog2/main.c prog2/ 2>/dev/null || true
cp -f prog2/Makefile prog2/ 2>/dev/null || true
cp -f prog2/params.h prog2/ 2>/dev/null || true

echo "✅ Setup complete!"
echo ""
echo "Next steps:"
echo "   cd prog1 && make clean && make"
echo "   cd prog2 && make clean && make"
echo ""
echo "You can now run the pipeline using the examples in README.md"
