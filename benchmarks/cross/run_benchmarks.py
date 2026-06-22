#!/usr/bin/env python3
"""Cross-implementation benchmark runner.

Compiles/runs the shared benchmark sources (fib, pi, mandelbrot, intloop,
primes; the numpy-style float-array kernels np_matmul / np_saxpy / np_ufuncs;
and the physics + neural-net kernels nbody / nn_inference / nn_train / nn_quant)
under six backends and reports wall-clock timings:

  * micropython  -- the unix port interpreter (ports/unix/build-standard)
  * pypy         -- PyPy3 JIT
  * nuitka       -- Nuitka, CPython source compiled to a native binary
  * codon        -- Codon, ahead-of-time native compiler
  * rpython      -- ShivyC's py2c transpiles the rpython-annotated source to C,
                    then gcc -O3 builds a native binary WITHOUT the interpreter
  * shivycx      -- ShivyC's own compiler (shivyc.main) builds a native binary
                    with its own backend (no external gcc -O3 step)

If a ShivyC backend (rpython or shivycx) fails to build, a warning is printed
and that backend is dropped from the timing table rather than reported.

Every benchmark reads its workload size from the command line and converts it to
an integer (``int(sys.argv[1])``). That value is invisible to the compilers at
build time, so none of them can constant-fold the work away. Under py2c that
call lowers to ``atoi(argv[1])`` and ``main`` becomes ``int main(int, char**)``.

If a ShivyC backend builds but computes the wrong answer (a silent miscompile),
it is verified against a CPython reference, warned about, and dropped too.

Usage:
    python3 run_benchmarks.py [--<bench> N ...] [--repeats R] [--build-only]
    # e.g. python3 run_benchmarks.py --fib 33 --primes 100000 --repeats 5
"""
import argparse
import os
import shutil
import statistics
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
BIN = os.path.join(HERE, "bin")

SHIVYC_DIR = os.environ.get("SHIVYC_DIR", os.path.expanduser("~/ShivyC"))
PY2C = os.path.join(SHIVYC_DIR, "tools", "py2c.py")
SHIVYC_MAIN_OK = os.path.isdir(os.path.join(SHIVYC_DIR, "shivyc"))

MICROPYTHON = os.path.join(REPO, "ports", "unix", "build-standard", "micropython")
NUITKA = shutil.which("nuitka") or os.path.expanduser("~/.local/bin/nuitka")

# Nuitka and py2c need writable scratch space; keep it off the sandboxed HOME.
SCRATCH = os.environ.get("CROSS_SCRATCH", "/tmp/cross_bench")
os.environ.setdefault("NUITKA_CACHE_DIR", os.path.join(SCRATCH, "nuitka_cache"))

# Each benchmark maps to a default workload size (the dynamic command-line N).
DEFAULT_SIZES = {
    "fib": 35,
    "pi": 20_000_000,
    "mandelbrot": 250,
    "intloop": 2500,
    "primes": 150_000,
    # numpy-style float-array kernels (adapted from ShivyC's rpython2c/numpy).
    "np_matmul": 160,      # NxN matrix multiply -> O(N^3)
    "np_saxpy": 200_000,   # vector length; kernel repeated internally
    "np_ufuncs": 4_000,    # sigmoid + L2 norm; repeated internally
    # physics + neural-net kernels (adapted from rpython2c/nbody and rpython2c/nn).
    "nbody": 20_000,       # gravity integration steps (5 bodies)
    "nn_inference": 20_000,  # dense MLP forward-pass repetitions
    "nn_train": 30_000,    # MLP training epochs (XOR)
    "nn_quant": 300_000,   # 8-bit quantized MLP inference repetitions
}
BENCHMARKS = tuple(DEFAULT_SIZES)

# Backends built/run by ShivyC; on a silent miscompile they are dropped + warned.
SHIVYC_BACKENDS = ("rpython", "shivycx")


