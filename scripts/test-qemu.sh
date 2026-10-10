#!/usr/bin/env bash
# QEMU boot tests: Multiboot header, timer ticks, and exception reporting.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ELF="${1:-$ROOT/build/kernix.elf}"
QEMU="${QEMU:-qemu-system-i386}"
BANNER="KerniX boot OK"
# isa-debug-exit: writing code 0 yields process exit status 1.
EXPECT_EXIT=1

if [[ ! -f "$ELF" ]]; then
    echo "missing kernel ELF: $ELF (run make first)" >&2
    exit 1
fi

python3 - "$ELF" <<'PY'
import struct
import sys

path = sys.argv[1]
MB_MAGIC = 0x1BADB002

with open(path, "rb") as f:
    data = f.read()

if data[:4] != b"\x7fELF":
    sys.exit(f"not an ELF: {path}")
if data[4] != 1:
    sys.exit("expected ELF32")

e_phoff, = struct.unpack_from("<I", data, 0x1C)
e_phentsize, e_phnum = struct.unpack_from("<HH", data, 0x2A)

found = False
for i in range(e_phnum):
    off = e_phoff + i * e_phentsize
    p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align = (
        struct.unpack_from("<IIIIIIII", data, off)
    )
    if p_type != 1:  # PT_LOAD
        continue
    chunk = data[p_offset : p_offset + p_filesz]
    window = chunk[:8192]
    for j in range(0, max(0, len(window) - 12), 4):
        magic, flags, checksum = struct.unpack_from("<III", window, j)
        if magic == MB_MAGIC and (magic + flags + checksum) & 0xFFFFFFFF == 0:
            found = True
            print(f"ok multiboot header at file+0x{p_offset + j:x} (flags=0x{flags:x})")
            break
    if found:
        break

if not found:
    sys.exit("Multiboot header (magic 0x1BADB002) not found in first 8 KiB of any PT_LOAD")
PY

run_qemu() {
    local append="${1:-}"
    local serial
    serial="$(mktemp)"
    local -a cmd=(
        "$QEMU"
        -kernel "$ELF"
        -serial stdio
        -display none
        -no-reboot
        -device isa-debug-exit,iobase=0xf4,iosize=0x04
    )
    if [[ -n "$append" ]]; then
        cmd+=(-append "$append")
    fi

    set +e
    "${cmd[@]}" >"$serial" 2>/dev/null
    local status=$?
    set -e

    if [[ "$status" -ne "$EXPECT_EXIT" ]]; then
        echo "FAIL qemu exit: got $status want $EXPECT_EXIT (append='$append')" >&2
        echo "--- serial ---" >&2
        cat "$serial" >&2 || true
        rm -f "$serial"
        return 1
    fi

    if ! grep -q "$BANNER" "$serial"; then
        echo "FAIL serial banner '$BANNER' not found (append='$append')" >&2
        echo "--- serial ---" >&2
        cat "$serial" >&2 || true
        rm -f "$serial"
        return 1
    fi

    printf '%s' "$serial"
}

SERIAL="$(run_qemu)"
trap 'rm -f "$SERIAL" "$SERIAL_DE" "$SERIAL_BP" "$SERIAL_PF" 2>/dev/null' EXIT

if ! grep -q "timer ok ticks=" "$SERIAL"; then
    echo "FAIL timer ok message not found" >&2
    echo "--- serial ---" >&2
    cat "$SERIAL" >&2 || true
    exit 1
fi
echo "ok timer ticks"

if ! grep -q "pmm: frames total=" "$SERIAL"; then
    echo "FAIL pmm stats not found" >&2
    echo "--- serial ---" >&2
    cat "$SERIAL" >&2 || true
    exit 1
fi
echo "ok pmm stats"

if ! grep -q "higher half" "$SERIAL"; then
    echo "FAIL higher-half banner not found" >&2
    echo "--- serial ---" >&2
    cat "$SERIAL" >&2 || true
    exit 1
fi
echo "ok higher half"

if ! grep -q "paging: 4K tables active" "$SERIAL"; then
    echo "FAIL paging init message not found" >&2
    echo "--- serial ---" >&2
    cat "$SERIAL" >&2 || true
    exit 1
fi
echo "ok paging"

if ! grep -q "heap: .* KiB ready" "$SERIAL"; then
    echo "FAIL heap ready message not found" >&2
    echo "--- serial ---" >&2
    cat "$SERIAL" >&2 || true
    exit 1
fi
echo "ok heap"

SERIAL_DE="$(run_qemu "fault=de")"
if ! grep -q "exception 0 (divide error)" "$SERIAL_DE"; then
    echo "FAIL divide error exception not reported" >&2
    echo "--- serial ---" >&2
    cat "$SERIAL_DE" >&2 || true
    exit 1
fi
echo "ok divide error fault"

SERIAL_BP="$(run_qemu "fault=bp")"
if ! grep -q "exception 3 (breakpoint)" "$SERIAL_BP"; then
    echo "FAIL breakpoint exception not reported" >&2
    echo "--- serial ---" >&2
    cat "$SERIAL_BP" >&2 || true
    exit 1
fi
echo "ok breakpoint fault"

SERIAL_PF="$(run_qemu "fault=pf")"
if ! grep -q "exception 14 (page fault)" "$SERIAL_PF"; then
    echo "FAIL page fault exception not reported" >&2
    echo "--- serial ---" >&2
    cat "$SERIAL_PF" >&2 || true
    exit 1
fi
if ! grep -q "page fault cr2=deadb000" "$SERIAL_PF"; then
    echo "FAIL page fault cr2/decode not reported" >&2
    echo "--- serial ---" >&2
    cat "$SERIAL_PF" >&2 || true
    exit 1
fi
echo "ok page fault"

echo "qemu boot test passed"
