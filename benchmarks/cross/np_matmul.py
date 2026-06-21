"""Naive NxN matrix multiply over float arrays (numpy-style kernel).

Mirrors examples/rpython2c/numpy/matmul.py, but uses portable `list[float]`
arrays (allocated through a typed `zeros()` helper) instead of the ShivyC-only
`malloc`/`f64*` native pointers, so the one source runs on every backend. The
matrix dimension N is read from the command line. The unquoted `list[float]`
annotations let py2c lower the lists to native `double*` while CPython, PyPy,
MicroPython, Codon, and Nuitka ignore the hints.
"""
import sys


def zeros(m: int) -> list[float]:
    v = []
    i = 0
    while i < m:
        v.append(0.0)
        i = i + 1
    return v


def matmul(a: list[float], b: list[float], c: list[float], n: int) -> None:
    i = 0
    while i < n:
        j = 0
        while j < n:
            acc = 0.0
            k = 0
            while k < n:
                acc = acc + a[i * n + k] * b[k * n + j]
                k = k + 1
            c[i * n + j] = acc
            j = j + 1
        i = i + 1


def trace(c: list[float], n: int) -> float:
    acc = 0.0
    i = 0
    while i < n:
        acc = acc + c[i * n + i]
        i = i + 1
    return acc


def main() -> int:
    n = int(sys.argv[1])
    size = n * n
    a = zeros(size)
    b = zeros(size)
    c = zeros(size)
    i = 0
    while i < size:
        a[i] = (i % 7) * 1.0
        b[i] = (i % 5) * 1.0
        i = i + 1
    matmul(a, b, c, n)
    acc = trace(c, n)
    print(int(acc))
    return int(acc) % 256


if __name__ == "__main__":
    sys.exit(main())
