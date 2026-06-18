// Forward declarations for module-level function objects defined in other translation units.
#define CML_DECL_VAR(obj) extern const mp_obj_fun_builtin_var_t obj;
#define CML_DECL2(obj) extern const mp_obj_fun_builtin_fixed_t obj;
#define CML_DECL1(obj) extern const mp_obj_fun_builtin_fixed_t obj;
#define CML_DECL0(obj) extern const mp_obj_fun_builtin_fixed_t obj;
#define CML_DECL3(obj) extern const mp_obj_fun_builtin_fixed_t obj;

CML_DECL_VAR(cml_empty_fn_obj)
CML_DECL_VAR(cml_zeros_fn_obj)
CML_DECL_VAR(cml_ones_fn_obj)
CML_DECL_VAR(cml_rand_fn_obj)
CML_DECL_VAR(cml_randn_fn_obj)
CML_DECL_VAR(cml_full_fn_obj)
CML_DECL_VAR(cml_eye_fn_obj)
CML_DECL_VAR(cml_arange_fn_obj)
CML_DECL_VAR(cml_linspace_fn_obj)
CML_DECL1(cml_tensor_fn_obj)
CML_DECL1(cml_zeros_like_fn_obj)
CML_DECL1(cml_ones_like_fn_obj)
CML_DECL1(cml_randn_like_fn_obj)
CML_DECL2(cml_add_fn_obj)
CML_DECL2(cml_sub_fn_obj)
CML_DECL2(cml_mul_fn_obj)
CML_DECL2(cml_div_fn_obj)
CML_DECL2(cml_matmul_fn_obj)
CML_DECL2(cml_pow_fn_obj)
CML_DECL_VAR(cml_sum_fn_obj)
CML_DECL_VAR(cml_mean_fn_obj)
CML_DECL_VAR(cml_max_fn_obj)
CML_DECL_VAR(cml_min_fn_obj)
CML_DECL1(cml_relu_fn_obj)
CML_DECL1(cml_sigmoid_fn_obj)
CML_DECL1(cml_tanh_fn_obj)
CML_DECL1(cml_gelu_fn_obj)
CML_DECL2(cml_softmax_fn_obj)
CML_DECL2(cml_reshape_fn_obj)
CML_DECL3(cml_transpose_fn_obj)
CML_DECL2(cml_squeeze_fn_obj)
CML_DECL2(cml_unsqueeze_fn_obj)
CML_DECL2(cml_cat_fn_obj)
CML_DECL2(cml_stack_fn_obj)
CML_DECL1(cml_clone_fn_obj)
CML_DECL1(cml_detach_fn_obj)
CML_DECL1(cml_contiguous_fn_obj)
CML_DECL_VAR(cml_linear_fn_obj)
CML_DECL_VAR(cml_linear_relu_fn_obj)
CML_DECL_VAR(cml_nn_linear_fn_obj)
CML_DECL0(cml_nn_relu_fn_obj)
CML_DECL0(cml_nn_sequential_fn_obj)
CML_DECL2(cml_mse_loss_fn_obj)
CML_DECL2(cml_cross_entropy_loss_fn_obj)
CML_DECL_VAR(cml_backward_fn_obj)
CML_DECL1(cml_zero_grad_tensor_fn_obj)
CML_DECL0(cml_no_grad_fn_obj)
CML_DECL0(cml_enable_grad_fn_obj)
CML_DECL0(cml_is_grad_enabled_fn_obj)
CML_DECL1(cml_set_eager_mode_fn_obj)
CML_DECL0(cml_is_eager_mode_fn_obj)
CML_DECL1(cml_inference_mode_fn_obj)
CML_DECL1(cml_set_num_threads_fn_obj)
CML_DECL0(cml_get_num_threads_fn_obj)
CML_DECL0(cml_reset_ir_fn_obj)
CML_DECL0(cml_reset_ir_soft_fn_obj)
CML_DECL_VAR(cml_optim_adam_fn_obj)
CML_DECL_VAR(cml_optim_sgd_fn_obj)
CML_DECL1(cml_runtime_from_module_fn_obj)
CML_DECL1(cml_runtime_load_aot_fn_obj)
CML_DECL1(cml_runtime_load_pte_fn_obj)
CML_DECL_VAR(cml_runtime_export_pte_fn_obj)

#undef CML_DECL_VAR
#undef CML_DECL2
#undef CML_DECL1
#undef CML_DECL0
#undef CML_DECL3

extern const mp_obj_type_t cml_type_Tensor;
extern const mp_obj_type_t cml_type_Module;
extern const mp_obj_type_t cml_type_Optimizer;
extern const mp_obj_type_t cml_type_StateDict;
extern const mp_obj_type_t cml_type_RuntimeModule;
