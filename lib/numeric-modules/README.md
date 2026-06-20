# Numeric modules (ulab + scipy + torch_c + matplotlib)

This directory bundles **micropython-ulab** (NumPy-like arrays + embedded SciPy subset) with **C-ML** (`import cml` / `import torch`, torch_c-compatible API) and a frozen **matplotlib** pyplot subset for a single `USER_C_MODULES` path plus training plots.

Git submodules (fetched automatically by the build script):

- `micropython-ulab` — https://github.com/OpenSourceJesus/micropython-ulab
- `lib/C-ML` — https://github.com/OpenSourceJesus/C-ML
- `scipy` — https://github.com/OpenSourceJesus/scipy (upstream reference; runtime SciPy is `from ulab import scipy`)
- `matplotlib` — https://github.com/OpenSourceJesus/matplotlib (upstream reference; runtime plotting is `import matplotlib.pyplot`)

Symlinks expose both build trees under one `USER_C_MODULES` root (required by the Make build):

```
ulab/ -> ../../micropython-ulab/code
cml/  -> ../cml-micropython/cml
```

## Modules at runtime

| Import | Role |
|--------|------|
| `from ulab import numpy` | NumPy-like `ndarray` |
| `from ulab import scipy` | SciPy subset (integrate, linalg, optimize, signal, special) |
| `import cml` / `import torch` | torch_c tensors, autograd, nn |
| `import matplotlib.pyplot` | SVG line plots (see `lib/matplotlib-micropython`) |

The top-level `scipy/` directory in the repo root is **upstream CPython SciPy** (reference only). On MicroPython, use `from ulab import scipy`.

The top-level `matplotlib/` directory is **upstream CPython Matplotlib** (reference only). On MicroPython, use `import matplotlib.pyplot as plt` from the frozen package in `lib/matplotlib-micropython`.

## Build (unix)

**Recommended** — run from anywhere; the script resolves paths from the repo root:

```bash
/path/to/micropython/build-ml.sh
```

Manual equivalent (note: `cd ../../ports/unix`, not `cd ports/unix`, if you are still inside `lib/numeric-modules`):

```bash
REPO=/path/to/micropython
ln -sfn ../../micropython-ulab/code "$REPO/lib/numeric-modules/ulab"
ln -sfn ../cml-micropython/cml "$REPO/lib/numeric-modules/cml"
make -C "$REPO/ports/unix" VARIANT=ml submodules
make -C "$REPO/ports/unix" VARIANT=ml
"$REPO/ports/unix/build-ml/micropython" "$REPO/tests/ml/integration.py"
"$REPO/ports/unix/build-ml/micropython" "$REPO/tests/ml/matplotlib_integration.py"
```

Individual stacks:

```bash
make -C /path/to/micropython/ports/unix VARIANT=cml
make -C /path/to/micropython/ports/unix USER_C_MODULES=../../micropython-ulab
```

## CMake ports (rp2, esp32, …)

```bash
make USER_C_MODULES=../../lib/numeric-modules/micropython.cmake
```

Requires `MICROPY_PY_FFI=1` when using the cml FFI shims (as in the `ml` unix variant).

## ulab ↔ torch_c interop

After `cml.init()`, `cml.tensor()` accepts ulab ndarrays directly (via the buffer protocol):

```python
import cml
from ulab import numpy as np
from ulab import scipy

cml.init()
x = np.array([[1.0, 2.0], [3.0, 4.0]])
t = cml.tensor(x)          # ndarray → Tensor (float32)
y = np.array(t.tolist())   # Tensor → ndarray
z = scipy.special.erf(np.array([1.0, 2.0]))
cml.cleanup()
```

C-ML defaults to float32; ulab follows the port float width (often float64 on unix). Values are converted when importing ndarrays.
