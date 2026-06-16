#include <stdint.h>

// Object-model-only Linux port: no Python source compiler, REPL, or bytecode VM.

#define MICROPY_CONFIG_ROM_LEVEL            (MICROPY_CONFIG_ROM_LEVEL_MINIMUM)

#define MICROPY_ENABLE_COMPILER             (0)
#define MICROPY_HELPER_REPL                 (0)
#define MICROPY_ENABLE_EXTERNAL_IMPORT      (0)
#define MICROPY_MODULE_FROZEN_MPY           (0)
#define MICROPY_PERSISTENT_CODE_LOAD        (0)
#define MICROPY_ENABLE_GC                   (1)

// No sys module or import path.
#define MICROPY_PY_SYS                      (0)
#define MICROPY_PY_SYS_PATH_ARGV_DEFAULTS     (0)

// Keep core concrete types usable from C.
#define MICROPY_PY_BUILTINS                 (1)

#define MICROPY_MIN_USE_STDOUT              (1)
#define MICROPY_HEAP_SIZE                   (32768)

typedef long mp_off_t;

#include <alloca.h>

#define MICROPY_HW_BOARD_NAME "objcore"
#define MICROPY_HW_MCU_NAME "linux-x86_64"

#define MP_STATE_PORT MP_STATE_VM
