/*
 * MicroPython _ast node types.
 */
#include <string.h>
#include "py/obj.h"
#include "py/objlist.h"
#include "py/objstr.h"
#include "py/objtuple.h"
#include "py/runtime.h"
#include "extmod/modast/ast_types.h"

#if MICROPY_PY_AST


static const qstr ast_Module_fields[] = {
    MP_QSTR_body,
    MP_QSTR_type_ignores,
};

static const qstr ast_Interactive_fields[] = {
    MP_QSTR_body,
};

static const qstr ast_Expression_fields[] = {
    MP_QSTR_body,
};

static const qstr ast_FunctionType_fields[] = {
    MP_QSTR_argtypes,
    MP_QSTR_returns,
};

static const qstr ast_FunctionDef_fields[] = {
    MP_QSTR_name,
    MP_QSTR_args,
    MP_QSTR_body,
    MP_QSTR_decorator_list,
    MP_QSTR_returns,
    MP_QSTR_type_comment,
    MP_QSTR_type_params,
};

static const qstr ast_AsyncFunctionDef_fields[] = {
    MP_QSTR_name,
    MP_QSTR_args,
    MP_QSTR_body,
    MP_QSTR_decorator_list,
    MP_QSTR_returns,
    MP_QSTR_type_comment,
    MP_QSTR_type_params,
};

static const qstr ast_ClassDef_fields[] = {
    MP_QSTR_name,
    MP_QSTR_bases,
    MP_QSTR_keywords,
    MP_QSTR_body,
    MP_QSTR_decorator_list,
    MP_QSTR_type_params,
};

static const qstr ast_Return_fields[] = {
    MP_QSTR_value,
};

static const qstr ast_Delete_fields[] = {
    MP_QSTR_targets,
};

static const qstr ast_Assign_fields[] = {
    MP_QSTR_targets,
    MP_QSTR_value,
    MP_QSTR_type_comment,
};

static const qstr ast_AugAssign_fields[] = {
    MP_QSTR_target,
    MP_QSTR_op,
    MP_QSTR_value,
};

static const qstr ast_AnnAssign_fields[] = {
    MP_QSTR_target,
    MP_QSTR_annotation,
    MP_QSTR_value,
    MP_QSTR_simple,
};

static const qstr ast_For_fields[] = {
    MP_QSTR_target,
    MP_QSTR_iter,
    MP_QSTR_body,
    MP_QSTR_orelse,
    MP_QSTR_type_comment,
};

static const qstr ast_AsyncFor_fields[] = {
    MP_QSTR_target,
    MP_QSTR_iter,
    MP_QSTR_body,
    MP_QSTR_orelse,
    MP_QSTR_type_comment,
};

static const qstr ast_While_fields[] = {
    MP_QSTR_test,
    MP_QSTR_body,
    MP_QSTR_orelse,
};

static const qstr ast_If_fields[] = {
    MP_QSTR_test,
    MP_QSTR_body,
    MP_QSTR_orelse,
};

static const qstr ast_With_fields[] = {
    MP_QSTR_items,
    MP_QSTR_body,
    MP_QSTR_type_comment,
};

static const qstr ast_AsyncWith_fields[] = {
    MP_QSTR_items,
    MP_QSTR_body,
    MP_QSTR_type_comment,
};

static const qstr ast_Raise_fields[] = {
    MP_QSTR_exc,
    MP_QSTR_cause,
};

static const qstr ast_Try_fields[] = {
    MP_QSTR_body,
    MP_QSTR_handlers,
    MP_QSTR_orelse,
    MP_QSTR_finalbody,
};

static const qstr ast_Assert_fields[] = {
    MP_QSTR_test,
    MP_QSTR_msg,
};

static const qstr ast_Import_fields[] = {
    MP_QSTR_names,
};

static const qstr ast_ImportFrom_fields[] = {
    MP_QSTR_module,
    MP_QSTR_names,
    MP_QSTR_level,
};

static const qstr ast_Global_fields[] = {
    MP_QSTR_names,
};

static const qstr ast_Nonlocal_fields[] = {
    MP_QSTR_names,
};

static const qstr ast_Expr_fields[] = {
    MP_QSTR_value,
};

static const qstr ast_BoolOp_fields[] = {
    MP_QSTR_op,
    MP_QSTR_values,
};

static const qstr ast_NamedExpr_fields[] = {
    MP_QSTR_target,
    MP_QSTR_value,
};

static const qstr ast_BinOp_fields[] = {
    MP_QSTR_left,
    MP_QSTR_op,
    MP_QSTR_right,
};

static const qstr ast_UnaryOp_fields[] = {
    MP_QSTR_op,
    MP_QSTR_operand,
};

static const qstr ast_Lambda_fields[] = {
    MP_QSTR_args,
    MP_QSTR_body,
};

static const qstr ast_IfExp_fields[] = {
    MP_QSTR_test,
    MP_QSTR_body,
    MP_QSTR_orelse,
};

static const qstr ast_Dict_fields[] = {
    MP_QSTR_keys,
    MP_QSTR_values,
};

static const qstr ast_Set_fields[] = {
    MP_QSTR_elts,
};

static const qstr ast_ListComp_fields[] = {
    MP_QSTR_elt,
    MP_QSTR_generators,
};

static const qstr ast_SetComp_fields[] = {
    MP_QSTR_elt,
    MP_QSTR_generators,
};

static const qstr ast_DictComp_fields[] = {
    MP_QSTR_key,
    MP_QSTR_value,
    MP_QSTR_generators,
};

static const qstr ast_GeneratorExp_fields[] = {
    MP_QSTR_elt,
    MP_QSTR_generators,
};

static const qstr ast_Await_fields[] = {
    MP_QSTR_value,
};

static const qstr ast_Yield_fields[] = {
    MP_QSTR_value,
};

static const qstr ast_YieldFrom_fields[] = {
    MP_QSTR_value,
};

static const qstr ast_Compare_fields[] = {
    MP_QSTR_left,
    MP_QSTR_ops,
    MP_QSTR_comparators,
};

static const qstr ast_Call_fields[] = {
    MP_QSTR_func,
    MP_QSTR_args,
    MP_QSTR_keywords,
};

static const qstr ast_FormattedValue_fields[] = {
    MP_QSTR_value,
    MP_QSTR_conversion,
    MP_QSTR_format_spec,
};

static const qstr ast_JoinedStr_fields[] = {
    MP_QSTR_values,
};

static const qstr ast_Constant_fields[] = {
    MP_QSTR_value,
    MP_QSTR_kind,
};

static const qstr ast_Attribute_fields[] = {
    MP_QSTR_value,
    MP_QSTR_attr,
    MP_QSTR_ctx,
};

static const qstr ast_Subscript_fields[] = {
    MP_QSTR_value,
    MP_QSTR_slice,
    MP_QSTR_ctx,
};

static const qstr ast_Starred_fields[] = {
    MP_QSTR_value,
    MP_QSTR_ctx,
};

static const qstr ast_Name_fields[] = {
    MP_QSTR_id,
    MP_QSTR_ctx,
};

static const qstr ast_List_fields[] = {
    MP_QSTR_elts,
    MP_QSTR_ctx,
};

static const qstr ast_Tuple_fields[] = {
    MP_QSTR_elts,
    MP_QSTR_ctx,
};

static const qstr ast_Slice_fields[] = {
    MP_QSTR_lower,
    MP_QSTR_upper,
    MP_QSTR_step,
};

static const qstr ast_comprehension_fields[] = {
    MP_QSTR_target,
    MP_QSTR_iter,
    MP_QSTR_ifs,
    MP_QSTR_is_async,
};

static const qstr ast_ExceptHandler_fields[] = {
    MP_QSTR_type,
    MP_QSTR_name,
    MP_QSTR_body,
};

static const qstr ast_arguments_fields[] = {
    MP_QSTR_posonlyargs,
    MP_QSTR_args,
    MP_QSTR_vararg,
    MP_QSTR_kwonlyargs,
    MP_QSTR_kw_defaults,
    MP_QSTR_kwarg,
    MP_QSTR_defaults,
};

static const qstr ast_arg_fields[] = {
    MP_QSTR_arg,
    MP_QSTR_annotation,
    MP_QSTR_type_comment,
};

static const qstr ast_keyword_fields[] = {
    MP_QSTR_arg,
    MP_QSTR_value,
};

static const qstr ast_alias_fields[] = {
    MP_QSTR_name,
    MP_QSTR_asname,
};

static const qstr ast_withitem_fields[] = {
    MP_QSTR_context_expr,
    MP_QSTR_optional_vars,
};

