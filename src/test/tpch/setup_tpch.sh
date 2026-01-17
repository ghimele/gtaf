#!/bin/bash
# Setup script for TPC-H benchmark with GTAF

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GTAF_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "=== TPC-H Setup for GTAF ==="
echo ""

# Parse arguments
SCALE_FACTOR=${1:-1}
TPCH_DIR="${2:-$GTAF_ROOT/tpch-data}"

echo "Configuration:"
echo "  Scale Factor: SF$SCALE_FACTOR"
echo "  TPC-H directory: $TPCH_DIR"
echo ""

# Step 1: Clone tpch-kit if not exists
if [ ! -d "$TPCH_DIR/tpch-kit" ]; then
    echo "Step 1: Cloning TPC-H data generator..."
    mkdir -p "$TPCH_DIR"
    cd "$TPCH_DIR"
    git clone https://github.com/gregrahn/tpch-kit.git
    echo "  ✓ Cloned"
else
    echo "Step 1: TPC-H kit already exists, skipping..."
fi

# Step 2: Build dbgen
echo ""
echo "Step 2: Building data generator..."
cd "$TPCH_DIR/tpch-kit/dbgen"

if [ ! -f "dbgen" ]; then
    make
    echo "  ✓ Built dbgen"
else
    echo "  ✓ dbgen already built"
fi

# Step 3: Generate data
echo ""
echo "Step 3: Generating TPC-H data (Scale Factor $SCALE_FACTOR)..."
echo "  This may take a while depending on scale factor..."
echo "  SF1 (~1 GB): ~1 minute"
echo "  SF10 (~10 GB): ~10 minutes"
echo "  SF100 (~100 GB): ~100 minutes"
echo ""

./dbgen -s $SCALE_FACTOR -f

echo "  ✓ Generated data files"
echo ""
ls -lh *.tbl
echo ""

# Step 4: Copy tbl files
echo "Step 4: Copying .tbl files to TPC-H directory..."
cp "$TPCH_DIR/tpch-kit/dbgen/"*.tbl "$GTAF_ROOT/test/tpch/data/"
echo "  ✓ Copied .tbl files"

# Step 5: Import to GTAF
echo "Step 5: Importing to GTAF..."
OUTPUT_FILE="$GTAF_ROOT/test/tpch/data/tpch_sf${SCALE_FACTOR}.dat"

cd "$GTAF_ROOT"
./build/gtaf_tpch_import "$GTAF_ROOT/test/tpch/data/" "$OUTPUT_FILE"

echo ""
echo "=== Setup Complete ==="
echo ""
echo "Data location:"
echo "  Raw .tbl files: $GTAF_ROOT/test/tpch/data/*.tbl"
echo "  GTAF file: $OUTPUT_FILE"
echo ""
echo "To query the data:"
echo "  ./build/gtaf_tpch_query $OUTPUT_FILE"
echo ""
echo "File size:"
ls -lh "$OUTPUT_FILE"
