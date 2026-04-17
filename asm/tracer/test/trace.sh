#!/bin/bash
# Trace a .76 program. Usage: ./trace.sh <program.76> <start_addr_octal>
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TRACER_DIR="$SCRIPT_DIR/.."
TRACER="$TRACER_DIR/build/tracer.76"
BOOTSTRAP="$SCRIPT_DIR/build/bootstrap.76"
EMU_DIR="$SCRIPT_DIR/../../../univac-1219-emu"
EMU="$EMU_DIR/target/release/emulator"
ASM="$EMU_DIR/target/release/assembler"

# Build tracer if needed
make -C "$TRACER_DIR" -q 2>/dev/null || make -C "$TRACER_DIR" >/dev/null

# Assemble bootstrap if needed
mkdir -p "$SCRIPT_DIR/build"
if [ ! -f "$BOOTSTRAP" ] || [ "$SCRIPT_DIR/bootstrap.univac" -nt "$BOOTSTRAP" ]; then
    "$ASM" "$SCRIPT_DIR/bootstrap.univac" -o "$BOOTSTRAP"
fi

[ -f "$1" ] || { echo "Usage: $0 <program.76> <start_addr_octal>"; exit 1; }

TEMP=$(mktemp --suffix=.76); trap "rm -f $TEMP" EXIT
python3 << EOF > "$TEMP"
import sys

def w2v(w):
    """Convert 18-bit word to 3 6-bit values"""
    return [(w>>12)&0o77, (w>>6)&0o77, w&0o77]

def patch(d, p, w):
    """Patch 3 values at position p with word w"""
    v = w2v(w)
    d[p], d[p+1], d[p+2] = str(v[0]), str(v[1]), str(v[2])

def find_pattern(d, pattern):
    """Find first occurrence of pattern (list of ints) in d"""
    pattern = [str(x) for x in pattern]
    for i in range(len(d) - len(pattern) + 1):
        if d[i:i+len(pattern)] == pattern:
            return i
    return -1

def find_sadd(d):
    """Find SADD position: the 3 frames after the last 77 header (0,0,63)"""
    header = ['0', '0', '63']
    for i in range(len(d) - 6, -1, -1):
        if d[i:i+3] == header:
            return i + 3
    return -1

def find_bootstrap_target(d):
    """Find TARGET_ADDR in bootstrap by looking for JP &O1000 followed by DATA"""
    # JP &O1000 = 0o341000 = [28, 8, 0]
    jp_pattern = [28, 8, 0]
    i = find_pattern(d, jp_pattern)
    if i >= 0:
        # TARGET_ADDR DATA is right after JP
        return i + 3
    return -1

# Read files
tracer = [l.strip() for l in open("$TRACER") if l.strip()]
bootstrap = [l.strip() for l in open("$BOOTSTRAP") if l.strip()]
prog = [l.strip() for l in open("$1") if l.strip()]

# Find T (TRACED_PC) - first occurrence of 5,0,0 (0o50000 default)
T = find_pattern(tracer, [5, 0, 0])
if T < 0:
    print("ERROR: Could not find TRACED_PC (5,0,0) in tracer", file=sys.stderr)
    sys.exit(1)

# Find T2 (bootstrap TARGET_ADDR) - after JP &O1000
T2 = find_bootstrap_target(bootstrap)
if T2 < 0:
    print("ERROR: Could not find bootstrap TARGET_ADDR in bootstrap", file=sys.stderr)
    sys.exit(1)

# Patch addresses
start_addr = $((8#$2))
patch(tracer, T, start_addr)      # TRACED_PC initial value
patch(bootstrap, T2, start_addr)  # bootstrap TARGET_ADDR
patch(tracer, find_sadd(tracer), 0o540)          # tracer SADD -> continue loading
patch(bootstrap, find_sadd(bootstrap), 0o540)   # bootstrap SADD -> continue loading
patch(prog, find_sadd(prog), 0o750)              # program SADD -> bootstrap

# Output combined tape: tracer + bootstrap + program
for v in tracer:
    print(f" {v} ")
for v in bootstrap:
    print(f" {v} ")
for v in prog:
    print(f" {v} ")
EOF
cd "$EMU_DIR" && "$EMU" --tape-file "$TEMP"