static const qstr ast_type_ignore_fields[] = {
    MP_QSTR_lineno,
    MP_QSTR_tag,
};

static const qstr ast_TypeIgnore_fields[] = {
    MP_QSTR_lineno,
    MP_QSTR_tag,
};

const qstr *const mp_ast_field_names[MP_AST_NUM_TYPES] = {
    [MP_AST_AST] = NULL,
    [MP_AST_mod] = NULL,
    [MP_AST_MODULE] = ast_Module_fields,
    [MP_AST_INTERACTIVE] = ast_Interactive_fields,
    [MP_AST_EXPRESSION] = ast_Expression_fields,
    [MP_AST_FUNCTIONTYPE] = ast_FunctionType_fields,
    [MP_AST_stmt] = NULL,
    [MP_AST_FUNCTIONDEF] = ast_FunctionDef_fields,
    [MP_AST_ASYNCFUNCTIONDEF] = ast_AsyncFunctionDef_fields,
    [MP_AST_CLASSDEF] = ast_ClassDef_fields,
    [MP_AST_RETURN] = ast_Return_fields,
    [MP_AST_DELETE] = ast_Delete_fields,
    [MP_AST_ASSIGN] = ast_Assign_fields,
    [MP_AST_AUGASSIGN] = ast_AugAssign_fields,
    [MP_AST_ANNASSIGN] = ast_AnnAssign_fields,
    [MP_AST_FOR] = ast_For_fields,
    [MP_AST_ASYNCFOR] = ast_AsyncFor_fields,
    [MP_AST_WHILE] = ast_While_fields,
    [MP_AST_IF] = ast_If_fields,
    [MP_AST_WITH] = ast_With_fields,
    [MP_AST_ASYNCWITH] = ast_AsyncWith_fields,
    [MP_AST_RAISE] = ast_Raise_fields,
    [MP_AST_TRY] = ast_Try_fields,
    [MP_AST_ASSERT] = ast_Assert_fields,
    [MP_AST_IMPORT] = ast_Import_fields,
    [MP_AST_IMPORTFROM] = ast_ImportFrom_fields,
    [MP_AST_GLOBAL] = ast_Global_fields,
    [MP_AST_NONLOCAL] = ast_Nonlocal_fields,
    [MP_AST_EXPR] = ast_Expr_fields,
    [MP_AST_PASS] = NULL,
    [MP_AST_BREAK] = NULL,
    [MP_AST_CONTINUE] = NULL,
    [MP_AST_expr] = NULL,
    [MP_AST_BOOLOP] = ast_BoolOp_fields,
    [MP_AST_NAMEDEXPR] = ast_NamedExpr_fields,
    [MP_AST_BINOP] = ast_BinOp_fields,
    [MP_AST_UNARYOP] = ast_UnaryOp_fields,
    [MP_AST_LAMBDA] = ast_Lambda_fields,
    [MP_AST_IFEXP] = ast_IfExp_fields,
    [MP_AST_DICT] = ast_Dict_fields,
    [MP_AST_SET] = ast_Set_fields,
    [MP_AST_LISTCOMP] = ast_ListComp_fields,
    [MP_AST_SETCOMP] = ast_SetComp_fields,
    [MP_AST_DICTCOMP] = ast_DictComp_fields,
    [MP_AST_GENERATOREXP] = ast_GeneratorExp_fields,
    [MP_AST_AWAIT] = ast_Await_fields,
    [MP_AST_YIELD] = ast_Yield_fields,
    [MP_AST_YIELDFROM] = ast_YieldFrom_fields,
    [MP_AST_COMPARE] = ast_Compare_fields,
    [MP_AST_CALL] = ast_Call_fields,
    [MP_AST_FORMATTEDVALUE] = ast_FormattedValue_fields,
    [MP_AST_JOINEDSTR] = ast_JoinedStr_fields,
    [MP_AST_CONSTANT] = ast_Constant_fields,
    [MP_AST_ATTRIBUTE] = ast_Attribute_fields,
    [MP_AST_SUBSCRIPT] = ast_Subscript_fields,
    [MP_AST_STARRED] = ast_Starred_fields,
    [MP_AST_NAME] = ast_Name_fields,
    [MP_AST_LIST] = ast_List_fields,
    [MP_AST_TUPLE] = ast_Tuple_fields,
    [MP_AST_SLICE] = ast_Slice_fields,
    [MP_AST_EXPR_CONTEXT] = NULL,
    [MP_AST_LOAD] = NULL,
    [MP_AST_STORE] = NULL,
    [MP_AST_DEL] = NULL,
    [MP_AST_boolop] = NULL,
    [MP_AST_AND] = NULL,
    [MP_AST_OR] = NULL,
    [MP_AST_operator] = NULL,
    [MP_AST_ADD] = NULL,
    [MP_AST_SUB] = NULL,
    [MP_AST_MULT] = NULL,
    [MP_AST_MATMULT] = NULL,
    [MP_AST_DIV] = NULL,
    [MP_AST_MOD] = NULL,
    [MP_AST_POW] = NULL,
    [MP_AST_LSHIFT] = NULL,
    [MP_AST_RSHIFT] = NULL,
    [MP_AST_BITOR] = NULL,
    [MP_AST_BITXOR] = NULL,
    [MP_AST_BITAND] = NULL,
    [MP_AST_FLOORDIV] = NULL,
    [MP_AST_unaryop] = NULL,
    [MP_AST_INVERT] = NULL,
    [MP_AST_NOT] = NULL,
    [MP_AST_UADD] = NULL,
    [MP_AST_USUB] = NULL,
    [MP_AST_cmpop] = NULL,
    [MP_AST_EQ] = NULL,
    [MP_AST_NOTEQ] = NULL,
    [MP_AST_LT] = NULL,
    [MP_AST_LTE] = NULL,
    [MP_AST_GT] = NULL,
    [MP_AST_GTE] = NULL,
    [MP_AST_IS] = NULL,
    [MP_AST_ISNOT] = NULL,
    [MP_AST_IN] = NULL,
    [MP_AST_NOTIN] = NULL,
    [MP_AST_COMPREHENSION] = ast_comprehension_fields,
    [MP_AST_excepthandler] = NULL,
    [MP_AST_EXCEPTHANDLER] = ast_ExceptHandler_fields,
    [MP_AST_ARGUMENTS] = ast_arguments_fields,
    [MP_AST_arg] = ast_arg_fields,
    [MP_AST_KEYWORD] = ast_keyword_fields,
    [MP_AST_alias] = ast_alias_fields,
    [MP_AST_WITHITEM] = ast_withitem_fields,
    [MP_AST_type_ignore] = ast_type_ignore_fields,
    [MP_AST_TYPEIGNORE] = ast_TypeIgnore_fields,
};

