"""Dense MLP forward-pass inference benchmark (float), shared by all backends.

Adapted from examples/rpython2c/nn/neural_net.py: a feed-forward network whose
forward pass is a matrix-vector product plus a sigmoid. The example's POD `Layer`
class (`f64*` weights, libm `exp`) is ShivyC-only, so here the weights are
portable `list[float]` arrays (via a typed `zeros()` helper) and the activation
is the hand-rolled `my_exp` sigmoid.

The number of inference repetitions is read from the command line and converted
to an integer, so the work cannot be constant-folded at build time. The program
prints a checksum of the accumulated outputs (a float, compared tolerantly).
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


def forward(w: list[float], b: list[float], x: list[float], out: list[float],
            n_in: int, n_out: int) -> None:
    j = 0
    while j < n_out:
        acc = b[j]
        i = 0
        while i < n_in:
            acc = acc + w[j * n_in + i] * x[i]
            i = i + 1
        out[j] = 1.0 / (1.0 + my_exp(-acc))
        j = j + 1


def checksum(out: list[float], n: int) -> float:
    acc = 0.0
    i = 0
    while i < n:
        acc = acc + out[i]
        i = i + 1
    return acc


def main() -> int:
    reps = int(sys.argv[1])
    n_in = 64
    n_hid = 32
    n_out = 16
    w1 = zeros(n_hid * n_in)
    b1 = zeros(n_hid)
    w2 = zeros(n_out * n_hid)
    b2 = zeros(n_out)
    x = zeros(n_in)
    h = zeros(n_hid)
    y = zeros(n_out)
    i = 0
    while i < n_hid * n_in:
        w1[i] = ((i % 7) - 3) * 0.1
        i = i + 1
    i = 0
    while i < n_hid:
        b1[i] = 0.1
        i = i + 1
    i = 0
    while i < n_out * n_hid:
        w2[i] = ((i % 5) - 2) * 0.1
        i = i + 1
    i = 0
    while i < n_out:
        b2[i] = -0.2
        i = i + 1
    i = 0
    while i < n_in:
        x[i] = (i % 9) * 0.25
        i = i + 1
    total = 0.0
    r = 0
    while r < reps:
        forward(w1, b1, x, h, n_in, n_hid)
        forward(w2, b2, h, y, n_hid, n_out)
        total = total + checksum(y, n_out)
        r = r + 1
    print(total)
    return int(total) % 256


if __name__ == "__main__":
    sys.exit(main())
