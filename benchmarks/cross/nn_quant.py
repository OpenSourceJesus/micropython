"""8-bit quantized MLP inference benchmark (integer), shared by all backends.

Adapted from examples/rpython2c/nn/quant_mlp.py: a post-training-quantized
network whose linear layers are integer dot products with a byte-sum correction
term, requantized through a shift, ReLU, and saturation back to u8. The example
uses ShivyC-only `u8*` arrays and a PSADBW length contract; here the byte buffers
are portable `list[int]` arrays (via a typed `zeros_int()` helper) and the kernel
is plain integer arithmetic.

The number of inference repetitions is read from the command line and converted
to an integer, so the work cannot be constant-folded at build time. Everything is
integer and every accumulator stays below 2**31 (so it agrees whether the backend
uses 32-bit or wider ints); the program prints an exact integer checksum.
"""
import sys

N_IN = 32
N_HID = 16
N_OUT = 8
ZPW = 40
SHIFT = 9


def zeros_int(m: int) -> list[int]:
    v = []
    i = 0
    while i < m:
        v.append(0)
        i = i + 1
    return v


def bytesum(x: list[int], n: int) -> int:
    acc = 0
    i = 0
    while i < n:
        acc = acc + x[i]
        i = i + 1
    return acc


def qdot(w: list[int], x: list[int], off: int, n: int) -> int:
    acc = 0
    i = 0
    while i < n:
        acc = acc + w[off + i] * x[i]
        i = i + 1
    return acc


def qlinear(w: list[int], x: list[int], xsum: int, out: list[int],
            n_in: int, n_out: int, zpw: int, shift: int) -> None:
    o = 0
    while o < n_out:
        acc = qdot(w, x, o * n_in, n_in) - zpw * xsum
        acc = acc >> shift
        if acc < 0:
            acc = 0
        if acc > 255:
            acc = 255
        out[o] = acc
        o = o + 1


def main() -> int:
    reps = int(sys.argv[1])
    x = zeros_int(N_IN)
    i = 0
    while i < N_IN:
        x[i] = (i * 5 + 17) % 200
        i = i + 1
    w1 = zeros_int(N_HID * N_IN)
    i = 0
    while i < N_HID * N_IN:
        w1[i] = (i * 11 + 7) % 128
        i = i + 1
    w2 = zeros_int(N_OUT * N_HID)
    i = 0
    while i < N_OUT * N_HID:
        w2[i] = (i * 13 + 5) % 128
        i = i + 1
    h = zeros_int(N_HID)
    y = zeros_int(N_OUT)

    total = 0
    r = 0
    while r < reps:
        xsum = bytesum(x, N_IN)
        qlinear(w1, x, xsum, h, N_IN, N_HID, ZPW, SHIFT)
        hsum = bytesum(h, N_HID)
        qlinear(w2, h, hsum, y, N_HID, N_OUT, ZPW, SHIFT)
        o = 0
        while o < N_OUT:
            total = total + y[o]
            o = o + 1
        r = r + 1
    print(total)
    return total % 250


if __name__ == "__main__":
    sys.exit(main())
