#!/bin/bash
make clean && make

echo "--- Phase 1 ---"
./mmu 256 BACKING_STORE.bin addresses.txt
diff -q output256.csv correct256.csv > /dev/null && echo "PASS (8pts)" || echo "FAIL"

echo "--- Phase 2 ---"
./mmu 128 BACKING_STORE.bin addresses.txt
diff -q output128.csv correct128.csv > /dev/null && echo "PASS (12pts)" || echo "FAIL"
