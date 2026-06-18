#include "modcml.h"

#include <stdlib.h>
#include <string.h>

bool cml_ready;

void cml_require_init(void) {
    if (!cml_ready) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("call init() first"));
    }
}

void cml_raise_if_error(void) {
    if (torch_has_error()) {
        const char *msg = torch_get_last_error();
        if (msg == NULL) {
            mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("unknown C-ML error"));
        }
        mp_obj_t args[] = { mp_obj_new_str(msg, strlen(msg)) };
        nlr_raise(mp_obj_new_exception_args(&mp_type_RuntimeError, 1, args));
    }
}

mp_obj_t cml_new_tensor(Tensor *t, bool owns) {
    if (t == NULL) {
        cml_raise_if_error();
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("tensor creation failed"));
    }
    cml_tensor_obj_t *o = mp_obj_malloc(cml_tensor_obj_t, &cml_type_Tensor);
    o->tensor = t;
    o->owns = owns;
    return MP_OBJ_FROM_PTR(o);
}

mp_obj_t cml_new_module(Module *m, bool owns) {
    if (m == NULL) {
        cml_raise_if_error();
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("module creation failed"));
    }
    cml_module_obj_t *o = mp_obj_malloc(cml_module_obj_t, &cml_type_Module);
    o->module = m;
    o->owns = owns;
    return MP_OBJ_FROM_PTR(o);
}

mp_obj_t cml_new_optimizer(Optimizer *o) {
    if (o == NULL) {
        cml_raise_if_error();
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("optimizer creation failed"));
    }
    cml_optim_obj_t *obj = mp_obj_malloc(cml_optim_obj_t, &cml_type_Optimizer);
    obj->optimizer = o;
    return MP_OBJ_FROM_PTR(obj);
}

mp_obj_t cml_new_statedict(StateDict *sd) {
    if (sd == NULL) {
        cml_raise_if_error();
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("state_dict creation failed"));
    }
    cml_statedict_obj_t *o = mp_obj_malloc(cml_statedict_obj_t, &cml_type_StateDict);
    o->sd = sd;
    return MP_OBJ_FROM_PTR(o);
}

mp_obj_t cml_new_runtime(TorchRuntimeModule *rt) {
    if (rt == NULL) {
        cml_raise_if_error();
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("runtime creation failed"));
    }
    cml_runtime_obj_t *o = mp_obj_malloc(cml_runtime_obj_t, &cml_type_RuntimeModule);
    o->runtime = rt;
    return MP_OBJ_FROM_PTR(o);
}

bool cml_is_tensor(mp_obj_t obj) {
    return mp_obj_is_type(obj, &cml_type_Tensor);
}

cml_tensor_obj_t *cml_as_tensor(mp_obj_t obj) {
    if (!cml_is_tensor(obj)) {
        mp_raise_TypeError(MP_ERROR_TEXT("expected Tensor"));
    }
    return MP_OBJ_TO_PTR(obj);
}

cml_module_obj_t *cml_as_module(mp_obj_t obj) {
    if (!mp_obj_is_type(obj, &cml_type_Module)) {
        mp_raise_TypeError(MP_ERROR_TEXT("expected Module"));
    }
    return MP_OBJ_TO_PTR(obj);
}

cml_optim_obj_t *cml_as_optimizer(mp_obj_t obj) {
    if (!mp_obj_is_type(obj, &cml_type_Optimizer)) {
        mp_raise_TypeError(MP_ERROR_TEXT("expected Optimizer"));
    }
    return MP_OBJ_TO_PTR(obj);
}

cml_statedict_obj_t *cml_as_statedict(mp_obj_t obj) {
    if (!mp_obj_is_type(obj, &cml_type_StateDict)) {
        mp_raise_TypeError(MP_ERROR_TEXT("expected StateDict"));
    }
    return MP_OBJ_TO_PTR(obj);
}

cml_runtime_obj_t *cml_as_runtime(mp_obj_t obj) {
    if (!mp_obj_is_type(obj, &cml_type_RuntimeModule)) {
        mp_raise_TypeError(MP_ERROR_TEXT("expected RuntimeModule"));
    }
    return MP_OBJ_TO_PTR(obj);
}