const uint8_t mp_ast_num_fields[MP_AST_NUM_TYPES] = {
    [MP_AST_AST] = 0,
    [MP_AST_mod] = 0,
    [MP_AST_MODULE] = 2,
    [MP_AST_INTERACTIVE] = 1,
    [MP_AST_EXPRESSION] = 1,
    [MP_AST_FUNCTIONTYPE] = 2,
    [MP_AST_stmt] = 0,
    [MP_AST_FUNCTIONDEF] = 7,
    [MP_AST_ASYNCFUNCTIONDEF] = 7,
    [MP_AST_CLASSDEF] = 6,
    [MP_AST_RETURN] = 1,
    [MP_AST_DELETE] = 1,
    [MP_AST_ASSIGN] = 3,
    [MP_AST_AUGASSIGN] = 3,
    [MP_AST_ANNASSIGN] = 4,
    [MP_AST_FOR] = 5,
    [MP_AST_ASYNCFOR] = 5,
    [MP_AST_WHILE] = 3,
    [MP_AST_IF] = 3,
    [MP_AST_WITH] = 3,
    [MP_AST_ASYNCWITH] = 3,
    [MP_AST_RAISE] = 2,
    [MP_AST_TRY] = 4,
    [MP_AST_ASSERT] = 2,
    [MP_AST_IMPORT] = 1,
    [MP_AST_IMPORTFROM] = 3,
    [MP_AST_GLOBAL] = 1,
    [MP_AST_NONLOCAL] = 1,
    [MP_AST_EXPR] = 1,
    [MP_AST_PASS] = 0,
    [MP_AST_BREAK] = 0,
    [MP_AST_CONTINUE] = 0,
    [MP_AST_expr] = 0,
    [MP_AST_BOOLOP] = 2,
    [MP_AST_NAMEDEXPR] = 2,
    [MP_AST_BINOP] = 3,
    [MP_AST_UNARYOP] = 2,
    [MP_AST_LAMBDA] = 2,
    [MP_AST_IFEXP] = 3,
    [MP_AST_DICT] = 2,
    [MP_AST_SET] = 1,
    [MP_AST_LISTCOMP] = 2,
    [MP_AST_SETCOMP] = 2,
    [MP_AST_DICTCOMP] = 3,
    [MP_AST_GENERATOREXP] = 2,
    [MP_AST_AWAIT] = 1,
    [MP_AST_YIELD] = 1,
    [MP_AST_YIELDFROM] = 1,
    [MP_AST_COMPARE] = 3,
    [MP_AST_CALL] = 3,
    [MP_AST_FORMATTEDVALUE] = 3,
    [MP_AST_JOINEDSTR] = 1,
    [MP_AST_CONSTANT] = 2,
    [MP_AST_ATTRIBUTE] = 3,
    [MP_AST_SUBSCRIPT] = 3,
    [MP_AST_STARRED] = 2,
    [MP_AST_NAME] = 2,
    [MP_AST_LIST] = 2,
    [MP_AST_TUPLE] = 2,
    [MP_AST_SLICE] = 3,
    [MP_AST_EXPR_CONTEXT] = 0,
    [MP_AST_LOAD] = 0,
    [MP_AST_STORE] = 0,
    [MP_AST_DEL] = 0,
    [MP_AST_boolop] = 0,
    [MP_AST_AND] = 0,
    [MP_AST_OR] = 0,
    [MP_AST_operator] = 0,
    [MP_AST_ADD] = 0,
    [MP_AST_SUB] = 0,
    [MP_AST_MULT] = 0,
    [MP_AST_MATMULT] = 0,
    [MP_AST_DIV] = 0,
    [MP_AST_MOD] = 0,
    [MP_AST_POW] = 0,
    [MP_AST_LSHIFT] = 0,
    [MP_AST_RSHIFT] = 0,
    [MP_AST_BITOR] = 0,
    [MP_AST_BITXOR] = 0,
    [MP_AST_BITAND] = 0,
    [MP_AST_FLOORDIV] = 0,
    [MP_AST_unaryop] = 0,
    [MP_AST_INVERT] = 0,
    [MP_AST_NOT] = 0,
    [MP_AST_UADD] = 0,
    [MP_AST_USUB] = 0,
    [MP_AST_cmpop] = 0,
    [MP_AST_EQ] = 0,
    [MP_AST_NOTEQ] = 0,
    [MP_AST_LT] = 0,
    [MP_AST_LTE] = 0,
    [MP_AST_GT] = 0,
    [MP_AST_GTE] = 0,
    [MP_AST_IS] = 0,
    [MP_AST_ISNOT] = 0,
    [MP_AST_IN] = 0,
    [MP_AST_NOTIN] = 0,
    [MP_AST_COMPREHENSION] = 4,
    [MP_AST_excepthandler] = 0,
    [MP_AST_EXCEPTHANDLER] = 3,
    [MP_AST_ARGUMENTS] = 7,
    [MP_AST_arg] = 3,
    [MP_AST_KEYWORD] = 2,
    [MP_AST_alias] = 2,
    [MP_AST_WITHITEM] = 2,
    [MP_AST_type_ignore] = 2,
    [MP_AST_TYPEIGNORE] = 2,
};

static mp_obj_t ast_type_fields[MP_AST_NUM_TYPES];
static mp_obj_t ast_type_attributes[MP_AST_NUM_TYPES];
static mp_obj_t ast_singletons[MP_AST_NUM_TYPES];
static void ast_node_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    mp_obj_ast_node_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] == MP_OBJ_NULL) {
        if (attr == MP_QSTR__fields) { dest[0] = ast_type_fields[self->type_id]; return; }
        if (attr == MP_QSTR__attributes) { dest[0] = ast_type_attributes[self->type_id]; return; }
        if (attr == MP_QSTR_lineno) { dest[0] = mp_obj_new_int(self->lineno); return; }
        if (attr == MP_QSTR_col_offset) { dest[0] = mp_obj_new_int(self->col_offset); return; }
        if (attr == MP_QSTR_end_lineno) { dest[0] = mp_obj_new_int(self->end_lineno); return; }
        if (attr == MP_QSTR_end_col_offset) { dest[0] = mp_obj_new_int(self->end_col_offset); return; }
        const qstr *fields = mp_ast_field_names[self->type_id];
        size_t n = mp_ast_num_fields[self->type_id];
        for (size_t i = 0; i < n; i++) {
            if (fields[i] == attr) { dest[0] = self->fields[i]; return; }
        }
    } else {
        if (attr == MP_QSTR_lineno) { self->lineno = mp_obj_get_int(dest[1]); dest[0] = MP_OBJ_NULL; return; }
        if (attr == MP_QSTR_col_offset) { self->col_offset = mp_obj_get_int(dest[1]); dest[0] = MP_OBJ_NULL; return; }
        if (attr == MP_QSTR_end_lineno) { self->end_lineno = mp_obj_get_int(dest[1]); dest[0] = MP_OBJ_NULL; return; }
        if (attr == MP_QSTR_end_col_offset) { self->end_col_offset = mp_obj_get_int(dest[1]); dest[0] = MP_OBJ_NULL; return; }
        const qstr *fields = mp_ast_field_names[self->type_id];
        size_t n = mp_ast_num_fields[self->type_id];
        for (size_t i = 0; i < n; i++) {
            if (fields[i] == attr) { self->fields[i] = dest[1]; dest[0] = MP_OBJ_NULL; return; }
        }
    }
}

static void ast_node_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    mp_obj_ast_node_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "<%q at %p>", mp_ast_types[self->type_id]->name, self);
}

static mp_obj_t ast_node_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    uint16_t type_id = MP_AST_NUM_TYPES;
    for (uint16_t i = 0; i < MP_AST_NUM_TYPES; i++) {
        if (mp_ast_types[i] == type) {
            type_id = i;
            break;
        }
    }
    if (type_id >= MP_AST_NUM_TYPES) {
        mp_raise_TypeError(MP_ERROR_TEXT("unknown AST type"));
    }

    size_t n_fields = mp_ast_num_fields[type_id];
    const qstr *field_names = mp_ast_field_names[type_id];
    mp_obj_t fields_buf[16];
    mp_obj_t *fields = n_fields <= 16 ? fields_buf : m_new(mp_obj_t, n_fields);
    for (size_t i = 0; i < n_fields; i++) {
        fields[i] = mp_const_none;
    }

    uint16_t lineno = 0;
    uint16_t col_offset = 0;
    uint16_t end_lineno = 0;
    uint16_t end_col_offset = 0;

    size_t pos = 0;
    for (size_t i = 0; i < n_args && pos < n_fields; i++, pos++) {
        fields[pos] = args[i];
    }

    for (size_t i = 0; i < n_kw; i++) {
        qstr kw = mp_obj_str_get_qstr(args[n_args + 2 * i]);
        mp_obj_t val = args[n_args + 2 * i + 1];
        if (kw == MP_QSTR_lineno) {
            lineno = mp_obj_get_int(val);
        } else if (kw == MP_QSTR_col_offset) {
            col_offset = mp_obj_get_int(val);
        } else if (kw == MP_QSTR_end_lineno) {
            end_lineno = mp_obj_get_int(val);
        } else if (kw == MP_QSTR_end_col_offset) {
            end_col_offset = mp_obj_get_int(val);
        } else {
            bool found = false;
            for (size_t j = 0; j < n_fields; j++) {
                if (field_names[j] == kw) {
                    fields[j] = val;
                    found = true;
                    break;
                }
            }
            if (!found) {
                if (fields != fields_buf) {
                    m_del(mp_obj_t, fields, n_fields);
                }
                mp_raise_TypeError(MP_ERROR_TEXT("unexpected keyword argument"));
            }
        }
    }

    mp_obj_t node = mp_ast_make(type_id, lineno, col_offset, n_fields, fields);
    if (fields != fields_buf) {
        m_del(mp_obj_t, fields, n_fields);
    }
    mp_obj_ast_node_t *self = MP_OBJ_TO_PTR(node);
    if (end_lineno) {
        self->end_lineno = end_lineno;
    }
    if (end_col_offset) {
        self->end_col_offset = end_col_offset;
    }
    return node;
}


MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_AST,
    MP_QSTR_AST,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_mod,
    MP_QSTR_mod,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_AST,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Module,
    MP_QSTR_Module,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_mod,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Interactive,
    MP_QSTR_Interactive,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_mod,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Expression,
    MP_QSTR_Expression,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_mod,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_FunctionType,
    MP_QSTR_FunctionType,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_mod,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_stmt,
    MP_QSTR_stmt,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_AST,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_FunctionDef,
    MP_QSTR_FunctionDef,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_AsyncFunctionDef,
    MP_QSTR_AsyncFunctionDef,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_ClassDef,
    MP_QSTR_ClassDef,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Return,
    MP_QSTR_Return,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Delete,
    MP_QSTR_Delete,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Assign,
    MP_QSTR_Assign,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_AugAssign,
    MP_QSTR_AugAssign,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_AnnAssign,
    MP_QSTR_AnnAssign,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_For,
    MP_QSTR_For,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_AsyncFor,
    MP_QSTR_AsyncFor,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_While,
    MP_QSTR_While,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_If,
    MP_QSTR_If,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_With,
    MP_QSTR_With,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_AsyncWith,
    MP_QSTR_AsyncWith,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Raise,
    MP_QSTR_Raise,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Try,
    MP_QSTR_Try,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Assert,
    MP_QSTR_Assert,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Import,
    MP_QSTR_Import,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_ImportFrom,
    MP_QSTR_ImportFrom,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Global,
    MP_QSTR_Global,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Nonlocal,
    MP_QSTR_Nonlocal,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Expr,
    MP_QSTR_Expr,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Pass,
    MP_QSTR_Pass,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Break,
    MP_QSTR_Break,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Continue,
    MP_QSTR_Continue,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_stmt,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_expr,
    MP_QSTR_expr,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_AST,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_BoolOp,
    MP_QSTR_BoolOp,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_NamedExpr,
    MP_QSTR_NamedExpr,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_BinOp,
    MP_QSTR_BinOp,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_UnaryOp,
    MP_QSTR_UnaryOp,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Lambda,
    MP_QSTR_Lambda,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_IfExp,
    MP_QSTR_IfExp,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Dict,
    MP_QSTR_Dict,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Set,
    MP_QSTR_Set,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_ListComp,
    MP_QSTR_ListComp,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_SetComp,
    MP_QSTR_SetComp,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_DictComp,
    MP_QSTR_DictComp,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_GeneratorExp,
    MP_QSTR_GeneratorExp,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Await,
    MP_QSTR_Await,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Yield,
    MP_QSTR_Yield,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_YieldFrom,
    MP_QSTR_YieldFrom,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Compare,
    MP_QSTR_Compare,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Call,
    MP_QSTR_Call,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_FormattedValue,
    MP_QSTR_FormattedValue,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_JoinedStr,
    MP_QSTR_JoinedStr,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Constant,
    MP_QSTR_Constant,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Attribute,
    MP_QSTR_Attribute,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Subscript,
    MP_QSTR_Subscript,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Starred,
    MP_QSTR_Starred,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Name,
    MP_QSTR_Name,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_List,
    MP_QSTR_List,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Tuple,
    MP_QSTR_Tuple,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Slice,
    MP_QSTR_Slice,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_expr_context,
    MP_QSTR_expr_context,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_AST,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Load,
    MP_QSTR_Load,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr_context,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Store,
    MP_QSTR_Store,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr_context,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Del,
    MP_QSTR_Del,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_expr_context,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_boolop,
    MP_QSTR_boolop,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_AST,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_And,
    MP_QSTR_And,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_boolop,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Or,
    MP_QSTR_Or,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_boolop,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_operator,
    MP_QSTR_operator,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_AST,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Add,
    MP_QSTR_Add,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_operator,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Sub,
    MP_QSTR_Sub,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_operator,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Mult,
    MP_QSTR_Mult,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_operator,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_MatMult,
    MP_QSTR_MatMult,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_operator,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Div,
    MP_QSTR_Div,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_operator,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Mod,
    MP_QSTR_Mod,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_operator,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Pow,
    MP_QSTR_Pow,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_operator,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_LShift,
    MP_QSTR_LShift,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_operator,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_RShift,
    MP_QSTR_RShift,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_operator,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_BitOr,
    MP_QSTR_BitOr,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_operator,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_BitXor,
    MP_QSTR_BitXor,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_operator,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_BitAnd,
    MP_QSTR_BitAnd,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_operator,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_FloorDiv,
    MP_QSTR_FloorDiv,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_operator,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_unaryop,
    MP_QSTR_unaryop,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_AST,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Invert,
    MP_QSTR_Invert,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_unaryop,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Not,
    MP_QSTR_Not,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_unaryop,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_UAdd,
    MP_QSTR_UAdd,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_unaryop,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_USub,
    MP_QSTR_USub,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_unaryop,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_cmpop,
    MP_QSTR_cmpop,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_AST,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Eq,
    MP_QSTR_Eq,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_cmpop,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_NotEq,
    MP_QSTR_NotEq,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_cmpop,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Lt,
    MP_QSTR_Lt,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_cmpop,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_LtE,
    MP_QSTR_LtE,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_cmpop,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Gt,
    MP_QSTR_Gt,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_cmpop,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_GtE,
    MP_QSTR_GtE,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_cmpop,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_Is,
    MP_QSTR_Is,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_cmpop,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_IsNot,
    MP_QSTR_IsNot,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_cmpop,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_In,
    MP_QSTR_In,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_cmpop,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_NotIn,
    MP_QSTR_NotIn,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_cmpop,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_comprehension,
    MP_QSTR_comprehension,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_AST,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_excepthandler,
    MP_QSTR_excepthandler,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_AST,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_ExceptHandler,
    MP_QSTR_ExceptHandler,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_excepthandler,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_arguments,
    MP_QSTR_arguments,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_AST,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_arg,
    MP_QSTR_arg,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_AST,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_keyword,
    MP_QSTR_keyword,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_AST,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_alias,
    MP_QSTR_alias,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_AST,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_withitem,
    MP_QSTR_withitem,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_AST,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_type_ignore,
    MP_QSTR_type_ignore,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_AST,
    attr, ast_node_attr,
    print, ast_node_print
    );

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ast_TypeIgnore,
    MP_QSTR_TypeIgnore,
    MP_TYPE_FLAG_NONE,
    make_new, ast_node_make_new,
    parent, &mp_type_ast_type_ignore,
    attr, ast_node_attr,
    print, ast_node_print
    );

