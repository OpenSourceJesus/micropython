
/*
 * Convert MicroPython parse tree to CPython-compatible AST nodes.
 */
#include <stdarg.h>
#include <string.h>
#include "py/lexer.h"
#include "py/parse.h"
#include "py/parsenum.h"
#include "py/objlist.h"
#include "py/objstr.h"
#include "py/objtuple.h"
#include "py/runtime.h"
#include "py/misc.h"
#include "extmod/modast/ast_types.h"

#if MICROPY_PY_AST

typedef enum {
#define DEF_RULE(rule, comp, kind, ...) PN_##rule,
#define DEF_RULE_NC(rule, kind, ...)
    #include "py/grammar.h"
#undef DEF_RULE
#undef DEF_RULE_NC
    PN_const_object,
#define DEF_RULE(rule, comp, kind, ...)
#define DEF_RULE_NC(rule, kind, ...) PN_##rule,
    #include "py/grammar.h"
#undef DEF_RULE
#undef DEF_RULE_NC
} pn_kind_t;

#define MP_PARSE_NODE_TESTLIST_COMP_HAS_COMP_FOR(pns) \
    (MP_PARSE_NODE_STRUCT_NUM_NODES(pns) == 2 && \
    MP_PARSE_NODE_IS_STRUCT_KIND(pns->nodes[1], PN_comp_for))

static uint16_t ast_line_of(mp_parse_node_t pn) {
    if (MP_PARSE_NODE_IS_STRUCT(pn)) {
        return ((mp_parse_node_struct_t *)pn)->source_line;
    }
    return 1;
}

static mp_obj_t ast_list_new(void) {
    return mp_obj_new_list(0, NULL);
}

static void ast_list_append(mp_obj_t list, mp_obj_t item) {
    mp_obj_list_append(list, item);
}

static mp_obj_t ast_list_from_nodes(size_t n, mp_obj_t *items) {
    return mp_obj_new_list(n, items);
}

static mp_obj_t ast_conv_node(mp_parse_node_t pn);
static mp_obj_t ast_conv_struct(mp_parse_node_struct_t *pns);

static mp_obj_t ast_conv_nodes_from_struct(mp_parse_node_struct_t *pns) {
    size_t n = MP_PARSE_NODE_STRUCT_NUM_NODES(pns);
    mp_obj_t list = ast_list_new();
    for (size_t i = 0; i < n; i++) {
        mp_obj_t o = ast_conv_node(pns->nodes[i]);
        if (o != mp_const_none) {
            ast_list_append(list, o);
        }
    }
    return list;
}

static mp_obj_t ast_conv_suite(mp_parse_node_t pn) {
    if (MP_PARSE_NODE_IS_NULL(pn)) {
        return ast_list_new();
    }
    if (MP_PARSE_NODE_IS_STRUCT_KIND(pn, PN_suite_block_stmts)) {
        return ast_conv_nodes_from_struct((mp_parse_node_struct_t *)pn);
    }
    mp_parse_node_struct_t *pns = (mp_parse_node_struct_t *)pn;
    if (MP_PARSE_NODE_STRUCT_KIND(pns) == PN_suite_block_stmts) {
        return ast_conv_nodes_from_struct(pns);
    }
    // single stmt suite
    mp_obj_t list = ast_list_new();
    ast_list_append(list, ast_conv_node(pn));
    return list;
}

static mp_obj_t ast_const_from_leaf(mp_parse_node_t pn) {
    if (MP_PARSE_NODE_IS_SMALL_INT(pn)) {
        return mp_ast_make(MP_AST_CONSTANT, ast_line_of(pn), 0, 2,
            (mp_obj_t[]){mp_obj_new_int(MP_PARSE_NODE_LEAF_SMALL_INT(pn)), mp_const_none});
    }
    if (MP_PARSE_NODE_IS_ID(pn)) {
        return mp_ast_make(MP_AST_NAME, ast_line_of(pn), 0, 2,
            (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_PARSE_NODE_LEAF_ARG(pn)), mp_ast_singleton(MP_AST_LOAD)});
    }
    if (MP_PARSE_NODE_IS_STRUCT_KIND(pn, PN_const_object)) {
        mp_obj_t val = mp_parse_node_extract_const_object((mp_parse_node_struct_t *)pn);
        return mp_ast_make(MP_AST_CONSTANT, ast_line_of(pn), 0, 2, (mp_obj_t[]){val, mp_const_none});
    }
    if (MP_PARSE_NODE_IS_LEAF(pn)) {
        uintptr_t arg = MP_PARSE_NODE_LEAF_ARG(pn);
        if (MP_PARSE_NODE_LEAF_KIND(pn) == MP_PARSE_NODE_STRING) {
            return mp_ast_make(MP_AST_CONSTANT, ast_line_of(pn), 0, 2,
                (mp_obj_t[]){MP_OBJ_NEW_QSTR(arg), mp_const_none});
        }
        if (MP_PARSE_NODE_IS_TOKEN(pn)) {
            mp_obj_t val = mp_const_none;
            switch (arg) {
                case MP_TOKEN_KW_NONE: val = mp_const_none; break;
                case MP_TOKEN_KW_TRUE: val = mp_const_true; break;
                case MP_TOKEN_KW_FALSE: val = mp_const_false; break;
                default: val = mp_const_none; break;
            }
            return mp_ast_make(MP_AST_CONSTANT, ast_line_of(pn), 0, 2, (mp_obj_t[]){val, mp_const_none});
        }
    }
    return ast_conv_node(pn);
}


static mp_obj_t ast_conv_target(mp_parse_node_t pn, mp_obj_t ctx) {
    if (MP_PARSE_NODE_IS_ID(pn)) {
        mp_obj_t n = mp_ast_make(MP_AST_NAME, ast_line_of(pn), 0, 2,
            (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_PARSE_NODE_LEAF_ARG(pn)), ctx});
        return n;
    }
    if (MP_PARSE_NODE_IS_STRUCT_KIND(pn, PN_star_expr)) {
        mp_parse_node_struct_t *pns = (mp_parse_node_struct_t *)pn;
        mp_obj_t v = ast_conv_node(pns->nodes[0]);
        return mp_ast_make(MP_AST_STARRED, ast_line_of(pn), 0, 2, (mp_obj_t[]){v, ctx});
    }
    if (MP_PARSE_NODE_IS_STRUCT(pn)) {
        mp_parse_node_struct_t *pns = (mp_parse_node_struct_t *)pn;
        if (MP_PARSE_NODE_STRUCT_KIND(pns) == PN_atom_expr_normal) {
            mp_obj_t base = ast_conv_node(pns->nodes[0]);
            mp_parse_node_t trail = pns->nodes[1];
            if (MP_PARSE_NODE_IS_STRUCT_KIND(trail, PN_atom_expr_trailers)) {
                mp_parse_node_struct_t *pts = (mp_parse_node_struct_t *)trail;
                size_t nt = MP_PARSE_NODE_STRUCT_NUM_NODES(pts);
                for (size_t i = 0; i < nt; i++) {
                    mp_parse_node_struct_t *tr = (mp_parse_node_struct_t *)pts->nodes[i];
                    if (MP_PARSE_NODE_STRUCT_KIND(tr) == PN_trailer_period) {
                        base = mp_ast_make(MP_AST_ATTRIBUTE, tr->source_line, 0, 3,
                            (mp_obj_t[]){base, MP_OBJ_NEW_QSTR(MP_PARSE_NODE_LEAF_ARG(tr->nodes[0])), ctx});
                    } else if (MP_PARSE_NODE_STRUCT_KIND(tr) == PN_trailer_bracket) {
                        mp_obj_t sl = ast_conv_node(tr->nodes[0]);
                        base = mp_ast_make(MP_AST_SUBSCRIPT, tr->source_line, 0, 3,
                            (mp_obj_t[]){base, sl, ctx});
                    }
                }
            }
            return base;
        }
        if (MP_PARSE_NODE_STRUCT_KIND(pns) == PN_testlist_star_expr
            || MP_PARSE_NODE_STRUCT_KIND(pns) == PN_testlist_comp
            || MP_PARSE_NODE_STRUCT_KIND(pns) == PN_exprlist) {
            size_t n = MP_PARSE_NODE_STRUCT_NUM_NODES(pns);
            if (n == 1) {
                return ast_conv_target(pns->nodes[0], ctx);
            }
            mp_obj_t *elts = m_new(mp_obj_t, n);
            for (size_t i = 0; i < n; i++) {
                elts[i] = ast_conv_target(pns->nodes[i], ctx);
            }
            mp_obj_t tup = mp_ast_make(MP_AST_TUPLE, pns->source_line, 0, 2,
                (mp_obj_t[]){mp_obj_new_list(n, elts), ctx});
            return tup;
        }
        if (MP_PARSE_NODE_STRUCT_KIND(pns) == PN_atom_paren) {
            mp_parse_node_t inner = pns->nodes[0];
            if (MP_PARSE_NODE_IS_NULL(inner)) {
                return mp_ast_make(MP_AST_TUPLE, pns->source_line, 0, 2,
                    (mp_obj_t[]){ast_list_new(), ctx});
            }
            return ast_conv_target(inner, ctx);
        }
    }
    return ast_conv_node(pn);
}

