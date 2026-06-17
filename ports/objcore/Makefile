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

.PHONY: all run translate install test test-transpile test-stdlib test-stdlib-compile translate-stdlib libstdlib

translate: $(SCRIPT_C)

test: test-transpile test-stdlib test-stdlib-compile

test-transpile:
	$(Q)python3 -m unittest discover -s tests -p 'test_*.py' -q

test-stdlib:
	$(Q)python3 -m unittest tests.test_stdlib_translate -v

test-stdlib-compile:
	$(Q)python3 -m unittest tests.test_stdlib_compile -v

translate-stdlib:
	$(Q)python3 translate_stdlib.py

# Shared library: transpiled python-stdlib C sources (library modules only) +
# MicroPython bridge/core.  Test and example modules are transpiled but omitted
# from the .so — they are covered by tests/test_stdlib_compile.py instead.
STDLIB_DIR = stdlib_translated
STDLIB_SO = $(BUILD)/libmpstdlib.so
STDLIB_SO_DIR = $(BUILD)/stdlib_so
STDLIB_SO_CFLAGS = -std=c99 -fPIC -Os -DNDEBUG -Wall \
	-I. -I$(TOP) -I$(BUILD) -I$(BUILD)/genhdr -I$(STDLIB_DIR)
STDLIB_SRC_ALL = $(wildcard $(STDLIB_DIR)/*.c)
STDLIB_SRC = $(filter-out $(STDLIB_DIR)/mp_stdlib_bridge.c,$(STDLIB_SRC_ALL))
STDLIB_SRC := $(foreach f,$(STDLIB_SRC),$(if $(findstring _test_,$(notdir $(f))),,$(if $(findstring _tests,$(notdir $(f))),,$(if $(findstring _example_,$(notdir $(f))),,$(if $(findstring unittest_,$(notdir $(f))),,$(if $(findstring tarfile_write_,$(notdir $(f))),,$(if $(findstring fnmatch_fnmatch,$(notdir $(f))),,$(if $(findstring uu_uu,$(notdir $(f))),,$f))))))))
STDLIB_OBJ = $(addprefix $(STDLIB_SO_DIR)/,$(notdir $(STDLIB_SRC:.c=.o)))
STDLIB_PY_O = $(patsubst $(BUILD)/py/%,$(STDLIB_SO_DIR)/py/%,$(PY_CORE_O))

libstdlib: $(STDLIB_SO)

$(STDLIB_SO_DIR)/py/%.o: $(TOP)/py/%.c $(BUILD)/genhdr/moduledefs.h $(BUILD)/genhdr/mpversion.h $(BUILD)/genhdr/qstrdefs.generated.h
	$(ECHO) "CC $< (PIC)"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) -fPIC $(INC) -I$(BUILD) -I$(BUILD)/genhdr -DNDEBUG -c $< -o $@

$(STDLIB_SO_DIR)/mp_stdlib_bridge.o: $(STDLIB_DIR)/mp_stdlib_bridge.c mpconfigstdlib.h | $(BUILD)/genhdr/qstrdefs.generated.h
	$(ECHO) "CC $<"
	$(Q)mkdir -p $(STDLIB_SO_DIR)
	$(Q)$(CC) $(STDLIB_SO_CFLAGS) -c $< -o $@

$(STDLIB_SO_DIR)/%.o: $(STDLIB_DIR)/%.c | $(BUILD)/genhdr/qstrdefs.generated.h
	$(ECHO) "CC $<"
	$(Q)mkdir -p $(STDLIB_SO_DIR)
	$(Q)$(CC) $(STDLIB_SO_CFLAGS) -c $< -o $@

$(STDLIB_SO): $(STDLIB_OBJ) $(STDLIB_SO_DIR)/mp_stdlib_bridge.o $(STDLIB_PY_O) | $(BUILD)/genhdr/qstrdefs.generated.h
	$(ECHO) "LINK $@"
	$(Q)$(CC) -shared -o $@ $(STDLIB_OBJ) $(STDLIB_SO_DIR)/mp_stdlib_bridge.o $(STDLIB_PY_O) -lm

install: $(BUILD)/objcore
	$(Q)mkdir -p $(TOP)/build
	$(Q)cp $(BUILD)/objcore $(TOP)/build/objcore

run: $(BUILD)/objcore
	$(BUILD)/objcore

include $(TOP)/py/mkrules.mk
