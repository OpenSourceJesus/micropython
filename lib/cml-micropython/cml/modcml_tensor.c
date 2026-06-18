#include "modcml.h"

#include <string.h>

static mp_obj_t cml_tensor_del(mp_obj_t self_in) {
    cml_tensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->owns && self->tensor) {
        torch_tensor_free(self->tensor);
        self->tensor = NULL;
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_tensor_del_obj, cml_tensor_del);

static mp_obj_t cml_tensor_tolist(mp_obj_t self_in) {
    return cml_tensor_to_mp_list(cml_as_tensor(self_in)->tensor);
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_tensor_tolist_obj, cml_tensor_tolist);

static mp_obj_t cml_tensor_shape(mp_obj_t self_in) {
    return cml_shape_to_tuple(cml_as_tensor(self_in)->tensor);
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_tensor_shape_obj, cml_tensor_shape);

static mp_obj_t cml_tensor_ndim(mp_obj_t self_in) {
    return mp_obj_new_int(torch_tensor_ndim(cml_as_tensor(self_in)->tensor));
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_tensor_ndim_obj, cml_tensor_ndim);

static mp_obj_t cml_tensor_numel(mp_obj_t self_in) {
    return mp_obj_new_int_from_uint(torch_tensor_numel(cml_as_tensor(self_in)->tensor));
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_tensor_numel_obj, cml_tensor_numel);

static mp_obj_t cml_tensor_dtype(mp_obj_t self_in) {
    return cml_dtype_to_str(torch_tensor_dtype(cml_as_tensor(self_in)->tensor));
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_tensor_dtype_obj, cml_tensor_dtype);

static mp_obj_t cml_tensor_device(mp_obj_t self_in) {
    return cml_device_to_str(torch_tensor_device(cml_as_tensor(self_in)->tensor));
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_tensor_device_obj, cml_tensor_device);

static mp_obj_t cml_tensor_item(mp_obj_t self_in) {
    return mp_obj_new_float(torch_tensor_item_float(cml_as_tensor(self_in)->tensor));
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_tensor_item_obj, cml_tensor_item);

static mp_obj_t cml_tensor_set_item(mp_obj_t self_in, mp_obj_t val) {
    torch_tensor_set_item_float(cml_as_tensor(self_in)->tensor, (float)mp_obj_get_float(val));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(cml_tensor_set_item_obj, cml_tensor_set_item);

static mp_obj_t cml_tensor_is_contiguous(mp_obj_t self_in) {
    return mp_obj_new_bool(torch_tensor_is_contiguous(cml_as_tensor(self_in)->tensor));
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_tensor_is_contiguous_obj, cml_tensor_is_contiguous);

static mp_obj_t cml_tensor_requires_grad(mp_obj_t self_in) {
    return mp_obj_new_bool(torch_tensor_requires_grad(cml_as_tensor(self_in)->tensor));
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_tensor_requires_grad_obj, cml_tensor_requires_grad);

static mp_obj_t cml_tensor_set_requires_grad(mp_obj_t self_in, mp_obj_t val) {
    torch_tensor_set_requires_grad(cml_as_tensor(self_in)->tensor, mp_obj_is_true(val));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(cml_tensor_set_requires_grad_obj, cml_tensor_set_requires_grad);

static mp_obj_t cml_tensor_retain(mp_obj_t self_in) {
    torch_tensor_retain(cml_as_tensor(self_in)->tensor);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_tensor_retain_obj, cml_tensor_retain);

static mp_obj_t cml_tensor_free_fn(mp_obj_t self_in) {
    cml_tensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->tensor) {
        torch_tensor_free(self->tensor);
        self->tensor = NULL;
        self->owns = false;
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_tensor_free_fn_obj, cml_tensor_free_fn);

static mp_obj_t cml_tensor_grad(mp_obj_t self_in) {
    Tensor *g = torch_get_grad(cml_as_tensor(self_in)->tensor);
    if (g == NULL) {
        return mp_const_none;
    }
    return cml_new_tensor(g, false);
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_tensor_grad_obj, cml_tensor_grad);

static mp_obj_t cml_tensor_realize(mp_obj_t self_in) {
    if (torch_realize(cml_as_tensor(self_in)->tensor) != 0) {
        cml_raise_if_error();
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("realize failed"));
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_tensor_realize_obj, cml_tensor_realize);

static void cml_tensor_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    cml_tensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_print_str(print, "Tensor(");
    if (self->tensor) {
        mp_obj_t shape = cml_shape_to_tuple(self->tensor);
        mp_obj_print_helper(print, shape, PRINT_REPR);
    } else {
        mp_print_str(print, "freed");
    }
    mp_print_str(print, ")");
}

static void cml_tensor_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    cml_tensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] == MP_OBJ_NULL) {
        if (attr == MP_QSTR_shape) {
            dest[0] = cml_shape_to_tuple(self->tensor);
        } else if (attr == MP_QSTR_ndim) {
            dest[0] = mp_obj_new_int(torch_tensor_ndim(self->tensor));
        } else if (attr == MP_QSTR_dtype) {
            dest[0] = cml_dtype_to_str(torch_tensor_dtype(self->tensor));
        } else if (attr == MP_QSTR_device) {
            dest[0] = cml_device_to_str(torch_tensor_device(self->tensor));
        } else {
            dest[1] = MP_OBJ_SENTINEL;
        }
    } else {
        dest[1] = MP_OBJ_SENTINEL;
    }
}

