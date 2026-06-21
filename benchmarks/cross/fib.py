"""Recursive Fibonacci benchmark, shared by all backends.

`n` is read from the command line and converted to an integer, so the
compilers (Nuitka, Codon, ShivyC->gcc -O3) cannot fold the call away at build
time -- the work is genuinely dynamic. Under ShivyC's py2c, `int(sys.argv[1])`
lowers to `atoi(argv[1])` and `main` becomes `int main(int argc, char** argv)`.

The type annotations are plain `int` hints: valid Python (ignored by CPython /
PyPy / MicroPython / Nuitka), understood by Codon, and used by py2c as rpython
annotations to emit concrete C `long` arithmetic.
"""
import sys


def fib(n: int) -> int:
    if n < 2:
        return n
    return fib(n - 1) + fib(n - 2)


def main() -> int:
    n = int(sys.argv[1])
    result = fib(n)
    print(result)
    return result % 256


if __name__ == "__main__":
    sys.exit(main())