static mp_obj_t ast_binop_token(mp_token_kind_t tok) {
    switch (tok) {
        case MP_TOKEN_OP_PIPE: return mp_ast_singleton(MP_AST_BITOR);
        case MP_TOKEN_OP_CARET: return mp_ast_singleton(MP_AST_BITXOR);
        case MP_TOKEN_OP_AMPERSAND: return mp_ast_singleton(MP_AST_BITAND);
        case MP_TOKEN_OP_DBL_LESS: return mp_ast_singleton(MP_AST_LSHIFT);
        case MP_TOKEN_OP_DBL_MORE: return mp_ast_singleton(MP_AST_RSHIFT);
        case MP_TOKEN_OP_PLUS: return mp_ast_singleton(MP_AST_ADD);
        case MP_TOKEN_OP_MINUS: return mp_ast_singleton(MP_AST_SUB);
        case MP_TOKEN_OP_STAR: return mp_ast_singleton(MP_AST_MULT);
        case MP_TOKEN_OP_AT: return mp_ast_singleton(MP_AST_MATMULT);
        case MP_TOKEN_OP_SLASH: return mp_ast_singleton(MP_AST_DIV);
        case MP_TOKEN_OP_DBL_SLASH: return mp_ast_singleton(MP_AST_FLOORDIV);
        case MP_TOKEN_OP_PERCENT: return mp_ast_singleton(MP_AST_MOD);
        default: return mp_ast_singleton(MP_AST_ADD);
    }
}

static mp_obj_t ast_cmpop_from_node(mp_parse_node_t pn) {
    if (MP_PARSE_NODE_IS_TOKEN(pn)) {
        mp_token_kind_t tok = MP_PARSE_NODE_LEAF_ARG(pn);
        if (tok == MP_TOKEN_KW_IN) return mp_ast_singleton(MP_AST_IN);
        mp_binary_op_t op = MP_BINARY_OP_LESS + (tok - MP_TOKEN_OP_LESS);
        switch (op) {
            case MP_BINARY_OP_LESS: return mp_ast_singleton(MP_AST_LT);
            case MP_BINARY_OP_LESS_EQUAL: return mp_ast_singleton(MP_AST_LTE);
            case MP_BINARY_OP_MORE: return mp_ast_singleton(MP_AST_GT);
            case MP_BINARY_OP_MORE_EQUAL: return mp_ast_singleton(MP_AST_GTE);
            case MP_BINARY_OP_EQUAL: return mp_ast_singleton(MP_AST_EQ);
            case MP_BINARY_OP_NOT_EQUAL: return mp_ast_singleton(MP_AST_NOTEQ);
            default: break;
        }
    } else if (MP_PARSE_NODE_IS_STRUCT(pn)) {
        mp_parse_node_struct_t *pns = (mp_parse_node_struct_t *)pn;
        if (MP_PARSE_NODE_STRUCT_KIND(pns) == PN_comp_op_not_in) {
            return mp_ast_singleton(MP_AST_NOTIN);
        }
        if (MP_PARSE_NODE_STRUCT_KIND(pns) == PN_comp_op_is) {
            return MP_PARSE_NODE_IS_NULL(pns->nodes[0])
                ? mp_ast_singleton(MP_AST_IS) : mp_ast_singleton(MP_AST_ISNOT);
        }
    }
    return mp_ast_singleton(MP_AST_EQ);
}

static mp_obj_t ast_conv_atom_expr(mp_parse_node_struct_t *pns, bool allow_await) {
    mp_obj_t cur = ast_conv_node(pns->nodes[0]);
    if (MP_PARSE_NODE_IS_NULL(pns->nodes[1])) {
        return cur;
    }
    mp_parse_node_struct_t **pns_trail = (mp_parse_node_struct_t **)&pns->nodes[1];
    size_t num_trail = 1;
    if (MP_PARSE_NODE_STRUCT_KIND(pns_trail[0]) == PN_atom_expr_trailers) {
        num_trail = MP_PARSE_NODE_STRUCT_NUM_NODES(pns_trail[0]);
        pns_trail = (mp_parse_node_struct_t **)&pns_trail[0]->nodes[0];
    }
    for (size_t i = 0; i < num_trail; i++) {
        mp_parse_node_struct_t *tr = pns_trail[i];
        if (MP_PARSE_NODE_STRUCT_KIND(tr) == PN_trailer_paren) {
            mp_parse_node_t *args;
            size_t na = mp_parse_node_extract_list(&tr->nodes[0], PN_arglist, &args);
            mp_obj_t arglist = ast_list_new();
            mp_obj_t kwlist = ast_list_new();
            for (size_t j = 0; j < na; j++) {
                if (MP_PARSE_NODE_IS_STRUCT_KIND(args[j], PN_argument)) {
                    mp_parse_node_struct_t *arg = (mp_parse_node_struct_t *)args[j];
                    if (MP_PARSE_NODE_IS_ID(arg->nodes[0])) {
                        mp_obj_t kw = mp_ast_make(MP_AST_KEYWORD, arg->source_line, 0, 2,
                            (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_PARSE_NODE_LEAF_ARG(arg->nodes[0])), ast_conv_node(arg->nodes[1])});
                        ast_list_append(kwlist, kw);
                    } else {
                        ast_list_append(arglist, ast_conv_node(arg->nodes[0]));
                    }
                } else {
                    ast_list_append(arglist, ast_conv_node(args[j]));
                }
            }
            cur = mp_ast_make(MP_AST_CALL, tr->source_line, 0, 3, (mp_obj_t[]){cur, arglist, kwlist});
        } else if (MP_PARSE_NODE_STRUCT_KIND(tr) == PN_trailer_bracket) {
            mp_obj_t sl = ast_conv_node(tr->nodes[0]);
            cur = mp_ast_make(MP_AST_SUBSCRIPT, tr->source_line, 0, 3,
                (mp_obj_t[]){cur, sl, mp_ast_singleton(MP_AST_LOAD)});
        } else if (MP_PARSE_NODE_STRUCT_KIND(tr) == PN_trailer_period) {
            cur = mp_ast_make(MP_AST_ATTRIBUTE, tr->source_line, 0, 3,
                (mp_obj_t[]){cur, MP_OBJ_NEW_QSTR(MP_PARSE_NODE_LEAF_ARG(tr->nodes[0])), mp_ast_singleton(MP_AST_LOAD)});
        }
    }
    (void)allow_await;
    return cur;
}

static mp_obj_t ast_conv_comp(mp_parse_node_struct_t *pns, uint16_t comp_kind) {
    mp_obj_t elt = ast_conv_node(pns->nodes[0]);
    mp_obj_t gens = ast_list_new();
    mp_parse_node_struct_t *cf = (mp_parse_node_struct_t *)pns->nodes[1];
    while (cf && MP_PARSE_NODE_IS_STRUCT_KIND((mp_parse_node_t)cf, PN_comp_for)) {
        mp_obj_t target = ast_conv_target(cf->nodes[0], mp_ast_singleton(MP_AST_STORE));
        mp_obj_t iter = ast_conv_node(cf->nodes[1]);
        mp_obj_t ifs = ast_list_new();
        mp_parse_node_t ifn = cf->nodes[2];
        if (!MP_PARSE_NODE_IS_NULL(ifn)) {
            mp_parse_node_t *ifnodes;
            size_t ni = mp_parse_node_extract_list(&ifn, PN_comp_if, &ifnodes);
            for (size_t j = 0; j < ni; j++) {
                ast_list_append(ifs, ast_conv_node(ifnodes[j]));
            }
        }
        mp_obj_t comp = mp_ast_make(MP_AST_COMPREHENSION, cf->source_line, 0, 4,
            (mp_obj_t[]){target, iter, ifs, mp_const_false});
        ast_list_append(gens, comp);
        cf = MP_PARSE_NODE_IS_STRUCT(cf->nodes[3]) ? (mp_parse_node_struct_t *)cf->nodes[3] : NULL;
    }
    if (comp_kind == MP_AST_LISTCOMP) {
        return mp_ast_make(MP_AST_LISTCOMP, pns->source_line, 0, 2, (mp_obj_t[]){elt, gens});
    } else if (comp_kind == MP_AST_SETCOMP) {
        return mp_ast_make(MP_AST_SETCOMP, pns->source_line, 0, 2, (mp_obj_t[]){elt, gens});
    } else if (comp_kind == MP_AST_DICTCOMP) {
        mp_parse_node_struct_t *item = (mp_parse_node_struct_t *)pns->nodes[0];
        mp_obj_t key = ast_conv_node(item->nodes[0]);
        mp_obj_t val = ast_conv_node(item->nodes[1]);
        return mp_ast_make(MP_AST_DICTCOMP, pns->source_line, 0, 3, (mp_obj_t[]){key, val, gens});
    } else {
        return mp_ast_make(MP_AST_GENERATOREXP, pns->source_line, 0, 2, (mp_obj_t[]){elt, gens});
    }
}

