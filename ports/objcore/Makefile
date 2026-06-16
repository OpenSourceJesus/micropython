include ../../py/mkenv.mk

QSTR_DEFS = qstrdefsport.h

MICROPY_ROM_TEXT_COMPRESSION = 0

include $(TOP)/py/py.mk

SCRIPT ?= hello.py
SCRIPT_SRC = script.c
SCRIPT_C = $(BUILD)/$(SCRIPT_SRC)

INC += -I. -I$(TOP) -I$(BUILD)
LD = $(CC)

CFLAGS += $(INC) -Wall -Werror -std=c99 -Os -DNDEBUG
CFLAGS += -fdata-sections -ffunction-sections -fno-pie
LDFLAGS += -Wl,-Map=$@.map,--cref -Wl,--gc-sections -no-pie

SRC_C = \
	main.c \
	hal.c \
	$(SCRIPT_SRC) \
	shared/libc/printf.c \

OBJ += $(PY_CORE_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ_EXTRA_ORDER_DEPS += $(SCRIPT_C)

all: $(BUILD)/objcore

$(SCRIPT_C): $(SCRIPT) ast2objcore.py
	$(ECHO) "GEN $(SCRIPT_C)"
	$(Q)python3 ast2objcore.py $(SCRIPT) -o $(SCRIPT_C)

$(BUILD)/objcore: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $^
	$(Q)$(SIZE) $@

.PHONY: all run translate install test test-transpile test-stdlib translate-stdlib

translate: $(SCRIPT_C)

test: test-transpile test-stdlib

test-transpile:
	$(Q)python3 -m unittest discover -s tests -p 'test_*.py' -q

test-stdlib:
	$(Q)python3 -m unittest tests.test_stdlib_translate -v

translate-stdlib:
	$(Q)python3 translate_stdlib.py

install: $(BUILD)/objcore
	$(Q)mkdir -p $(TOP)/build
	$(Q)cp $(BUILD)/objcore $(TOP)/build/objcore

run: $(BUILD)/objcore
	$(BUILD)/objcore

include $(TOP)/py/mkrules.mk
