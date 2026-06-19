# C-ML for MicroPython

This directory wraps [C-ML](https://github.com/OpenSourceJesus/C-ML) `torch_c.h` as a MicroPython user C module with full API coverage.

## Build

```bash
cd ports/unix
make VARIANT=cml
./build-cml/micropython ../../tests/cml/basic.py
```

The build compiles a minimal CPU-only C-ML static library from `lib/C-ML` (git submodule), falling back to `~/C-ML` if absent.

OpenMP is disabled at runtime (`MKL_THREADING_LAYER=SEQUENTIAL`, single-thread BLAS) for embedded stability.

```bash
make VARIANT=cml CML_DIR=/path/to/C-ML
```

## Modules

Both `import cml` and `import torch` expose the same module (torch_c-compatible names included).

## API overview

### Lifecycle
`init()`, `cleanup()`, `version()`, `manual_seed()`

### Native types
- `Tensor` — wraps `Tensor*` with properties `.shape`, `.ndim`, `.dtype`, `.device` and methods `.tolist()`, `.item()`, `.realize()`, `.free()`, etc.
- `Module` — `.forward()`, `.train()`, `.eval()`, `.state_dict()`, `.load_state_dict()`, `.zero_grad()`
- `Optimizer` — `.step()`, `.zero_grad()`; factories `optim_adam()`, `optim_sgd()`
- `StateDict`, `RuntimeModule`

### Tensor factories
`empty`, `zeros`, `ones`, `rand`, `randn`, `full`, `eye`, `arange`, `linspace`, `tensor`, `*_like`

Optional second argument: options as `dict(dtype=..., device=..., requires_grad=...)` or tuple `(dtype, device, requires_grad)`.

### Tensor ops
`add`, `sub`, `mul`, `div`, `matmul`, `pow`, `sum`, `mean`, `max`, `min`, `relu`, `sigmoid`, `tanh`, `gelu`, `softmax`, `reshape`, `transpose`, `squeeze`, `unsqueeze`, `cat`, `stack`, `clone`, `detach`, `contiguous`, `linear`, `linear_relu`

### Autograd / eager
`backward`, `zero_grad`, `no_grad`, `enable_grad`, `is_grad_enabled`, `set_eager_mode`, `is_eager_mode`, `inference_mode`, `set_num_threads`, `get_num_threads`, `reset_ir`, `reset_ir_soft`

### nn
`nn_linear`, `nn_relu`, `nn_sequential`, `mse_loss`, `cross_entropy_loss`

### Runtime (AOT/PTE)
`runtime_from_module`, `runtime_load_aot`, `runtime_load_pte`, `runtime_export_pte`

### torch_c aliases
Every function is also available as `torch_<name>` (e.g. `torch_zeros`, `torch_matmul`, `torch_nn_linear`).

### Constants
`DTYPE_FLOAT32`, `DEVICE_CPU`, `TORCH_RUNTIME_EAGER`, etc.

## Example

```python
import cml

cml.init()
a = cml.zeros((2, 3))
b = cml.ones((3, 2))
c = cml.matmul(a, b)
print(c.shape, c.tolist())

linear = cml.nn_linear(4, 2)
out = linear.forward(cml.randn((8, 4)))
cml.cleanup()
```

## CMake ports

```bash
make USER_C_MODULES=../../lib/cml-micropython/micropython.cmake
```
