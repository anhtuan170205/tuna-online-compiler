#include "ast.h"
#include <stdlib.h>
#include <string.h>

/*
 * Append a new identifier name to a linked list of identifiers.
 * This is mainly used for function parameter lists.
 */
ast_id_list_t *id_append(ast_id_list_t *list, char *name)
{
	ast_id_list_t *node = (ast_id_list_t *)calloc(1, sizeof(ast_id_list_t));
	node->name = name;
	node->next = NULL;

	if (!list) return node;

	ast_id_list_t *cur = list;
	while (cur->next)
	{
		cur = cur->next;
	}
	cur->next = node;
	return list;
}

/*
 * Allocate and initialize a new expression node with a given kind.
 * All expression constructors use this helper.
 */
static ast_expr_t *new_expr(expr_kind_t k)
{
	ast_expr_t *e = (ast_expr_t *)calloc(1, sizeof(ast_expr_t));
	e->kind = k;
	return e;
}

/*
 * Allocate a new expression list node.
 * Expression lists are used for array literals and function arguments.
 */
static ast_expr_list_t *new_expr_list(ast_expr_t *e)
{
	ast_expr_list_t *el = calloc(1, sizeof(ast_expr_list_t));
	el->expr = e;
	return el;
}

/*
 * Allocate and initialize a new statement node with a given kind.
 * All statement constructors use this helper.
 */
static ast_stmt_t *new_stmt(stmt_kind_t k)
{
	ast_stmt_t *s = (ast_stmt_t *)calloc(1, sizeof(ast_stmt_t));
	s->kind = k;
	return s;
}

/* ============================= */
/* Expression constructors      */
/* ============================= */

/* Integer literal */
ast_expr_t *mk_int(int v)
{
	ast_expr_t *e = new_expr(EXPR_INT);
	e->ival = v;
	return e;
}

/* Boolean literal */
ast_expr_t *mk_bool(int v)
{
	ast_expr_t *e = new_expr(EXPR_BOOL);
	e->ival = v;
	return e;
}

/* String literal */
ast_expr_t *mk_string(char *s)
{
	ast_expr_t *e = new_expr(EXPR_STRING);
	e->sval = s;
	return e;
}

/* Variable reference */
ast_expr_t *mk_var(char *name)
{
	ast_expr_t *e = new_expr(EXPR_VAR);
	e->name = name;
	return e;
}

/* Binary expression (e.g., +, -, *, ==, AND, OR) */
ast_expr_t *mk_bin(int op, ast_expr_t *l, ast_expr_t *r)
{
	ast_expr_t *e = new_expr(EXPR_BIN);
	e->op = op;
	e->l = l;
	e->r = r;
	return e;
}

/* Unary expression (e.g., NOT, unary minus) */
ast_expr_t *mk_un(int op, ast_expr_t *e1)
{
	ast_expr_t *e = new_expr(EXPR_UN);
	e->op = op;
	e->l = e1;
	e->r = NULL;
	return e;
}

/* Array literal */
ast_expr_t *mk_array(ast_expr_list_t *el)
{
	ast_expr_t *e = new_expr(EXPR_ARRAY);
	e->elems = el;
	return e;
}

/* Array indexing expression (e.g., a[i]) */
ast_expr_t *mk_index(ast_expr_t *base, ast_expr_t *index)
{
	ast_expr_t *e = new_expr(EXPR_INDEX);
	e->base = base;
	e->index = index;
	return e;
}

/* len(expr) built-in */
ast_expr_t *mk_len(ast_expr_t *e1)
{
	ast_expr_t *e = new_expr(EXPR_LEN);
	e->l = e1;
	return e;
}

/* readint() built-in */
ast_expr_t *mk_readint(void)
{
	return new_expr(EXPR_READINT);
}

/* readstr() built-in */
ast_expr_t *mk_readstr(void)
{
	return new_expr(EXPR_READSTR);
}

/* rand(expr) built-in */
ast_expr_t *mk_rand(ast_expr_t *e1)
{
	ast_expr_t *e = new_expr(EXPR_RAND);
	e->l = e1;
	return e;
}