int cml_parse_shape(mp_obj_t shape_in, int *shape_out, size_t max_ndim) {
    size_t ndim = 0;

    if (mp_obj_is_type(shape_in, &mp_type_tuple) || mp_obj_is_type(shape_in, &mp_type_list)) {
        mp_obj_t *items = NULL;
        mp_obj_get_array(shape_in, &ndim, &items);
        if (ndim == 0 || ndim > max_ndim) {
            mp_raise_ValueError(MP_ERROR_TEXT("invalid shape"));
        }
        for (size_t i = 0; i < ndim; ++i) {
            shape_out[i] = mp_obj_get_int(items[i]);
            if (shape_out[i] < 0) {
                mp_raise_ValueError(MP_ERROR_TEXT("shape must be non-negative"));
            }
        }
        return (int)ndim;
    }

    shape_out[0] = mp_obj_get_int(shape_in);
    if (shape_out[0] < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("shape must be non-negative"));
    }
    return 1;
}

static TorchTensorOptions cml_default_options(void) {
    TorchTensorOptions opts = torch_options();
    opts = torch_options_dtype(opts, torch_get_default_dtype());
    opts = torch_options_device(opts, torch_get_default_device());
    return opts;
}

TorchTensorOptions cml_parse_options(mp_obj_t opts_in) {
    TorchTensorOptions opts = cml_default_options();
    if (opts_in == mp_const_none) {
        return opts;
    }
    if (!mp_obj_is_type(opts_in, &mp_type_tuple) && !mp_obj_is_type(opts_in, &mp_type_dict)) {
        mp_raise_TypeError(MP_ERROR_TEXT("options must be dict or tuple"));
    }
    if (mp_obj_is_type(opts_in, &mp_type_tuple)) {
        size_t n = 0;
        mp_obj_t *items = NULL;
        mp_obj_get_array(opts_in, &n, &items);
        if (n > 0) {
            opts = torch_options_dtype(opts, cml_parse_dtype(items[0]));
        }
        if (n > 1) {
            opts = torch_options_device(opts, cml_parse_device(items[1]));
        }
        if (n > 2) {
            opts = torch_options_requires_grad(opts, mp_obj_is_true(items[2]));
        }
        return opts;
    }
    mp_map_t *map = mp_obj_dict_get_map(opts_in);
    for (size_t i = 0; i < map->alloc; ++i) {
        if (mp_map_slot_is_filled(map, i)) {
            qstr key = MP_OBJ_QSTR_VALUE(map->table[i].key);
            mp_obj_t val = map->table[i].value;
            if (key == MP_QSTR_dtype) {
                opts = torch_options_dtype(opts, cml_parse_dtype(val));
            } else if (key == MP_QSTR_device) {
                opts = torch_options_device(opts, cml_parse_device(val));
            } else if (key == MP_QSTR_requires_grad) {
                opts = torch_options_requires_grad(opts, mp_obj_is_true(val));
            }
        }
    }
    return opts;
}

mp_obj_t cml_dtype_to_str(DType dtype) {
    switch (dtype) {
        case DTYPE_FLOAT32: return MP_OBJ_NEW_QSTR(MP_QSTR_float32);
        case DTYPE_FLOAT64: return MP_OBJ_NEW_QSTR(MP_QSTR_float64);
        case DTYPE_INT32: return MP_OBJ_NEW_QSTR(MP_QSTR_int32);
        case DTYPE_INT64: return MP_OBJ_NEW_QSTR(MP_QSTR_int64);
        case DTYPE_BOOL: return MP_OBJ_NEW_QSTR(MP_QSTR_bool);
        case DTYPE_FLOAT16: return MP_OBJ_NEW_QSTR(MP_QSTR_float16);
        case DTYPE_BFLOAT16: return MP_OBJ_NEW_QSTR(MP_QSTR_bfloat16);
        default: return MP_OBJ_NEW_QSTR(MP_QSTR_unknown);
    }
}

mp_obj_t cml_device_to_str(DeviceType device) {
    switch (device) {
        case DEVICE_CPU: return MP_OBJ_NEW_QSTR(MP_QSTR_cpu);
        case DEVICE_CUDA: return MP_OBJ_NEW_QSTR(MP_QSTR_cuda);
        case DEVICE_METAL: return MP_OBJ_NEW_QSTR(MP_QSTR_metal);
        case DEVICE_ROCM: return MP_OBJ_NEW_QSTR(MP_QSTR_rocm);
        case DEVICE_OPENCL: return MP_OBJ_NEW_QSTR(MP_QSTR_opencl);
        case DEVICE_AUTO: return MP_OBJ_NEW_QSTR(MP_QSTR_auto);
        default: return MP_OBJ_NEW_QSTR(MP_QSTR_unknown);
    }
}