def sh(cmd, **kw):
    return subprocess.run(cmd, cwd=kw.pop("cwd", HERE), text=True,
                          capture_output=True, **kw)


def warn_shivyc(backend, what, detail=""):
    """ShivyC-based backend failed: print a warning so it is dropped, not timed."""
    print("  WARNING: ShivyC backend %r %s; excluding from results."
          % (backend, what))
    if detail:
        print("           %s" % detail.strip().splitlines()[-1][:200])


# --------------------------------------------------------------------------
# Builders. Each returns the path to a runnable native binary, or None if the
# backend's toolchain is unavailable / the build failed.

def build_rpython(name):
    """rpython-annotated source -> C (py2c) -> native binary (gcc -O3)."""
    if not os.path.exists(PY2C):
        warn_shivyc("rpython", "py2c not found at %s" % PY2C)
        return None
    cdir = os.path.join(SCRATCH, name + "_c")
    shutil.rmtree(cdir, ignore_errors=True)
    os.makedirs(cdir, exist_ok=True)
    r = sh([sys.executable, PY2C, name + ".py", "--out", cdir])
    if r.returncode != 0:
        warn_shivyc("rpython", "py2c transpile failed", r.stderr)
        return None
    out = os.path.join(BIN, name + "_rpython")
    csrc = [os.path.join(cdir, f) for f in os.listdir(cdir) if f.endswith(".c")]
    r = sh(["gcc", "-O3", "-std=c99", "-I" + cdir, *csrc, "-o", out])
    if r.returncode != 0:
        warn_shivyc("rpython", "gcc -O3 build failed", r.stderr)
        return None
    return out


def build_shivycx(name):
    """rpython-annotated source -> native binary via ShivyC's own compiler.

    `shivyc.main` transpiles the source to C and links it with ShivyC's own
    backend (no external -O3 step). Run under the host Python with SHIVYC_DIR on
    PYTHONPATH so the `shivyc` package imports without being installed.
    """
    if not SHIVYC_MAIN_OK:
        warn_shivyc("shivycx", "shivyc package not found in %s" % SHIVYC_DIR)
        return None
    out = os.path.join(BIN, name + "_shivycx")
    env = dict(os.environ)
    env["PYTHONPATH"] = SHIVYC_DIR + os.pathsep + env.get("PYTHONPATH", "")
    r = sh([sys.executable, "-m", "shivyc.main", "--no-cache",
            os.path.join(HERE, name + ".py"), "-o", out], env=env)
    if r.returncode != 0 or not os.path.exists(out):
        warn_shivyc("shivycx", "shivyc.main build failed", r.stderr)
        return None
    return out


def build_codon(name):
    if not shutil.which("codon"):
        print("  [codon] codon not found; skipping")
        return None
    out = os.path.join(BIN, name + "_codon")
    r = sh(["codon", "build", "-release", "-exe", name + ".py", "-o", out])
    if r.returncode != 0:
        print("  [codon] build failed:\n%s" % r.stderr[-800:])
        return None
    return out


def build_nuitka(name):
    if not (NUITKA and os.path.exists(NUITKA)):
        print("  [nuitka] nuitka not found; skipping")
        return None
    os.makedirs(os.environ["NUITKA_CACHE_DIR"], exist_ok=True)
    odir = os.path.join(SCRATCH, "nuitka_" + name)
    r = sh([NUITKA, "--quiet", "--no-progressbar", "--assume-yes-for-downloads",
            "--output-dir=" + odir, "--output-filename=" + name + "_nuitka",
            name + ".py"], timeout=900)
    built = os.path.join(odir, name + "_nuitka")
    if r.returncode != 0 or not os.path.exists(built):
        print("  [nuitka] build failed:\n%s" % r.stderr[-800:])
        return None
    out = os.path.join(BIN, name + "_nuitka")
    shutil.copy2(built, out)
    return out


# --------------------------------------------------------------------------
# Backend command tables. A backend is (label, command-prefix) where the
# workload size N is appended at run time. Compiled backends are built first.

