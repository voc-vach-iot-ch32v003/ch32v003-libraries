#!/usr/bin/env bash
# Chạy toàn bộ kiểm thử logic thuần trên máy tính (không cần phần cứng).
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
LIBS="$HERE/../.."
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

gcc -std=gnu11 -Wall -Wextra -O1 -I"$LIBS/ch32v003_intmath/src" \
    "$HERE/test_intmath.c" "$LIBS/ch32v003_intmath/src/ch32v003_intmath.c" -lm -o "$OUT/test_intmath"
"$OUT/test_intmath"