DType cml_parse_dtype(mp_obj_t obj) {
    if (mp_obj_is_int(obj)) {
        return (DType)mp_obj_get_int(obj);
    }
    if (mp_obj_is_str(obj)) {
        size_t len = 0;
        const char *s = mp_obj_str_get_data(obj, &len);
        if (len == 7 && memcmp(s, "float32", 7) == 0) return DTYPE_FLOAT32;
        if (len == 7 && memcmp(s, "float64", 7) == 0) return DTYPE_FLOAT64;
        if (len == 5 && memcmp(s, "int32", 5) == 0) return DTYPE_INT32;
        if (len == 5 && memcmp(s, "int64", 5) == 0) return DTYPE_INT64;
        if (len == 4 && memcmp(s, "bool", 4) == 0) return DTYPE_BOOL;
        if (len == 7 && memcmp(s, "float16", 7) == 0) return DTYPE_FLOAT16;
        if (len == 8 && memcmp(s, "bfloat16", 8) == 0) return DTYPE_BFLOAT16;
    }
    mp_raise_ValueError(MP_ERROR_TEXT("unknown dtype"));
    return DTYPE_FLOAT32;
}

DeviceType cml_parse_device(mp_obj_t obj) {
    if (mp_obj_is_int(obj)) {
        return (DeviceType)mp_obj_get_int(obj);
    }
    if (mp_obj_is_str(obj)) {
        size_t len = 0;
        const char *s = mp_obj_str_get_data(obj, &len);
        if (len == 3 && memcmp(s, "cpu", 3) == 0) return DEVICE_CPU;
        if (len == 4 && memcmp(s, "cuda", 4) == 0) return DEVICE_CUDA;
        if (len == 5 && memcmp(s, "metal", 5) == 0) return DEVICE_METAL;
        if (len == 4 && memcmp(s, "rocm", 4) == 0) return DEVICE_ROCM;
        if (len == 6 && memcmp(s, "opencl", 6) == 0) return DEVICE_OPENCL;
        if (len == 4 && memcmp(s, "auto", 4) == 0) return DEVICE_AUTO;
    }
    mp_raise_ValueError(MP_ERROR_TEXT("unknown device"));
    return DEVICE_CPU;
}

mp_obj_t cml_shape_to_tuple(const Tensor *t) {
    const int *sizes = torch_tensor_sizes(t);
    int ndim = torch_tensor_ndim(t);
    mp_obj_t *items = m_new(mp_obj_t, ndim);
    for (int i = 0; i < ndim; ++i) {
        items[i] = mp_obj_new_int(sizes[i]);
    }
    return mp_obj_new_tuple(ndim, items);
}

static mp_obj_t cml_list_from_tensor_dim(Tensor *tensor, int dim, size_t *prefix, int prefix_len) {
    const int *sizes = torch_tensor_sizes(tensor);
    int ndim = torch_tensor_ndim(tensor);

    if (dim >= ndim) {
        size_t idx = 0;
        size_t stride = 1;
        for (int d = ndim - 1; d >= 0; --d) {
            idx += prefix[d] * stride;
            stride *= (size_t)sizes[d];
        }
        return mp_obj_new_float(tensor_get_float(tensor, idx));
    }

    mp_obj_t list = mp_obj_new_list(0, NULL);
    for (int i = 0; i < sizes[dim]; ++i) {
        prefix[dim] = (size_t)i;
        mp_obj_list_append(list, cml_list_from_tensor_dim(tensor, dim + 1, prefix, prefix_len));
    }
    return list;
}

mp_obj_t cml_tensor_to_mp_list(Tensor *tensor) {
    cml_require_init();
    if (tensor == NULL) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("null tensor"));
    }
    if (torch_tensor_ndim(tensor) == 0) {
        return mp_obj_new_float(torch_tensor_item_float(tensor));
    }
    size_t prefix[CML_MAX_NDIM] = {0};
    return cml_list_from_tensor_dim(tensor, 0, prefix, CML_MAX_NDIM);
}

static size_t cml_list_fill_shape(mp_obj_t data, int *shape, int ndim, int dim) {
    if (dim >= ndim) {
        return 1;
    }
    if (!mp_obj_is_type(data, &mp_type_list)) {
        mp_raise_TypeError(MP_ERROR_TEXT("expected nested list"));
    }
    size_t n = 0;
    mp_obj_t *items = NULL;
    mp_obj_get_array(data, &n, &items);
    if (n == 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("empty tensor data"));
    }
    shape[dim] = (int)n;
    return n * cml_list_fill_shape(items[0], shape, ndim, dim + 1);
}

