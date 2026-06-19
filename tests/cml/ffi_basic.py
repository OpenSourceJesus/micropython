# Test MicroPython ffi module and cffi compatibility shim.

try:
    import ffi
except ImportError:
    print("SKIP: ffi module not enabled")
    raise SystemExit

# libc smoke test via ffilib
import ffilib

libc = ffilib.libc()
getpid = libc.func("i", "getpid", "")
pid = getpid()
print("getpid:", pid)
assert pid > 0

# Raw ffi: resolve symbols from the main executable (requires -export-dynamic on Linux)
mod = ffi.open(None)
cml_is_initialized = mod.func("i", "cml_is_initialized", "")
print("cml_is_initialized:", cml_is_initialized())

# cffi shim
from cffi import FFI

ffi_obj = FFI()
ffi_obj.dlopen((None,))
ffi_obj.cdef(
    """
    int cml_is_initialized(void);
    int cml_get_init_count(void);
    """
)
print("init count:", ffi_obj.lib.cml_get_init_count())

# cml_ffi convenience bindings
from cml_ffi import lib as cml_lib

print("cuda available:", cml_lib.torch_cuda_is_available())

# Native cml module handles init safely
import cml

cml.init()
assert cml_lib.cml_is_initialized() != 0
cml.cleanup()

print("ffi ok")
