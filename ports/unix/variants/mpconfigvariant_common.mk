# Interpreter toggle for all unix variants.
#   make                         # micropython + mpython executables (default)
#   make INTERPRETER=0           # object-model only: libmicropython.a, no CLI
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
# Object-model only: build a static library, not an executable.
PROG :=
all: $(BUILD)/libmicropython.a
endif
