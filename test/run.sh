#!/usr/bin/env sh
# Build and run the native unit tests.
# Compiled as gnu++11 to mirror the AVR core (also guards the odr-use fix).
set -e
here="$(cd "$(dirname "$0")" && pwd)"
out="$(mktemp -d)/fasttimer_tests"

${CXX:-c++} -std=gnu++11 -Wall -Wextra \
    -I "$here/mock" -I "$here/../src" \
    "$here/test_main.cpp" -o "$out"

exec "$out"