const mp_obj_type_t *const mp_ast_types[MP_AST_NUM_TYPES] = {
    [MP_AST_AST] = &mp_type_ast_AST, 
    [MP_AST_mod] = &mp_type_ast_mod, 
    [MP_AST_MODULE] = &mp_type_ast_Module, 
    [MP_AST_INTERACTIVE] = &mp_type_ast_Interactive, 
    [MP_AST_EXPRESSION] = &mp_type_ast_Expression, 
    [MP_AST_FUNCTIONTYPE] = &mp_type_ast_FunctionType, 
    [MP_AST_stmt] = &mp_type_ast_stmt, 
    [MP_AST_FUNCTIONDEF] = &mp_type_ast_FunctionDef, 
    [MP_AST_ASYNCFUNCTIONDEF] = &mp_type_ast_AsyncFunctionDef, 
    [MP_AST_CLASSDEF] = &mp_type_ast_ClassDef, 
    [MP_AST_RETURN] = &mp_type_ast_Return, 
    [MP_AST_DELETE] = &mp_type_ast_Delete, 
    [MP_AST_ASSIGN] = &mp_type_ast_Assign, 
    [MP_AST_AUGASSIGN] = &mp_type_ast_AugAssign, 
    [MP_AST_ANNASSIGN] = &mp_type_ast_AnnAssign, 
    [MP_AST_FOR] = &mp_type_ast_For, 
    [MP_AST_ASYNCFOR] = &mp_type_ast_AsyncFor, 
    [MP_AST_WHILE] = &mp_type_ast_While, 
    [MP_AST_IF] = &mp_type_ast_If, 
    [MP_AST_WITH] = &mp_type_ast_With, 
    [MP_AST_ASYNCWITH] = &mp_type_ast_AsyncWith, 
    [MP_AST_RAISE] = &mp_type_ast_Raise, 
    [MP_AST_TRY] = &mp_type_ast_Try, 
    [MP_AST_ASSERT] = &mp_type_ast_Assert, 
    [MP_AST_IMPORT] = &mp_type_ast_Import, 
    [MP_AST_IMPORTFROM] = &mp_type_ast_ImportFrom, 
    [MP_AST_GLOBAL] = &mp_type_ast_Global, 
    [MP_AST_NONLOCAL] = &mp_type_ast_Nonlocal, 
    [MP_AST_EXPR] = &mp_type_ast_Expr, 
    [MP_AST_PASS] = &mp_type_ast_Pass, 
    [MP_AST_BREAK] = &mp_type_ast_Break, 
    [MP_AST_CONTINUE] = &mp_type_ast_Continue, 
    [MP_AST_expr] = &mp_type_ast_expr, 
    [MP_AST_BOOLOP] = &mp_type_ast_BoolOp, 
    [MP_AST_NAMEDEXPR] = &mp_type_ast_NamedExpr, 
    [MP_AST_BINOP] = &mp_type_ast_BinOp, 
    [MP_AST_UNARYOP] = &mp_type_ast_UnaryOp, 
    [MP_AST_LAMBDA] = &mp_type_ast_Lambda, 
    [MP_AST_IFEXP] = &mp_type_ast_IfExp, 
    [MP_AST_DICT] = &mp_type_ast_Dict, 
    [MP_AST_SET] = &mp_type_ast_Set, 
    [MP_AST_LISTCOMP] = &mp_type_ast_ListComp, 
    [MP_AST_SETCOMP] = &mp_type_ast_SetComp, 
    [MP_AST_DICTCOMP] = &mp_type_ast_DictComp, 
    [MP_AST_GENERATOREXP] = &mp_type_ast_GeneratorExp, 
    [MP_AST_AWAIT] = &mp_type_ast_Await, 
    [MP_AST_YIELD] = &mp_type_ast_Yield, 
    [MP_AST_YIELDFROM] = &mp_type_ast_YieldFrom, 
    [MP_AST_COMPARE] = &mp_type_ast_Compare, 
    [MP_AST_CALL] = &mp_type_ast_Call, 
    [MP_AST_FORMATTEDVALUE] = &mp_type_ast_FormattedValue, 
    [MP_AST_JOINEDSTR] = &mp_type_ast_JoinedStr, 
    [MP_AST_CONSTANT] = &mp_type_ast_Constant, 
    [MP_AST_ATTRIBUTE] = &mp_type_ast_Attribute, 
    [MP_AST_SUBSCRIPT] = &mp_type_ast_Subscript, 
    [MP_AST_STARRED] = &mp_type_ast_Starred, 
    [MP_AST_NAME] = &mp_type_ast_Name, 
    [MP_AST_LIST] = &mp_type_ast_List, 
    [MP_AST_TUPLE] = &mp_type_ast_Tuple, 
    [MP_AST_SLICE] = &mp_type_ast_Slice, 
    [MP_AST_EXPR_CONTEXT] = &mp_type_ast_expr_context, 
    [MP_AST_LOAD] = &mp_type_ast_Load, 
    [MP_AST_STORE] = &mp_type_ast_Store, 
    [MP_AST_DEL] = &mp_type_ast_Del, 
    [MP_AST_boolop] = &mp_type_ast_boolop, 
    [MP_AST_AND] = &mp_type_ast_And, 
    [MP_AST_OR] = &mp_type_ast_Or, 
    [MP_AST_operator] = &mp_type_ast_operator, 
    [MP_AST_ADD] = &mp_type_ast_Add, 
    [MP_AST_SUB] = &mp_type_ast_Sub, 
    [MP_AST_MULT] = &mp_type_ast_Mult, 
    [MP_AST_MATMULT] = &mp_type_ast_MatMult, 
    [MP_AST_DIV] = &mp_type_ast_Div, 
    [MP_AST_MOD] = &mp_type_ast_Mod, 
    [MP_AST_POW] = &mp_type_ast_Pow, 
    [MP_AST_LSHIFT] = &mp_type_ast_LShift, 
    [MP_AST_RSHIFT] = &mp_type_ast_RShift, 
    [MP_AST_BITOR] = &mp_type_ast_BitOr, 
    [MP_AST_BITXOR] = &mp_type_ast_BitXor, 
    [MP_AST_BITAND] = &mp_type_ast_BitAnd, 
    [MP_AST_FLOORDIV] = &mp_type_ast_FloorDiv, 
    [MP_AST_unaryop] = &mp_type_ast_unaryop, 
    [MP_AST_INVERT] = &mp_type_ast_Invert, 
    [MP_AST_NOT] = &mp_type_ast_Not, 
    [MP_AST_UADD] = &mp_type_ast_UAdd, 
    [MP_AST_USUB] = &mp_type_ast_USub, 
    [MP_AST_cmpop] = &mp_type_ast_cmpop, 
    [MP_AST_EQ] = &mp_type_ast_Eq, 
    [MP_AST_NOTEQ] = &mp_type_ast_NotEq, 
    [MP_AST_LT] = &mp_type_ast_Lt, 
    [MP_AST_LTE] = &mp_type_ast_LtE, 
    [MP_AST_GT] = &mp_type_ast_Gt, 
    [MP_AST_GTE] = &mp_type_ast_GtE, 
    [MP_AST_IS] = &mp_type_ast_Is, 
    [MP_AST_ISNOT] = &mp_type_ast_IsNot, 
    [MP_AST_IN] = &mp_type_ast_In, 
    [MP_AST_NOTIN] = &mp_type_ast_NotIn, 
    [MP_AST_COMPREHENSION] = &mp_type_ast_comprehension, 
    [MP_AST_excepthandler] = &mp_type_ast_excepthandler, 
    [MP_AST_EXCEPTHANDLER] = &mp_type_ast_ExceptHandler, 
    [MP_AST_ARGUMENTS] = &mp_type_ast_arguments, 
    [MP_AST_arg] = &mp_type_ast_arg, 
    [MP_AST_KEYWORD] = &mp_type_ast_keyword, 
    [MP_AST_alias] = &mp_type_ast_alias, 
    [MP_AST_WITHITEM] = &mp_type_ast_withitem, 
    [MP_AST_type_ignore] = &mp_type_ast_type_ignore, 
    [MP_AST_TYPEIGNORE] = &mp_type_ast_TypeIgnore, 
};

uint16_t mp_ast_type_id(mp_obj_t node) {
    if (!mp_obj_is_obj(node)) {
        return (uint16_t)-1;
    }
    mp_obj_ast_node_t *self = MP_OBJ_TO_PTR(node);
    return self->type_id;
}

mp_obj_t mp_ast_get_field(mp_obj_t node, qstr attr) {
    mp_obj_t dest[2] = {MP_OBJ_NULL, MP_OBJ_NULL};
    ast_node_attr(node, attr, dest);
    return dest[0];
}

void mp_ast_set_field(mp_obj_t node, qstr attr, mp_obj_t val) {
    mp_obj_t dest[2] = {node, val};
    ast_node_attr(node, attr, dest);
}

mp_obj_t mp_ast_make(uint16_t type_id, uint16_t lineno, uint16_t col_offset, size_t n_fields, mp_obj_t *fields) {
    assert(type_id < MP_AST_NUM_TYPES);
    assert(n_fields == mp_ast_num_fields[type_id]);
    mp_obj_ast_node_t *o = m_new_obj_var(mp_obj_ast_node_t, fields, mp_obj_t, n_fields);
    o->base.type = (mp_obj_type_t *)mp_ast_types[type_id];
    o->type_id = type_id;
    o->lineno = lineno;
    o->col_offset = col_offset;
    o->end_lineno = lineno;
    o->end_col_offset = col_offset;
    for (size_t i = 0; i < n_fields; i++) {
        o->fields[i] = fields[i] ? fields[i] : mp_const_none;
    }
    return MP_OBJ_FROM_PTR(o);
}

mp_obj_t mp_ast_singleton(uint16_t type_id) {
    return ast_singletons[type_id];
}