static mp_obj_t ast_conv_annotation(mp_parse_node_t pn) {
    if (MP_PARSE_NODE_IS_NULL(pn)) {
        return mp_const_none;
    }
    if (MP_PARSE_NODE_IS_STRUCT_KIND(pn, PN_generic_colon_test)) {
        mp_parse_node_struct_t *ps = (mp_parse_node_struct_t *)pn;
        if (MP_PARSE_NODE_STRUCT_NUM_NODES(ps) > 0) {
            pn = ps->nodes[0];
        }
    }
    return ast_conv_node(pn);
}

static mp_obj_t ast_conv_default(mp_parse_node_t pn) {
    if (MP_PARSE_NODE_IS_NULL(pn)) {
        return mp_const_none;
    }
    if (MP_PARSE_NODE_IS_STRUCT_KIND(pn, PN_generic_equal_test)) {
        mp_parse_node_struct_t *ps = (mp_parse_node_struct_t *)pn;
        if (MP_PARSE_NODE_STRUCT_NUM_NODES(ps) > 0) {
            pn = ps->nodes[0];
        }
    }
    return ast_conv_node(pn);
}

static mp_obj_t ast_make_arguments(mp_parse_node_t pn_params) {
    mp_obj_t posonly = ast_list_new();
    mp_obj_t args = ast_list_new();
    mp_obj_t kwonly = ast_list_new();
    mp_obj_t kw_defaults = ast_list_new();
    mp_obj_t defaults = ast_list_new();
    mp_obj_t vararg = mp_const_none;
    mp_obj_t kwarg = mp_const_none;
    if (!MP_PARSE_NODE_IS_NULL(pn_params)) {
        mp_parse_node_t *pnodes;
        size_t n = mp_parse_node_extract_list(&pn_params, PN_typedargslist, &pnodes);
        bool kwonly_section = false;
        for (size_t i = 0; i < n; i++) {
            mp_parse_node_t p = pnodes[i];
            if (MP_PARSE_NODE_IS_STRUCT_KIND(p, PN_typedargslist_star)) {
                mp_parse_node_struct_t *ps = (mp_parse_node_struct_t *)p;
                if (!MP_PARSE_NODE_IS_NULL(ps->nodes[0])) {
                    vararg = mp_ast_make(MP_AST_arg, ps->source_line, 0, 3,
                        (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_PARSE_NODE_LEAF_ARG(ps->nodes[0])), mp_const_none, mp_const_none});
                }
                kwonly_section = true;
            } else if (MP_PARSE_NODE_IS_STRUCT_KIND(p, PN_typedargslist_dbl_star)) {
                mp_parse_node_struct_t *ps = (mp_parse_node_struct_t *)p;
                kwarg = mp_ast_make(MP_AST_arg, ps->source_line, 0, 3,
                    (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_PARSE_NODE_LEAF_ARG(ps->nodes[0])), mp_const_none, mp_const_none});
            } else {
                qstr name = 0;
                mp_obj_t ann = mp_const_none;
                mp_obj_t def = mp_const_none;
                if (MP_PARSE_NODE_IS_ID(p)) {
                    name = MP_PARSE_NODE_LEAF_ARG(p);
                } else if (MP_PARSE_NODE_IS_STRUCT_KIND(p, PN_typedargslist_name)) {
                    mp_parse_node_struct_t *ps = (mp_parse_node_struct_t *)p;
                    if (!MP_PARSE_NODE_IS_ID(ps->nodes[0])) {
                        continue;
                    }
                    name = MP_PARSE_NODE_LEAF_ARG(ps->nodes[0]);
                    if (MP_PARSE_NODE_STRUCT_NUM_NODES(ps) > 1 && !MP_PARSE_NODE_IS_NULL(ps->nodes[1])) {
                        ann = ast_conv_annotation(ps->nodes[1]);
                    }
                    if (MP_PARSE_NODE_STRUCT_NUM_NODES(ps) > 2 && !MP_PARSE_NODE_IS_NULL(ps->nodes[2])) {
                        def = ast_conv_default(ps->nodes[2]);
                    }
                } else if (MP_PARSE_NODE_IS_STRUCT_KIND(p, PN_tfpdef)) {
                    mp_parse_node_struct_t *ps = (mp_parse_node_struct_t *)p;
                    if (!MP_PARSE_NODE_IS_ID(ps->nodes[0])) {
                        continue;
                    }
                    name = MP_PARSE_NODE_LEAF_ARG(ps->nodes[0]);
                    if (MP_PARSE_NODE_STRUCT_NUM_NODES(ps) > 1 && !MP_PARSE_NODE_IS_NULL(ps->nodes[1])) {
                        ann = ast_conv_annotation(ps->nodes[1]);
                    }
                } else {
                    continue;
                }
                size_t name_len;
                const char *name_str = (const char *)qstr_data(name, &name_len);
                mp_obj_t a = mp_ast_make(MP_AST_arg, ast_line_of(p), 0, 3,
                    (mp_obj_t[]){mp_obj_new_str(name_str, name_len), ann, mp_const_none});
                if (def != mp_const_none) {
                    if (kwonly_section) {
                        ast_list_append(kwonly, a);
                        ast_list_append(kw_defaults, def);
                    } else {
                        ast_list_append(args, a);
                        ast_list_append(defaults, def);
                    }
                } else if (kwonly_section) {
                    ast_list_append(kwonly, a);
                    ast_list_append(kw_defaults, mp_const_none);
                } else {
                    ast_list_append(args, a);
                }
            }
        }
    }
    return mp_ast_make(MP_AST_ARGUMENTS, ast_line_of(pn_params), 0, 7,
        (mp_obj_t[]){posonly, args, vararg, kwonly, kw_defaults, kwarg, defaults});
}


static mp_obj_t ast_conv_generic_all_nodes(mp_parse_node_struct_t *pns) {
    size_t n = MP_PARSE_NODE_STRUCT_NUM_NODES(pns);
    if (MP_PARSE_NODE_STRUCT_KIND(pns) == PN_pass_stmt && n == 0) {
        return mp_ast_make(MP_AST_PASS, pns->source_line, 0, 0, NULL);
    }
    if (n == 1) {
        return ast_conv_node(pns->nodes[0]);
    }
    mp_obj_t list = ast_list_new();
    for (size_t i = 0; i < n; i++) {
        mp_obj_t o = ast_conv_node(pns->nodes[i]);
        if (o != mp_const_none) {
            ast_list_append(list, o);
        }
    }
    return list;
}

static mp_obj_t ast_conv_funcdef_inner(mp_parse_node_struct_t *pns, bool is_async) {
    qstr name = MP_PARSE_NODE_LEAF_ARG(pns->nodes[0]);
    mp_obj_t args = ast_make_arguments(pns->nodes[1]);
    mp_obj_t returns = mp_const_none;
    if (!MP_PARSE_NODE_IS_NULL(pns->nodes[2])) {
        mp_parse_node_t ret = pns->nodes[2];
        if (MP_PARSE_NODE_IS_STRUCT_KIND(ret, PN_funcdefrettype)) {
            ret = ((mp_parse_node_struct_t *)ret)->nodes[0];
        }
        returns = ast_conv_node(ret);
    }
    mp_obj_t body = ast_conv_suite(pns->nodes[3]);
    mp_obj_t fields[] = {
        MP_OBJ_NEW_QSTR(name), args, body, ast_list_new(), returns, mp_const_none, ast_list_new()
    };
    return mp_ast_make(is_async ? MP_AST_ASYNCFUNCTIONDEF : MP_AST_FUNCTIONDEF,
        pns->source_line, 0, 7, fields);
}

static mp_obj_t ast_conv_classdef(mp_parse_node_struct_t *pns) {
    qstr name = MP_PARSE_NODE_LEAF_ARG(pns->nodes[0]);
    mp_obj_t bases = ast_list_new();
    mp_obj_t keywords = ast_list_new();
    mp_parse_node_t parents = pns->nodes[1];
    if (!MP_PARSE_NODE_IS_NULL(parents) && !MP_PARSE_NODE_IS_STRUCT_KIND(parents, PN_classdef_2)) {
        mp_parse_node_t *pnodes;
        size_t n = mp_parse_node_extract_list(&parents, PN_arglist, &pnodes);
        for (size_t i = 0; i < n; i++) {
            ast_list_append(bases, ast_conv_node(pnodes[i]));
        }
    }
    mp_obj_t body = ast_conv_suite(pns->nodes[2]);
    mp_obj_t fields[] = {MP_OBJ_NEW_QSTR(name), bases, keywords, body, ast_list_new(), ast_list_new()};
    return mp_ast_make(MP_AST_CLASSDEF, pns->source_line, 0, 6, fields);
}

