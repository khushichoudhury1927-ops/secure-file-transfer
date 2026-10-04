#!/usr/bin/env bash
# Run from anywhere: bash tests/run_tests.sh
cd "$(dirname "$0")/.." || exit 1
BIN=./build/sft
PORT=${1:-9100}
PASS=0
FAIL=0
TMP=$(mktemp -d)

check() {
    if [ "$2" -eq 0 ]; then echo "PASS: $1"; PASS=$((PASS+1));
    else echo "FAIL: $1"; FAIL=$((FAIL+1)); fi
}

rm -rf received
$BIN server $PORT > "$TMP/server.log" 2>&1 &
SPID=$!
trap 'kill $SPID 2>/dev/null; rm -rf "$TMP"' EXIT
sleep 1

echo "hello" > "$TMP/small.txt"
$BIN client $PORT "$TMP/small.txt" > /dev/null 2>&1
cmp -s "$TMP/small.txt" received/small.txt; check "small file transfer" $?

head -c 5000000 /dev/urandom > "$TMP/big.bin"
$BIN client $PORT "$TMP/big.bin" > /dev/null 2>&1
cmp -s "$TMP/big.bin" received/big.bin; check "5 MB file transfer" $?

: > "$TMP/empty.txt"
$BIN client $PORT "$TMP/empty.txt" > /dev/null 2>&1
cmp -s "$TMP/empty.txt" received/empty.txt; check "empty file transfer" $?

head -c 3000000 /dev/urandom > "$TMP/resume.bin"
SFT_STOP_AFTER=1000000 $BIN client $PORT "$TMP/resume.bin" > /dev/null 2>&1
test -f received/resume.bin.part; check "interrupted transfer leaves .part file" $?
$BIN client $PORT "$TMP/resume.bin" > "$TMP/resume.log" 2>&1
grep -q "Resuming from offset: [1-9]" "$TMP/resume.log"; check "client resumes from a non-zero offset" $?
cmp -s "$TMP/resume.bin" received/resume.bin; check "resumed file is identical" $?

$BIN client $PORT "$TMP/does-not-exist.bin" > /dev/null 2>&1
[ $? -ne 0 ]; check "missing file is reported as an error" $?

echo "Passed: $PASS  Failed: $FAIL"
[ "$FAIL" -eq 0 ]