/* Function call expression */
ast_expr_t *mk_call(char *name, ast_expr_list_t *args)
{
	ast_expr_t *e = new_expr(EXPR_CALL);
	e->callee = name;
	e->args = args;
	return e;
}

/* push(array, value) built-in */
ast_expr_t *mk_push(ast_expr_t *arr, ast_expr_t *val)
{
	ast_expr_t *e = new_expr(EXPR_PUSH);
	e->l = arr;
	e->r = val;
	return e;
}

/* remove(array, index) built-in */
ast_expr_t *mk_remove(ast_expr_t *arr, ast_expr_t *index)
{
	ast_expr_t *e = new_expr(EXPR_REMOVE);
	e->l = arr;
	e->r = index;
	return e;
}

/* chars(string) built-in */
ast_expr_t *mk_chars(ast_expr_t *e1)
{
	ast_expr_t *e = new_expr(EXPR_CHARS);
	e->l = e1;
	return e;
}

/* ============================= */
/* Statement constructors       */
/* ============================= */

/* Assignment statement */
ast_stmt_t *mk_set(ast_expr_t *target, ast_expr_t *e)
{
	ast_stmt_t *s = new_stmt(STMT_SET);
	s->target = target;
	s->expr = e;
	return s;
}

/* Print statement */
ast_stmt_t *mk_print(ast_expr_t *e)
{
	ast_stmt_t *s = new_stmt(STMT_PRINT);
	s->expr = e;
	return s;
}

/* If-then-else statement */
ast_stmt_t *mk_if(ast_expr_t *cond, ast_stmt_t *then_blk, ast_stmt_t *else_blk)
{
	ast_stmt_t *s = new_stmt(STMT_IF);
	s->expr = cond;
	s->then_blk = then_blk;
	s->else_blk = else_blk;
	return s;
}

/* While loop */
ast_stmt_t *mk_while(ast_expr_t *cond, ast_stmt_t *body_blk)
{
	ast_stmt_t *s = new_stmt(STMT_WHILE);
	s->expr = cond;
	s->then_blk = body_blk;
	return s;
}

/* Block statement (list of statements) */
ast_stmt_t *mk_block(ast_stmt_t *stmts)
{
	ast_stmt_t *s = new_stmt(STMT_BLOCK);
	s->stmts = stmts;
	return s;
}

/* Function declaration */
ast_stmt_t *mk_func(char *fname, ast_id_list_t *params, ast_stmt_t *body)
{
	ast_stmt_t *s = new_stmt(STMT_FUNC);
	s->fname = fname;
	s->params = params;
	s->body = body;
	return s;
}

/* Return statement */
ast_stmt_t *mk_return(ast_expr_t *e)
{
	ast_stmt_t *s = new_stmt(STMT_RETURN);
	s->ret_expr = e;
	return s;
}

/* Program root node */
ast_stmt_t *mk_program(ast_stmt_t *decls, ast_stmt_t *main_blk)
{
	ast_stmt_t *s = new_stmt(STMT_PROGRAM);
	s->decls = decls;
	s->main_blk = main_blk;
	return s;
}

/* Expression statement (e.g., function call used as a statement) */
ast_stmt_t *mk_expr_stmt(ast_expr_t *e)
{
	ast_stmt_t *s = new_stmt(STMT_EXPR);
	s->expr = e;
	return s;
}

/* ============================= */
/* Statement & expression lists */
/* ============================= */

/*
 * Append a statement to a statement list.
 * Used to build blocks and programs incrementally in the parser.
 */
ast_stmt_t *stmt_append(ast_stmt_t *list, ast_stmt_t *s)
{
	if (!s) return list;
	if (!list) return s;

	ast_stmt_t *cur = list;
	while (cur->next) 
	{
		cur = cur->next;
	}
	cur->next = s;
	return list;
}

/*
 * Append an expression to an expression list.
 * Used for array literals and function arguments.
 */
ast_expr_list_t *expr_append(ast_expr_list_t *list, ast_expr_t *e)
{
	if (!e) return list;
	if (!list) return new_expr_list(e);

	ast_expr_list_t *cur = list;
	while (cur->next)
	{
		cur = cur->next;
	}
	cur->next = new_expr_list(e);
	return list;
}
