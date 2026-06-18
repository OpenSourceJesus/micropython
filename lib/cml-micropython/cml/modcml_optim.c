#include "modcml.h"

static mp_obj_t cml_optim_del(mp_obj_t self_in) {
    cml_optim_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->optimizer) {
        torch_optim_free(self->optimizer);
        self->optimizer = NULL;
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_optim_del_obj, cml_optim_del);

static mp_obj_t cml_optim_obj_zero_grad(mp_obj_t self_in) {
    torch_optim_zero_grad(cml_as_optimizer(self_in)->optimizer);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_optim_obj_zero_grad_obj, cml_optim_obj_zero_grad);

static mp_obj_t cml_optim_obj_step(mp_obj_t self_in) {
    torch_optim_step(cml_as_optimizer(self_in)->optimizer);
    cml_raise_if_error();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_optim_obj_step_obj, cml_optim_obj_step);

static const mp_rom_map_elem_t cml_optim_locals[] = {
    { MP_ROM_QSTR(MP_QSTR_zero_grad), MP_ROM_PTR(&cml_optim_obj_zero_grad_obj) },
    { MP_ROM_QSTR(MP_QSTR_step), MP_ROM_PTR(&cml_optim_obj_step_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&cml_optim_del_obj) },
};
static MP_DEFINE_CONST_DICT(cml_optim_locals_dict, cml_optim_locals);

MP_DEFINE_CONST_OBJ_TYPE(
    cml_type_Optimizer,
    MP_QSTR_Optimizer,
    MP_TYPE_FLAG_NONE,
    locals_dict, &cml_optim_locals_dict
    );

static mp_obj_t cml_optim_adam_fn(size_t n_args, const mp_obj_t *args) {
    cml_require_init();
    Module *model = cml_as_module(args[0])->module;
    float lr = (float)mp_obj_get_float(args[1]);
    float weight_decay = (n_args > 2) ? (float)mp_obj_get_float(args[2]) : 0.0f;
    float beta1 = (n_args > 3) ? (float)mp_obj_get_float(args[3]) : 0.9f;
    float beta2 = (n_args > 4) ? (float)mp_obj_get_float(args[4]) : 0.999f;
    float eps = (n_args > 5) ? (float)mp_obj_get_float(args[5]) : 1e-8f;
    return cml_new_optimizer(torch_optim_adam(model, lr, weight_decay, beta1, beta2, eps));
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cml_optim_adam_fn_obj, 2, 6, cml_optim_adam_fn);

static mp_obj_t cml_optim_sgd_fn(size_t n_args, const mp_obj_t *args) {
    cml_require_init();
    Module *model = cml_as_module(args[0])->module;
    float lr = (float)mp_obj_get_float(args[1]);
    float momentum = (n_args > 2) ? (float)mp_obj_get_float(args[2]) : 0.0f;
    float weight_decay = (n_args > 3) ? (float)mp_obj_get_float(args[3]) : 0.0f;
    return cml_new_optimizer(torch_optim_sgd(model, lr, momentum, weight_decay));
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cml_optim_sgd_fn_obj, 2, 4, cml_optim_sgd_fn);