static const mp_rom_map_elem_t cml_tensor_locals[] = {
    { MP_ROM_QSTR(MP_QSTR_tolist), MP_ROM_PTR(&cml_tensor_tolist_obj) },
    { MP_ROM_QSTR(MP_QSTR_shape), MP_ROM_PTR(&cml_tensor_shape_obj) },
    { MP_ROM_QSTR(MP_QSTR_ndim), MP_ROM_PTR(&cml_tensor_ndim_obj) },
    { MP_ROM_QSTR(MP_QSTR_numel), MP_ROM_PTR(&cml_tensor_numel_obj) },
    { MP_ROM_QSTR(MP_QSTR_dtype), MP_ROM_PTR(&cml_tensor_dtype_obj) },
    { MP_ROM_QSTR(MP_QSTR_device), MP_ROM_PTR(&cml_tensor_device_obj) },
    { MP_ROM_QSTR(MP_QSTR_item), MP_ROM_PTR(&cml_tensor_item_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_item), MP_ROM_PTR(&cml_tensor_set_item_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_contiguous), MP_ROM_PTR(&cml_tensor_is_contiguous_obj) },
    { MP_ROM_QSTR(MP_QSTR_requires_grad), MP_ROM_PTR(&cml_tensor_requires_grad_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_requires_grad), MP_ROM_PTR(&cml_tensor_set_requires_grad_obj) },
    { MP_ROM_QSTR(MP_QSTR_retain), MP_ROM_PTR(&cml_tensor_retain_obj) },
    { MP_ROM_QSTR(MP_QSTR_free), MP_ROM_PTR(&cml_tensor_free_fn_obj) },
    { MP_ROM_QSTR(MP_QSTR_grad), MP_ROM_PTR(&cml_tensor_grad_obj) },
    { MP_ROM_QSTR(MP_QSTR_realize), MP_ROM_PTR(&cml_tensor_realize_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&cml_tensor_del_obj) },
};
static MP_DEFINE_CONST_DICT(cml_tensor_locals_dict, cml_tensor_locals);

MP_DEFINE_CONST_OBJ_TYPE(
    cml_type_Tensor,
    MP_QSTR_Tensor,
    MP_TYPE_FLAG_NONE,
    attr, cml_tensor_attr,
    print, cml_tensor_print,
    locals_dict, &cml_tensor_locals_dict
    );

// --- module-level tensor factories ---

