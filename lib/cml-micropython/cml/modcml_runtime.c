#include "modcml.h"

static mp_obj_t cml_runtime_del(mp_obj_t self_in) {
    cml_runtime_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->runtime) {
        torch_runtime_free(self->runtime);
        self->runtime = NULL;
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_runtime_del_obj, cml_runtime_del);

static mp_obj_t cml_runtime_forward(mp_obj_t self_in, mp_obj_t input) {
    cml_require_init();
    Tensor *out = torch_runtime_forward(cml_as_runtime(self_in)->runtime, cml_tensor_from_mp(input, NULL));
    cml_raise_if_error();
    return cml_new_tensor(out, true);
}
MP_DEFINE_CONST_FUN_OBJ_2(cml_runtime_forward_obj, cml_runtime_forward);

static const mp_rom_map_elem_t cml_runtime_locals[] = {
    { MP_ROM_QSTR(MP_QSTR_forward), MP_ROM_PTR(&cml_runtime_forward_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&cml_runtime_del_obj) },
};
static MP_DEFINE_CONST_DICT(cml_runtime_locals_dict, cml_runtime_locals);

MP_DEFINE_CONST_OBJ_TYPE(
    cml_type_RuntimeModule,
    MP_QSTR_RuntimeModule,
    MP_TYPE_FLAG_NONE,
    locals_dict, &cml_runtime_locals_dict
    );

static mp_obj_t cml_runtime_from_module_fn(mp_obj_t module) {
    cml_require_init();
    return cml_new_runtime(torch_runtime_from_module(cml_as_module(module)->module));
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_runtime_from_module_fn_obj, cml_runtime_from_module_fn);

static mp_obj_t cml_runtime_load_aot_fn(mp_obj_t path) {
    cml_require_init();
    size_t len = 0;
    const char *p = mp_obj_str_get_data(path, &len);
    return cml_new_runtime(torch_runtime_load_aot(p));
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_runtime_load_aot_fn_obj, cml_runtime_load_aot_fn);

static mp_obj_t cml_runtime_load_pte_fn(mp_obj_t path) {
    cml_require_init();
    size_t len = 0;
    const char *p = mp_obj_str_get_data(path, &len);
    return cml_new_runtime(torch_runtime_load_pte(p));
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_runtime_load_pte_fn_obj, cml_runtime_load_pte_fn);

static mp_obj_t cml_runtime_export_pte_fn(size_t n_args, const mp_obj_t *args) {
    cml_require_init();
    Module *module = cml_as_module(args[0])->module;
    Tensor *sample = cml_tensor_from_mp(args[1], NULL);
    size_t len = 0;
    const char *path = mp_obj_str_get_data(args[2], &len);
    const TorchPTEExportOptions *opts = NULL;
    if (n_args > 3 && args[3] != mp_const_none) {
        mp_raise_NotImplementedError(MP_ERROR_TEXT("export options not yet supported"));
    }
    if (torch_runtime_export_pte(module, sample, path, opts) != 0) {
        cml_raise_if_error();
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("export_pte failed"));
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cml_runtime_export_pte_fn_obj, 3, 4, cml_runtime_export_pte_fn);
