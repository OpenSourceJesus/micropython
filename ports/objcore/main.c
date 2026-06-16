#include <stdio.h>

#include "py/builtin.h"
#include "py/gc.h"
#include "py/lexer.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/runtime.h"
#include "script.h"

static char *stack_top;
static char heap[MICROPY_HEAP_SIZE];

int main(void) {
    int stack_dummy;
    stack_top = (char *)&stack_dummy;

    gc_init(heap, heap + sizeof(heap));
    mp_init();
    script_run();
    mp_deinit();
    return 0;
}

void gc_collect(void) {
    void *dummy;
    gc_collect_start();
    gc_collect_root(&dummy, ((mp_uint_t)stack_top - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
    gc_collect_end();
}

mp_lexer_t *mp_lexer_new_from_file(qstr filename) {
    (void)filename;
    mp_raise_OSError(MP_ENOENT);
}

mp_import_stat_t mp_import_stat(const char *path) {
    (void)path;
    return MP_IMPORT_STAT_NO_EXIST;
}

void nlr_jump_fail(void *val) {
    (void)val;
    for (;;) {
    }
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    fprintf(stderr, "Assertion '%s' failed at %s:%d in %s\n", expr, file, line, func);
    for (;;) {
    }
}
#endif