static mp_obj_t cml_shape_create(size_t n_args, const mp_obj_t *args,
    Tensor *(*creator)(int *, int, const TorchTensorOptions *)) {
    cml_require_init();
    int shape[CML_MAX_NDIM];
    int ndim = cml_parse_shape(args[0], shape, CML_MAX_NDIM);
    TorchTensorOptions opts = (n_args > 1) ? cml_parse_options(args[1]) : cml_parse_options(mp_const_none);
    return cml_new_tensor(creator(shape, ndim, &opts), true);
}

static mp_obj_t cml_empty_fn(size_t n_args, const mp_obj_t *args) {
    return cml_shape_create(n_args, args, torch_empty);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cml_empty_fn_obj, 1, 2, cml_empty_fn);

static mp_obj_t cml_zeros_fn(size_t n_args, const mp_obj_t *args) {
    return cml_shape_create(n_args, args, torch_zeros);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cml_zeros_fn_obj, 1, 2, cml_zeros_fn);

static mp_obj_t cml_ones_fn(size_t n_args, const mp_obj_t *args) {
    return cml_shape_create(n_args, args, torch_ones);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cml_ones_fn_obj, 1, 2, cml_ones_fn);

static mp_obj_t cml_rand_fn(size_t n_args, const mp_obj_t *args) {
    return cml_shape_create(n_args, args, torch_rand);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cml_rand_fn_obj, 1, 2, cml_rand_fn);

static mp_obj_t cml_randn_fn(size_t n_args, const mp_obj_t *args) {
    return cml_shape_create(n_args, args, torch_randn);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cml_randn_fn_obj, 1, 2, cml_randn_fn);

static mp_obj_t cml_full_fn(size_t n_args, const mp_obj_t *args) {
    cml_require_init();
    int shape[CML_MAX_NDIM];
    int ndim = cml_parse_shape(args[0], shape, CML_MAX_NDIM);
    float value = (float)mp_obj_get_float(args[1]);
    TorchTensorOptions opts = (n_args > 2) ? cml_parse_options(args[2]) : cml_parse_options(mp_const_none);
    return cml_new_tensor(torch_full(shape, ndim, &opts, value), true);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cml_full_fn_obj, 2, 3, cml_full_fn);

static mp_obj_t cml_eye_fn(size_t n_args, const mp_obj_t *args) {
    cml_require_init();
    int n = mp_obj_get_int(args[0]);
    TorchTensorOptions opts = (n_args > 1) ? cml_parse_options(args[1]) : cml_parse_options(mp_const_none);
    return cml_new_tensor(torch_eye(n, &opts), true);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cml_eye_fn_obj, 1, 2, cml_eye_fn);

static mp_obj_t cml_arange_fn(size_t n_args, const mp_obj_t *args) {
    cml_require_init();
    float start = (float)mp_obj_get_float(args[0]);
    float end = (float)mp_obj_get_float(args[1]);
    float step = (n_args > 2) ? (float)mp_obj_get_float(args[2]) : 1.0f;
    TorchTensorOptions opts = (n_args > 3) ? cml_parse_options(args[3]) : cml_parse_options(mp_const_none);
    return cml_new_tensor(torch_arange(start, end, step, &opts), true);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cml_arange_fn_obj, 2, 4, cml_arange_fn);

static mp_obj_t cml_linspace_fn(size_t n_args, const mp_obj_t *args) {
    cml_require_init();
    float start = (float)mp_obj_get_float(args[0]);
    float end = (float)mp_obj_get_float(args[1]);
    int steps = mp_obj_get_int(args[2]);
    TorchTensorOptions opts = (n_args > 3) ? cml_parse_options(args[3]) : cml_parse_options(mp_const_none);
    return cml_new_tensor(torch_linspace(start, end, steps, &opts), true);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cml_linspace_fn_obj, 3, 4, cml_linspace_fn);

