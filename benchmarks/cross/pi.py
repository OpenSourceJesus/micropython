"""Leibniz pi-approximation benchmark, shared by all backends.

The term count is read from the command line and converted to an integer so the
compilers cannot constant-fold the loop away -- the iteration count is only
known at runtime. Under ShivyC's py2c this is `atoi(argv[1])`.

Only plain `int`/`float` annotations are used so the one source compiles under
CPython, PyPy, MicroPython, Nuitka, Codon, and py2c->gcc -O3 alike.
"""
import sys


def calc_pi(terms: int) -> float:
    acc = 0.0
    sign = 1.0
    k = 0
    while k < terms:
        acc = acc + sign / (2.0 * k + 1.0)
        sign = -sign
        k = k + 1
    return acc * 4.0


def main() -> int:
    n = int(sys.argv[1])
    result = calc_pi(n)
    print(result)
    return int(result * 100.0) % 256


if __name__ == "__main__":
    sys.exit(main())
