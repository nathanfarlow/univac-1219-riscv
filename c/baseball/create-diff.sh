#!/bin/bash
# Create comprehensive diff of baseball game port

OUTPUT="diff.txt"

echo "BASEBALL GAME PORT - COMPLETE DIFF" > "$OUTPUT"
echo "===================================" >> "$OUTPUT"
echo "" >> "$OUTPUT"
echo "Original: /home/nathan/Downloads/baseballgameinc/" >> "$OUTPUT"
echo "Modified: c/baseball/" >> "$OUTPUT"
echo "Date: $(date)" >> "$OUTPUT"
echo "" >> "$OUTPUT"
echo "" >> "$OUTPUT"

# Diff each file
for file in baseball.h baseball.c players.c; do
    echo "========================================" >> "$OUTPUT"
    echo "FILE: $file" >> "$OUTPUT"
    echo "========================================" >> "$OUTPUT"
    diff -u /home/nathan/Downloads/baseballgameinc/"$file" "$file" >> "$OUTPUT" 2>&1
    echo "" >> "$OUTPUT"
    echo "" >> "$OUTPUT"
done

# Show new Makefile
echo "========================================" >> "$OUTPUT"
echo "NEW FILE: Makefile" >> "$OUTPUT"
echo "========================================" >> "$OUTPUT"
cat Makefile >> "$OUTPUT"
echo "" >> "$OUTPUT"
echo "" >> "$OUTPUT"

# Show toolchain change
echo "========================================" >> "$OUTPUT"
echo "CRITICAL CHANGE: toolchain/start.c" >> "$OUTPUT"
echo "========================================" >> "$OUTPUT"
echo "Stack pointer increased from 0x7000 (28KB) to 0x12000 (72KB)" >> "$OUTPUT"
echo "" >> "$OUTPUT"
diff -u <(echo 'lui sp, 7') <(echo 'lui sp, 0x12') >> "$OUTPUT" 2>&1

echo "Diff file created: $OUTPUT"
