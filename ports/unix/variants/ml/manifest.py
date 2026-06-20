include("$(PORT_DIR)/variants/manifest.py")

include("$(MPY_DIR)/extmod/asyncio")

# cffi compatibility shim and C-ML ffi bindings (require MICROPY_PY_FFI=1)
package("cffi", base_path="$(MPY_DIR)/lib/cml-micropython")
package("ctypes", base_path="$(MPY_DIR)/lib/cml-micropython")
package("cml_ffi", base_path="$(MPY_DIR)/lib/cml-micropython")
require("ffilib")
