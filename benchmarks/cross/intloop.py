"""Modular nested-loop integer kernel, shared by all backends.

An O(n^2) double loop with a branch in the body, accumulating into a value kept
small with a modulo. `n` is read from the command line and converted to an
integer so the work cannot be constant-folded at build time.

Every intermediate stays below 2**31, so the result is identical whether the
backend uses 32-bit ints (py2c -> gcc -O3), 64-bit ints (Codon), or arbitrary
precision (CPython / PyPy / MicroPython). Pure integer arithmetic, so it
compiles unchanged under every backend.
"""
import sys


def kernel(n: int) -> int:
    acc = 1
    i = 1
    while i <= n:
        j = 1
        while j <= n:
            if (i + j) % 2 == 0:
                acc = (acc + i * j) % 1000003
            else:
                acc = (acc + i + j) % 1000003
            j = j + 1
        i = i + 1
    return acc


def main() -> int:
    n = int(sys.argv[1])
    result = kernel(n)
    print(result)
    return result % 256


if __name__ == "__main__":
    sys.exit(main())
