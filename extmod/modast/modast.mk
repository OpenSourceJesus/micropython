ifeq ($(MICROPY_PY_AST),1)
CFLAGS_EXTMOD += -DMICROPY_PY_AST=1
SRC_EXTMOD_C += \
	extmod/modast/ast_types.c \
	extmod/modast/parse_to_ast.c \
	extmod/modast/modast.c
endif
