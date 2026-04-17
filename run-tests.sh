#!/bin/bash
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

FAILED=0

echo -e "${YELLOW}=== Building Emulator ===${NC}"
cd univac-1219-emu
cargo build --release
cd ..

filter_ocaml() {
    sed 's/\x1b\[[0-9;]*m//g' | grep -av 'Instructions executed:'
}

filter_univac() {
    local prog=$1
    sed 's/\x1b\[[0-9;]*m//g' |
    grep -av 'Loaded.*memory' |
    grep -av 'Unknown instruction!' |
    grep -av 'Stop called with bad index' |
    grep -av 'Infinite loop detected' |
    grep -av '^STOPPED:' |
    grep -av 'Total time:' |
    grep -av '^thread .* panicked at' |
    grep -av '^called `Result::unwrap' |
    grep -av '^note: run with'
}

normalize() {
    sed -i '/./,$!d; :a; /^\n*$/{$d; N; ba}' "$1"
}

test_program() {
    local prog=$1
    local input=""
    [[ $prog == test ]] && input="3"

    echo -e "\n${YELLOW}=== Testing $prog ===${NC}"
    cd "c/$prog"

    make clean &>/dev/null

    local ocaml=$(mktemp)
    local univac=$(mktemp)

    echo "$input" | make ocaml-run 2>&1 | filter_ocaml > "$ocaml"
    echo "$input" | make run 2>&1 | filter_univac "$prog" > "$univac"

    normalize "$ocaml"
    normalize "$univac"

    if diff -q "$ocaml" "$univac" &>/dev/null; then
        echo -e "${GREEN}✓ outputs match${NC}"
    else
        echo -e "${RED}✗ outputs differ${NC}"
        diff -u "$ocaml" "$univac" || true
        FAILED=1
    fi

    rm -f "$ocaml" "$univac"
    cd ../..
}

echo -e "${YELLOW}=== Fuzz Tests ===${NC}"
cd tools
dune clean && dune build
if dune test --force; then
    echo -e "${GREEN}✓ passed${NC}"
else
    echo -e "${RED}✗ failed (possibly timing)${NC}"
    FAILED=1
fi
cd ..

for prog in hello fizzbuzz smolnes; do
    test_program "$prog"
done

echo -e "\n${YELLOW}=== Diagnostic ===${NC}"
make -C asm/diagnostic clean &>/dev/null
make -C asm/diagnostic &>/dev/null
diagnostic=$(mktemp)
make -C asm/diagnostic run --no-print-directory 2>&1 | grep -av '^cd ' | filter_univac diagnostic > "$diagnostic"
normalize "$diagnostic"
snapshot=$(mktemp)
cp asm/diagnostic/snapshot.txt "$snapshot"
normalize "$snapshot"
if diff -q "$snapshot" "$diagnostic" &>/dev/null; then
    echo -e "${GREEN}✓ matches snapshot${NC}"
else
    echo -e "${RED}✗ output differs from snapshot${NC}"
    diff -u "$snapshot" "$diagnostic" || true
    FAILED=1
fi
rm -f "$diagnostic" "$snapshot"

echo -e "\n${YELLOW}=== Tracer Tests ===${NC}"
if ./asm/tracer/test/test.sh; then
    echo -e "${GREEN}✓ passed${NC}"
else
    echo -e "${RED}✗ failed${NC}"
    FAILED=1
fi

echo ""
[[ $FAILED -eq 0 ]] && echo -e "${GREEN}=== All passed ===${NC}" || echo -e "${RED}=== Some failed ===${NC}"
exit $FAILED