static mp_obj_t cml_tensor_fn(mp_obj_t data_in) {
    cml_require_init();
    bool owns = false;
    Tensor *t = cml_tensor_from_mp(data_in, &owns);
    return cml_new_tensor(t, owns);
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_tensor_fn_obj, cml_tensor_fn);

static mp_obj_t cml_zeros_like_fn(mp_obj_t a) {
    return cml_do_unary(a, torch_zeros_like);
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_zeros_like_fn_obj, cml_zeros_like_fn);

static mp_obj_t cml_ones_like_fn(mp_obj_t a) {
    return cml_do_unary(a, torch_ones_like);
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_ones_like_fn_obj, cml_ones_like_fn);

static mp_obj_t cml_randn_like_fn(mp_obj_t a) {
    return cml_do_unary(a, torch_randn_like);
}
MP_DEFINE_CONST_FUN_OBJ_1(cml_randn_like_fn_obj, cml_randn_like_fn);

#define DEF_BIN(name, cfn) \
static mp_obj_t cml_##name##_fn(mp_obj_t a, mp_obj_t b) { return cml_do_binary(a, b, cfn); } \
MP_DEFINE_CONST_FUN_OBJ_2(cml_##name##_fn_obj, cml_##name##_fn);

DEF_BIN(add, torch_add)
DEF_BIN(sub, torch_sub)
DEF_BIN(mul, torch_mul)
DEF_BIN(div, torch_div)
DEF_BIN(matmul, torch_matmul)
DEF_BIN(pow, torch_pow)

#undef DEF_BIN

#define DEF_UNARY(name, cfn) \
static mp_obj_t cml_##name##_fn(mp_obj_t a) { return cml_do_unary(a, cfn); } \
MP_DEFINE_CONST_FUN_OBJ_1(cml_##name##_fn_obj, cml_##name##_fn);

DEF_UNARY(relu, torch_relu)
DEF_UNARY(sigmoid, torch_sigmoid)
DEF_UNARY(tanh, torch_tanh)
DEF_UNARY(gelu, torch_gelu)
DEF_UNARY(clone, torch_clone)
DEF_UNARY(detach, torch_detach)
DEF_UNARY(contiguous, torch_contiguous)

#undef DEF_UNARY

static mp_obj_t cml_sum_fn(size_t n_args, const mp_obj_t *args) {
    return cml_do_unary_dim(args[0], args[1], (n_args > 2) ? args[2] : mp_const_false, torch_sum);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cml_sum_fn_obj, 2, 3, cml_sum_fn);

static mp_obj_t cml_mean_fn(size_t n_args, const mp_obj_t *args) {
    return cml_do_unary_dim(args[0], args[1], (n_args > 2) ? args[2] : mp_const_false, torch_mean);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cml_mean_fn_obj, 2, 3, cml_mean_fn);

static mp_obj_t cml_max_fn(size_t n_args, const mp_obj_t *args) {
    return cml_do_unary_dim(args[0], args[1], (n_args > 2) ? args[2] : mp_const_false, torch_max);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cml_max_fn_obj, 2, 3, cml_max_fn);

static mp_obj_t cml_min_fn(size_t n_args, const mp_obj_t *args) {
    return cml_do_unary_dim(args[0], args[1], (n_args > 2) ? args[2] : mp_const_false, torch_min);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cml_min_fn_obj, 2, 3, cml_min_fn);

static mp_obj_t cml_softmax_fn(mp_obj_t a, mp_obj_t dim) {
    cml_require_init();
    Tensor *out = torch_softmax(cml_tensor_from_mp(a, NULL), mp_obj_get_int(dim));
    cml_raise_if_error();
    return cml_new_tensor(out, true);
}
MP_DEFINE_CONST_FUN_OBJ_2(cml_softmax_fn_obj, cml_softmax_fn);

static mp_obj_t cml_reshape_fn(mp_obj_t a, mp_obj_t shape_in) {
    cml_require_init();
    int shape[CML_MAX_NDIM];
    int ndim = cml_parse_shape(shape_in, shape, CML_MAX_NDIM);
    Tensor *out = torch_reshape(cml_tensor_from_mp(a, NULL), shape, ndim);
    cml_raise_if_error();
    return cml_new_tensor(out, true);
}
MP_DEFINE_CONST_FUN_OBJ_2(cml_reshape_fn_obj, cml_reshape_fn);

static mp_obj_t cml_transpose_fn(mp_obj_t a, mp_obj_t d0, mp_obj_t d1) {
    cml_require_init();
    Tensor *out = torch_transpose(cml_tensor_from_mp(a, NULL), mp_obj_get_int(d0), mp_obj_get_int(d1));
    cml_raise_if_error();
    return cml_new_tensor(out, true);
}
MP_DEFINE_CONST_FUN_OBJ_3(cml_transpose_fn_obj, cml_transpose_fn);

static mp_obj_t cml_squeeze_fn(mp_obj_t a, mp_obj_t dim) {
    cml_require_init();
    Tensor *out = torch_squeeze(cml_tensor_from_mp(a, NULL), mp_obj_get_int(dim));
    cml_raise_if_error();
    return cml_new_tensor(out, true);
}
MP_DEFINE_CONST_FUN_OBJ_2(cml_squeeze_fn_obj, cml_squeeze_fn);

static mp_obj_t cml_unsqueeze_fn(mp_obj_t a, mp_obj_t dim) {
    cml_require_init();
    Tensor *out = torch_unsqueeze(cml_tensor_from_mp(a, NULL), mp_obj_get_int(dim));
    cml_raise_if_error();
    return cml_new_tensor(out, true);
}
MP_DEFINE_CONST_FUN_OBJ_2(cml_unsqueeze_fn_obj, cml_unsqueeze_fn);

static mp_obj_t cml_cat_fn(mp_obj_t tensors, mp_obj_t dim) {
    cml_require_init();
    int count = 0;
    Tensor **arr = cml_parse_tensor_list(tensors, &count);
    Tensor *out = torch_cat(arr, count, mp_obj_get_int(dim));
    m_del(Tensor *, arr, count);
    cml_raise_if_error();
    return cml_new_tensor(out, true);
}
MP_DEFINE_CONST_FUN_OBJ_2(cml_cat_fn_obj, cml_cat_fn);

static mp_obj_t cml_stack_fn(mp_obj_t tensors, mp_obj_t dim) {
    cml_require_init();
    int count = 0;
    Tensor **arr = cml_parse_tensor_list(tensors, &count);
    Tensor *out = torch_stack(arr, count, mp_obj_get_int(dim));
    m_del(Tensor *, arr, count);
    cml_raise_if_error();
    return cml_new_tensor(out, true);
}
MP_DEFINE_CONST_FUN_OBJ_2(cml_stack_fn_obj, cml_stack_fn);

static mp_obj_t cml_linear_fn(size_t n_args, const mp_obj_t *args) {
    cml_require_init();
    Tensor *input = cml_tensor_from_mp(args[0], NULL);
    Tensor *weight = cml_tensor_from_mp(args[1], NULL);
    Tensor *bias = (n_args > 2 && args[2] != mp_const_none) ? cml_tensor_from_mp(args[2], NULL) : NULL;
    return cml_new_tensor(torch_linear(input, weight, bias), true);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cml_linear_fn_obj, 2, 3, cml_linear_fn);

static mp_obj_t cml_linear_relu_fn(size_t n_args, const mp_obj_t *args) {
    cml_require_init();
    Tensor *input = cml_tensor_from_mp(args[0], NULL);
    Tensor *weight = cml_tensor_from_mp(args[1], NULL);
    Tensor *bias = (n_args > 2 && args[2] != mp_const_none) ? cml_tensor_from_mp(args[2], NULL) : NULL;
    return cml_new_tensor(torch_linear_relu(input, weight, bias), true);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cml_linear_relu_fn_obj, 2, 3, cml_linear_relu_fn);