def backends_for(name, arg):
    cmds = []
    if os.path.exists(MICROPYTHON):
        # Bump the GC heap so the float-array benchmarks (which box every list
        # element) do not exhaust MicroPython's small default heap.
        cmds.append(("micropython",
                     [MICROPYTHON, "-X", "heapsize=512M", name + ".py"]))
    else:
        print("  [micropython] %s not built; skipping" % MICROPYTHON)
    if shutil.which("pypy3"):
        cmds.append(("pypy", ["pypy3", name + ".py"]))
    for label, builder in (("nuitka", build_nuitka),
                           ("codon", build_codon),
                           ("rpython", build_rpython),
                           ("shivycx", build_shivycx)):
        path = builder(name)
        if path:
            cmds.append((label, [path]))
    return [(label, cmd + [str(arg)]) for label, cmd in cmds]


def time_cmd(cmd, repeats):
    """Best-of-`repeats` wall time (seconds); returns (best, output)."""
    best = None
    out = ""
    for _ in range(repeats):
        t0 = time.perf_counter()
        p = subprocess.run(cmd, cwd=HERE, text=True, capture_output=True)
        dt = time.perf_counter() - t0
        out = p.stdout.strip()
        best = dt if best is None else min(best, dt)
    return best, out


def outputs_match(a, b):
    """Equal as strings, or as floats within tolerance.

    ShivyC's runtime prints floats with fewer significant digits (e.g.
    `3.14159`) than CPython (`3.1415925...`), so a float benchmark must be
    compared numerically, not byte-for-byte, to avoid false 'wrong answer'.
    """
    if a == b:
        return True
    try:
        fa, fb = float(a), float(b)
    except ValueError:
        return False
    return abs(fa - fb) <= 1e-5 * max(1.0, abs(fb))


def reference_output(name, arg):
    """Trusted result for this workload, computed with host CPython."""
    p = subprocess.run([sys.executable, name + ".py", str(arg)],
                       cwd=HERE, text=True, capture_output=True)
    return p.stdout.strip()


def main():
    ap = argparse.ArgumentParser(
        description="Cross-implementation benchmark runner.")
    for name, default in DEFAULT_SIZES.items():
        ap.add_argument("--" + name, type=int, default=default,
                        metavar="N", help="%s workload size (default %d)"
                        % (name, default))
    ap.add_argument("--repeats", type=int, default=3)
    ap.add_argument("--build-only", action="store_true")
    args = ap.parse_args()

    os.makedirs(BIN, exist_ok=True)
    os.makedirs(SCRATCH, exist_ok=True)
    sizes = {name: getattr(args, name) for name in BENCHMARKS}

    for name in BENCHMARKS:
        arg = sizes[name]
        print("\n=== %s  (N=%d) ===" % (name, arg))
        rows = backends_for(name, arg)
        if args.build_only:
            continue
        ref = reference_output(name, arg)
        results = []
        for label, cmd in rows:
            best, out = time_cmd(cmd, args.repeats)
            ok = outputs_match(out, ref)
            # A ShivyC backend that builds but computes the wrong answer is a
            # silent miscompile: warn and drop it rather than report it.
            if not ok and label in SHIVYC_BACKENDS:
                warn_shivyc(label, "produced wrong output %r (expected %r)"
                            % (out, ref))
                continue
            results.append((label, best, out, ok))
        baseline = next((b for l, b, _, _ in results if l == "micropython"), None)
        results.sort(key=lambda r: r[1])
        print("  %-12s %12s %10s   %s" % ("backend", "best (s)", "speedup", "output"))
        for label, best, out, ok in results:
            sp = ("%6.1fx" % (baseline / best)) if baseline else "   -  "
            flag = "" if ok else "  (!= reference)"
            print("  %-12s %12.4f %10s   %s%s" % (label, best, sp, out, flag))


if __name__ == "__main__":
    main()
