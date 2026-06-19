# C-ML bindings via MicroPython ffi (statically linked symbols in the cml binary).
#
# Usage:
#   from cml_ffi import ffi, lib
#   lib.cml_is_initialized()
#
# For lifecycle (init/cleanup) and tensor APIs use the native `cml` module;
# some C-ML entry points are not safe to invoke through libffi on all platforms.

from cffi import FFI

_ffi = FFI()
_ffi.dlopen((None,))
_lib = _ffi.lib

# Read-only / lightweight queries (safe via libffi)
_lib._bind("cml_is_initialized", "i", "")
_lib._bind("cml_get_init_count", "i", "")
_lib._bind("torch_cuda_is_available", "i", "")

ffi = _ffi
lib = _lib