static mp_obj_t ast_conv_if_stmt(mp_parse_node_struct_t *pns) {
    mp_obj_t test = ast_conv_node(pns->nodes[0]);
    mp_obj_t body = ast_conv_suite(pns->nodes[1]);
    mp_obj_t orelse = ast_list_new();
    mp_parse_node_t *elifs;
    size_t ne = mp_parse_node_extract_list(&pns->nodes[2], PN_if_stmt_elif_list, &elifs);
    for (size_t i = 0; i < ne; i++) {
        mp_parse_node_struct_t *e = (mp_parse_node_struct_t *)elifs[i];
        mp_obj_t e_test = ast_conv_node(e->nodes[0]);
        mp_obj_t e_body = ast_conv_suite(e->nodes[1]);
        mp_obj_t elif_node = mp_ast_make(MP_AST_IF, e->source_line, 0, 3,
            (mp_obj_t[]){e_test, e_body, ast_list_new()});
        ast_list_append(orelse, elif_node);
    }
    if (!MP_PARSE_NODE_IS_NULL(pns->nodes[3])) {
        mp_obj_t eb = ast_conv_suite(pns->nodes[3]);
        if (mp_obj_is_type(eb, &mp_type_list)) {
            mp_obj_list_t *lst = MP_OBJ_TO_PTR(eb);
            for (size_t i = 0; i < lst->len; i++) {
                ast_list_append(orelse, lst->items[i]);
            }
        } else {
            ast_list_append(orelse, eb);
        }
    }
    return mp_ast_make(MP_AST_IF, pns->source_line, 0, 3, (mp_obj_t[]){test, body, orelse});
}

static mp_obj_t ast_conv_for_stmt_inner(mp_parse_node_struct_t *pns, bool is_async) {
    mp_obj_t target = ast_conv_target(pns->nodes[0], mp_ast_singleton(MP_AST_STORE));
    mp_obj_t iter = ast_conv_node(pns->nodes[1]);
    mp_obj_t body = ast_conv_suite(pns->nodes[2]);
    mp_obj_t orelse = MP_PARSE_NODE_IS_NULL(pns->nodes[3]) ? ast_list_new() : ast_conv_suite(pns->nodes[3]);
    return mp_ast_make(is_async ? MP_AST_ASYNCFOR : MP_AST_FOR, pns->source_line, 0, 5,
        (mp_obj_t[]){target, iter, body, orelse, mp_const_none});
}

static mp_obj_t ast_conv_while_stmt(mp_parse_node_struct_t *pns) {
    mp_obj_t test = ast_conv_node(pns->nodes[0]);
    mp_obj_t body = ast_conv_suite(pns->nodes[1]);
    mp_obj_t orelse = MP_PARSE_NODE_IS_NULL(pns->nodes[2]) ? ast_list_new() : ast_conv_suite(pns->nodes[2]);
    return mp_ast_make(MP_AST_WHILE, pns->source_line, 0, 3, (mp_obj_t[]){test, body, orelse});
}

static void ast_conv_try_except_handler(mp_parse_node_struct_t *eh, mp_obj_t handlers) {
    mp_obj_t type = mp_const_none;
    mp_obj_t name = mp_const_none;
    mp_parse_node_t exc = eh->nodes[0];
    if (!MP_PARSE_NODE_IS_NULL(exc)) {
        if (MP_PARSE_NODE_IS_STRUCT_KIND(exc, PN_try_stmt_as_name)) {
            mp_parse_node_struct_t *asn = (mp_parse_node_struct_t *)exc;
            type = ast_conv_node(asn->nodes[0]);
            if (MP_PARSE_NODE_STRUCT_NUM_NODES(asn) > 1 && !MP_PARSE_NODE_IS_NULL(asn->nodes[1])) {
                mp_parse_node_t asname = asn->nodes[1];
                if (MP_PARSE_NODE_IS_ID(asname)) {
                    name = MP_OBJ_NEW_QSTR(MP_PARSE_NODE_LEAF_ARG(asname));
                } else if (MP_PARSE_NODE_IS_STRUCT_KIND(asname, PN_as_name)) {
                    name = MP_OBJ_NEW_QSTR(MP_PARSE_NODE_LEAF_ARG(((mp_parse_node_struct_t *)asname)->nodes[1]));
                }
            }
        } else {
            type = ast_conv_node(exc);
        }
    }
    mp_obj_t hbody = ast_conv_suite(eh->nodes[1]);
    ast_list_append(handlers, mp_ast_make(MP_AST_EXCEPTHANDLER, eh->source_line, 0, 3,
        (mp_obj_t[]){type, name, hbody}));
}

static mp_obj_t ast_conv_try_stmt(mp_parse_node_struct_t *pns) {
    mp_obj_t body = ast_conv_suite(pns->nodes[0]);
    mp_obj_t handlers = ast_list_new();
    mp_obj_t orelse = ast_list_new();
    mp_obj_t finalbody = ast_list_new();
    mp_parse_node_t pn2 = pns->nodes[1];
    if (MP_PARSE_NODE_IS_STRUCT(pn2)) {
        mp_parse_node_struct_t *p2 = (mp_parse_node_struct_t *)pn2;
        if (MP_PARSE_NODE_STRUCT_KIND(p2) == PN_try_stmt_finally) {
            finalbody = ast_conv_suite(p2->nodes[0]);
        } else if (MP_PARSE_NODE_STRUCT_KIND(p2) == PN_try_stmt_except_and_more) {
            mp_parse_node_t *excepts;
            size_t ne = mp_parse_node_extract_list(&p2->nodes[0], PN_try_stmt_except_list, &excepts);
            for (size_t i = 0; i < ne; i++) {
                ast_conv_try_except_handler((mp_parse_node_struct_t *)excepts[i], handlers);
            }
            if (!MP_PARSE_NODE_IS_NULL(p2->nodes[1])) {
                orelse = ast_conv_suite(p2->nodes[1]);
            }
            if (!MP_PARSE_NODE_IS_NULL(p2->nodes[2])) {
                mp_parse_node_struct_t *fin = (mp_parse_node_struct_t *)p2->nodes[2];
                finalbody = ast_conv_suite(fin->nodes[0]);
            }
        } else {
            mp_parse_node_t *excepts;
            size_t ne = mp_parse_node_extract_list(&pn2, PN_try_stmt_except_list, &excepts);
            for (size_t i = 0; i < ne; i++) {
                ast_conv_try_except_handler((mp_parse_node_struct_t *)excepts[i], handlers);
            }
        }
    }
    return mp_ast_make(MP_AST_TRY, pns->source_line, 0, 4, (mp_obj_t[]){body, handlers, orelse, finalbody});
}

static mp_obj_t ast_conv_with_stmt_inner(mp_parse_node_struct_t *pns, bool is_async) {
    mp_parse_node_t *items;
    size_t n = mp_parse_node_extract_list(&pns->nodes[0], PN_with_stmt_list, &items);
    mp_obj_t witems = ast_list_new();
    for (size_t i = 0; i < n; i++) {
        mp_parse_node_struct_t *it = (mp_parse_node_struct_t *)items[i];
        mp_obj_t ctx = ast_conv_node(it->nodes[0]);
        mp_obj_t opt = MP_PARSE_NODE_IS_NULL(it->nodes[1]) ? mp_const_none
            : ast_conv_target(it->nodes[1], mp_ast_singleton(MP_AST_STORE));
        ast_list_append(witems, mp_ast_make(MP_AST_WITHITEM, it->source_line, 0, 2, (mp_obj_t[]){ctx, opt}));
    }
    mp_obj_t body = ast_conv_suite(pns->nodes[1]);
    return mp_ast_make(is_async ? MP_AST_ASYNCWITH : MP_AST_WITH, pns->source_line, 0, 3,
        (mp_obj_t[]){witems, body, mp_const_none});
}

