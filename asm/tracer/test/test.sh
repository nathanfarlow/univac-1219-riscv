#!/bin/bash
# Test tracer against reference programs
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TRACER_DIR="$SCRIPT_DIR/.."
EMU_DIR="$SCRIPT_DIR/../../../univac-1219-emu"
EMU="$EMU_DIR/target/release/emulator"


test_program() {
    local name=$1
    local tape=$2
    local start=$3
    local num_insns=$4
    local filter_start=${5:-$start}  # Optional: filter start address, defaults to start

    local TRACER_OUT=$(mktemp)
    local TRACER_RAW=$(mktemp)
    local VERBOSE_OUT=$(mktemp)

    # Run tracer and verbose emulator in parallel
    (
        # tee saves raw output for program output extraction; pipe through awk+head for SIGPIPE termination
        "$SCRIPT_DIR/trace.sh" "$tape" "$start" 2>&1 | tr -d '\r' | tee "$TRACER_RAW" | \
            awk -v start="$filter_start" '/^0[0-7]{5} / && $1 >= start {print $1, $3, $4, $6, $7}' | \
            head -$num_insns > "$TRACER_OUT" || true
    ) &
    local tracer_pid=$!

    (
        # SR is 5-bit binary in emulator, 6-digit octal in tracer - convert
        # ICR is 1 digit in emulator, 6-digit octal in tracer - zero-pad
        (cd "$EMU_DIR" && "$EMU" --tape-file "$tape" --verbose 2>&1) | \
            sed -nE 's/.*P: ([0-9]+) AU: ([0-9]+) AL: ([0-9]+) ICR: ([0-9]+) SR: ([0-9]+).*/\1 \2 \3 \5 \4/p' | \
            awk -v start="$filter_start" '{
                if ($1 >= start) {
                    sr_bin = $4; sr_dec = 0;
                    for (i = 1; i <= length(sr_bin); i++) {
                        sr_dec = sr_dec * 2 + substr(sr_bin, i, 1);
                    }
                    printf "%s %s %s %06o %06o\n", $1, $2, $3, sr_dec, $5;
                }
            }' | head -$num_insns > "$VERBOSE_OUT" || true
    ) &
    local verbose_pid=$!

    wait $tracer_pid
    wait $verbose_pid

    # Extract program output (non-trace lines that aren't empty)
    local PROGRAM_OUT=$(grep -v '^0[0-7]\{5\} ' "$TRACER_RAW" | grep -v '^$' | tr -d '\n' || true)

    # Compare PC, AU, AL, SR, ICR values
    local DIFF_OUT=$(mktemp)
    if diff -u "$VERBOSE_OUT" "$TRACER_OUT" > "$DIFF_OUT" 2>&1; then
        echo "  PASS: $name"
        if [ -n "$PROGRAM_OUT" ]; then
            echo "  Program output: $PROGRAM_OUT"
        fi
        rm -f "$TRACER_OUT" "$TRACER_RAW" "$VERBOSE_OUT" "$DIFF_OUT"
        return 0
    else
        echo "  FAIL: $name"
        if [ -n "$PROGRAM_OUT" ]; then
            echo "  Program output: $PROGRAM_OUT"
        fi
        echo "  First differences:"
        head -20 "$DIFF_OUT"
        rm -f "$TRACER_OUT" "$TRACER_RAW" "$VERBOSE_OUT" "$DIFF_OUT"
        return 1
    fi
}

# Build everything sequentially (dune can't run in parallel)
make -C "$TRACER_DIR" -q 2>/dev/null || make -C "$TRACER_DIR" >/dev/null
make -C "$SCRIPT_DIR/standalone" -q 2>/dev/null || make -C "$SCRIPT_DIR/standalone" >/dev/null
make -C "$SCRIPT_DIR/../../../c/hello" -q 2>/dev/null || make -C "$SCRIPT_DIR/../../../c/hello" >/dev/null
make -C "$SCRIPT_DIR/../../../c/fizzbuzz" -q 2>/dev/null || make -C "$SCRIPT_DIR/../../../c/fizzbuzz" >/dev/null


# Run all tests in parallel
RESULT_DIR=$(mktemp -d)
trap "rm -rf $RESULT_DIR" EXIT

run_test() {
    local name=$1
    shift
    if test_program "$name" "$@"; then
        echo "0" > "$RESULT_DIR/$name"
    else
        echo "1" > "$RESULT_DIR/$name"
    fi
}

run_test "standalone" "$SCRIPT_DIR/standalone/build/standalone.76" "050000" 122 &
run_test "hello" "$SCRIPT_DIR/../../../c/hello/build/hello.76" "002150" 9700 &
run_test "fizzbuzz" "$SCRIPT_DIR/../../../c/fizzbuzz/build/fizzbuzz.76" "002150" 50000 &


wait

FAILED=0
for name in standalone hello fizzbuzz; do
    if [ "$(cat "$RESULT_DIR/$name")" != "0" ]; then
        FAILED=1
    fi
done

if [ $FAILED -eq 0 ]; then
    echo "All tests passed!"
else
    echo "Some tests failed."
    exit 1
fi
