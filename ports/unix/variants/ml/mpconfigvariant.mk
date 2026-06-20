FROZEN_MANIFEST ?= $(VARIANT_DIR)/manifest.py
USER_C_MODULES = $(TOP)/lib/numeric-modules

GIT_SUBMODULES += micropython-ulab lib/C-ML scipy

# ffi module (libffi) for dynamic C bindings alongside the native cml module
MICROPY_PY_FFI = 1
