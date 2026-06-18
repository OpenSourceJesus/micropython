#include "modcml.h"

static mp_obj_t cml_backward_fn(size_t n_args, const mp_obj_t *args) {
    cml_require_init();
    Tensor *tensor = cml_tensor_from_mp(args[0], NULL);
    Tensor *grad = (n_args > 1 && args[1] != mp_const_none) ? cml_tensor_from_mp(args[1], NULL) : NULL;
    bool retain_graph = (n_args > 2) ? mp_obj_is_true(args[2]) : false;
    bool create_graph = (n_args > 3) ? mp_obj_is_true(args[3]) : false;
    torch_backward(tensor, grad, retain_graph, create_graph);
    cml_raise_if_error();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cml_backward_fn_obj, 1, 4, cml_backward_fn);

static mp_obj_t cml_zero_grad_tensor_fn(mp_obj_t tensor) {
    torch_zero_grad(cml_tensor_from_mp(tensor, NULL));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_zero_grad_tensor_fn_obj, cml_zero_grad_tensor_fn);

static mp_obj_t cml_no_grad_fn(void) {
    torch_no_grad();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(cml_no_grad_fn_obj, cml_no_grad_fn);

static mp_obj_t cml_enable_grad_fn(void) {
    torch_enable_grad();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(cml_enable_grad_fn_obj, cml_enable_grad_fn);

static mp_obj_t cml_is_grad_enabled_fn(void) {
    return mp_obj_new_bool(torch_is_grad_enabled());
}
MP_DEFINE_CONST_FUN_OBJ_0(cml_is_grad_enabled_fn_obj, cml_is_grad_enabled_fn);

static mp_obj_t cml_set_eager_mode_fn(mp_obj_t enabled) {
    torch_set_eager_mode(mp_obj_is_true(enabled));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_set_eager_mode_fn_obj, cml_set_eager_mode_fn);

static mp_obj_t cml_is_eager_mode_fn(void) {
    return mp_obj_new_bool(torch_is_eager_mode());
}
MP_DEFINE_CONST_FUN_OBJ_0(cml_is_eager_mode_fn_obj, cml_is_eager_mode_fn);

static mp_obj_t cml_inference_mode_fn(mp_obj_t enabled) {
    torch_inference_mode(mp_obj_is_true(enabled));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_inference_mode_fn_obj, cml_inference_mode_fn);

static mp_obj_t cml_set_num_threads_fn(mp_obj_t n) {
    torch_set_num_threads(mp_obj_get_int(n));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_set_num_threads_fn_obj, cml_set_num_threads_fn);

static mp_obj_t cml_get_num_threads_fn(void) {
    return mp_obj_new_int(torch_get_num_threads());
}
MP_DEFINE_CONST_FUN_OBJ_0(cml_get_num_threads_fn_obj, cml_get_num_threads_fn);

static mp_obj_t cml_reset_ir_fn(void) {
    torch_reset_ir();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(cml_reset_ir_fn_obj, cml_reset_ir_fn);

static mp_obj_t cml_reset_ir_soft_fn(void) {
    torch_reset_ir_soft();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(cml_reset_ir_soft_fn_obj, cml_reset_ir_soft_fn);