static mp_obj_t ast_conv_expr_stmt(mp_parse_node_struct_t *pns) {
    mp_parse_node_t lhs = pns->nodes[0];
    mp_parse_node_t rhs = pns->nodes[1];
    if (MP_PARSE_NODE_IS_NULL(rhs)) {
        return mp_ast_make(MP_AST_EXPR, pns->source_line, 0, 1, (mp_obj_t[]){ast_conv_node(lhs)});
    }
    if (MP_PARSE_NODE_IS_STRUCT(rhs)) {
        mp_parse_node_struct_t *pns1 = (mp_parse_node_struct_t *)rhs;
        int kind = MP_PARSE_NODE_STRUCT_KIND(pns1);
        if (kind == PN_annassign) {
            mp_obj_t target = ast_conv_target(lhs, mp_ast_singleton(MP_AST_STORE));
            mp_obj_t ann = ast_conv_node(pns1->nodes[0]);
            mp_obj_t val = mp_const_none;
            if (!MP_PARSE_NODE_IS_NULL(pns1->nodes[1])) {
                mp_parse_node_t assign_pn = pns1->nodes[1];
                if (MP_PARSE_NODE_IS_STRUCT_KIND(assign_pn, PN_expr_stmt_assign)) {
                    val = ast_conv_node(((mp_parse_node_struct_t *)assign_pn)->nodes[0]);
                } else {
                    val = ast_conv_node(assign_pn);
                }
            }
            return mp_ast_make(MP_AST_ANNASSIGN, pns->source_line, 0, 4,
                (mp_obj_t[]){target, ann, val, mp_const_false});
        }
        if (kind == PN_expr_stmt_augassign) {
            mp_token_kind_t tok = MP_PARSE_NODE_LEAF_ARG(pns1->nodes[0]);
            mp_obj_t op = ast_binop_token(tok - MP_TOKEN_DEL_PIPE_EQUAL + MP_TOKEN_OP_PIPE);
            return mp_ast_make(MP_AST_AUGASSIGN, pns->source_line, 0, 3,
                (mp_obj_t[]){ast_conv_target(lhs, mp_ast_singleton(MP_AST_STORE)), op, ast_conv_node(pns1->nodes[1])});
        }
        if (kind == PN_expr_stmt_assign_list) {
            mp_obj_t val = ast_conv_node(pns1->nodes[MP_PARSE_NODE_STRUCT_NUM_NODES(pns1) - 1]);
            mp_obj_t targets = ast_list_new();
            ast_list_append(targets, ast_conv_target(lhs, mp_ast_singleton(MP_AST_STORE)));
            for (size_t i = 0; i < (size_t)MP_PARSE_NODE_STRUCT_NUM_NODES(pns1) - 1; i++) {
                ast_list_append(targets, ast_conv_target(pns1->nodes[i], mp_ast_singleton(MP_AST_STORE)));
            }
            return mp_ast_make(MP_AST_ASSIGN, pns->source_line, 0, 3,
                (mp_obj_t[]){targets, val, mp_const_none});
        }
    }
    mp_obj_t targets = ast_list_new();
    ast_list_append(targets, ast_conv_target(lhs, mp_ast_singleton(MP_AST_STORE)));
    return mp_ast_make(MP_AST_ASSIGN, pns->source_line, 0, 3,
        (mp_obj_t[]){targets, ast_conv_node(rhs), mp_const_none});
}

static mp_obj_t ast_conv_or_and_test(mp_parse_node_struct_t *pns) {
    size_t n = MP_PARSE_NODE_STRUCT_NUM_NODES(pns);
    mp_obj_t op = mp_ast_singleton(MP_PARSE_NODE_STRUCT_KIND(pns) == PN_or_test ? MP_AST_OR : MP_AST_AND);
    mp_obj_t vals = ast_list_new();
    for (size_t i = 0; i < n; i++) {
        ast_list_append(vals, ast_conv_node(pns->nodes[i]));
    }
    return mp_ast_make(MP_AST_BOOLOP, pns->source_line, 0, 2, (mp_obj_t[]){op, vals});
}

static mp_obj_t ast_conv_comparison(mp_parse_node_struct_t *pns) {
    int num = MP_PARSE_NODE_STRUCT_NUM_NODES(pns);
    mp_obj_t left = ast_conv_node(pns->nodes[0]);
    mp_obj_t ops = ast_list_new();
    mp_obj_t comps = ast_list_new();
    for (int i = 1; i + 1 < num; i += 2) {
        ast_list_append(ops, ast_cmpop_from_node(pns->nodes[i]));
        ast_list_append(comps, ast_conv_node(pns->nodes[i + 1]));
    }
    return mp_ast_make(MP_AST_COMPARE, pns->source_line, 0, 3, (mp_obj_t[]){left, ops, comps});
}

static mp_obj_t ast_conv_binary_op(mp_parse_node_struct_t *pns) {
    uint16_t op_id = MP_AST_BITOR + (MP_PARSE_NODE_STRUCT_KIND(pns) - PN_expr);
    size_t n = MP_PARSE_NODE_STRUCT_NUM_NODES(pns);
    mp_obj_t left = ast_conv_node(pns->nodes[0]);
    for (size_t i = 1; i < n; i++) {
        mp_obj_t right = ast_conv_node(pns->nodes[i]);
        left = mp_ast_make(MP_AST_BINOP, pns->source_line, 0, 3,
            (mp_obj_t[]){left, mp_ast_singleton(op_id), right});
    }
    return left;
}

static mp_obj_t ast_conv_term(mp_parse_node_struct_t *pns) {
    size_t n = MP_PARSE_NODE_STRUCT_NUM_NODES(pns);
    mp_obj_t left = ast_conv_node(pns->nodes[0]);
    for (size_t i = 1; i + 1 < n; i += 2) {
        mp_obj_t op = ast_binop_token(MP_PARSE_NODE_LEAF_ARG(pns->nodes[i]));
        mp_obj_t right = ast_conv_node(pns->nodes[i + 1]);
        left = mp_ast_make(MP_AST_BINOP, pns->source_line, 0, 3, (mp_obj_t[]){left, op, right});
    }
    return left;
}

static mp_obj_t ast_conv_factor_2(mp_parse_node_struct_t *pns) {
    mp_token_kind_t tok = MP_PARSE_NODE_LEAF_ARG(pns->nodes[0]);
    uint16_t op = MP_AST_INVERT;
    if (tok == MP_TOKEN_OP_PLUS) op = MP_AST_UADD;
    else if (tok == MP_TOKEN_OP_MINUS) op = MP_AST_USUB;
    return mp_ast_make(MP_AST_UNARYOP, pns->source_line, 0, 2,
        (mp_obj_t[]){mp_ast_singleton(op), ast_conv_node(pns->nodes[1])});
}

static mp_obj_t ast_conv_power(mp_parse_node_struct_t *pns) {
    mp_obj_t left = ast_conv_node(pns->nodes[0]);
    if (MP_PARSE_NODE_STRUCT_NUM_NODES(pns) > 1) {
        mp_obj_t right = ast_conv_node(pns->nodes[1]);
        left = mp_ast_make(MP_AST_BINOP, pns->source_line, 0, 3,
            (mp_obj_t[]){left, mp_ast_singleton(MP_AST_POW), right});
    }
    return left;
}

static mp_obj_t ast_conv_test_if_expr(mp_parse_node_struct_t *pns) {
    if (MP_PARSE_NODE_IS_NULL(pns->nodes[1])) {
        return ast_conv_node(pns->nodes[0]);
    }
    mp_parse_node_struct_t *elsep = (mp_parse_node_struct_t *)pns->nodes[1];
    return mp_ast_make(MP_AST_IFEXP, pns->source_line, 0, 3,
        (mp_obj_t[]){ast_conv_node(elsep->nodes[0]), ast_conv_node(pns->nodes[0]), ast_conv_node(elsep->nodes[1])});
}

static mp_obj_t ast_conv_lambdef(mp_parse_node_struct_t *pns) {
    mp_obj_t args = ast_make_arguments(pns->nodes[0]);
    mp_obj_t body = ast_conv_node(pns->nodes[1]);
    return mp_ast_make(MP_AST_LAMBDA, pns->source_line, 0, 2, (mp_obj_t[]){args, body});
}

static mp_obj_t ast_conv_yield_expr(mp_parse_node_struct_t *pns) {
    if (MP_PARSE_NODE_IS_NULL(pns->nodes[0])) {
        return mp_ast_make(MP_AST_YIELD, pns->source_line, 0, 1, (mp_obj_t[]){mp_const_none});
    }
    if (MP_PARSE_NODE_IS_STRUCT_KIND(pns->nodes[0], PN_yield_arg_from)) {
        mp_parse_node_struct_t *yf = (mp_parse_node_struct_t *)pns->nodes[0];
        return mp_ast_make(MP_AST_YIELDFROM, pns->source_line, 0, 1, (mp_obj_t[]){ast_conv_node(yf->nodes[0])});
    }
    return mp_ast_make(MP_AST_YIELD, pns->source_line, 0, 1, (mp_obj_t[]){ast_conv_node(pns->nodes[0])});
}

