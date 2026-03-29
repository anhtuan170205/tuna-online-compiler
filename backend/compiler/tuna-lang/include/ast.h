#ifndef AST_H
#define AST_H

/*
 * AST (Abstract Syntax Tree) definitions for the Tuna language.
 *
 * The parser (Bison) builds these nodes using the constructor functions
 * declared at the bottom of this header (mk_int, mk_bin, mk_if, ...).
 *
 * The interpreter walks these AST nodes to evaluate expressions and
 * execute statements.
 */

/* ============================================================
 *  Identifier Lists (used for function parameters)
 * ============================================================ */

/*
 * Linked list of identifier names.
 * Used mainly to store function parameter names in order.
 */
typedef struct ast_id_list
{
	char *name;               /* identifier name */
	struct ast_id_list *next; /* next parameter */
} ast_id_list_t;

/*
 * Append an identifier name to an identifier list.
 * Returns the head of the list (same head if list was non-empty).
 */
ast_id_list_t *id_append(ast_id_list_t *list, char *name);


/* ============================================================
 *  Expression Nodes
 * ============================================================ */

/*
 * Expression kinds supported by Tuna.
 * Each kind uses different fields inside ast_expr_t.
 */
typedef enum
{
	EXPR_INT,      /* integer literal */
	EXPR_BOOL,     /* boolean literal (stored in ival as 0/1) */
	EXPR_STRING,   /* string literal */
	EXPR_VAR,      /* variable reference */

	EXPR_BIN,      /* binary operator expression (l op r) */
	EXPR_UN,       /* unary operator expression (op l) */

	EXPR_ARRAY,    /* array literal [e1, e2, ...] */
	EXPR_INDEX,    /* array indexing base[index] */

	EXPR_LEN,      /* len(expr) built-in */
	EXPR_READINT,  /* readint() built-in */
	EXPR_READSTR,  /* readstr() built-in */
	EXPR_RAND,     /* rand(expr) built-in */

	EXPR_CALL,     /* function call f(args...) */

	EXPR_PUSH,     /* push(array, value) built-in */
	EXPR_REMOVE,   /* remove(array, index) built-in */
	EXPR_CHARS     /* chars(string) built-in -> array of 1-char strings */
} expr_kind_t;

/*
 * Expression AST node.
 *
 * NOTE: This struct is "union-like" but implemented with plain fields:
 * depending on 'kind', only some fields are meaningful.
 */
typedef struct ast_expr
{
	expr_kind_t kind;

	/* ---- literal / variable fields ---- */
	int ival;        /* for EXPR_INT and EXPR_BOOL */
	char *sval;      /* for EXPR_STRING */
	char *name;      /* for EXPR_VAR */

	/* ---- operator fields ---- */
	int op;                /* operator token code (from parser) */
	struct ast_expr *l;    /* left operand (or single operand for unary) */
	struct ast_expr *r;    /* right operand (binary only) */

	/* ---- indexing fields ---- */
	struct ast_expr *base;   /* array expression being indexed */
	struct ast_expr *index;  /* index expression */

	/* ---- array literal fields ---- */
	struct ast_expr_list *elems; /* elements for EXPR_ARRAY */

	/* ---- function call fields ---- */
	char *callee;                /* function name for EXPR_CALL */
	struct ast_expr_list *args;  /* argument expressions */
} ast_expr_t;

/*
 * Linked list of expressions.
 * Used for:
 * - array literal elements
 * - function call argument lists
 */
typedef struct ast_expr_list
{
	ast_expr_t *expr;               /* one expression node */
	struct ast_expr_list *next;     /* next element */
} ast_expr_list_t;


/* ============================================================
 *  Statement Nodes
 * ============================================================ */

/*
 * Statement kinds supported by Tuna.
 */
typedef enum
{
	STMT_SET,      /* assignment: target = expr */
	STMT_PRINT,    /* print expr */
	STMT_IF,       /* if (expr) then_blk else else_blk */
	STMT_WHILE,    /* while (expr) then_blk */
	STMT_BLOCK,    /* { stmt; stmt; ... } */

	STMT_FUNC,     /* function declaration */
	STMT_RETURN,   /* return expr */
	STMT_PROGRAM,  /* program root node (decls + main) */
	STMT_EXPR      /* expression used as statement (e.g., function call) */
} stmt_kind_t;

