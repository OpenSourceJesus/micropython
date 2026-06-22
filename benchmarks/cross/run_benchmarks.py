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

For each backend it reports both the run time and the compile time (interpreted
backends have none), and -- unless --no-plot is given -- writes one matplotlib
graph per benchmark (run vs compile time) plus a combined speedup overview to
the plots/ directory.

Usage:
    python3 run_benchmarks.py [--<bench> N ...] [--repeats R] [--build-only]
                              [--no-plot] [--plot-dir DIR]
    # e.g. python3 run_benchmarks.py --fib 33 --primes 100000 --repeats 5
"""
import argparse
import json
import os
import shutil
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


def timed_sh(cmd, **kw):
    """Like sh(), but also returns the wall-clock duration of the command."""
    t0 = time.perf_counter()
    r = sh(cmd, **kw)
    return r, time.perf_counter() - t0


def warn_shivyc(backend, what, detail=""):
    """ShivyC-based backend failed: print a warning so it is dropped, not timed."""
    print("  WARNING: ShivyC backend %r %s; excluding from results."
          % (backend, what))
    if detail:
        print("           %s" % detail.strip().splitlines()[-1][:200])


# --------------------------------------------------------------------------
# Builders. Each returns (path, build_seconds) for a runnable native binary,
# or None if the backend's toolchain is unavailable / the build failed.

def build_rpython(name):
    """rpython-annotated source -> C (py2c) -> native binary (gcc -O3)."""
    if not os.path.exists(PY2C):
        warn_shivyc("rpython", "py2c not found at %s" % PY2C)
        return None
    cdir = os.path.join(SCRATCH, name + "_c")
    shutil.rmtree(cdir, ignore_errors=True)
    os.makedirs(cdir, exist_ok=True)
    r, t_transpile = timed_sh([sys.executable, PY2C, name + ".py", "--out", cdir])
    if r.returncode != 0:
        warn_shivyc("rpython", "py2c transpile failed", r.stderr)
        return None
    out = os.path.join(BIN, name + "_rpython")
    csrc = [os.path.join(cdir, f) for f in os.listdir(cdir) if f.endswith(".c")]
    r, t_gcc = timed_sh(["gcc", "-O3", "-std=c99", "-I" + cdir, *csrc, "-o", out])
    if r.returncode != 0:
        warn_shivyc("rpython", "gcc -O3 build failed", r.stderr)
        return None
    return out, t_transpile + t_gcc


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
    r, t_build = timed_sh([sys.executable, "-m", "shivyc.main", "--no-cache",
                           os.path.join(HERE, name + ".py"), "-o", out], env=env)
    if r.returncode != 0 or not os.path.exists(out):
        warn_shivyc("shivycx", "shivyc.main build failed", r.stderr)
        return None
    return out, t_build


def build_codon(name):
    if not shutil.which("codon"):
        print("  [codon] codon not found; skipping")
        return None
    out = os.path.join(BIN, name + "_codon")
    r, t_build = timed_sh(["codon", "build", "-release", "-exe",
                           name + ".py", "-o", out])
    if r.returncode != 0:
        print("  [codon] build failed:\n%s" % r.stderr[-800:])
        return None
    return out, t_build


def build_nuitka(name):
    if not (NUITKA and os.path.exists(NUITKA)):
        print("  [nuitka] nuitka not found; skipping")
        return None
    os.makedirs(os.environ["NUITKA_CACHE_DIR"], exist_ok=True)
    odir = os.path.join(SCRATCH, "nuitka_" + name)
    r, t_build = timed_sh(
        [NUITKA, "--quiet", "--no-progressbar", "--assume-yes-for-downloads",
         "--output-dir=" + odir, "--output-filename=" + name + "_nuitka",
         name + ".py"], timeout=900)
    built = os.path.join(odir, name + "_nuitka")
    if r.returncode != 0 or not os.path.exists(built):
        print("  [nuitka] build failed:\n%s" % r.stderr[-800:])
        return None
    out = os.path.join(BIN, name + "_nuitka")
    shutil.copy2(built, out)
    return out, t_build


# --------------------------------------------------------------------------
# Backend command tables. A backend is (label, command-prefix, build_seconds)
# where the workload size N is appended at run time. Compiled backends are built
# first; interpreted backends have no build step (build_seconds is None).

def backends_for(name, arg):
    cmds = []
    if os.path.exists(MICROPYTHON):
        # Bump the GC heap so the float-array benchmarks (which box every list
        # element) do not exhaust MicroPython's small default heap.
        cmds.append(("micropython",
                     [MICROPYTHON, "-X", "heapsize=512M", name + ".py"], None))
    else:
        print("  [micropython] %s not built; skipping" % MICROPYTHON)
    if shutil.which("pypy3"):
        cmds.append(("pypy", ["pypy3", name + ".py"], None))
    for label, builder in (("nuitka", build_nuitka),
                           ("codon", build_codon),
                           ("rpython", build_rpython),
                           ("shivycx", build_shivycx)):
        built = builder(name)
        if built:
            path, build_s = built
            cmds.append((label, [path], build_s))
    return [(label, cmd + [str(arg)], build_s) for label, cmd, build_s in cmds]


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
    ap.add_argument("--no-plot", action="store_true",
                    help="skip generating the matplotlib graphs")
    ap.add_argument("--plot-dir", default=os.path.join(HERE, "plots"),
                    help="directory for the generated PNG graphs")
    ap.add_argument("--replot", action="store_true",
                    help="skip benchmarking; redraw graphs from the saved "
                         "plots/results.json")
    args = ap.parse_args()

    if args.replot:
        with open(os.path.join(args.plot_dir, "results.json")) as f:
            make_plots(json.load(f), args.plot_dir)
        return

    os.makedirs(BIN, exist_ok=True)
    os.makedirs(SCRATCH, exist_ok=True)
    sizes = {name: getattr(args, name) for name in BENCHMARKS}

    # name -> list of dicts {backend, run, build, output, ok}
    all_results = {}

    for name in BENCHMARKS:
        arg = sizes[name]
        print("\n=== %s  (N=%d) ===" % (name, arg))
        rows = backends_for(name, arg)
        if args.build_only:
            continue
        ref = reference_output(name, arg)
        results = []
        for label, cmd, build_s in rows:
            best, out = time_cmd(cmd, args.repeats)
            ok = outputs_match(out, ref)
            # A ShivyC backend that builds but computes the wrong answer is a
            # silent miscompile: warn and drop it rather than report it.
            if not ok and label in SHIVYC_BACKENDS:
                warn_shivyc(label, "produced wrong output %r (expected %r)"
                            % (out, ref))
                continue
            results.append({"backend": label, "run": best, "build": build_s,
                            "output": out, "ok": ok})
        baseline = next((r["run"] for r in results
                         if r["backend"] == "micropython"), None)
        results.sort(key=lambda r: r["run"])
        all_results[name] = {"arg": arg, "results": results}
        print("  %-12s %12s %12s %10s   %s"
              % ("backend", "run (s)", "compile (s)", "speedup", "output"))
        for r in results:
            sp = ("%6.1fx" % (baseline / r["run"])) if baseline else "   -  "
            flag = "" if r["ok"] else "  (!= reference)"
            comp = "-" if r["build"] is None else "%.3f" % r["build"]
            print("  %-12s %12.4f %12s %10s   %s%s"
                  % (r["backend"], r["run"], comp, sp, r["output"], flag))

    if not args.build_only and not args.no_plot:
        make_plots(all_results, args.plot_dir)


# --------------------------------------------------------------------------
# Plotting: one grouped-bar graph per benchmark (run time vs compile time per
# backend, log scale), plus a combined run-time speedup overview.

# Stable color per backend so graphs are comparable across benchmarks.
BACKEND_COLORS = {
    "micropython": "#888888",
    "pypy": "#4c72b0",
    "nuitka": "#dd8452",
    "codon": "#55a868",
    "rpython": "#c44e52",
    "shivycx": "#8172b3",
}


def make_plots(all_results, plot_dir):
    try:
        # Keep matplotlib's font cache in writable scratch (sandbox-friendly).
        os.environ.setdefault("MPLCONFIGDIR", os.path.join(SCRATCH, "mpl"))
        os.makedirs(os.environ["MPLCONFIGDIR"], exist_ok=True)
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        import numpy as np
    except Exception as e:  # noqa: BLE001
        print("\n[plot] matplotlib/numpy unavailable (%s); skipping graphs" % e)
        return

    os.makedirs(plot_dir, exist_ok=True)
    # Persist the raw data so graphs can be redrawn with --replot (no re-run).
    with open(os.path.join(plot_dir, "results.json"), "w") as f:
        json.dump(all_results, f, indent=2)
    written = []
    for name, data in all_results.items():
        results = data["results"]
        if not results:
            continue
        labels = [r["backend"] for r in results]
        runs = [r["run"] for r in results]
        builds = [(r["build"] or 0.0) for r in results]
        colors = [BACKEND_COLORS.get(l, "#333333") for l in labels]

        x = np.arange(len(labels))
        width = 0.38
        fig, ax = plt.subplots(figsize=(max(6, 1.2 * len(labels)), 4.5))
        run_bars = ax.bar(x - width / 2, runs, width, color=colors,
                          label="run time")
        # Compile-time bars are hatched and lighter; interpreted backends show 0.
        comp_bars = ax.bar(x + width / 2, builds, width, color=colors,
                           alpha=0.45, hatch="//", label="compile time")

        ax.set_yscale("log")
        ax.set_ylabel("seconds (log scale)")
        ax.set_title("%s  (N=%d)" % (name, data["arg"]))
        ax.set_xticks(x)
        ax.set_xticklabels(labels, rotation=20, ha="right")
        ax.grid(axis="y", which="both", linestyle=":", alpha=0.4)

        for bars, vals in ((run_bars, runs), (comp_bars, builds)):
            for rect, v in zip(bars, vals):
                if v <= 0:
                    continue
                ax.annotate("%.3g" % v,
                            (rect.get_x() + rect.get_width() / 2,
                             rect.get_height()),
                            textcoords="offset points", xytext=(0, 2),
                            ha="center", va="bottom", fontsize=7)

        # Legend that explains the two bar styles (color encodes backend).
        from matplotlib.patches import Patch
        # Results are sorted fastest-first, so the tallest bar is on the right;
        # put the legend upper-left over the shortest bars to avoid overlap.
        ax.legend(handles=[Patch(facecolor="#666", label="run time"),
                           Patch(facecolor="#666", alpha=0.45, hatch="//",
                                 label="compile time")],
                  fontsize=8, loc="upper left")
        ax.margins(y=0.18)
        fig.tight_layout()
        path = os.path.join(plot_dir, name + ".png")
        fig.savefig(path, dpi=120)
        plt.close(fig)
        written.append(path)

    _plot_overview(all_results, plot_dir)
    print("\n[plot] wrote %d graph(s) to %s" % (len(written) + 1, plot_dir))


def _plot_overview(all_results, plot_dir):
    """One combined chart: speedup vs micropython for every benchmark/backend."""
    import matplotlib.pyplot as plt
    import numpy as np

    names = [n for n, d in all_results.items() if d["results"]]
    backends = [b for b in BACKEND_COLORS]
    x = np.arange(len(names))
    nb = len(backends)
    width = 0.8 / max(1, nb)
    fig, ax = plt.subplots(figsize=(max(8, 1.6 * len(names)), 5))
    for bi, backend in enumerate(backends):
        speeds = []
        for n in names:
            res = {r["backend"]: r for r in all_results[n]["results"]}
            base = res.get("micropython")
            this = res.get(backend)
            if base and this and this["run"] > 0:
                speeds.append(base["run"] / this["run"])
            else:
                speeds.append(0.0)
        ax.bar(x + (bi - nb / 2) * width + width / 2, speeds, width,
               label=backend, color=BACKEND_COLORS[backend])
    ax.set_yscale("log")
    ax.set_ylabel("speedup vs micropython (log scale)")
    ax.set_title("Run-time speedup by backend (higher is better)")
    ax.set_xticks(x)
    ax.set_xticklabels(names, rotation=30, ha="right")
    ax.grid(axis="y", which="both", linestyle=":", alpha=0.4)
    ax.legend(fontsize=8, ncol=3)
    fig.tight_layout()
    fig.savefig(os.path.join(plot_dir, "_overview.png"), dpi=120)
    plt.close(fig)


if __name__ == "__main__":
    main()