static mp_obj_t ast_conv_atom_paren(mp_parse_node_struct_t *pns) {
    if (MP_PARSE_NODE_IS_NULL(pns->nodes[0])) {
        return mp_ast_make(MP_AST_TUPLE, pns->source_line, 0, 2,
            (mp_obj_t[]){ast_list_new(), mp_ast_singleton(MP_AST_LOAD)});
    }
    mp_parse_node_struct_t *inner = (mp_parse_node_struct_t *)pns->nodes[0];
    if (MP_PARSE_NODE_STRUCT_KIND(inner) == PN_testlist_comp) {
        if (MP_PARSE_NODE_TESTLIST_COMP_HAS_COMP_FOR(inner)) {
            return ast_conv_comp(inner, MP_AST_GENERATOREXP);
        }
        size_t n = MP_PARSE_NODE_STRUCT_NUM_NODES(inner);
        if (n == 1) {
            return ast_conv_node(inner->nodes[0]);
        }
        mp_obj_t *elts = m_new(mp_obj_t, n);
        for (size_t i = 0; i < n; i++) elts[i] = ast_conv_node(inner->nodes[i]);
        return mp_ast_make(MP_AST_TUPLE, pns->source_line, 0, 2,
            (mp_obj_t[]){mp_obj_new_list(n, elts), mp_ast_singleton(MP_AST_LOAD)});
    }
    return ast_conv_node(pns->nodes[0]);
}

static mp_obj_t ast_conv_atom_bracket(mp_parse_node_struct_t *pns) {
    if (MP_PARSE_NODE_IS_NULL(pns->nodes[0])) {
        return mp_ast_make(MP_AST_LIST, pns->source_line, 0, 2,
            (mp_obj_t[]){ast_list_new(), mp_ast_singleton(MP_AST_LOAD)});
    }
    mp_parse_node_t inner_pn = pns->nodes[0];
    if (MP_PARSE_NODE_IS_STRUCT_KIND(inner_pn, PN_testlist_comp)) {
        mp_parse_node_struct_t *inner = (mp_parse_node_struct_t *)inner_pn;
        if (MP_PARSE_NODE_TESTLIST_COMP_HAS_COMP_FOR(inner)) {
            return ast_conv_comp(inner, MP_AST_LISTCOMP);
        }
        return mp_ast_make(MP_AST_LIST, pns->source_line, 0, 2,
            (mp_obj_t[]){ast_conv_nodes_from_struct(inner), mp_ast_singleton(MP_AST_LOAD)});
    }
    mp_obj_t list = ast_list_new();
    ast_list_append(list, ast_conv_node(pns->nodes[0]));
    return mp_ast_make(MP_AST_LIST, pns->source_line, 0, 2,
        (mp_obj_t[]){list, mp_ast_singleton(MP_AST_LOAD)});
}

static mp_obj_t ast_conv_atom_brace(mp_parse_node_struct_t *pns) {
    mp_parse_node_t pn = pns->nodes[0];
    if (MP_PARSE_NODE_IS_NULL(pn)) {
        return mp_ast_make(MP_AST_DICT, pns->source_line, 0, 2, (mp_obj_t[]){ast_list_new(), ast_list_new()});
    }
    if (MP_PARSE_NODE_IS_STRUCT_KIND(pn, PN_dictorsetmaker_item)) {
        mp_parse_node_struct_t *item = (mp_parse_node_struct_t *)pn;
        return mp_ast_make(MP_AST_DICT, pns->source_line, 0, 2, (mp_obj_t[]){
            ast_list_from_nodes(1, (mp_obj_t[]){ast_conv_node(item->nodes[0])}),
            ast_list_from_nodes(1, (mp_obj_t[]){ast_conv_node(item->nodes[1])})});
    }
    if (!MP_PARSE_NODE_IS_STRUCT_KIND(pn, PN_dictorsetmaker)) {
        // single-element set
        mp_obj_t elts = ast_list_new();
        ast_list_append(elts, ast_conv_node(pn));
        return mp_ast_make(MP_AST_SET, pns->source_line, 0, 1, (mp_obj_t[]){elts});
    }
    mp_parse_node_struct_t *dm = (mp_parse_node_struct_t *)pn;
    mp_parse_node_t tail = dm->nodes[1];
    if (MP_PARSE_NODE_IS_STRUCT_KIND(tail, PN_comp_for)) {
        if (MP_PARSE_NODE_IS_STRUCT_KIND(dm->nodes[0], PN_dictorsetmaker_item)) {
            return ast_conv_comp(dm, MP_AST_DICTCOMP);
        } else {
            return ast_conv_comp(dm, MP_AST_SETCOMP);
        }
    }
    if (MP_PARSE_NODE_IS_STRUCT_KIND(tail, PN_dictorsetmaker_list)) {
        mp_obj_t keys = ast_list_new();
        mp_obj_t vals = ast_list_new();
        mp_obj_t elts = ast_list_new();
        bool is_dict = MP_PARSE_NODE_IS_STRUCT_KIND(dm->nodes[0], PN_dictorsetmaker_item);
        if (is_dict) {
            mp_parse_node_struct_t *it = (mp_parse_node_struct_t *)dm->nodes[0];
            ast_list_append(keys, ast_conv_node(it->nodes[0]));
            ast_list_append(vals, ast_conv_node(it->nodes[1]));
        } else {
            ast_list_append(elts, ast_conv_node(dm->nodes[0]));
        }
        mp_parse_node_t *nodes;
        size_t n = mp_parse_node_extract_list(&((mp_parse_node_struct_t *)tail)->nodes[0], PN_dictorsetmaker_list2, &nodes);
        for (size_t i = 0; i < n; i++) {
            if (is_dict) {
                mp_parse_node_struct_t *it = (mp_parse_node_struct_t *)nodes[i];
                ast_list_append(keys, ast_conv_node(it->nodes[0]));
                ast_list_append(vals, ast_conv_node(it->nodes[1]));
            } else {
                ast_list_append(elts, ast_conv_node(nodes[i]));
            }
        }
        if (is_dict) {
            return mp_ast_make(MP_AST_DICT, pns->source_line, 0, 2, (mp_obj_t[]){keys, vals});
        }
        return mp_ast_make(MP_AST_SET, pns->source_line, 0, 1, (mp_obj_t[]){elts});
    }
    // fallback: treat as single-element set
    mp_obj_t elts = ast_list_new();
    ast_list_append(elts, ast_conv_node(dm->nodes[0]));
    return mp_ast_make(MP_AST_SET, pns->source_line, 0, 1, (mp_obj_t[]){elts});
}

static mp_obj_t ast_conv_raise_stmt(mp_parse_node_struct_t *pns) {
    mp_obj_t exc = mp_const_none;
    mp_obj_t cause = mp_const_none;
    if (!MP_PARSE_NODE_IS_NULL(pns->nodes[0])) {
        mp_parse_node_struct_t *arg = (mp_parse_node_struct_t *)pns->nodes[0];
        exc = ast_conv_node(arg->nodes[0]);
        if (!MP_PARSE_NODE_IS_NULL(arg->nodes[1])) {
            cause = ast_conv_node(arg->nodes[1]);
        }
    }
    return mp_ast_make(MP_AST_RAISE, pns->source_line, 0, 2, (mp_obj_t[]){exc, cause});
}

static mp_obj_t ast_dotted_name_to_str(mp_parse_node_t pn) {
    if (MP_PARSE_NODE_IS_ID(pn)) {
        size_t len;
        const char *s = (const char *)qstr_data(MP_PARSE_NODE_LEAF_ARG(pn), &len);
        return mp_obj_new_str(s, len);
    }
    mp_parse_node_struct_t *pns = (mp_parse_node_struct_t *)pn;
    size_t n = MP_PARSE_NODE_STRUCT_NUM_NODES(pns);
    size_t total = 0;
    for (size_t i = 0; i < n; i++) {
        size_t len;
        qstr_data(MP_PARSE_NODE_LEAF_ARG(pns->nodes[i]), &len);
        total += len + (i > 0 ? 1 : 0);
    }
    char *buf = mp_local_alloc(total);
    char *dest = buf;
    for (size_t i = 0; i < n; i++) {
        if (i > 0) {
            *dest++ = '.';
        }
        size_t len;
        const byte *s = qstr_data(MP_PARSE_NODE_LEAF_ARG(pns->nodes[i]), &len);
        memcpy(dest, s, len);
        dest += len;
    }
    mp_obj_t result = mp_obj_new_str(buf, total);
    mp_local_free(buf);
    return result;
}

static mp_obj_t ast_as_name_to_str(mp_parse_node_t pn) {
    if (MP_PARSE_NODE_IS_NULL(pn)) {
        return mp_const_none;
    }
    if (MP_PARSE_NODE_IS_ID(pn)) {
        size_t len;
        const char *s = (const char *)qstr_data(MP_PARSE_NODE_LEAF_ARG(pn), &len);
        return mp_obj_new_str(s, len);
    }
    if (MP_PARSE_NODE_IS_STRUCT_KIND(pn, PN_as_name)) {
        mp_parse_node_struct_t *pns = (mp_parse_node_struct_t *)pn;
        size_t len;
        const char *s = (const char *)qstr_data(MP_PARSE_NODE_LEAF_ARG(pns->nodes[1]), &len);
        return mp_obj_new_str(s, len);
    }
    return mp_const_none;
}

