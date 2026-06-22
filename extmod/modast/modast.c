/*
 * MicroPython _ast module.
 */
#include "py/runtime.h"
#include "py/builtin.h"
#include "extmod/modast/ast_types.h"

#if MICROPY_PY_AST

static mp_obj_t mod_ast_parse(size_t n_args, const mp_obj_t *args) {
    mp_arg_check_num(n_args, 0, 1, 3, false);
    mp_obj_t source = args[0];
    mp_obj_t filename = n_args >= 2 ? args[1] : mp_obj_new_str("<unknown>", 9);
    mp_obj_t mode = n_args >= 3 ? args[2] : MP_OBJ_NEW_QSTR(MP_QSTR_exec);
    if (!mp_obj_is_str(source)) {
        mp_raise_TypeError(MP_ERROR_TEXT("source must be str"));
    }
    if (!mp_obj_is_str(filename)) {
        mp_raise_TypeError(MP_ERROR_TEXT("filename must be str"));
    }
    if (!mp_obj_is_str(mode)) {
        mp_raise_TypeError(MP_ERROR_TEXT("mode must be str"));
    }
    return mp_ast_parse_to_obj(source, filename, mode);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_ast_parse_obj, 1, 3, mod_ast_parse);

static const mp_rom_map_elem_t mod_ast_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR__ast) },
    { MP_ROM_QSTR(MP_QSTR_parse), MP_ROM_PTR(&mod_ast_parse_obj) },
    { MP_ROM_QSTR(MP_QSTR_PyCF_ONLY_AST), MP_ROM_INT(0x400) },
    { MP_ROM_QSTR(MP_QSTR_AST), MP_ROM_PTR(&mp_type_ast_AST) },
    { MP_ROM_QSTR(MP_QSTR_mod), MP_ROM_PTR(&mp_type_ast_mod) },
    { MP_ROM_QSTR(MP_QSTR_Module), MP_ROM_PTR(&mp_type_ast_Module) },
    { MP_ROM_QSTR(MP_QSTR_Interactive), MP_ROM_PTR(&mp_type_ast_Interactive) },
    { MP_ROM_QSTR(MP_QSTR_Expression), MP_ROM_PTR(&mp_type_ast_Expression) },
    { MP_ROM_QSTR(MP_QSTR_FunctionType), MP_ROM_PTR(&mp_type_ast_FunctionType) },
    { MP_ROM_QSTR(MP_QSTR_stmt), MP_ROM_PTR(&mp_type_ast_stmt) },
    { MP_ROM_QSTR(MP_QSTR_FunctionDef), MP_ROM_PTR(&mp_type_ast_FunctionDef) },
    { MP_ROM_QSTR(MP_QSTR_AsyncFunctionDef), MP_ROM_PTR(&mp_type_ast_AsyncFunctionDef) },
    { MP_ROM_QSTR(MP_QSTR_ClassDef), MP_ROM_PTR(&mp_type_ast_ClassDef) },
    { MP_ROM_QSTR(MP_QSTR_Return), MP_ROM_PTR(&mp_type_ast_Return) },
    { MP_ROM_QSTR(MP_QSTR_Delete), MP_ROM_PTR(&mp_type_ast_Delete) },
    { MP_ROM_QSTR(MP_QSTR_Assign), MP_ROM_PTR(&mp_type_ast_Assign) },
    { MP_ROM_QSTR(MP_QSTR_AugAssign), MP_ROM_PTR(&mp_type_ast_AugAssign) },
    { MP_ROM_QSTR(MP_QSTR_AnnAssign), MP_ROM_PTR(&mp_type_ast_AnnAssign) },
    { MP_ROM_QSTR(MP_QSTR_For), MP_ROM_PTR(&mp_type_ast_For) },
    { MP_ROM_QSTR(MP_QSTR_AsyncFor), MP_ROM_PTR(&mp_type_ast_AsyncFor) },
    { MP_ROM_QSTR(MP_QSTR_While), MP_ROM_PTR(&mp_type_ast_While) },
    { MP_ROM_QSTR(MP_QSTR_If), MP_ROM_PTR(&mp_type_ast_If) },
    { MP_ROM_QSTR(MP_QSTR_With), MP_ROM_PTR(&mp_type_ast_With) },
    { MP_ROM_QSTR(MP_QSTR_AsyncWith), MP_ROM_PTR(&mp_type_ast_AsyncWith) },
    { MP_ROM_QSTR(MP_QSTR_Raise), MP_ROM_PTR(&mp_type_ast_Raise) },
    { MP_ROM_QSTR(MP_QSTR_Try), MP_ROM_PTR(&mp_type_ast_Try) },
    { MP_ROM_QSTR(MP_QSTR_Assert), MP_ROM_PTR(&mp_type_ast_Assert) },
    { MP_ROM_QSTR(MP_QSTR_Import), MP_ROM_PTR(&mp_type_ast_Import) },
    { MP_ROM_QSTR(MP_QSTR_ImportFrom), MP_ROM_PTR(&mp_type_ast_ImportFrom) },
    { MP_ROM_QSTR(MP_QSTR_Global), MP_ROM_PTR(&mp_type_ast_Global) },
    { MP_ROM_QSTR(MP_QSTR_Nonlocal), MP_ROM_PTR(&mp_type_ast_Nonlocal) },
    { MP_ROM_QSTR(MP_QSTR_Expr), MP_ROM_PTR(&mp_type_ast_Expr) },
    { MP_ROM_QSTR(MP_QSTR_Pass), MP_ROM_PTR(&mp_type_ast_Pass) },
    { MP_ROM_QSTR(MP_QSTR_Break), MP_ROM_PTR(&mp_type_ast_Break) },
    { MP_ROM_QSTR(MP_QSTR_Continue), MP_ROM_PTR(&mp_type_ast_Continue) },
    { MP_ROM_QSTR(MP_QSTR_expr), MP_ROM_PTR(&mp_type_ast_expr) },
    { MP_ROM_QSTR(MP_QSTR_BoolOp), MP_ROM_PTR(&mp_type_ast_BoolOp) },
    { MP_ROM_QSTR(MP_QSTR_NamedExpr), MP_ROM_PTR(&mp_type_ast_NamedExpr) },
    { MP_ROM_QSTR(MP_QSTR_BinOp), MP_ROM_PTR(&mp_type_ast_BinOp) },
    { MP_ROM_QSTR(MP_QSTR_UnaryOp), MP_ROM_PTR(&mp_type_ast_UnaryOp) },
    { MP_ROM_QSTR(MP_QSTR_Lambda), MP_ROM_PTR(&mp_type_ast_Lambda) },
    { MP_ROM_QSTR(MP_QSTR_IfExp), MP_ROM_PTR(&mp_type_ast_IfExp) },
    { MP_ROM_QSTR(MP_QSTR_Dict), MP_ROM_PTR(&mp_type_ast_Dict) },
    { MP_ROM_QSTR(MP_QSTR_Set), MP_ROM_PTR(&mp_type_ast_Set) },
    { MP_ROM_QSTR(MP_QSTR_ListComp), MP_ROM_PTR(&mp_type_ast_ListComp) },
    { MP_ROM_QSTR(MP_QSTR_SetComp), MP_ROM_PTR(&mp_type_ast_SetComp) },
    { MP_ROM_QSTR(MP_QSTR_DictComp), MP_ROM_PTR(&mp_type_ast_DictComp) },
    { MP_ROM_QSTR(MP_QSTR_GeneratorExp), MP_ROM_PTR(&mp_type_ast_GeneratorExp) },
    { MP_ROM_QSTR(MP_QSTR_Await), MP_ROM_PTR(&mp_type_ast_Await) },
    { MP_ROM_QSTR(MP_QSTR_Yield), MP_ROM_PTR(&mp_type_ast_Yield) },
    { MP_ROM_QSTR(MP_QSTR_YieldFrom), MP_ROM_PTR(&mp_type_ast_YieldFrom) },
    { MP_ROM_QSTR(MP_QSTR_Compare), MP_ROM_PTR(&mp_type_ast_Compare) },
    { MP_ROM_QSTR(MP_QSTR_Call), MP_ROM_PTR(&mp_type_ast_Call) },
    { MP_ROM_QSTR(MP_QSTR_FormattedValue), MP_ROM_PTR(&mp_type_ast_FormattedValue) },
    { MP_ROM_QSTR(MP_QSTR_JoinedStr), MP_ROM_PTR(&mp_type_ast_JoinedStr) },
    { MP_ROM_QSTR(MP_QSTR_Constant), MP_ROM_PTR(&mp_type_ast_Constant) },
    { MP_ROM_QSTR(MP_QSTR_Attribute), MP_ROM_PTR(&mp_type_ast_Attribute) },
    { MP_ROM_QSTR(MP_QSTR_Subscript), MP_ROM_PTR(&mp_type_ast_Subscript) },
    { MP_ROM_QSTR(MP_QSTR_Starred), MP_ROM_PTR(&mp_type_ast_Starred) },
    { MP_ROM_QSTR(MP_QSTR_Name), MP_ROM_PTR(&mp_type_ast_Name) },
    { MP_ROM_QSTR(MP_QSTR_List), MP_ROM_PTR(&mp_type_ast_List) },
    { MP_ROM_QSTR(MP_QSTR_Tuple), MP_ROM_PTR(&mp_type_ast_Tuple) },
    { MP_ROM_QSTR(MP_QSTR_Slice), MP_ROM_PTR(&mp_type_ast_Slice) },
    { MP_ROM_QSTR(MP_QSTR_expr_context), MP_ROM_PTR(&mp_type_ast_expr_context) },
    { MP_ROM_QSTR(MP_QSTR_Load), MP_ROM_PTR(&mp_type_ast_Load) },
    { MP_ROM_QSTR(MP_QSTR_Store), MP_ROM_PTR(&mp_type_ast_Store) },
    { MP_ROM_QSTR(MP_QSTR_Del), MP_ROM_PTR(&mp_type_ast_Del) },
    { MP_ROM_QSTR(MP_QSTR_boolop), MP_ROM_PTR(&mp_type_ast_boolop) },
    { MP_ROM_QSTR(MP_QSTR_And), MP_ROM_PTR(&mp_type_ast_And) },
    { MP_ROM_QSTR(MP_QSTR_Or), MP_ROM_PTR(&mp_type_ast_Or) },
    { MP_ROM_QSTR(MP_QSTR_operator), MP_ROM_PTR(&mp_type_ast_operator) },
    { MP_ROM_QSTR(MP_QSTR_Add), MP_ROM_PTR(&mp_type_ast_Add) },
    { MP_ROM_QSTR(MP_QSTR_Sub), MP_ROM_PTR(&mp_type_ast_Sub) },
    { MP_ROM_QSTR(MP_QSTR_Mult), MP_ROM_PTR(&mp_type_ast_Mult) },
    { MP_ROM_QSTR(MP_QSTR_MatMult), MP_ROM_PTR(&mp_type_ast_MatMult) },
    { MP_ROM_QSTR(MP_QSTR_Div), MP_ROM_PTR(&mp_type_ast_Div) },
    { MP_ROM_QSTR(MP_QSTR_Mod), MP_ROM_PTR(&mp_type_ast_Mod) },
    { MP_ROM_QSTR(MP_QSTR_Pow), MP_ROM_PTR(&mp_type_ast_Pow) },
    { MP_ROM_QSTR(MP_QSTR_LShift), MP_ROM_PTR(&mp_type_ast_LShift) },
    { MP_ROM_QSTR(MP_QSTR_RShift), MP_ROM_PTR(&mp_type_ast_RShift) },
    { MP_ROM_QSTR(MP_QSTR_BitOr), MP_ROM_PTR(&mp_type_ast_BitOr) },
    { MP_ROM_QSTR(MP_QSTR_BitXor), MP_ROM_PTR(&mp_type_ast_BitXor) },
    { MP_ROM_QSTR(MP_QSTR_BitAnd), MP_ROM_PTR(&mp_type_ast_BitAnd) },
    { MP_ROM_QSTR(MP_QSTR_FloorDiv), MP_ROM_PTR(&mp_type_ast_FloorDiv) },
    { MP_ROM_QSTR(MP_QSTR_unaryop), MP_ROM_PTR(&mp_type_ast_unaryop) },
    { MP_ROM_QSTR(MP_QSTR_Invert), MP_ROM_PTR(&mp_type_ast_Invert) },
    { MP_ROM_QSTR(MP_QSTR_Not), MP_ROM_PTR(&mp_type_ast_Not) },
    { MP_ROM_QSTR(MP_QSTR_UAdd), MP_ROM_PTR(&mp_type_ast_UAdd) },
    { MP_ROM_QSTR(MP_QSTR_USub), MP_ROM_PTR(&mp_type_ast_USub) },
    { MP_ROM_QSTR(MP_QSTR_cmpop), MP_ROM_PTR(&mp_type_ast_cmpop) },
    { MP_ROM_QSTR(MP_QSTR_Eq), MP_ROM_PTR(&mp_type_ast_Eq) },
    { MP_ROM_QSTR(MP_QSTR_NotEq), MP_ROM_PTR(&mp_type_ast_NotEq) },
    { MP_ROM_QSTR(MP_QSTR_Lt), MP_ROM_PTR(&mp_type_ast_Lt) },
    { MP_ROM_QSTR(MP_QSTR_LtE), MP_ROM_PTR(&mp_type_ast_LtE) },
    { MP_ROM_QSTR(MP_QSTR_Gt), MP_ROM_PTR(&mp_type_ast_Gt) },
    { MP_ROM_QSTR(MP_QSTR_GtE), MP_ROM_PTR(&mp_type_ast_GtE) },
    { MP_ROM_QSTR(MP_QSTR_Is), MP_ROM_PTR(&mp_type_ast_Is) },
    { MP_ROM_QSTR(MP_QSTR_IsNot), MP_ROM_PTR(&mp_type_ast_IsNot) },
    { MP_ROM_QSTR(MP_QSTR_In), MP_ROM_PTR(&mp_type_ast_In) },
    { MP_ROM_QSTR(MP_QSTR_NotIn), MP_ROM_PTR(&mp_type_ast_NotIn) },
    { MP_ROM_QSTR(MP_QSTR_comprehension), MP_ROM_PTR(&mp_type_ast_comprehension) },
    { MP_ROM_QSTR(MP_QSTR_excepthandler), MP_ROM_PTR(&mp_type_ast_excepthandler) },
    { MP_ROM_QSTR(MP_QSTR_ExceptHandler), MP_ROM_PTR(&mp_type_ast_ExceptHandler) },
    { MP_ROM_QSTR(MP_QSTR_arguments), MP_ROM_PTR(&mp_type_ast_arguments) },
    { MP_ROM_QSTR(MP_QSTR_arg), MP_ROM_PTR(&mp_type_ast_arg) },
    { MP_ROM_QSTR(MP_QSTR_keyword), MP_ROM_PTR(&mp_type_ast_keyword) },
    { MP_ROM_QSTR(MP_QSTR_alias), MP_ROM_PTR(&mp_type_ast_alias) },
    { MP_ROM_QSTR(MP_QSTR_withitem), MP_ROM_PTR(&mp_type_ast_withitem) },
    { MP_ROM_QSTR(MP_QSTR_type_ignore), MP_ROM_PTR(&mp_type_ast_type_ignore) },
    { MP_ROM_QSTR(MP_QSTR_TypeIgnore), MP_ROM_PTR(&mp_type_ast_TypeIgnore) },
};
static MP_DEFINE_CONST_DICT(mod_ast_globals, mod_ast_globals_table);

const mp_obj_module_t mp_module__ast = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mod_ast_globals,
};

MP_REGISTER_MODULE(MP_QSTR__ast, mp_module__ast);

#endif // MICROPY_PY_AST
