#include "modcml.h"

static mp_obj_t cml_module_del(mp_obj_t self_in) {
    cml_module_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->owns && self->module) {
        module_free(self->module);
        self->module = NULL;
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_module_del_obj, cml_module_del);

static mp_obj_t cml_module_forward(mp_obj_t self_in, mp_obj_t input) {
    cml_require_init();
    Tensor *out = torch_module_forward(cml_as_module(self_in)->module, cml_tensor_from_mp(input, NULL));
    cml_raise_if_error();
    return cml_new_tensor(out, true);
}
MP_DEFINE_CONST_FUN_OBJ_2(cml_module_forward_obj, cml_module_forward);

static mp_obj_t cml_module_train(mp_obj_t self_in) {
    torch_module_train(cml_as_module(self_in)->module);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_module_train_obj, cml_module_train);

static mp_obj_t cml_module_eval(mp_obj_t self_in) {
    torch_module_eval(cml_as_module(self_in)->module);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_module_eval_obj, cml_module_eval);

static mp_obj_t cml_module_is_training(mp_obj_t self_in) {
    return mp_obj_new_bool(torch_module_is_training(cml_as_module(self_in)->module));
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_module_is_training_obj, cml_module_is_training);

static mp_obj_t cml_module_zero_grad(mp_obj_t self_in) {
    torch_module_zero_grad(cml_as_module(self_in)->module);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_module_zero_grad_obj, cml_module_zero_grad);

static mp_obj_t cml_module_state_dict(mp_obj_t self_in) {
    cml_require_init();
    return cml_new_statedict(torch_module_state_dict(cml_as_module(self_in)->module, ""));
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_module_state_dict_obj, cml_module_state_dict);

static mp_obj_t cml_module_load_state_dict(size_t n_args, const mp_obj_t *args) {
    cml_require_init();
    bool strict = (n_args > 2) ? mp_obj_is_true(args[2]) : true;
    int rc = torch_module_load_state_dict(cml_as_module(args[0])->module, cml_as_statedict(args[1])->sd, strict);
    if (rc != 0) {
        cml_raise_if_error();
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("load_state_dict failed"));
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cml_module_load_state_dict_obj, 2, 3, cml_module_load_state_dict);

static mp_obj_t cml_sequential_forward(mp_obj_t self_in, mp_obj_t input) {
    cml_require_init();
    Sequential *seq = (Sequential *)cml_as_module(self_in)->module;
    Tensor *out = torch_nn_sequential_forward(seq, cml_tensor_from_mp(input, NULL));
    cml_raise_if_error();
    return cml_new_tensor(out, true);
}
MP_DEFINE_CONST_FUN_OBJ_2(cml_sequential_forward_obj, cml_sequential_forward);

static mp_obj_t cml_sequential_add(mp_obj_t self_in, mp_obj_t layer) {
    cml_module_obj_t *layer_obj = cml_as_module(layer);
    torch_nn_sequential_add((Sequential *)cml_as_module(self_in)->module, layer_obj->module);
    // Sequential takes ownership; prevent GC of layer wrapper from freeing the C module early.
    layer_obj->owns = false;
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(cml_sequential_add_obj, cml_sequential_add);

static const mp_rom_map_elem_t cml_module_locals[] = {
    { MP_ROM_QSTR(MP_QSTR_forward), MP_ROM_PTR(&cml_module_forward_obj) },
    { MP_ROM_QSTR(MP_QSTR_train), MP_ROM_PTR(&cml_module_train_obj) },
    { MP_ROM_QSTR(MP_QSTR_eval), MP_ROM_PTR(&cml_module_eval_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_training), MP_ROM_PTR(&cml_module_is_training_obj) },
    { MP_ROM_QSTR(MP_QSTR_zero_grad), MP_ROM_PTR(&cml_module_zero_grad_obj) },
    { MP_ROM_QSTR(MP_QSTR_state_dict), MP_ROM_PTR(&cml_module_state_dict_obj) },
    { MP_ROM_QSTR(MP_QSTR_load_state_dict), MP_ROM_PTR(&cml_module_load_state_dict_obj) },
    { MP_ROM_QSTR(MP_QSTR_add), MP_ROM_PTR(&cml_sequential_add_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&cml_module_del_obj) },
};
static MP_DEFINE_CONST_DICT(cml_module_locals_dict, cml_module_locals);

MP_DEFINE_CONST_OBJ_TYPE(
    cml_type_Module,
    MP_QSTR_Module,
    MP_TYPE_FLAG_NONE,
    locals_dict, &cml_module_locals_dict
    );

static mp_obj_t cml_statedict_get(mp_obj_t self_in, mp_obj_t key) {
    size_t len = 0;
    const char *k = mp_obj_str_get_data(key, &len);
    Tensor *t = torch_state_dict_get(cml_as_statedict(self_in)->sd, k);
    if (t == NULL) {
        return mp_const_none;
    }
    return cml_new_tensor(t, false);
}
MP_DEFINE_CONST_FUN_OBJ_2(cml_statedict_get_obj, cml_statedict_get);

static mp_obj_t cml_statedict_del(mp_obj_t self_in) {
    cml_statedict_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->sd) {
        torch_state_dict_free(self->sd);
        self->sd = NULL;
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_statedict_del_obj, cml_statedict_del);

static const mp_rom_map_elem_t cml_statedict_locals[] = {
    { MP_ROM_QSTR(MP_QSTR_get), MP_ROM_PTR(&cml_statedict_get_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&cml_statedict_del_obj) },
};
static MP_DEFINE_CONST_DICT(cml_statedict_locals_dict, cml_statedict_locals);

MP_DEFINE_CONST_OBJ_TYPE(
    cml_type_StateDict,
    MP_QSTR_StateDict,
    MP_TYPE_FLAG_NONE,
    locals_dict, &cml_statedict_locals_dict
    );

static mp_obj_t cml_nn_linear_fn(size_t n_args, const mp_obj_t *args) {
    cml_require_init();
    bool bias = (n_args > 2) ? mp_obj_is_true(args[2]) : true;
    return cml_new_module((Module *)torch_nn_linear(mp_obj_get_int(args[0]), mp_obj_get_int(args[1]), bias), true);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cml_nn_linear_fn_obj, 2, 3, cml_nn_linear_fn);

static mp_obj_t cml_nn_relu_fn(void) {
    cml_require_init();
    return cml_new_module((Module *)torch_nn_relu(), true);
}
MP_DEFINE_CONST_FUN_OBJ_0(cml_nn_relu_fn_obj, cml_nn_relu_fn);

static mp_obj_t cml_nn_sequential_fn(void) {
    cml_require_init();
    return cml_new_module((Module *)torch_nn_sequential(), true);
}
MP_DEFINE_CONST_FUN_OBJ_0(cml_nn_sequential_fn_obj, cml_nn_sequential_fn);

static mp_obj_t cml_mse_loss_fn(mp_obj_t input, mp_obj_t target) {
    return cml_do_binary(input, target, torch_nn_mse_loss);
}
MP_DEFINE_CONST_FUN_OBJ_2(cml_mse_loss_fn_obj, cml_mse_loss_fn);

static mp_obj_t cml_cross_entropy_loss_fn(mp_obj_t input, mp_obj_t target) {
    return cml_do_binary(input, target, torch_nn_cross_entropy_loss);
}
MP_DEFINE_CONST_FUN_OBJ_2(cml_cross_entropy_loss_fn_obj, cml_cross_entropy_loss_fn);