static mp_obj_t ast_conv_import_name(mp_parse_node_struct_t *pns) {
    mp_obj_t names = ast_list_new();
    mp_parse_node_t *pnodes;
    size_t n = mp_parse_node_extract_list(&pns->nodes[0], PN_dotted_as_names, &pnodes);
    for (size_t i = 0; i < n; i++) {
        mp_obj_t name;
        mp_obj_t asname = mp_const_none;
        uint16_t lineno = ast_line_of(pnodes[i]);
        if (MP_PARSE_NODE_IS_STRUCT_KIND(pnodes[i], PN_dotted_as_name)) {
            mp_parse_node_struct_t *na = (mp_parse_node_struct_t *)pnodes[i];
            lineno = na->source_line;
            name = ast_dotted_name_to_str(na->nodes[0]);
            asname = ast_as_name_to_str(na->nodes[1]);
        } else {
            name = ast_dotted_name_to_str(pnodes[i]);
        }
        ast_list_append(names, mp_ast_make(MP_AST_alias, lineno, 0, 2, (mp_obj_t[]){name, asname}));
    }
    return mp_ast_make(MP_AST_IMPORT, pns->source_line, 0, 1, (mp_obj_t[]){names});
}

static mp_obj_t ast_conv_global_nonlocal_inner(mp_parse_node_struct_t *pns, bool is_global) {
    mp_parse_node_t *pnodes;
    size_t n = mp_parse_node_extract_list(&pns->nodes[0], PN_name_list, &pnodes);
    mp_obj_t names = ast_list_new();
    for (size_t i = 0; i < n; i++) {
        ast_list_append(names, MP_OBJ_NEW_QSTR(MP_PARSE_NODE_LEAF_ARG(pnodes[i])));
    }
    return mp_ast_make(is_global ? MP_AST_GLOBAL : MP_AST_NONLOCAL, pns->source_line, 0, 1, (mp_obj_t[]){names});
}

static mp_obj_t ast_conv_assert_stmt(mp_parse_node_struct_t *pns) {
    mp_obj_t msg = MP_PARSE_NODE_IS_NULL(pns->nodes[1]) ? mp_const_none : ast_conv_node(pns->nodes[1]);
    return mp_ast_make(MP_AST_ASSERT, pns->source_line, 0, 2, (mp_obj_t[]){ast_conv_node(pns->nodes[0]), msg});
}

static mp_obj_t ast_conv_return_stmt(mp_parse_node_struct_t *pns) {
    mp_obj_t val = MP_PARSE_NODE_IS_NULL(pns->nodes[0]) ? mp_const_none : ast_conv_node(pns->nodes[0]);
    return mp_ast_make(MP_AST_RETURN, pns->source_line, 0, 1, (mp_obj_t[]){val});
}

static mp_obj_t ast_conv_del_stmt(mp_parse_node_struct_t *pns) {
    mp_obj_t targets = ast_list_new();
    mp_parse_node_t *pnodes;
    size_t n = mp_parse_node_extract_list(&pns->nodes[0], PN_exprlist, &pnodes);
    for (size_t i = 0; i < n; i++) {
        ast_list_append(targets, ast_conv_target(pnodes[i], mp_ast_singleton(MP_AST_DEL)));
    }
    return mp_ast_make(MP_AST_DELETE, pns->source_line, 0, 1, (mp_obj_t[]){targets});
}

static mp_obj_t ast_conv_not_test_2(mp_parse_node_struct_t *pns) {
    return mp_ast_make(MP_AST_UNARYOP, pns->source_line, 0, 2,
        (mp_obj_t[]){mp_ast_singleton(MP_AST_NOT), ast_conv_node(pns->nodes[0])});
}

static mp_obj_t ast_conv_break_cont_stmt(mp_parse_node_struct_t *pns) {
    return mp_ast_make(MP_PARSE_NODE_STRUCT_KIND(pns) == PN_break_stmt ? MP_AST_BREAK : MP_AST_CONTINUE,
        pns->source_line, 0, 0, NULL);
}

static mp_obj_t ast_conv_namedexpr(mp_parse_node_struct_t *pns) {
    return mp_ast_make(MP_AST_NAMEDEXPR, pns->source_line, 0, 2,
        (mp_obj_t[]){ast_conv_target(pns->nodes[0], mp_ast_singleton(MP_AST_STORE)), ast_conv_node(pns->nodes[1])});
}

#if MICROPY_PY_ASYNC_AWAIT
static mp_obj_t ast_conv_atom_expr_await(mp_parse_node_struct_t *pns) {
    mp_obj_t val = ast_conv_atom_expr(pns, true);
    return mp_ast_make(MP_AST_AWAIT, pns->source_line, 0, 1, (mp_obj_t[]){val});
}
#endif

static mp_obj_t ast_conv_generic_tuple(mp_parse_node_struct_t *pns) {
    size_t n = MP_PARSE_NODE_STRUCT_NUM_NODES(pns);
    if (n == 1) {
        return ast_conv_node(pns->nodes[0]);
    }
    mp_obj_t *elts = m_new(mp_obj_t, n);
    for (size_t i = 0; i < n; i++) {
        elts[i] = ast_conv_node(pns->nodes[i]);
    }
    return mp_ast_make(MP_AST_TUPLE, pns->source_line, 0, 2,
        (mp_obj_t[]){mp_obj_new_list(n, elts), mp_ast_singleton(MP_AST_LOAD)});
}

static mp_obj_t ast_conv_funcdef(mp_parse_node_struct_t *pns) {
    return ast_conv_funcdef_inner(pns, false);
}

static mp_obj_t ast_conv_for_stmt(mp_parse_node_struct_t *pns) {
    return ast_conv_for_stmt_inner(pns, false);
}

static mp_obj_t ast_conv_with_stmt(mp_parse_node_struct_t *pns) {
    return ast_conv_with_stmt_inner(pns, false);
}

static mp_obj_t ast_conv_global_nonlocal_stmt(mp_parse_node_struct_t *pns) {
    bool is_global = MP_PARSE_NODE_STRUCT_KIND(pns) == PN_global_stmt;
    return ast_conv_global_nonlocal_inner(pns, is_global);
}

static mp_obj_t ast_conv_import_from(mp_parse_node_struct_t *pns) {
    mp_obj_t names = ast_list_new();
    mp_obj_t level = mp_obj_new_int(0);
    mp_obj_t module = mp_const_none;
    mp_parse_node_t pn_import_source = pns->nodes[0];

    if (!MP_PARSE_NODE_IS_NULL(pn_import_source)) {
        if (MP_PARSE_NODE_IS_STRUCT_KIND(pn_import_source, PN_import_from_2b)) {
            mp_parse_node_struct_t *pns2b = (mp_parse_node_struct_t *)pn_import_source;
            mp_parse_node_t *nodes;
            size_t nrel = mp_parse_node_extract_list(&pns2b->nodes[0], PN_one_or_more_period_or_ellipsis, &nodes);
            for (size_t i = 0; i < nrel; i++) {
                if (MP_PARSE_NODE_IS_TOKEN_KIND(nodes[i], MP_TOKEN_DEL_PERIOD)) {
                    level = mp_obj_new_int(mp_obj_get_int(level) + 1);
                } else {
                    level = mp_obj_new_int(mp_obj_get_int(level) + 3);
                }
            }
            pn_import_source = pns2b->nodes[1];
        }
        if (!MP_PARSE_NODE_IS_NULL(pn_import_source)) {
            module = ast_dotted_name_to_str(pn_import_source);
        }
    }

    if (!MP_PARSE_NODE_IS_NULL(pns->nodes[1])) {
        if (MP_PARSE_NODE_IS_TOKEN_KIND(pns->nodes[1], MP_TOKEN_OP_STAR)) {
            ast_list_append(names, mp_ast_make(MP_AST_alias, pns->source_line, 0, 2,
                (mp_obj_t[]){mp_obj_new_str_from_cstr("*"), mp_const_none}));
        } else {
            mp_parse_node_t *pnodes;
            size_t n = mp_parse_node_extract_list(&pns->nodes[1], PN_import_as_names, &pnodes);
            for (size_t i = 0; i < n; i++) {
                mp_obj_t nm;
                mp_obj_t asname = mp_const_none;
                uint16_t lineno = ast_line_of(pnodes[i]);
                if (MP_PARSE_NODE_IS_STRUCT_KIND(pnodes[i], PN_import_as_name)) {
                    mp_parse_node_struct_t *na = (mp_parse_node_struct_t *)pnodes[i];
                    lineno = na->source_line;
                    size_t len;
                    const char *s = (const char *)qstr_data(MP_PARSE_NODE_LEAF_ARG(na->nodes[0]), &len);
                    nm = mp_obj_new_str(s, len);
                    asname = ast_as_name_to_str(na->nodes[1]);
                } else {
                    size_t len;
                    const char *s = (const char *)qstr_data(MP_PARSE_NODE_LEAF_ARG(pnodes[i]), &len);
                    nm = mp_obj_new_str(s, len);
                }
                ast_list_append(names, mp_ast_make(MP_AST_alias, lineno, 0, 2, (mp_obj_t[]){nm, asname}));
            }
        }
    }
    return mp_ast_make(MP_AST_IMPORTFROM, pns->source_line, 0, 3, (mp_obj_t[]){module, names, level});
}

