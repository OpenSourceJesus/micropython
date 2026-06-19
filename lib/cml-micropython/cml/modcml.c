#include "modcml.h"

#include <stdlib.h>

static void cml_disable_openmp(void) {
    // Embedded MicroPython: avoid MKL/OpenMP thread pools (segfault in nn_sequential).
    setenv("MKL_THREADING_LAYER", "SEQUENTIAL", 1);
    setenv("MKL_DYNAMIC", "FALSE", 1);
    setenv("OMP_NUM_THREADS", "1", 1);
    setenv("MKL_NUM_THREADS", "1", 1);
    setenv("OPENBLAS_NUM_THREADS", "1", 1);
    setenv("BLIS_NUM_THREADS", "1", 1);
}

static mp_obj_t cml_init_fn(void) {
    if (!cml_ready) {
        setenv("CUDA_VISIBLE_DEVICES", "", 1);
        cml_disable_openmp();
        if (torch_init() != 0) {
            mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("torch_init failed"));
        }
        torch_set_num_threads(1);
        torch_set_default_device(DEVICE_CPU);
        torch_set_default_dtype(DTYPE_FLOAT32);
        torch_inference_mode(true);
        cml_ready = true;
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(cml_init_fn_obj, cml_init_fn);

static mp_obj_t cml_cleanup_fn(void) {
    if (cml_ready) {
        torch_cleanup();
        cml_ready = false;
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(cml_cleanup_fn_obj, cml_cleanup_fn);

static mp_obj_t cml_version_fn(void) {
    int major = 0;
    int minor = 0;
    int patch = 0;
    const char *version_string = NULL;
    torch_get_version(&major, &minor, &patch, &version_string);
    if (version_string == NULL) {
        version_string = "unknown";
    }
    return mp_obj_new_str(version_string, strlen(version_string));
}
MP_DEFINE_CONST_FUN_OBJ_0(cml_version_fn_obj, cml_version_fn);

static mp_obj_t cml_manual_seed_fn(mp_obj_t seed_obj) {
    torch_manual_seed((uint64_t)mp_obj_get_int(seed_obj));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_manual_seed_fn_obj, cml_manual_seed_fn);

static mp_obj_t cml_cuda_is_available_fn(void) {
    return mp_obj_new_bool(torch_cuda_is_available());
}
MP_DEFINE_CONST_FUN_OBJ_0(cml_cuda_is_available_fn_obj, cml_cuda_is_available_fn);

static mp_obj_t cml_cuda_device_count_fn(void) {
    return mp_obj_new_int(torch_cuda_device_count());
}
MP_DEFINE_CONST_FUN_OBJ_0(cml_cuda_device_count_fn_obj, cml_cuda_device_count_fn);

static mp_obj_t cml_get_default_device_fn(void) {
    return cml_device_to_str(torch_get_default_device());
}
MP_DEFINE_CONST_FUN_OBJ_0(cml_get_default_device_fn_obj, cml_get_default_device_fn);

static mp_obj_t cml_set_default_device_fn(mp_obj_t device) {
    torch_set_default_device(cml_parse_device(device));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_set_default_device_fn_obj, cml_set_default_device_fn);

static mp_obj_t cml_get_default_dtype_fn(void) {
    return cml_dtype_to_str(torch_get_default_dtype());
}
MP_DEFINE_CONST_FUN_OBJ_0(cml_get_default_dtype_fn_obj, cml_get_default_dtype_fn);

static mp_obj_t cml_set_default_dtype_fn(mp_obj_t dtype) {
    torch_set_default_dtype(cml_parse_dtype(dtype));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_set_default_dtype_fn_obj, cml_set_default_dtype_fn);

static mp_obj_t cml_get_last_error_fn(void) {
    const char *msg = torch_get_last_error();
    if (msg == NULL) {
        return mp_const_none;
    }
    return mp_obj_new_str(msg, strlen(msg));
}
MP_DEFINE_CONST_FUN_OBJ_0(cml_get_last_error_fn_obj, cml_get_last_error_fn);

static mp_obj_t cml_get_last_error_code_fn(void) {
    return mp_obj_new_int(torch_get_last_error_code());
}
MP_DEFINE_CONST_FUN_OBJ_0(cml_get_last_error_code_fn_obj, cml_get_last_error_code_fn);

static mp_obj_t cml_has_error_fn(void) {
    return mp_obj_new_bool(torch_has_error());
}
MP_DEFINE_CONST_FUN_OBJ_0(cml_has_error_fn_obj, cml_has_error_fn);

#include "modcml_decls.inc"

static const mp_rom_map_elem_t cml_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_cml) },

    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&cml_init_fn_obj) },
    { MP_ROM_QSTR(MP_QSTR_cleanup), MP_ROM_PTR(&cml_cleanup_fn_obj) },
    { MP_ROM_QSTR(MP_QSTR_version), MP_ROM_PTR(&cml_version_fn_obj) },
    { MP_ROM_QSTR(MP_QSTR_manual_seed), MP_ROM_PTR(&cml_manual_seed_fn_obj) },
    { MP_ROM_QSTR(MP_QSTR_cuda_is_available), MP_ROM_PTR(&cml_cuda_is_available_fn_obj) },
    { MP_ROM_QSTR(MP_QSTR_cuda_device_count), MP_ROM_PTR(&cml_cuda_device_count_fn_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_default_device), MP_ROM_PTR(&cml_get_default_device_fn_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_default_device), MP_ROM_PTR(&cml_set_default_device_fn_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_default_dtype), MP_ROM_PTR(&cml_get_default_dtype_fn_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_default_dtype), MP_ROM_PTR(&cml_set_default_dtype_fn_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_last_error), MP_ROM_PTR(&cml_get_last_error_fn_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_last_error_code), MP_ROM_PTR(&cml_get_last_error_code_fn_obj) },
    { MP_ROM_QSTR(MP_QSTR_has_error), MP_ROM_PTR(&cml_has_error_fn_obj) },

    { MP_ROM_QSTR(MP_QSTR_DTYPE_FLOAT32), MP_ROM_INT(DTYPE_FLOAT32) },
    { MP_ROM_QSTR(MP_QSTR_DTYPE_FLOAT64), MP_ROM_INT(DTYPE_FLOAT64) },
    { MP_ROM_QSTR(MP_QSTR_DTYPE_INT32), MP_ROM_INT(DTYPE_INT32) },
    { MP_ROM_QSTR(MP_QSTR_DTYPE_INT64), MP_ROM_INT(DTYPE_INT64) },
    { MP_ROM_QSTR(MP_QSTR_DTYPE_BOOL), MP_ROM_INT(DTYPE_BOOL) },
    { MP_ROM_QSTR(MP_QSTR_DTYPE_FLOAT16), MP_ROM_INT(DTYPE_FLOAT16) },
    { MP_ROM_QSTR(MP_QSTR_DTYPE_BFLOAT16), MP_ROM_INT(DTYPE_BFLOAT16) },
    { MP_ROM_QSTR(MP_QSTR_DEVICE_CPU), MP_ROM_INT(DEVICE_CPU) },
    { MP_ROM_QSTR(MP_QSTR_DEVICE_CUDA), MP_ROM_INT(DEVICE_CUDA) },
    { MP_ROM_QSTR(MP_QSTR_DEVICE_METAL), MP_ROM_INT(DEVICE_METAL) },
    { MP_ROM_QSTR(MP_QSTR_DEVICE_ROCM), MP_ROM_INT(DEVICE_ROCM) },
    { MP_ROM_QSTR(MP_QSTR_DEVICE_OPENCL), MP_ROM_INT(DEVICE_OPENCL) },
    { MP_ROM_QSTR(MP_QSTR_DEVICE_AUTO), MP_ROM_INT(DEVICE_AUTO) },

    #include "modcml_table.inc"
};
static MP_DEFINE_CONST_DICT(cml_module_globals, cml_module_globals_table);

const mp_obj_module_t cml_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&cml_module_globals,
};

const mp_obj_module_t torch_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&cml_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_cml, cml_user_cmodule);
MP_REGISTER_MODULE(MP_QSTR_torch, torch_user_cmodule);