/*
 * Statement AST node.
 *
 * Like expressions, this is union-like: fields used depend on 'kind'.
 * Statements are also arranged as linked lists using 'next'.
 */
typedef struct ast_stmt
{
	stmt_kind_t kind;

	/* next statement in the same block / list */
	struct ast_stmt *next;

	/* ---- assignment / print / conditions ---- */
	ast_expr_t *target; /* for STMT_SET: lvalue (var or index expr) */
	ast_expr_t *expr;   /* for STMT_SET rhs, STMT_PRINT expr,
	                       STMT_IF condition, STMT_WHILE condition */

	/* ---- control flow blocks ---- */
	struct ast_stmt *then_blk; /* STMT_IF then-block, STMT_WHILE body */
	struct ast_stmt *else_blk; /* STMT_IF else-block */

	/* ---- block statement ---- */
	struct ast_stmt *stmts;    /* head of statement list for STMT_BLOCK */

	/* ---- function declaration ---- */
	char *fname;           /* function name */
	ast_id_list_t *params; /* parameter list */
	struct ast_stmt *body; /* function body (usually STMT_BLOCK) */

	/* ---- return ---- */
	ast_expr_t *ret_expr;  /* return expression (can be NULL) */

	/* ---- program root ---- */
	struct ast_stmt *decls;     /* list of STMT_FUNC declarations */
	struct ast_stmt *main_blk;  /* main block statement */
} ast_stmt_t;


/* ============================================================
 *  Expression Constructors (used by parser actions)
 * ============================================================ */

ast_expr_t *mk_int(int v);                         /* integer literal */
ast_expr_t *mk_bool(int v);                        /* boolean literal */
ast_expr_t *mk_string(char *s);                    /* string literal */
ast_expr_t *mk_var(char *name);                    /* variable reference */

ast_expr_t *mk_bin(int op, ast_expr_t *l, ast_expr_t *r); /* binary op */
ast_expr_t *mk_un(int op, ast_expr_t *e);                 /* unary op */

ast_expr_t *mk_array(ast_expr_list_t *el);               /* array literal */
ast_expr_t *mk_index(ast_expr_t *base, ast_expr_t *index);/* base[index] */

ast_expr_t *mk_len(ast_expr_t *e);                       /* len(e) */
ast_expr_t *mk_readint(void);                            /* readint() */
ast_expr_t *mk_readstr(void);                            /* readstr() */
ast_expr_t *mk_rand(ast_expr_t *e1);                     /* rand(e1) */

ast_expr_t *mk_call(char *fname, ast_expr_list_t *args);  /* f(args...) */

ast_expr_t *mk_push(ast_expr_t *arr, ast_expr_t *val);    /* push(arr,val) */
ast_expr_t *mk_remove(ast_expr_t *arr, ast_expr_t *index);/* remove(arr,i) */
ast_expr_t *mk_chars(ast_expr_t *e);                      /* chars(str) */


/* ============================================================
 *  Statement Constructors (used by parser actions)
 * ============================================================ */

ast_stmt_t *mk_set(ast_expr_t *target, ast_expr_t *e); /* assignment */
ast_stmt_t *mk_print(ast_expr_t *e);                   /* print */
ast_stmt_t *mk_if(ast_expr_t *cond, ast_stmt_t *then_blk, ast_stmt_t *else_blk);
ast_stmt_t *mk_while(ast_expr_t *cond, ast_stmt_t *body_blk);
ast_stmt_t *mk_block(ast_stmt_t *stmts);               /* { ... } */
ast_stmt_t *mk_expr_stmt(ast_expr_t *e);               /* expr; */

ast_stmt_t *mk_func(char *fname, ast_id_list_t *params, ast_stmt_t *body);
ast_stmt_t *mk_return(ast_expr_t *e);
ast_stmt_t *mk_program(ast_stmt_t *decls, ast_stmt_t *main_blk);


/* ============================================================
 *  List Utilities (used to build linked lists in parser)
 * ============================================================ */

/* Append a statement to a statement list; returns the list head */
ast_stmt_t *stmt_append(ast_stmt_t *list, ast_stmt_t *s);

/* Append an expression to an expression list; returns the list head */
ast_expr_list_t *expr_append(ast_expr_list_t *list, ast_expr_t *e);

#endif // AST_H