static size_t cml_flatten_list(mp_obj_t data, float *out, size_t offset, int ndim, int dim) {
    if (dim >= ndim) {
        out[offset] = (float)mp_obj_get_float(data);
        return offset + 1;
    }

    if (!mp_obj_is_type(data, &mp_type_list)) {
        mp_raise_TypeError(MP_ERROR_TEXT("expected nested list"));
    }

    size_t n = 0;
    mp_obj_t *items = NULL;
    mp_obj_get_array(data, &n, &items);
    size_t pos = offset;
    for (size_t i = 0; i < n; ++i) {
        pos = cml_flatten_list(items[i], out, pos, ndim, dim + 1);
    }
    return pos;
}

static int cml_infer_ndim(mp_obj_t data) {
    if (mp_obj_is_type(data, &mp_type_list)) {
        size_t n = 0;
        mp_obj_t *items = NULL;
        mp_obj_get_array(data, &n, &items);
        if (n == 0) {
            return 1;
        }
        return 1 + cml_infer_ndim(items[0]);
    }
    return 0;
}

Tensor *cml_tensor_from_mp(mp_obj_t obj, bool *owns_out) {
    cml_require_init();
    if (cml_is_tensor(obj)) {
        if (owns_out) {
            *owns_out = false;
        }
        return cml_as_tensor(obj)->tensor;
    }

    if (!mp_obj_is_type(obj, &mp_type_list)) {
        mp_raise_TypeError(MP_ERROR_TEXT("expected Tensor or list"));
    }

    int ndim = cml_infer_ndim(obj);
    if (ndim <= 0 || ndim > CML_MAX_NDIM) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid tensor data"));
    }

    int shape[CML_MAX_NDIM];
    size_t numel = cml_list_fill_shape(obj, shape, ndim, 0);

    float *data = m_new(float, numel);
    size_t written = cml_flatten_list(obj, data, 0, ndim, 0);
    if (written != numel) {
        m_del(float, data, numel);
        mp_raise_ValueError(MP_ERROR_TEXT("tensor data size mismatch"));
    }

    TorchTensorOptions opts = cml_default_options();
    Tensor *tensor = torch_from_blob(data, shape, ndim, &opts);
    if (tensor == NULL) {
        m_del(float, data, numel);
        cml_raise_if_error();
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("tensor creation failed"));
    }
    if (owns_out) {
        *owns_out = true;
    }
    return tensor;
}

Tensor **cml_parse_tensor_list(mp_obj_t list_in, int *count_out) {
    if (!mp_obj_is_type(list_in, &mp_type_list)) {
        mp_raise_TypeError(MP_ERROR_TEXT("expected list of Tensor"));
    }
    size_t n = 0;
    mp_obj_t *items = NULL;
    mp_obj_get_array(list_in, &n, &items);
    if (n == 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("empty tensor list"));
    }
    Tensor **out = m_new(Tensor *, n);
    for (size_t i = 0; i < n; ++i) {
        out[i] = cml_as_tensor(items[i])->tensor;
    }
    *count_out = (int)n;
    return out;
}

mp_obj_t cml_do_binary(mp_obj_t a, mp_obj_t b, Tensor *(*op)(Tensor *, Tensor *)) {
    cml_require_init();
    Tensor *ta = cml_tensor_from_mp(a, NULL);
    Tensor *tb = cml_tensor_from_mp(b, NULL);
    Tensor *out = op(ta, tb);
    cml_raise_if_error();
    return cml_new_tensor(out, true);
}

mp_obj_t cml_do_unary(mp_obj_t a, Tensor *(*op)(Tensor *)) {
    cml_require_init();
    Tensor *ta = cml_tensor_from_mp(a, NULL);
    Tensor *out = op(ta);
    cml_raise_if_error();
    return cml_new_tensor(out, true);
}

mp_obj_t cml_do_unary_dim(mp_obj_t a, mp_obj_t dim, mp_obj_t keepdim,
    Tensor *(*op)(Tensor *, int, bool)) {
    cml_require_init();
    Tensor *ta = cml_tensor_from_mp(a, NULL);
    Tensor *out = op(ta, mp_obj_get_int(dim), mp_obj_is_true(keepdim));
    cml_raise_if_error();
    return cml_new_tensor(out, true);
}

mp_obj_t cml_do_unary_dim_only(mp_obj_t a, mp_obj_t dim, Tensor *(*op)(Tensor *, int)) {
    cml_require_init();
    Tensor *ta = cml_tensor_from_mp(a, NULL);
    Tensor *out = op(ta, mp_obj_get_int(dim));
    cml_raise_if_error();
    return cml_new_tensor(out, true);
}
