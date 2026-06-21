"""numpy-style elementwise ufuncs (sigmoid + L2 norm) over float arrays.

Mirrors examples/rpython2c/numpy/ufuncs.py, but uses portable `list[float]`
arrays and hand-rolled `expf`/`sqrtf` (series / Newton) so the one source runs
on every backend -- the libm names in the original are ShivyC-only.
"""
import sys


def zeros(m: int) -> list[float]:
    v = []
    i = 0
    while i < m:
        v.append(0.0)
        i = i + 1
    return v


def my_exp(x: float) -> float:
    # exp via range-reduction + Taylor series; plenty accurate for a checksum.
    neg = 0
    if x < 0.0:
        neg = 1
        x = -x
    term = 1.0
    acc = 1.0
    k = 1
    while k < 30:
        term = term * x / k
        acc = acc + term
        k = k + 1
    if neg == 1:
        return 1.0 / acc
    return acc


def my_sqrt(x: float) -> float:
    if x <= 0.0:
        return 0.0
    guess = x
    i = 0
    while i < 40:
        guess = (guess + x / guess) / 2.0
        i = i + 1
    return guess


def sigmoid(x: list[float], out: list[float], n: int) -> None:
    i = 0
    while i < n:
        out[i] = 1.0 / (1.0 + my_exp(-x[i]))
        i = i + 1


def l2norm(x: list[float], n: int) -> float:
    acc = 0.0
    i = 0
    while i < n:
        acc = acc + x[i] * x[i]
        i = i + 1
    return my_sqrt(acc)


def main() -> int:
    n = int(sys.argv[1])
    x = zeros(n)
    out = zeros(n)
    i = 0
    while i < n:
        x[i] = (i % 64 - 32) / 8.0
        i = i + 1
    reps = 2000
    r = 0
    acc = 0.0
    while r < reps:
        sigmoid(x, out, n)
        s = 0.0
        i = 0
        while i < n:
            s = s + out[i]
            i = i + 1
        acc = acc + s + l2norm(x, n)
        r = r + 1
    print(int(acc) % 1000000)
    return int(acc) % 256


if __name__ == "__main__":
    sys.exit(main())