static mp_obj_t ast_conv_subscript(mp_parse_node_struct_t *pns) {
    mp_obj_t lower = mp_const_none;
    mp_obj_t upper = mp_const_none;
    mp_obj_t step = mp_const_none;
    if (MP_PARSE_NODE_STRUCT_KIND(pns) == PN_subscript_2) {
        lower = ast_conv_node(pns->nodes[0]);
        pns = (mp_parse_node_struct_t *)pns->nodes[1];
    }
    if (!MP_PARSE_NODE_IS_NULL(pns->nodes[0])) {
        upper = ast_conv_node(pns->nodes[0]);
    }
    if (MP_PARSE_NODE_STRUCT_NUM_NODES(pns) > 1 && !MP_PARSE_NODE_IS_NULL(pns->nodes[1])) {
        step = ast_conv_node(pns->nodes[1]);
    }
    return mp_ast_make(MP_AST_SLICE, pns->source_line, 0, 3, (mp_obj_t[]){lower, upper, step});
}

static mp_obj_t ast_conv_dictorsetmaker_item(mp_parse_node_struct_t *pns) {
    if (MP_PARSE_NODE_STRUCT_NUM_NODES(pns) == 3) {
        return mp_ast_make(MP_AST_TUPLE, pns->source_line, 0, 2, (mp_obj_t[]){
            ast_list_from_nodes(2, (mp_obj_t[]){ast_conv_node(pns->nodes[0]), ast_conv_node(pns->nodes[2])}),
            mp_ast_singleton(MP_AST_LOAD)});
    }
    return ast_conv_node(pns->nodes[0]);
}

static mp_obj_t ast_conv_star_expr(mp_parse_node_struct_t *pns) {
    return mp_ast_make(MP_AST_STARRED, pns->source_line, 0, 2,
        (mp_obj_t[]){ast_conv_node(pns->nodes[0]), mp_ast_singleton(MP_AST_LOAD)});
}

static mp_obj_t ast_conv_atom_expr_normal(mp_parse_node_struct_t *pns) {
    return ast_conv_atom_expr(pns, false);
}

static mp_obj_t ast_conv_trailer_paren(mp_parse_node_struct_t *pns) {
    return ast_conv_node(pns->nodes[0]);
}

static mp_obj_t ast_conv_trailer_bracket(mp_parse_node_struct_t *pns) {
    return ast_conv_subscript(pns);
}

static mp_obj_t ast_conv_trailer_period(mp_parse_node_struct_t *pns) {
    return MP_OBJ_NEW_QSTR(MP_PARSE_NODE_LEAF_ARG(pns->nodes[0]));
}

static mp_obj_t ast_conv_yield_stmt(mp_parse_node_struct_t *pns) {
    return mp_ast_make(MP_AST_EXPR, pns->source_line, 0, 1, (mp_obj_t[]){ast_conv_node(pns->nodes[0])});
}

typedef mp_obj_t (*ast_conv_fn_t)(mp_parse_node_struct_t *);
static mp_obj_t ast_conv_decorated(mp_parse_node_struct_t *pns) {
    mp_parse_node_struct_t *body = (mp_parse_node_struct_t *)pns->nodes[1];
    mp_obj_t node;
    if (MP_PARSE_NODE_STRUCT_KIND(body) == PN_funcdef) {
        node = ast_conv_funcdef_inner(body, false);
    #if MICROPY_PY_ASYNC_AWAIT
    } else if (MP_PARSE_NODE_STRUCT_KIND(body) == PN_async_funcdef) {
        node = ast_conv_funcdef_inner((mp_parse_node_struct_t *)body->nodes[0], true);
    #endif
    } else {
        node = ast_conv_classdef(body);
    }
    // TODO: attach decorator_list
    return node;
}

static mp_obj_t ast_conv_async_stmt(mp_parse_node_struct_t *pns) {
    mp_parse_node_struct_t *inner = (mp_parse_node_struct_t *)pns->nodes[0];
    if (MP_PARSE_NODE_STRUCT_KIND(inner) == PN_for_stmt) {
        return ast_conv_for_stmt_inner(inner, true);
    }
    return ast_conv_with_stmt_inner(inner, true);
}

static mp_obj_t ast_conv_const_object(mp_parse_node_struct_t *pns) {
    mp_obj_t val = mp_parse_node_extract_const_object(pns);
    return mp_ast_make(MP_AST_CONSTANT, pns->source_line, 0, 2, (mp_obj_t[]){val, mp_const_none});
}

static const ast_conv_fn_t ast_conv_function[PN_const_object + 1] = {
#define c(f) ast_conv_##f
#define DEF_RULE(rule, comp, kind, ...) comp,
#define DEF_RULE_NC(rule, kind, ...)
    #include "py/grammar.h"
#undef c
#undef DEF_RULE
#undef DEF_RULE_NC
    ast_conv_const_object,
};

static mp_obj_t ast_conv_struct(mp_parse_node_struct_t *pns) {
    size_t kind = MP_PARSE_NODE_STRUCT_KIND(pns);
    if (kind < PN_const_object) {
        ast_conv_fn_t fn = ast_conv_function[kind];
        if (fn != NULL) {
            return fn(pns);
        }
        return ast_conv_generic_all_nodes(pns);
    }
    if (kind == PN_const_object) {
        return ast_conv_const_object(pns);
    }
    return ast_conv_generic_all_nodes(pns);
}

static mp_obj_t ast_conv_node(mp_parse_node_t pn) {
    if (MP_PARSE_NODE_IS_NULL(pn)) {
        return mp_const_none;
    }
    if (MP_PARSE_NODE_IS_SMALL_INT(pn)) {
        return ast_const_from_leaf(pn);
    }
    if (MP_PARSE_NODE_IS_LEAF(pn)) {
        if (MP_PARSE_NODE_IS_ID(pn)) {
            return mp_ast_make(MP_AST_NAME, ast_line_of(pn), 0, 2,
                (mp_obj_t[]){MP_OBJ_NEW_QSTR(MP_PARSE_NODE_LEAF_ARG(pn)), mp_ast_singleton(MP_AST_LOAD)});
        }
        if (MP_PARSE_NODE_LEAF_KIND(pn) == MP_PARSE_NODE_STRING || MP_PARSE_NODE_IS_TOKEN(pn)) {
            return ast_const_from_leaf(pn);
        }
        return mp_const_none;
    }
    return ast_conv_struct((mp_parse_node_struct_t *)pn);
}

mp_obj_t mp_ast_parse_to_obj(mp_obj_t source, mp_obj_t filename, mp_obj_t mode) {
    mp_ast_init_types();
    size_t len;
    const char *src = mp_obj_str_get_data(source, &len);
    qstr source_name = mp_obj_str_get_qstr(filename);
    mp_lexer_t *lex = mp_lexer_new_from_str_len(source_name, src, len, 0);
    mp_parse_input_kind_t kind = MP_PARSE_FILE_INPUT;
    if (mp_obj_is_str(mode)) {
        size_t ml;
        const char *ms = mp_obj_str_get_data(mode, &ml);
        if (ml == 4 && memcmp(ms, "eval", 4) == 0) {
            kind = MP_PARSE_EVAL_INPUT;
        } else if (ml == 6 && memcmp(ms, "single", 6) == 0) {
            kind = MP_PARSE_SINGLE_INPUT;
        }
    }
    mp_parse_tree_t tree = mp_parse(lex, kind);
    mp_obj_t result;
    if (kind == MP_PARSE_EVAL_INPUT) {
        result = mp_ast_make(MP_AST_EXPRESSION, 1, 0, 1, (mp_obj_t[]){ast_conv_node(tree.root)});
    } else if (kind == MP_PARSE_SINGLE_INPUT) {
        mp_obj_t body = ast_list_new();
        mp_obj_t n = ast_conv_node(tree.root);
        if (n != mp_const_none) ast_list_append(body, n);
        result = mp_ast_make(MP_AST_INTERACTIVE, 1, 0, 1, (mp_obj_t[]){body});
    } else {
        mp_obj_t body = ast_conv_node(tree.root);
        if (!mp_obj_is_type(body, &mp_type_list)) {
            mp_obj_t lst = ast_list_new();
            if (body != mp_const_none) ast_list_append(lst, body);
            body = lst;
        }
        result = mp_ast_make(MP_AST_MODULE, 1, 0, 2, (mp_obj_t[]){body, ast_list_new()});
    }
    mp_parse_tree_clear(&tree);
    return result;
}

#endif // MICROPY_PY_AST
