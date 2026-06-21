FROZEN_MANIFEST ?= $(VARIANT_DIR)/manifest.py
include $(VARIANT_DIR)/../mpconfigvariant_common.mk
USER_C_MODULES = $(TOP)/lib/cml-micropython

GIT_SUBMODULES += lib/C-ML

# ffi module (libffi) for dynamic C bindings alongside the native cml module
MICROPY_PY_FFI = 1
