"""MLP training benchmark -- XOR via forward + backprop + SGD, all backends.

Adapted from examples/rpython2c/nn/torch_mlp.py: a 2 -> 2 -> 1 sigmoid network
trained with full-batch gradient descent. The example uses ShivyC-only `f64*`
arrays and a bundled mini-PyTorch (`from rpy_torch import ...`); here every buffer
is a portable `list[float]` (via a typed `zeros()` helper) and the kernels
(`linear`, `sigmoid`, the gradients, and the SGD step) are inlined as plain
helper functions with the hand-rolled `my_exp`.

The number of epochs is read from the command line and converted to an integer,
so the work cannot be constant-folded at build time. Training is deterministic
and convergent (fixed initial weights), so the printed final loss agrees across
backends under the runner's tolerant comparison.
"""
import sys

NIN = 2
NH = 2
NOUT = 1
NS = 4


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


def linear(w: list[float], b: list[float], x: list[float], out: list[float],
           n_in: int, n_out: int) -> None:
    o = 0
    while o < n_out:
        acc = b[o]
        i = 0
        while i < n_in:
            acc = acc + w[o * n_in + i] * x[i]
            i = i + 1
        out[o] = acc
        o = o + 1


def sigmoid(z: list[float], out: list[float], n: int) -> None:
    i = 0
    while i < n:
        out[i] = 1.0 / (1.0 + my_exp(-z[i]))
        i = i + 1


def sigmoid_grad(a: list[float], grad_a: list[float], grad_z: list[float],
                 n: int) -> None:
    i = 0
    while i < n:
        grad_z[i] = grad_a[i] * a[i] * (1.0 - a[i])
        i = i + 1


def linear_grad(w: list[float], x: list[float], grad_z: list[float],
                grad_w: list[float], grad_b: list[float], grad_x: list[float],
                n_in: int, n_out: int) -> None:
    i = 0
    while i < n_in:
        grad_x[i] = 0.0
        i = i + 1
    o = 0
    while o < n_out:
        grad_b[o] = grad_z[o]
        i = 0
        while i < n_in:
            grad_w[o * n_in + i] = grad_z[o] * x[i]
            grad_x[i] = grad_x[i] + grad_z[o] * w[o * n_in + i]
            i = i + 1
        o = o + 1


def sgd_step(w: list[float], grad: list[float], lr: float, n: int) -> None:
    i = 0
    while i < n:
        w[i] = w[i] - lr * grad[i]
        i = i + 1


def main() -> int:
    epochs = int(sys.argv[1])
    data = zeros(NS * NIN)
    tgt = zeros(NS)
    data[0] = 0.0; data[1] = 0.0; tgt[0] = 0.0
    data[2] = 0.0; data[3] = 1.0; tgt[1] = 1.0
    data[4] = 1.0; data[5] = 0.0; tgt[2] = 1.0
    data[6] = 1.0; data[7] = 1.0; tgt[3] = 0.0

    w1 = zeros(NH * NIN)
    b1 = zeros(NH)
    w2 = zeros(NOUT * NH)
    b2 = zeros(NOUT)
    w1[0] = 0.5; w1[1] = -0.4; w1[2] = -0.3; w1[3] = 0.6
    b1[0] = 0.1; b1[1] = -0.1
    w2[0] = 0.7; w2[1] = -0.5
    b2[0] = 0.2

    x = zeros(NIN)
    z1 = zeros(NH)
    h1 = zeros(NH)
    z2 = zeros(NOUT)
    p = zeros(NOUT)
    gp = zeros(NOUT)
    gz2 = zeros(NOUT)
    gw2 = zeros(NOUT * NH)
    gb2 = zeros(NOUT)
    gh1 = zeros(NH)
    gz1 = zeros(NH)
    gw1 = zeros(NH * NIN)
    gb1 = zeros(NH)
    gx = zeros(NIN)
    aw1 = zeros(NH * NIN)
    ab1 = zeros(NH)
    aw2 = zeros(NOUT * NH)
    ab2 = zeros(NOUT)

    lr = 2.0
    loss_final = 0.0
    e = 0
    while e < epochs:
        k = 0
        while k < NH * NIN:
            aw1[k] = 0.0
            k = k + 1
        k = 0
        while k < NH:
            ab1[k] = 0.0
            k = k + 1
        k = 0
        while k < NOUT * NH:
            aw2[k] = 0.0
            k = k + 1
        ab2[0] = 0.0
        epoch_loss = 0.0
        s = 0
        while s < NS:
            x[0] = data[s * NIN]
            x[1] = data[s * NIN + 1]
            linear(w1, b1, x, z1, NIN, NH)
            sigmoid(z1, h1, NH)
            linear(w2, b2, h1, z2, NH, NOUT)
            sigmoid(z2, p, NOUT)
            diff = p[0] - tgt[s]
            epoch_loss = epoch_loss + diff * diff
            gp[0] = (2.0 / NS) * (p[0] - tgt[s])
            sigmoid_grad(p, gp, gz2, NOUT)
            linear_grad(w2, h1, gz2, gw2, gb2, gh1, NH, NOUT)
            sigmoid_grad(h1, gh1, gz1, NH)
            linear_grad(w1, x, gz1, gw1, gb1, gx, NIN, NH)
            k = 0
            while k < NOUT * NH:
                aw2[k] = aw2[k] + gw2[k]
                k = k + 1
            ab2[0] = ab2[0] + gb2[0]
            k = 0
            while k < NH * NIN:
                aw1[k] = aw1[k] + gw1[k]
                k = k + 1
            k = 0
            while k < NH:
                ab1[k] = ab1[k] + gb1[k]
                k = k + 1
            s = s + 1
        sgd_step(w2, aw2, lr, NOUT * NH)
        sgd_step(b2, ab2, lr, NOUT)
        sgd_step(w1, aw1, lr, NH * NIN)
        sgd_step(b1, ab1, lr, NH)
        loss_final = epoch_loss / NS
        e = e + 1

    print(loss_final)
    return int(loss_final * 1000.0) % 256


if __name__ == "__main__":
    sys.exit(main())
