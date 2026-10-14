#!/bin/sh
set -eu
check() {
    expected="$1"; shift
    actual=$(./apc "$@")
    if [ "$actual" != "$expected" ]; then
        echo "FAIL: $*"
        echo " expected: $expected"
        echo "      got: $actual"
        exit 1
    fi
    echo "PASS: $*"
}
check '123 + 456 = 579' 123 + 456
check '1000 - 1 = 999' 1000 - 1
check '12345678901234567890 + 10 = 12345678901234567900' 12345678901234567890 + 10
check '12.50 + 0.25 = 12.75' 12.50 + 0.25
check '12.5 * 2 = 25' 12.5 '*' 2
check '7 / 2 = 3.5' 7 / 2
check '17 % 5 = 2' 17 '%' 5
check '2 ^ 10 = 1024' 2 '^' 10
