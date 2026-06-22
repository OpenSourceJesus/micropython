/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2018 Paul Sokolovsky
 * Copyright (c) 2017-2022 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"

#if MICROPY_VFS
#include "extmod/vfs.h"
#endif

static mp_obj_t mp_os_getenv(size_t n_args, const mp_obj_t *args) {
    const char *s = getenv(mp_obj_str_get_str(args[0]));
    if (s == NULL) {
        if (n_args == 2) {
            return args[1];
        }
        return mp_const_none;
    }
    return mp_obj_new_str_from_cstr(s);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_os_getenv_obj, 1, 2, mp_os_getenv);

static mp_obj_t mp_os_putenv(mp_obj_t key_in, mp_obj_t value_in) {
    const char *key = mp_obj_str_get_str(key_in);
    const char *value = mp_obj_str_get_str(value_in);
    int ret;

    #if _WIN32
    ret = _putenv_s(key, value);
    #else
    ret = setenv(key, value, 1);
    #endif

    if (ret == -1) {
        mp_raise_OSError(errno);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_os_putenv_obj, mp_os_putenv);

static mp_obj_t mp_os_unsetenv(mp_obj_t key_in) {
    const char *key = mp_obj_str_get_str(key_in);
    int ret;

    #if _WIN32
    ret = _putenv_s(key, "");
    #else
    ret = unsetenv(key);
    #endif

    if (ret == -1) {
        mp_raise_OSError(errno);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_os_unsetenv_obj, mp_os_unsetenv);

static mp_obj_t mp_os_environ_get(size_t n_args, const mp_obj_t *args) {
    if (n_args > 1 && !mp_obj_is_str(args[0])) {
        args++;
        n_args--;
    }
    return mp_os_getenv(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_os_environ_get_obj, 1, 3, mp_os_environ_get);

static mp_obj_t mp_os_environ_pop(size_t n_args, const mp_obj_t *args) {
    if (n_args > 1 && !mp_obj_is_str(args[0])) {
        args++;
        n_args--;
    }
    const char *key = mp_obj_str_get_str(args[0]);
    const char *s = getenv(key);
    if (s == NULL) {
        if (n_args > 1) {
            return args[1];
        }
        mp_raise_msg_varg(&mp_type_KeyError, MP_ERROR_TEXT("%s"), key);
    }
    mp_obj_t old = mp_obj_new_str_from_cstr(s);
    mp_os_unsetenv(args[0]);
    return old;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_os_environ_pop_obj, 1, 3, mp_os_environ_pop);

static const mp_rom_map_elem_t mp_os_environ_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get), MP_ROM_PTR(&mp_os_environ_get_obj) },
    { MP_ROM_QSTR(MP_QSTR_pop), MP_ROM_PTR(&mp_os_environ_pop_obj) },
};
static MP_DEFINE_CONST_DICT(mp_os_environ_locals_dict, mp_os_environ_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_os_environ,
    MP_QSTR_environ,
    MP_TYPE_FLAG_NONE,
    locals_dict, &mp_os_environ_locals_dict
    );

static const mp_obj_base_t mp_os_environ_obj = { (mp_obj_type_t *)&mp_type_os_environ };

#if MICROPY_VFS
#define MICROPY_PY_OS_UNIX_EXTRA_GLOBALS \
    { MP_ROM_QSTR(MP_QSTR_makedirs), MP_ROM_PTR(&mp_os_makedirs_obj) },
#else
#define MICROPY_PY_OS_UNIX_EXTRA_GLOBALS
#endif

#define MICROPY_PY_OS_EXTRA_GLOBALS \
    { MP_ROM_QSTR(MP_QSTR_environ), MP_ROM_PTR(&mp_os_environ_obj) }, \
    MICROPY_PY_OS_UNIX_EXTRA_GLOBALS

#if MICROPY_VFS
static void mp_os_makedirs_one(const char *path, bool exist_ok) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_vfs_mkdir(mp_obj_new_str_from_cstr(path));
        nlr_pop();
    } else {
        if (!exist_ok) {
            nlr_raise(nlr.ret_val);
        }
    }
}

static mp_obj_t mp_os_makedirs(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_path, ARG_exist_ok };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_path, MP_ARG_REQUIRED | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_exist_ok, MP_ARG_KW_ONLY | MP_ARG_BOOL, { .u_bool = false } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    const char *path = mp_obj_str_get_str(args[ARG_path].u_obj);
    bool exist_ok = args[ARG_exist_ok].u_bool;
    size_t len = strlen(path);
    if (len == 0) {
        return mp_const_none;
    }

    char buf[256];
    if (len >= sizeof(buf)) {
        mp_raise_OSError(ENAMETOOLONG);
    }
    memcpy(buf, path, len + 1);

    for (char *p = buf + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mp_os_makedirs_one(buf, exist_ok);
            *p = '/';
        }
    }
    mp_os_makedirs_one(buf, exist_ok);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(mp_os_makedirs_obj, 1, mp_os_makedirs);
#endif

static mp_obj_t mp_os_system(mp_obj_t cmd_in) {
    const char *cmd = mp_obj_str_get_str(cmd_in);

    MP_THREAD_GIL_EXIT();
    int r = system(cmd);
    MP_THREAD_GIL_ENTER();

    RAISE_ERRNO(r, errno);

    return MP_OBJ_NEW_SMALL_INT(r);
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_os_system_obj, mp_os_system);

static mp_obj_t mp_os_errno(size_t n_args, const mp_obj_t *args) {
    if (n_args == 0) {
        return MP_OBJ_NEW_SMALL_INT(errno);
    }

    errno = mp_obj_get_int(args[0]);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_os_errno_obj, 0, 1, mp_os_errno);
