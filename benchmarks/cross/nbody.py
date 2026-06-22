"""N-body gravity simulation (float-heavy), shared by all backends.

Adapted from examples/rpython2c/nbody/nbody.py, but the bodies are stored in flat
parallel `list[float]` arrays (allocated through a typed `zeros()` helper) instead
of the ShivyC-only POD `Body` class with `Body*` pointers, and `sqrt` is the
hand-rolled `my_sqrt` (the example's bare libm name is ShivyC-only). So the one
source runs on every backend.

The number of integration steps is read from the command line and converted to
an integer, so the work cannot be constant-folded at build time. The program
prints the system's total energy (kinetic + potential) after the run -- a
bounded, conserved quantity, so the float output stays close across backends and
matches under the runner's tolerant comparison.
"""
import sys


def zeros(m: int) -> list[float]:
    v = []
    i = 0
    while i < m:
        v.append(0.0)
        i = i + 1
    return v


def my_sqrt(x: float) -> float:
    if x <= 0.0:
        return 0.0
    guess = x
    i = 0
    while i < 40:
        guess = (guess + x / guess) / 2.0
        i = i + 1
    return guess


def advance(x: list[float], y: list[float], z: list[float],
            vx: list[float], vy: list[float], vz: list[float],
            mass: list[float], n: int, dt: float) -> None:
    i = 0
    while i < n:
        j = i + 1
        while j < n:
            dx = x[i] - x[j]
            dy = y[i] - y[j]
            dz = z[i] - z[j]
            r2 = dx * dx + dy * dy + dz * dz + 0.0001
            dist = my_sqrt(r2)
            mag = dt / (r2 * dist)
            mi = mass[i]
            mj = mass[j]
            vx[i] = vx[i] - dx * mj * mag
            vy[i] = vy[i] - dy * mj * mag
            vz[i] = vz[i] - dz * mj * mag
            vx[j] = vx[j] + dx * mi * mag
            vy[j] = vy[j] + dy * mi * mag
            vz[j] = vz[j] + dz * mi * mag
            j = j + 1
        i = i + 1
    i = 0
    while i < n:
        x[i] = x[i] + dt * vx[i]
        y[i] = y[i] + dt * vy[i]
        z[i] = z[i] + dt * vz[i]
        i = i + 1


def energy(x: list[float], y: list[float], z: list[float],
           vx: list[float], vy: list[float], vz: list[float],
           mass: list[float], n: int) -> float:
    e = 0.0
    i = 0
    while i < n:
        e = e + 0.5 * mass[i] * (vx[i] * vx[i] + vy[i] * vy[i] + vz[i] * vz[i])
        j = i + 1
        while j < n:
            dx = x[i] - x[j]
            dy = y[i] - y[j]
            dz = z[i] - z[j]
            dist = my_sqrt(dx * dx + dy * dy + dz * dz + 0.0001)
            e = e - mass[i] * mass[j] / dist
            j = j + 1
        i = i + 1
    return e


def main() -> int:
    steps = int(sys.argv[1])
    n = 5
    x = zeros(n)
    y = zeros(n)
    z = zeros(n)
    vx = zeros(n)
    vy = zeros(n)
    vz = zeros(n)
    mass = zeros(n)
    # A small deterministic cluster: a heavy central body plus four satellites.
    i = 0
    while i < n:
        x[i] = (i - 2) * 1.0
        y[i] = (i % 3) * 0.5
        z[i] = (i % 2) * 0.3
        vx[i] = (i % 2) * 0.2 - 0.1
        vy[i] = (i - 2) * 0.05
        vz[i] = 0.0
        mass[i] = 1.0
        i = i + 1
    mass[0] = 40.0
    dt = 0.001
    s = 0
    while s < steps:
        advance(x, y, z, vx, vy, vz, mass, n, dt)
        s = s + 1
    e = energy(x, y, z, vx, vy, vz, mass, n)
    print(e)
    return int(e) % 256


if __name__ == "__main__":
    sys.exit(main())
