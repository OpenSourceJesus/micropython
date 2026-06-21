# Interpreter toggle for variants that include mpconfigvariant_common.h.
#   make                         # interpreter enabled (default)
#   make INTERPRETER=0           # object-model only, no Python VM/REPL
#   make INTERPRETER=1           # explicit enable
INTERPRETER ?= 1

ifeq ($(filter 0 1,$(INTERPRETER)),)
$(error INTERPRETER must be 0 or 1 (got '$(INTERPRETER)'))
endif

CFLAGS += -DMICROPY_ENABLE_INTERPRETER=$(INTERPRETER)

ifneq ($(INTERPRETER),1)
# Separate build tree so toggling does not reuse stale .o files.
BUILD := $(BUILD)-nointerp
# Frozen .mpy modules require the bytecode VM.
FROZEN_MANIFEST :=
endif
