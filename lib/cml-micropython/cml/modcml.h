#ifndef MICROPY_INCLUDED_CML_MODCML_H
#define MICROPY_INCLUDED_CML_MODCML_H

#include "py/obj.h"
#include "py/objlist.h"
#include "py/runtime.h"

#include "cml.h"
#include "torch/torch_c.h"
#include "torch/torch_eager.h"

#define CML_MAX_NDIM 8

extern const mp_obj_type_t cml_type_Tensor;
extern const mp_obj_type_t cml_type_Module;
extern const mp_obj_type_t cml_type_Optimizer;
extern const mp_obj_type_t cml_type_StateDict;
extern const mp_obj_type_t cml_type_RuntimeModule;

typedef struct _cml_tensor_obj_t {
    mp_obj_base_t base;
    Tensor *tensor;
    bool owns;
} cml_tensor_obj_t;

typedef struct _cml_module_obj_t {
    mp_obj_base_t base;
    Module *module;
    bool owns;
} cml_module_obj_t;

typedef struct _cml_optim_obj_t {
    mp_obj_base_t base;
    Optimizer *optimizer;
} cml_optim_obj_t;

typedef struct _cml_statedict_obj_t {
    mp_obj_base_t base;
    StateDict *sd;
} cml_statedict_obj_t;

typedef struct _cml_runtime_obj_t {
    mp_obj_base_t base;
    TorchRuntimeModule *runtime;
} cml_runtime_obj_t;

extern bool cml_ready;

void cml_require_init(void);
void cml_raise_if_error(void);

mp_obj_t cml_new_tensor(Tensor *t, bool owns);
mp_obj_t cml_new_module(Module *m, bool owns);
mp_obj_t cml_new_optimizer(Optimizer *o);
mp_obj_t cml_new_statedict(StateDict *sd);
mp_obj_t cml_new_runtime(TorchRuntimeModule *rt);

cml_tensor_obj_t *cml_as_tensor(mp_obj_t obj);
cml_module_obj_t *cml_as_module(mp_obj_t obj);
cml_optim_obj_t *cml_as_optimizer(mp_obj_t obj);
cml_statedict_obj_t *cml_as_statedict(mp_obj_t obj);
cml_runtime_obj_t *cml_as_runtime(mp_obj_t obj);

bool cml_is_tensor(mp_obj_t obj);
Tensor *cml_tensor_from_mp(mp_obj_t obj, bool *owns_out);

int cml_parse_shape(mp_obj_t shape_in, int *shape_out, size_t max_ndim);
TorchTensorOptions cml_parse_options(mp_obj_t opts_in);
mp_obj_t cml_tensor_to_mp_list(Tensor *tensor);
mp_obj_t cml_shape_to_tuple(const Tensor *t);
mp_obj_t cml_dtype_to_str(DType dtype);
mp_obj_t cml_device_to_str(DeviceType device);
DType cml_parse_dtype(mp_obj_t obj);
DeviceType cml_parse_device(mp_obj_t obj);

Tensor **cml_parse_tensor_list(mp_obj_t list_in, int *count_out);

mp_obj_t cml_do_binary(mp_obj_t a, mp_obj_t b, Tensor *(*op)(Tensor *, Tensor *));
mp_obj_t cml_do_unary(mp_obj_t a, Tensor *(*op)(Tensor *));
mp_obj_t cml_do_unary_dim(mp_obj_t a, mp_obj_t dim, mp_obj_t keepdim,
    Tensor *(*op)(Tensor *, int, bool));
mp_obj_t cml_do_unary_dim_only(mp_obj_t a, mp_obj_t dim, Tensor *(*op)(Tensor *, int));

#endif