void mp_ast_init_types(void) {
    static bool inited = false;
    if (inited) return;
    inited = true;

    ast_type_fields[MP_AST_AST] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_AST] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_mod] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_mod] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_MODULE] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_body), MP_OBJ_NEW_QSTR(MP_QSTR_type_ignores)});
    ast_type_attributes[MP_AST_MODULE] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_INTERACTIVE] = mp_obj_new_tuple(1, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_body)});
    ast_type_attributes[MP_AST_INTERACTIVE] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_EXPRESSION] = mp_obj_new_tuple(1, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_body)});
    ast_type_attributes[MP_AST_EXPRESSION] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_FUNCTIONTYPE] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_argtypes), MP_OBJ_NEW_QSTR(MP_QSTR_returns)});
    ast_type_attributes[MP_AST_FUNCTIONTYPE] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_stmt] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_stmt] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_FUNCTIONDEF] = mp_obj_new_tuple(7, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_name), MP_OBJ_NEW_QSTR(MP_QSTR_args), MP_OBJ_NEW_QSTR(MP_QSTR_body), MP_OBJ_NEW_QSTR(MP_QSTR_decorator_list), MP_OBJ_NEW_QSTR(MP_QSTR_returns), MP_OBJ_NEW_QSTR(MP_QSTR_type_comment), MP_OBJ_NEW_QSTR(MP_QSTR_type_params)});
    ast_type_attributes[MP_AST_FUNCTIONDEF] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_ASYNCFUNCTIONDEF] = mp_obj_new_tuple(7, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_name), MP_OBJ_NEW_QSTR(MP_QSTR_args), MP_OBJ_NEW_QSTR(MP_QSTR_body), MP_OBJ_NEW_QSTR(MP_QSTR_decorator_list), MP_OBJ_NEW_QSTR(MP_QSTR_returns), MP_OBJ_NEW_QSTR(MP_QSTR_type_comment), MP_OBJ_NEW_QSTR(MP_QSTR_type_params)});
    ast_type_attributes[MP_AST_ASYNCFUNCTIONDEF] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_CLASSDEF] = mp_obj_new_tuple(6, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_name), MP_OBJ_NEW_QSTR(MP_QSTR_bases), MP_OBJ_NEW_QSTR(MP_QSTR_keywords), MP_OBJ_NEW_QSTR(MP_QSTR_body), MP_OBJ_NEW_QSTR(MP_QSTR_decorator_list), MP_OBJ_NEW_QSTR(MP_QSTR_type_params)});
    ast_type_attributes[MP_AST_CLASSDEF] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_RETURN] = mp_obj_new_tuple(1, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_value)});
    ast_type_attributes[MP_AST_RETURN] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_DELETE] = mp_obj_new_tuple(1, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_targets)});
    ast_type_attributes[MP_AST_DELETE] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_ASSIGN] = mp_obj_new_tuple(3, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_targets), MP_OBJ_NEW_QSTR(MP_QSTR_value), MP_OBJ_NEW_QSTR(MP_QSTR_type_comment)});
    ast_type_attributes[MP_AST_ASSIGN] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_AUGASSIGN] = mp_obj_new_tuple(3, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_target), MP_OBJ_NEW_QSTR(MP_QSTR_op), MP_OBJ_NEW_QSTR(MP_QSTR_value)});
    ast_type_attributes[MP_AST_AUGASSIGN] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_ANNASSIGN] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_target), MP_OBJ_NEW_QSTR(MP_QSTR_annotation), MP_OBJ_NEW_QSTR(MP_QSTR_value), MP_OBJ_NEW_QSTR(MP_QSTR_simple)});
    ast_type_attributes[MP_AST_ANNASSIGN] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_FOR] = mp_obj_new_tuple(5, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_target), MP_OBJ_NEW_QSTR(MP_QSTR_iter), MP_OBJ_NEW_QSTR(MP_QSTR_body), MP_OBJ_NEW_QSTR(MP_QSTR_orelse), MP_OBJ_NEW_QSTR(MP_QSTR_type_comment)});
    ast_type_attributes[MP_AST_FOR] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_ASYNCFOR] = mp_obj_new_tuple(5, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_target), MP_OBJ_NEW_QSTR(MP_QSTR_iter), MP_OBJ_NEW_QSTR(MP_QSTR_body), MP_OBJ_NEW_QSTR(MP_QSTR_orelse), MP_OBJ_NEW_QSTR(MP_QSTR_type_comment)});
    ast_type_attributes[MP_AST_ASYNCFOR] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_WHILE] = mp_obj_new_tuple(3, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_test), MP_OBJ_NEW_QSTR(MP_QSTR_body), MP_OBJ_NEW_QSTR(MP_QSTR_orelse)});
    ast_type_attributes[MP_AST_WHILE] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_IF] = mp_obj_new_tuple(3, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_test), MP_OBJ_NEW_QSTR(MP_QSTR_body), MP_OBJ_NEW_QSTR(MP_QSTR_orelse)});
    ast_type_attributes[MP_AST_IF] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_WITH] = mp_obj_new_tuple(3, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_items), MP_OBJ_NEW_QSTR(MP_QSTR_body), MP_OBJ_NEW_QSTR(MP_QSTR_type_comment)});
    ast_type_attributes[MP_AST_WITH] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_ASYNCWITH] = mp_obj_new_tuple(3, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_items), MP_OBJ_NEW_QSTR(MP_QSTR_body), MP_OBJ_NEW_QSTR(MP_QSTR_type_comment)});
    ast_type_attributes[MP_AST_ASYNCWITH] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_RAISE] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_exc), MP_OBJ_NEW_QSTR(MP_QSTR_cause)});
    ast_type_attributes[MP_AST_RAISE] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_TRY] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_body), MP_OBJ_NEW_QSTR(MP_QSTR_handlers), MP_OBJ_NEW_QSTR(MP_QSTR_orelse), MP_OBJ_NEW_QSTR(MP_QSTR_finalbody)});
    ast_type_attributes[MP_AST_TRY] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_ASSERT] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_test), MP_OBJ_NEW_QSTR(MP_QSTR_msg)});
    ast_type_attributes[MP_AST_ASSERT] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_IMPORT] = mp_obj_new_tuple(1, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_names)});
    ast_type_attributes[MP_AST_IMPORT] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_IMPORTFROM] = mp_obj_new_tuple(3, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_module), MP_OBJ_NEW_QSTR(MP_QSTR_names), MP_OBJ_NEW_QSTR(MP_QSTR_level)});
    ast_type_attributes[MP_AST_IMPORTFROM] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_GLOBAL] = mp_obj_new_tuple(1, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_names)});
    ast_type_attributes[MP_AST_GLOBAL] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_NONLOCAL] = mp_obj_new_tuple(1, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_names)});
    ast_type_attributes[MP_AST_NONLOCAL] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_EXPR] = mp_obj_new_tuple(1, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_value)});
    ast_type_attributes[MP_AST_EXPR] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_PASS] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_PASS] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_BREAK] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_BREAK] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_CONTINUE] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_CONTINUE] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_expr] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_expr] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_BOOLOP] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_op), MP_OBJ_NEW_QSTR(MP_QSTR_values)});
    ast_type_attributes[MP_AST_BOOLOP] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_NAMEDEXPR] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_target), MP_OBJ_NEW_QSTR(MP_QSTR_value)});
    ast_type_attributes[MP_AST_NAMEDEXPR] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_BINOP] = mp_obj_new_tuple(3, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_left), MP_OBJ_NEW_QSTR(MP_QSTR_op), MP_OBJ_NEW_QSTR(MP_QSTR_right)});
    ast_type_attributes[MP_AST_BINOP] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_UNARYOP] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_op), MP_OBJ_NEW_QSTR(MP_QSTR_operand)});
    ast_type_attributes[MP_AST_UNARYOP] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_LAMBDA] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_args), MP_OBJ_NEW_QSTR(MP_QSTR_body)});
    ast_type_attributes[MP_AST_LAMBDA] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_IFEXP] = mp_obj_new_tuple(3, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_test), MP_OBJ_NEW_QSTR(MP_QSTR_body), MP_OBJ_NEW_QSTR(MP_QSTR_orelse)});
    ast_type_attributes[MP_AST_IFEXP] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_DICT] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_keys), MP_OBJ_NEW_QSTR(MP_QSTR_values)});
    ast_type_attributes[MP_AST_DICT] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_SET] = mp_obj_new_tuple(1, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_elts)});
    ast_type_attributes[MP_AST_SET] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_LISTCOMP] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_elt), MP_OBJ_NEW_QSTR(MP_QSTR_generators)});
    ast_type_attributes[MP_AST_LISTCOMP] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_SETCOMP] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_elt), MP_OBJ_NEW_QSTR(MP_QSTR_generators)});
    ast_type_attributes[MP_AST_SETCOMP] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_DICTCOMP] = mp_obj_new_tuple(3, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_key), MP_OBJ_NEW_QSTR(MP_QSTR_value), MP_OBJ_NEW_QSTR(MP_QSTR_generators)});
    ast_type_attributes[MP_AST_DICTCOMP] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_GENERATOREXP] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_elt), MP_OBJ_NEW_QSTR(MP_QSTR_generators)});
    ast_type_attributes[MP_AST_GENERATOREXP] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_AWAIT] = mp_obj_new_tuple(1, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_value)});
    ast_type_attributes[MP_AST_AWAIT] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_YIELD] = mp_obj_new_tuple(1, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_value)});
    ast_type_attributes[MP_AST_YIELD] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_YIELDFROM] = mp_obj_new_tuple(1, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_value)});
    ast_type_attributes[MP_AST_YIELDFROM] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_COMPARE] = mp_obj_new_tuple(3, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_left), MP_OBJ_NEW_QSTR(MP_QSTR_ops), MP_OBJ_NEW_QSTR(MP_QSTR_comparators)});
    ast_type_attributes[MP_AST_COMPARE] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_CALL] = mp_obj_new_tuple(3, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_func), MP_OBJ_NEW_QSTR(MP_QSTR_args), MP_OBJ_NEW_QSTR(MP_QSTR_keywords)});
    ast_type_attributes[MP_AST_CALL] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_FORMATTEDVALUE] = mp_obj_new_tuple(3, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_value), MP_OBJ_NEW_QSTR(MP_QSTR_conversion), MP_OBJ_NEW_QSTR(MP_QSTR_format_spec)});
    ast_type_attributes[MP_AST_FORMATTEDVALUE] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_JOINEDSTR] = mp_obj_new_tuple(1, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_values)});
    ast_type_attributes[MP_AST_JOINEDSTR] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_CONSTANT] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_value), MP_OBJ_NEW_QSTR(MP_QSTR_kind)});
    ast_type_attributes[MP_AST_CONSTANT] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_ATTRIBUTE] = mp_obj_new_tuple(3, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_value), MP_OBJ_NEW_QSTR(MP_QSTR_attr), MP_OBJ_NEW_QSTR(MP_QSTR_ctx)});
    ast_type_attributes[MP_AST_ATTRIBUTE] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_SUBSCRIPT] = mp_obj_new_tuple(3, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_value), MP_OBJ_NEW_QSTR(MP_QSTR_slice), MP_OBJ_NEW_QSTR(MP_QSTR_ctx)});
    ast_type_attributes[MP_AST_SUBSCRIPT] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_STARRED] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_value), MP_OBJ_NEW_QSTR(MP_QSTR_ctx)});
    ast_type_attributes[MP_AST_STARRED] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_NAME] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_id), MP_OBJ_NEW_QSTR(MP_QSTR_ctx)});
    ast_type_attributes[MP_AST_NAME] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_LIST] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_elts), MP_OBJ_NEW_QSTR(MP_QSTR_ctx)});
    ast_type_attributes[MP_AST_LIST] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_TUPLE] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_elts), MP_OBJ_NEW_QSTR(MP_QSTR_ctx)});
    ast_type_attributes[MP_AST_TUPLE] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_SLICE] = mp_obj_new_tuple(3, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lower), MP_OBJ_NEW_QSTR(MP_QSTR_upper), MP_OBJ_NEW_QSTR(MP_QSTR_step)});
    ast_type_attributes[MP_AST_SLICE] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_EXPR_CONTEXT] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_EXPR_CONTEXT] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_LOAD] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_LOAD] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_STORE] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_STORE] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_DEL] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_DEL] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_boolop] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_boolop] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_AND] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_AND] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_OR] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_OR] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_operator] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_operator] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_ADD] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_ADD] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_SUB] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_SUB] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_MULT] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_MULT] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_MATMULT] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_MATMULT] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_DIV] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_DIV] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_MOD] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_MOD] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_POW] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_POW] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_LSHIFT] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_LSHIFT] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_RSHIFT] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_RSHIFT] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_BITOR] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_BITOR] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_BITXOR] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_BITXOR] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_BITAND] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_BITAND] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_FLOORDIV] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_FLOORDIV] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_unaryop] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_unaryop] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_INVERT] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_INVERT] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_NOT] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_NOT] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_UADD] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_UADD] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_USUB] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_USUB] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_cmpop] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_cmpop] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_EQ] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_EQ] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_NOTEQ] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_NOTEQ] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_LT] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_LT] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_LTE] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_LTE] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_GT] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_GT] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_GTE] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_GTE] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_IS] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_IS] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_ISNOT] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_ISNOT] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_IN] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_IN] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_NOTIN] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_NOTIN] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_COMPREHENSION] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_target), MP_OBJ_NEW_QSTR(MP_QSTR_iter), MP_OBJ_NEW_QSTR(MP_QSTR_ifs), MP_OBJ_NEW_QSTR(MP_QSTR_is_async)});
    ast_type_attributes[MP_AST_COMPREHENSION] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_excepthandler] = mp_const_empty_tuple;
    ast_type_attributes[MP_AST_excepthandler] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_EXCEPTHANDLER] = mp_obj_new_tuple(3, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_type), MP_OBJ_NEW_QSTR(MP_QSTR_name), MP_OBJ_NEW_QSTR(MP_QSTR_body)});
    ast_type_attributes[MP_AST_EXCEPTHANDLER] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_ARGUMENTS] = mp_obj_new_tuple(7, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_posonlyargs), MP_OBJ_NEW_QSTR(MP_QSTR_args), MP_OBJ_NEW_QSTR(MP_QSTR_vararg), MP_OBJ_NEW_QSTR(MP_QSTR_kwonlyargs), MP_OBJ_NEW_QSTR(MP_QSTR_kw_defaults), MP_OBJ_NEW_QSTR(MP_QSTR_kwarg), MP_OBJ_NEW_QSTR(MP_QSTR_defaults)});
    ast_type_attributes[MP_AST_ARGUMENTS] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_arg] = mp_obj_new_tuple(3, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_arg), MP_OBJ_NEW_QSTR(MP_QSTR_annotation), MP_OBJ_NEW_QSTR(MP_QSTR_type_comment)});
    ast_type_attributes[MP_AST_arg] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_KEYWORD] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_arg), MP_OBJ_NEW_QSTR(MP_QSTR_value)});
    ast_type_attributes[MP_AST_KEYWORD] = mp_obj_new_tuple(4, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_col_offset), MP_OBJ_NEW_QSTR(MP_QSTR_end_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_end_col_offset)});
    ast_type_fields[MP_AST_alias] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_name), MP_OBJ_NEW_QSTR(MP_QSTR_asname)});
    ast_type_attributes[MP_AST_alias] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_WITHITEM] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_context_expr), MP_OBJ_NEW_QSTR(MP_QSTR_optional_vars)});
    ast_type_attributes[MP_AST_WITHITEM] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_type_ignore] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_tag)});
    ast_type_attributes[MP_AST_type_ignore] = mp_const_empty_tuple;
    ast_type_fields[MP_AST_TYPEIGNORE] = mp_obj_new_tuple(2, (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_QSTR_lineno), MP_OBJ_NEW_QSTR(MP_QSTR_tag)});
    ast_type_attributes[MP_AST_TYPEIGNORE] = mp_const_empty_tuple;
    ast_singletons[MP_AST_AST] = mp_ast_make(MP_AST_AST, 0, 0, 0, NULL);
    ast_singletons[MP_AST_mod] = mp_ast_make(MP_AST_mod, 0, 0, 0, NULL);
    ast_singletons[MP_AST_MODULE] = mp_const_none;
    ast_singletons[MP_AST_INTERACTIVE] = mp_const_none;
    ast_singletons[MP_AST_EXPRESSION] = mp_const_none;
    ast_singletons[MP_AST_FUNCTIONTYPE] = mp_const_none;
    ast_singletons[MP_AST_stmt] = mp_ast_make(MP_AST_stmt, 0, 0, 0, NULL);
    ast_singletons[MP_AST_FUNCTIONDEF] = mp_const_none;
    ast_singletons[MP_AST_ASYNCFUNCTIONDEF] = mp_const_none;
    ast_singletons[MP_AST_CLASSDEF] = mp_const_none;
    ast_singletons[MP_AST_RETURN] = mp_const_none;
    ast_singletons[MP_AST_DELETE] = mp_const_none;
    ast_singletons[MP_AST_ASSIGN] = mp_const_none;
    ast_singletons[MP_AST_AUGASSIGN] = mp_const_none;
    ast_singletons[MP_AST_ANNASSIGN] = mp_const_none;
    ast_singletons[MP_AST_FOR] = mp_const_none;
    ast_singletons[MP_AST_ASYNCFOR] = mp_const_none;
    ast_singletons[MP_AST_WHILE] = mp_const_none;
    ast_singletons[MP_AST_IF] = mp_const_none;
    ast_singletons[MP_AST_WITH] = mp_const_none;
    ast_singletons[MP_AST_ASYNCWITH] = mp_const_none;
    ast_singletons[MP_AST_RAISE] = mp_const_none;
    ast_singletons[MP_AST_TRY] = mp_const_none;
    ast_singletons[MP_AST_ASSERT] = mp_const_none;
    ast_singletons[MP_AST_IMPORT] = mp_const_none;
    ast_singletons[MP_AST_IMPORTFROM] = mp_const_none;
    ast_singletons[MP_AST_GLOBAL] = mp_const_none;
    ast_singletons[MP_AST_NONLOCAL] = mp_const_none;
    ast_singletons[MP_AST_EXPR] = mp_const_none;
    ast_singletons[MP_AST_PASS] = mp_const_none;
    ast_singletons[MP_AST_BREAK] = mp_const_none;
    ast_singletons[MP_AST_CONTINUE] = mp_const_none;
    ast_singletons[MP_AST_expr] = mp_ast_make(MP_AST_expr, 0, 0, 0, NULL);
    ast_singletons[MP_AST_BOOLOP] = mp_const_none;
    ast_singletons[MP_AST_NAMEDEXPR] = mp_const_none;
    ast_singletons[MP_AST_BINOP] = mp_const_none;
    ast_singletons[MP_AST_UNARYOP] = mp_const_none;
    ast_singletons[MP_AST_LAMBDA] = mp_const_none;
    ast_singletons[MP_AST_IFEXP] = mp_const_none;
    ast_singletons[MP_AST_DICT] = mp_const_none;
    ast_singletons[MP_AST_SET] = mp_const_none;
    ast_singletons[MP_AST_LISTCOMP] = mp_const_none;
    ast_singletons[MP_AST_SETCOMP] = mp_const_none;
    ast_singletons[MP_AST_DICTCOMP] = mp_const_none;
    ast_singletons[MP_AST_GENERATOREXP] = mp_const_none;
    ast_singletons[MP_AST_AWAIT] = mp_const_none;
    ast_singletons[MP_AST_YIELD] = mp_const_none;
    ast_singletons[MP_AST_YIELDFROM] = mp_const_none;
    ast_singletons[MP_AST_COMPARE] = mp_const_none;
    ast_singletons[MP_AST_CALL] = mp_const_none;
    ast_singletons[MP_AST_FORMATTEDVALUE] = mp_const_none;
    ast_singletons[MP_AST_JOINEDSTR] = mp_const_none;
    ast_singletons[MP_AST_CONSTANT] = mp_const_none;
    ast_singletons[MP_AST_ATTRIBUTE] = mp_const_none;
    ast_singletons[MP_AST_SUBSCRIPT] = mp_const_none;
    ast_singletons[MP_AST_STARRED] = mp_const_none;
    ast_singletons[MP_AST_NAME] = mp_const_none;
    ast_singletons[MP_AST_LIST] = mp_const_none;
    ast_singletons[MP_AST_TUPLE] = mp_const_none;
    ast_singletons[MP_AST_SLICE] = mp_const_none;
    ast_singletons[MP_AST_EXPR_CONTEXT] = mp_ast_make(MP_AST_EXPR_CONTEXT, 0, 0, 0, NULL);
    ast_singletons[MP_AST_LOAD] = mp_ast_make(MP_AST_LOAD, 0, 0, 0, NULL);
    ast_singletons[MP_AST_STORE] = mp_ast_make(MP_AST_STORE, 0, 0, 0, NULL);
    ast_singletons[MP_AST_DEL] = mp_ast_make(MP_AST_DEL, 0, 0, 0, NULL);
    ast_singletons[MP_AST_boolop] = mp_ast_make(MP_AST_boolop, 0, 0, 0, NULL);
    ast_singletons[MP_AST_AND] = mp_ast_make(MP_AST_AND, 0, 0, 0, NULL);
    ast_singletons[MP_AST_OR] = mp_ast_make(MP_AST_OR, 0, 0, 0, NULL);
    ast_singletons[MP_AST_operator] = mp_ast_make(MP_AST_operator, 0, 0, 0, NULL);
    ast_singletons[MP_AST_ADD] = mp_ast_make(MP_AST_ADD, 0, 0, 0, NULL);
    ast_singletons[MP_AST_SUB] = mp_ast_make(MP_AST_SUB, 0, 0, 0, NULL);
    ast_singletons[MP_AST_MULT] = mp_ast_make(MP_AST_MULT, 0, 0, 0, NULL);
    ast_singletons[MP_AST_MATMULT] = mp_ast_make(MP_AST_MATMULT, 0, 0, 0, NULL);
    ast_singletons[MP_AST_DIV] = mp_ast_make(MP_AST_DIV, 0, 0, 0, NULL);
    ast_singletons[MP_AST_MOD] = mp_ast_make(MP_AST_MOD, 0, 0, 0, NULL);
    ast_singletons[MP_AST_POW] = mp_ast_make(MP_AST_POW, 0, 0, 0, NULL);
    ast_singletons[MP_AST_LSHIFT] = mp_ast_make(MP_AST_LSHIFT, 0, 0, 0, NULL);
    ast_singletons[MP_AST_RSHIFT] = mp_ast_make(MP_AST_RSHIFT, 0, 0, 0, NULL);
    ast_singletons[MP_AST_BITOR] = mp_ast_make(MP_AST_BITOR, 0, 0, 0, NULL);
    ast_singletons[MP_AST_BITXOR] = mp_ast_make(MP_AST_BITXOR, 0, 0, 0, NULL);
    ast_singletons[MP_AST_BITAND] = mp_ast_make(MP_AST_BITAND, 0, 0, 0, NULL);
    ast_singletons[MP_AST_FLOORDIV] = mp_ast_make(MP_AST_FLOORDIV, 0, 0, 0, NULL);
    ast_singletons[MP_AST_unaryop] = mp_ast_make(MP_AST_unaryop, 0, 0, 0, NULL);
    ast_singletons[MP_AST_INVERT] = mp_ast_make(MP_AST_INVERT, 0, 0, 0, NULL);
    ast_singletons[MP_AST_NOT] = mp_ast_make(MP_AST_NOT, 0, 0, 0, NULL);
    ast_singletons[MP_AST_UADD] = mp_ast_make(MP_AST_UADD, 0, 0, 0, NULL);
    ast_singletons[MP_AST_USUB] = mp_ast_make(MP_AST_USUB, 0, 0, 0, NULL);
    ast_singletons[MP_AST_cmpop] = mp_ast_make(MP_AST_cmpop, 0, 0, 0, NULL);
    ast_singletons[MP_AST_EQ] = mp_ast_make(MP_AST_EQ, 0, 0, 0, NULL);
    ast_singletons[MP_AST_NOTEQ] = mp_ast_make(MP_AST_NOTEQ, 0, 0, 0, NULL);
    ast_singletons[MP_AST_LT] = mp_ast_make(MP_AST_LT, 0, 0, 0, NULL);
    ast_singletons[MP_AST_LTE] = mp_ast_make(MP_AST_LTE, 0, 0, 0, NULL);
    ast_singletons[MP_AST_GT] = mp_ast_make(MP_AST_GT, 0, 0, 0, NULL);
    ast_singletons[MP_AST_GTE] = mp_ast_make(MP_AST_GTE, 0, 0, 0, NULL);
    ast_singletons[MP_AST_IS] = mp_ast_make(MP_AST_IS, 0, 0, 0, NULL);
    ast_singletons[MP_AST_ISNOT] = mp_ast_make(MP_AST_ISNOT, 0, 0, 0, NULL);
    ast_singletons[MP_AST_IN] = mp_ast_make(MP_AST_IN, 0, 0, 0, NULL);
    ast_singletons[MP_AST_NOTIN] = mp_ast_make(MP_AST_NOTIN, 0, 0, 0, NULL);
    ast_singletons[MP_AST_COMPREHENSION] = mp_const_none;
    ast_singletons[MP_AST_excepthandler] = mp_ast_make(MP_AST_excepthandler, 0, 0, 0, NULL);
    ast_singletons[MP_AST_EXCEPTHANDLER] = mp_const_none;
    ast_singletons[MP_AST_ARGUMENTS] = mp_const_none;
    ast_singletons[MP_AST_arg] = mp_const_none;
    ast_singletons[MP_AST_KEYWORD] = mp_const_none;
    ast_singletons[MP_AST_alias] = mp_const_none;
    ast_singletons[MP_AST_WITHITEM] = mp_const_none;
    ast_singletons[MP_AST_type_ignore] = mp_const_none;
    ast_singletons[MP_AST_TYPEIGNORE] = mp_const_none;
}
#endif // MICROPY_PY_AST