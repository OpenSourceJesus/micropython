#ifndef MICROPY_INCLUDED_EXTMOD_MODAST_AST_TYPES_H
#define MICROPY_INCLUDED_EXTMOD_MODAST_AST_TYPES_H

#include "py/obj.h"

#define MP_AST_NUM_TYPES (107)

typedef struct _mp_obj_ast_node_t {
    mp_obj_base_t base;
    uint16_t type_id;
    uint16_t lineno;
    uint16_t col_offset;
    uint16_t end_lineno;
    uint16_t end_col_offset;
    mp_obj_t fields[];
} mp_obj_ast_node_t;

extern const mp_obj_type_t *const mp_ast_types[MP_AST_NUM_TYPES];
extern const qstr *const mp_ast_field_names[MP_AST_NUM_TYPES];
extern const uint8_t mp_ast_num_fields[MP_AST_NUM_TYPES];

enum {
    MP_AST_AST,
    MP_AST_mod,
    MP_AST_MODULE,
    MP_AST_INTERACTIVE,
    MP_AST_EXPRESSION,
    MP_AST_FUNCTIONTYPE,
    MP_AST_stmt,
    MP_AST_FUNCTIONDEF,
    MP_AST_ASYNCFUNCTIONDEF,
    MP_AST_CLASSDEF,
    MP_AST_RETURN,
    MP_AST_DELETE,
    MP_AST_ASSIGN,
    MP_AST_AUGASSIGN,
    MP_AST_ANNASSIGN,
    MP_AST_FOR,
    MP_AST_ASYNCFOR,
    MP_AST_WHILE,
    MP_AST_IF,
    MP_AST_WITH,
    MP_AST_ASYNCWITH,
    MP_AST_RAISE,
    MP_AST_TRY,
    MP_AST_ASSERT,
    MP_AST_IMPORT,
    MP_AST_IMPORTFROM,
    MP_AST_GLOBAL,
    MP_AST_NONLOCAL,
    MP_AST_EXPR,
    MP_AST_PASS,
    MP_AST_BREAK,
    MP_AST_CONTINUE,
    MP_AST_expr,
    MP_AST_BOOLOP,
    MP_AST_NAMEDEXPR,
    MP_AST_BINOP,
    MP_AST_UNARYOP,
    MP_AST_LAMBDA,
    MP_AST_IFEXP,
    MP_AST_DICT,
    MP_AST_SET,
    MP_AST_LISTCOMP,
    MP_AST_SETCOMP,
    MP_AST_DICTCOMP,
    MP_AST_GENERATOREXP,
    MP_AST_AWAIT,
    MP_AST_YIELD,
    MP_AST_YIELDFROM,
    MP_AST_COMPARE,
    MP_AST_CALL,
    MP_AST_FORMATTEDVALUE,
    MP_AST_JOINEDSTR,
    MP_AST_CONSTANT,
    MP_AST_ATTRIBUTE,
    MP_AST_SUBSCRIPT,
    MP_AST_STARRED,
    MP_AST_NAME,
    MP_AST_LIST,
    MP_AST_TUPLE,
    MP_AST_SLICE,
    MP_AST_EXPR_CONTEXT,
    MP_AST_LOAD,
    MP_AST_STORE,
    MP_AST_DEL,
    MP_AST_boolop,
    MP_AST_AND,
    MP_AST_OR,
    MP_AST_operator,
    MP_AST_ADD,
    MP_AST_SUB,
    MP_AST_MULT,
    MP_AST_MATMULT,
    MP_AST_DIV,
    MP_AST_MOD,
    MP_AST_POW,
    MP_AST_LSHIFT,
    MP_AST_RSHIFT,
    MP_AST_BITOR,
    MP_AST_BITXOR,
    MP_AST_BITAND,
    MP_AST_FLOORDIV,
    MP_AST_unaryop,
    MP_AST_INVERT,
    MP_AST_NOT,
    MP_AST_UADD,
    MP_AST_USUB,
    MP_AST_cmpop,
    MP_AST_EQ,
    MP_AST_NOTEQ,
    MP_AST_LT,
    MP_AST_LTE,
    MP_AST_GT,
    MP_AST_GTE,
    MP_AST_IS,
    MP_AST_ISNOT,
    MP_AST_IN,
    MP_AST_NOTIN,
    MP_AST_COMPREHENSION,
    MP_AST_excepthandler,
    MP_AST_EXCEPTHANDLER,
    MP_AST_ARGUMENTS,
    MP_AST_arg,
    MP_AST_KEYWORD,
    MP_AST_alias,
    MP_AST_WITHITEM,
    MP_AST_type_ignore,
    MP_AST_TYPEIGNORE,
};mp_obj_t mp_ast_make(uint16_t type_id, uint16_t lineno, uint16_t col_offset, size_t n_fields, mp_obj_t *fields);
mp_obj_t mp_ast_singleton(uint16_t type_id);
mp_obj_t mp_ast_get_field(mp_obj_t node, qstr attr);
void mp_ast_set_field(mp_obj_t node, qstr attr, mp_obj_t val);
uint16_t mp_ast_type_id(mp_obj_t node);
void mp_ast_init_types(void);
extern const mp_obj_type_t mp_type_ast_AST;
extern const mp_obj_type_t mp_type_ast_mod;
extern const mp_obj_type_t mp_type_ast_Module;
extern const mp_obj_type_t mp_type_ast_Interactive;
extern const mp_obj_type_t mp_type_ast_Expression;
extern const mp_obj_type_t mp_type_ast_FunctionType;
extern const mp_obj_type_t mp_type_ast_stmt;
extern const mp_obj_type_t mp_type_ast_FunctionDef;
extern const mp_obj_type_t mp_type_ast_AsyncFunctionDef;
extern const mp_obj_type_t mp_type_ast_ClassDef;
extern const mp_obj_type_t mp_type_ast_Return;
extern const mp_obj_type_t mp_type_ast_Delete;
extern const mp_obj_type_t mp_type_ast_Assign;
extern const mp_obj_type_t mp_type_ast_AugAssign;
extern const mp_obj_type_t mp_type_ast_AnnAssign;
extern const mp_obj_type_t mp_type_ast_For;
extern const mp_obj_type_t mp_type_ast_AsyncFor;
extern const mp_obj_type_t mp_type_ast_While;
extern const mp_obj_type_t mp_type_ast_If;
extern const mp_obj_type_t mp_type_ast_With;
extern const mp_obj_type_t mp_type_ast_AsyncWith;
extern const mp_obj_type_t mp_type_ast_Raise;
extern const mp_obj_type_t mp_type_ast_Try;
extern const mp_obj_type_t mp_type_ast_Assert;
extern const mp_obj_type_t mp_type_ast_Import;
extern const mp_obj_type_t mp_type_ast_ImportFrom;
extern const mp_obj_type_t mp_type_ast_Global;
extern const mp_obj_type_t mp_type_ast_Nonlocal;
extern const mp_obj_type_t mp_type_ast_Expr;
extern const mp_obj_type_t mp_type_ast_Pass;
extern const mp_obj_type_t mp_type_ast_Break;
extern const mp_obj_type_t mp_type_ast_Continue;
extern const mp_obj_type_t mp_type_ast_expr;
extern const mp_obj_type_t mp_type_ast_BoolOp;
extern const mp_obj_type_t mp_type_ast_NamedExpr;
extern const mp_obj_type_t mp_type_ast_BinOp;
extern const mp_obj_type_t mp_type_ast_UnaryOp;
extern const mp_obj_type_t mp_type_ast_Lambda;
extern const mp_obj_type_t mp_type_ast_IfExp;
extern const mp_obj_type_t mp_type_ast_Dict;
extern const mp_obj_type_t mp_type_ast_Set;
extern const mp_obj_type_t mp_type_ast_ListComp;
extern const mp_obj_type_t mp_type_ast_SetComp;
extern const mp_obj_type_t mp_type_ast_DictComp;
extern const mp_obj_type_t mp_type_ast_GeneratorExp;
extern const mp_obj_type_t mp_type_ast_Await;
extern const mp_obj_type_t mp_type_ast_Yield;
extern const mp_obj_type_t mp_type_ast_YieldFrom;
extern const mp_obj_type_t mp_type_ast_Compare;
extern const mp_obj_type_t mp_type_ast_Call;
extern const mp_obj_type_t mp_type_ast_FormattedValue;
extern const mp_obj_type_t mp_type_ast_JoinedStr;
extern const mp_obj_type_t mp_type_ast_Constant;
extern const mp_obj_type_t mp_type_ast_Attribute;
extern const mp_obj_type_t mp_type_ast_Subscript;
extern const mp_obj_type_t mp_type_ast_Starred;
extern const mp_obj_type_t mp_type_ast_Name;
extern const mp_obj_type_t mp_type_ast_List;
extern const mp_obj_type_t mp_type_ast_Tuple;
extern const mp_obj_type_t mp_type_ast_Slice;
extern const mp_obj_type_t mp_type_ast_expr_context;
extern const mp_obj_type_t mp_type_ast_Load;
extern const mp_obj_type_t mp_type_ast_Store;
extern const mp_obj_type_t mp_type_ast_Del;
extern const mp_obj_type_t mp_type_ast_boolop;
extern const mp_obj_type_t mp_type_ast_And;
extern const mp_obj_type_t mp_type_ast_Or;
extern const mp_obj_type_t mp_type_ast_operator;
extern const mp_obj_type_t mp_type_ast_Add;
extern const mp_obj_type_t mp_type_ast_Sub;
extern const mp_obj_type_t mp_type_ast_Mult;
extern const mp_obj_type_t mp_type_ast_MatMult;
extern const mp_obj_type_t mp_type_ast_Div;
extern const mp_obj_type_t mp_type_ast_Mod;
extern const mp_obj_type_t mp_type_ast_Pow;
extern const mp_obj_type_t mp_type_ast_LShift;
extern const mp_obj_type_t mp_type_ast_RShift;
extern const mp_obj_type_t mp_type_ast_BitOr;
extern const mp_obj_type_t mp_type_ast_BitXor;
extern const mp_obj_type_t mp_type_ast_BitAnd;
extern const mp_obj_type_t mp_type_ast_FloorDiv;
extern const mp_obj_type_t mp_type_ast_unaryop;
extern const mp_obj_type_t mp_type_ast_Invert;
extern const mp_obj_type_t mp_type_ast_Not;
extern const mp_obj_type_t mp_type_ast_UAdd;
extern const mp_obj_type_t mp_type_ast_USub;
extern const mp_obj_type_t mp_type_ast_cmpop;
extern const mp_obj_type_t mp_type_ast_Eq;
extern const mp_obj_type_t mp_type_ast_NotEq;
extern const mp_obj_type_t mp_type_ast_Lt;
extern const mp_obj_type_t mp_type_ast_LtE;
extern const mp_obj_type_t mp_type_ast_Gt;
extern const mp_obj_type_t mp_type_ast_GtE;
extern const mp_obj_type_t mp_type_ast_Is;
extern const mp_obj_type_t mp_type_ast_IsNot;
extern const mp_obj_type_t mp_type_ast_In;
extern const mp_obj_type_t mp_type_ast_NotIn;
extern const mp_obj_type_t mp_type_ast_comprehension;
extern const mp_obj_type_t mp_type_ast_excepthandler;
extern const mp_obj_type_t mp_type_ast_ExceptHandler;
extern const mp_obj_type_t mp_type_ast_arguments;
extern const mp_obj_type_t mp_type_ast_arg;
extern const mp_obj_type_t mp_type_ast_keyword;
extern const mp_obj_type_t mp_type_ast_alias;
extern const mp_obj_type_t mp_type_ast_withitem;
extern const mp_obj_type_t mp_type_ast_type_ignore;
extern const mp_obj_type_t mp_type_ast_TypeIgnore;
mp_obj_t mp_ast_parse_to_obj(mp_obj_t source, mp_obj_t filename, mp_obj_t mode);

#endif
