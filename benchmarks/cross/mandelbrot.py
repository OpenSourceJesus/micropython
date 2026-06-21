"""Mandelbrot escape-time benchmark (float-heavy), shared by all backends.

Renders a `size` x `size` grid and sums the escape iteration counts. `size` is
read from the command line and converted to an integer, so the work cannot be
constant-folded at build time. Pure scalar `int`/`float` arithmetic only, so it
compiles unchanged under CPython, PyPy, MicroPython, Nuitka, Codon, ShivyC's
py2c->gcc -O3 path, and ShivyC's own compiler.
"""
import sys


def mandelbrot(size: int) -> int:
    total = 0
    py = 0
    while py < size:
        px = 0
        while px < size:
            cr = -2.0 + 3.0 * px / size
            ci = -1.5 + 3.0 * py / size
            zr = 0.0
            zi = 0.0
            n = 0
            while n < 100:
                zr2 = zr * zr
                zi2 = zi * zi
                if zr2 + zi2 > 4.0:
                    break
                zi = 2.0 * zr * zi + ci
                zr = zr2 - zi2 + cr
                n = n + 1
            total = total + n
            px = px + 1
        py = py + 1
    return total


def main() -> int:
    size = int(sys.argv[1])
    result = mandelbrot(size)
    print(result)
    return result % 256


if __name__ == "__main__":
    sys.exit(main())
