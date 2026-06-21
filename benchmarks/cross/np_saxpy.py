"""SAXPY (out = alpha*x + y) elementwise kernel (numpy-style), all backends.

Mirrors examples/rpython2c/numpy/saxpy.py, but uses portable `list[float]`
arrays (via a typed `zeros()` helper) instead of the ShivyC-only `f32*` native
arrays, so the one source runs on every backend. The vector length N is read
from the command line; the kernel is repeated to make the timing meaningful.
"""
import sys


def zeros(m: int) -> list[float]:
    v = []
    i = 0
    while i < m:
        v.append(0.0)
        i = i + 1
    return v


def saxpy(alpha: float, x: list[float], y: list[float], out: list[float], n: int) -> None:
    i = 0
    while i < n:
        out[i] = alpha * x[i] + y[i]
        i = i + 1


def reduce_sum(out: list[float], n: int) -> float:
    acc = 0.0
    i = 0
    while i < n:
        acc = acc + out[i]
        i = i + 1
    return acc


def main() -> int:
    n = int(sys.argv[1])
    x = zeros(n)
    y = zeros(n)
    out = zeros(n)
    i = 0
    while i < n:
        x[i] = (i % 17) * 1.0
        y[i] = (i % 13) * 1.0
        i = i + 1
    reps = 200
    r = 0
    acc = 0.0
    while r < reps:
        saxpy(2.5, x, y, out, n)
        acc = acc + reduce_sum(out, n)
        r = r + 1
    print(int(acc) % 1000000)
    return int(acc) % 256


if __name__ == "__main__":
    sys.exit(main())
