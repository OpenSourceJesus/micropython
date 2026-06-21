# This is the default variant when you `make` the Unix port.

FROZEN_MANIFEST ?= $(VARIANT_DIR)/manifest.py
include $(VARIANT_DIR)/../mpconfigvariant_common.mk
