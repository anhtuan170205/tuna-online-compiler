#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "interp.h"
#include "env.h"
#include "value.h"
#include "tuna.tab.h"
#include "runtime.h"

/* ============================================================
 *  Helper Data Structures
 * ============================================================ */

/*
 * Reference to a single element inside an array.
 * Used when assigning into an indexed lvalue (e.g., a[i] = rhs).
 *
 * owner: the array object that owns the element (kept alive via refcount)
 * cell : pointer directly to the element slot inside the array items[]
 */
typedef struct 
{
	array_obj_t *owner;
	value_t *cell;
} cell_ref_t;

/*
 * Result of executing a statement.
 *
 * has_return: indicates a RETURN statement was executed
 * r_value   : return value (only meaningful if has_return == 1)
 */
typedef struct
{
	int has_return;
	value_t r_value;
} exec_res_t;

/*
 * Linked-list node for storing function definitions declared in the program.
 * Functions are registered once before execution, then looked up by name.
 */
typedef struct func_def
{
	char* name;                 /* function name */
	ast_id_list_t* params;      /* parameter names */
	ast_stmt_t* body;           /* function body AST */
	struct func_def *next;      /* next node in registry */
} func_def_t;


/* ============================================================
 *  Global Function Registry
 * ============================================================ */

/* Head of linked list of all registered functions */
static func_def_t *g_funcs = NULL;


/* ============================================================
 *  Forward Declarations
 * ============================================================ */

/* Expression evaluation */
static value_t eval_expr(env_t *env, ast_expr_t *e);

/* Statement execution */
static exec_res_t exec_stmt(env_t *env, ast_stmt_t *s);
static exec_res_t exec_stmt_list(env_t *env, ast_stmt_t *head);

/* Assignment helpers */
static void assign_lvalue(env_t *env, ast_expr_t *lv, value_t rhs);
static cell_ref_t get_index_cell(env_t *env, ast_expr_t *lv);


/* ============================================================
 *  Utility Helpers
 * ============================================================ */

/*
 * Construct an exec result that indicates "no return encountered".
 * r_value is unused in this case (but initialized to a harmless value).
 */
static exec_res_t no_return(void)
{
	return (exec_res_t){0, v_int(0)};
}

/*
 * Truthiness rules:
 * - integers and booleans are truthy iff their integer value != 0
 * - other types are considered falsy by default in this interpreter
 */
static int is_truthy(value_t v) 
{
	return (v.kind == VAL_BOOL || v.kind == VAL_INT) && v.u.ival != 0;
}

/*
 * Numeric types in Tuna:
 * - int
 * - bool (treated like 0/1 for arithmetic/comparison)
 */
static int is_number(value_t v)
{
	return v.kind == VAL_INT || v.kind == VAL_BOOL;
}


/* ============================================================
 *  Function Registry
 * ============================================================ */

/*
 * Register a function definition AST node into the global registry.
 * Called before running the main block, for all top-level function declarations.
 */
static void func_register(ast_stmt_t *s)
{
	func_def_t *f = calloc(1, sizeof(func_def_t));
	f->name = s->fname;
	f->params = s->params;
	f->body = s->body;

	/* push-front into linked list */
	f->next = g_funcs;
	g_funcs = f;
}

/*
 * Look up a function definition by name in the global registry.
 * Returns NULL if not found.
 */
static func_def_t *func_lookup(const char *name)
{
	for (func_def_t *f = g_funcs; f != NULL; f = f->next)
	{
		if (strcmp(f->name, name) == 0)
		{
			return f;
		}
	}
	return NULL;
}


/* ============================================================
 *  Printing Values
 * ============================================================ */

/*
 * Print a value in a user-friendly way.
 * Arrays are printed recursively as [v1, v2, ...].
 */
static void print_value(value_t v)
{
	switch (v.kind)
	{
		case VAL_INT:
			printf("%d", v.u.ival);
			break;

		case VAL_BOOL:
			printf("%s", v.u.ival ? "true" : "false");
			break;

		case VAL_STRING:
			printf("%s", v.u.sval);
			break;
		
		case VAL_ARRAY:
			printf("[");
			for (int i = 0; i < v.u.arr->len; i++)
			{
				if (i > 0) printf(", ");
				print_value(v.u.arr->items[i]);
			}
			printf("]");
			break;
		
		default:
			printf("<unknown>");
			break;
	}
}


/* ============================================================
 *  Top-Level Runner
 * ============================================================ */

/*
 * Entry point of the interpreter.
 *
 * If the AST root is a full program node:
 *   - register all function declarations
 *   - execute the main block
 *
 * Otherwise:
 *   - execute the AST as a single statement (useful for tests)
 */
void tuna_run(ast_stmt_t *program)
{
	/* seed RNG used by rand(expr) */
	srand((unsigned int)time(NULL));

	/* global environment */
	env_t *env = env_new();

	if (program != NULL && program->kind == STMT_PROGRAM)
	{
		/* first pass: collect function declarations */
		for (ast_stmt_t *d = program->decls; d != NULL; d = d->next)
		{
			if (d != NULL && d->kind == STMT_FUNC)
			{
				func_register(d);
			}
		}

		/* second pass: execute main block */
		exec_res_t r = exec_stmt(env, program->main_blk);
		if (r.has_return)
		{
			/* avoid leaking the returned heap value (if any) */
			v_release(r.r_value);
		}
	}
	else
	{
		/* fallback: interpret a single statement */
		exec_res_t r = exec_stmt(env, program);
		if (r.has_return)
		{
			v_release(r.r_value);
		}
	}

	env_free(env);
}


/* ============================================================
 *  Statement Execution
 * ============================================================ */

/*
 * Execute a single statement node.
 * Propagates return upwards using exec_res_t.
 */
static exec_res_t exec_stmt(env_t *env, ast_stmt_t *s)
{
	if (!s) return no_return();

	switch (s->kind)
	{
		/* Execute a block: just execute each statement in sequence */
		case STMT_BLOCK:
			return exec_stmt_list(env, s->stmts);
		
		/* Assignment: evaluate RHS and assign into LHS target */
		case STMT_SET:
		{
			value_t val = eval_expr(env, s->expr);
			assign_lvalue(env, s->target, val);

			/* RHS evaluation may allocate memory -> release our local reference */
			v_release(val);
			return no_return();
		}

		/* Print statement */
		case STMT_PRINT:
		{
			value_t val = eval_expr(env, s->expr);
			print_value(val);
			printf("\n");
			v_release(val);
			return no_return();
		}

		/* If-then-else: choose branch based on truthiness */
		case STMT_IF:
		{
			value_t cond = eval_expr(env, s->expr);
			int t = is_truthy(cond);
			v_release(cond);

			if (t) return exec_stmt(env, s->then_blk);
			else   return exec_stmt(env, s->else_blk);
		}

		/* While loop: keep evaluating condition and executing body */
		case STMT_WHILE:
		{
			while (1)
			{
				value_t cond = eval_expr(env, s->expr);
				int t = is_truthy(cond);
				v_release(cond);

				if (!t) break;

				exec_res_t r = exec_stmt(env, s->then_blk);
				if (r.has_return) return r; /* propagate early return */
			}
			return no_return();
		}

		/* Return statement: evaluate expression (or default 0) */
		case STMT_RETURN:
		{
			value_t r_val = s->ret_expr ? eval_expr(env, s->ret_expr) : v_int(0);
			return (exec_res_t){1, r_val};
		}

		/* Function declaration is handled during registration pass */
		case STMT_FUNC:
			return no_return();

		/* Program node itself is not executed here */
		case STMT_PROGRAM:
			return no_return();

		/* Expression statement: evaluate and discard result */
		case STMT_EXPR:
		{
			value_t val = eval_expr(env, s->expr);
			v_release(val);
			return no_return();
		}
	}

	return no_return();
}

/*
 * Execute a linked list of statements until:
 * - list ends, or
 * - a return is encountered (propagate upward)
 */
static exec_res_t exec_stmt_list(env_t *env, ast_stmt_t *head)
{
	for (ast_stmt_t *cur = head; cur != NULL; cur = cur->next)
	{
		exec_res_t r = exec_stmt(env, cur);
		if (r.has_return)
		{
			return r;
		}
	}
	return no_return();
}


/* ============================================================
 *  Expression Evaluation
 * ============================================================ */

/*
 * Evaluate an expression node and return a value_t.
 * Caller is responsible for v_release() of the returned value.
 */
static value_t eval_expr(env_t *env, ast_expr_t *e)
{
	if (!e) return v_int(0);

	switch (e->kind)
	{
		/* literals */
		case EXPR_INT:    return v_int(e->ival);
		case EXPR_BOOL:   return v_bool(e->ival);
		case EXPR_STRING: return v_string(strdup(e->sval));

		/* variable lookup */
		case EXPR_VAR:
		{
			value_t val;
			if (!env_get(env, e->name, &val))
			{
				RAISE_RUNTIME_ERROR(RT_UNDEFINED_VARIABLE, .name = e->name);
			}
			return val;
		}

		/* unary operators: NOT, unary minus */
		case EXPR_UN:
		{
			value_t val = eval_expr(env, e->l);

			if (e->op == NOT)
			{
				int res = !is_truthy(val);
				v_release(val);
				return v_bool(res);
			}

			if (e->op == UMINUS)
			{
				if (!is_number(val))
				{
					v_release(val);
					RAISE_RUNTIME_ERROR(RT_TYPE_MISMATCH, .op = "-", .detail = "expects number");
				}

				int res = -val.u.ival;
				v_release(val);
				return v_int(res);
			}

			/* unreachable for valid AST, but keep safe default */
			v_release(val);
			return v_int(0);
		}

		/* binary operators (+, -, *, /, %, comparisons, AND/OR) */
		case EXPR_BIN:
		{
			/*
			 * Short-circuit boolean operators:
			 * - OR: if left is truthy, return true without evaluating right
			 * - AND: if left is falsy, return false without evaluating right
			 */
			if (e->op == OR)
			{
				value_t left = eval_expr(env, e->l);
				if (is_truthy(left))
				{
					v_release(left);
					return v_bool(1);
				}
				v_release(left);

				value_t right = eval_expr(env, e->r);
				int res = is_truthy(right);
				v_release(right);
				return v_bool(res);
			}

			if (e->op == AND)
			{
				value_t left = eval_expr(env, e->l);
				if (!is_truthy(left))
				{
					v_release(left);
					return v_bool(0);
				}
				v_release(left);

				value_t right = eval_expr(env, e->r);
				int res = is_truthy(right);
				v_release(right);
				return v_bool(res);
			}

			/* normal binary evaluation: evaluate both sides first */
			value_t left = eval_expr(env, e->l);
			value_t right = eval_expr(env, e->r);
			int op = e->op;

			/*
			 * Special handling for +:
			 * - disallow array concatenation
			 * - allow string concatenation (string + any-number)
			 * - otherwise numeric addition
			 */
			if (op == '+')
			{
				if (left.kind == VAL_ARRAY || right.kind == VAL_ARRAY)
				{
					v_release(left);
					v_release(right);
					RAISE_RUNTIME_ERROR(RT_TYPE_MISMATCH, .op = "+", .detail = "cannot add arrays");
				}

				/* string concatenation if either operand is string */
				if (left.kind == VAL_STRING || right.kind == VAL_STRING)
				{
					char bufL[64], bufR[64];

					const char *sl =
						(left.kind == VAL_STRING)
							? left.u.sval
							: (snprintf(bufL, sizeof bufL, "%d", left.u.ival), bufL);

					const char *sr =
						(right.kind == VAL_STRING)
							? right.u.sval
							: (snprintf(bufR, sizeof bufR, "%d", right.u.ival), bufR);

					char *res = malloc(strlen(sl) + strlen(sr) + 1);
					strcpy(res, sl);
					strcat(res, sr);

					v_release(left);
					v_release(right);
					return v_string(res);
				}

				/* numeric addition */
				if (!is_number(left) || !is_number(right))
				{
					v_release(left);
					v_release(right);
					RAISE_RUNTIME_ERROR(RT_TYPE_MISMATCH, .op = "+", .detail = "expects numbers or strings");
				}

				int res = left.u.ival + right.u.ival;
				v_release(left);
				v_release(right);
				return v_int(res);
			}

			/* string equality/inequality supported */
			if (op == EQ || op == NEQ)
			{
				if (left.kind == VAL_STRING && right.kind == VAL_STRING)
				{
					const char *ls = left.u.sval ? left.u.sval : "";
					const char *rs = right.u.sval ? right.u.sval : "";
					int same = (strcmp(ls, rs) == 0);

					v_release(left);
					v_release(right);
					return v_bool(op == EQ ? same : !same);
				}
			}

			/* all remaining arithmetic/comparisons require numbers */
			if (!is_number(left) || !is_number(right))
			{
				v_release(left);
				v_release(right);

				if (op == EQ || op == NEQ || op == LT || op == LE || op == GT || op == GE)
				{
					RAISE_RUNTIME_ERROR(RT_TYPE_MISMATCH, .op = "comparison");
				}
				else
				{
					RAISE_RUNTIME_ERROR(RT_TYPE_MISMATCH, .op = "arithmetic operation");
				}
			}

			int li = left.u.ival;
			int ri = right.u.ival;

			/* numeric ops + comparisons */
			switch (op)
			{
				case '+': v_release(left); v_release(right); return v_int(li + ri);
				case '-': v_release(left); v_release(right); return v_int(li - ri);
				case '*': v_release(left); v_release(right); return v_int(li * ri);

				case '/':
					if (ri == 0)
					{
						v_release(left);
						v_release(right);
						RAISE_RUNTIME_ERROR(RT_DIVISION_BY_ZERO);
					}
					v_release(left);
					v_release(right);
					return v_int(li / ri);

				case '%':
					if (ri == 0)
					{
						v_release(left);
						v_release(right);
						RAISE_RUNTIME_ERROR(RT_MODULO_BY_ZERO);
					}
					v_release(left);
					v_release(right);
					return v_int(li % ri);

				case EQ:  v_release(left); v_release(right); return v_bool(li == ri);
				case NEQ: v_release(left); v_release(right); return v_bool(li != ri);
				case LT:  v_release(left); v_release(right); return v_bool(li <  ri);
				case LE:  v_release(left); v_release(right); return v_bool(li <= ri);
				case GT:  v_release(left); v_release(right); return v_bool(li >  ri);
				case GE:  v_release(left); v_release(right); return v_bool(li >= ri);
			}

			/* safety fallback */
			v_release(left);
			v_release(right);
			return v_int(0);
		}

		/* array literal: evaluate each element into a new array object */
		case EXPR_ARRAY:
		{
			int n = 0;
			for (ast_expr_list_t *p = e->elems; p != NULL; p = p->next) { n++; }

			value_t arr = v_array(n);

			int i = 0;
			for (ast_expr_list_t *p = e->elems; p != NULL; p = p->next)
			{
				arr.u.arr->items[i++] = eval_expr(env, p->expr);
			}
			return arr;
		}

		/* indexing: base[index] */
		case EXPR_INDEX:
		{
			value_t base = eval_expr(env, e->base);
			value_t index = eval_expr(env, e->index);

			if (base.kind != VAL_ARRAY)
			{
				v_release(base);
				v_release(index);
				RAISE_RUNTIME_ERROR(RT_INDEX_NON_ARRAY);
			}

			if (!is_number(index))
			{
				v_release(base);
				v_release(index);
				RAISE_RUNTIME_ERROR(RT_INDEX_NOT_INT);
			}

			int idx = index.u.ival;
			v_release(index);

			if (idx < 0 || idx >= base.u.arr->len)
			{
				int n = base.u.arr->len;
				v_release(base);
				RAISE_RUNTIME_ERROR(RT_INDEX_OUT_OF_BOUNDS, .index = idx, .len = n);
			}

			/* clone element so caller owns its reference */
			value_t item = v_clone(base.u.arr->items[idx]);
			v_release(base);
			return item;
		}

		/* len(x): arrays => length, strings => strlen */
		case EXPR_LEN:
		{
			value_t val = eval_expr(env, e->l);

			if (val.kind == VAL_ARRAY)
			{
				int n = val.u.arr->len;
				v_release(val);
				return v_int(n);
			}

			if (val.kind == VAL_STRING)
			{
				int n = val.u.sval ? (int)strlen(val.u.sval) : 0;
				v_release(val);
				return v_int(n);
			}

			v_release(val);
			RAISE_RUNTIME_ERROR(RT_LEN_BAD_ARG);
		}

		/* readint(): read a line and parse integer */
		case EXPR_READINT:
		{
			char buf[256];
			if (!fgets(buf, sizeof buf, stdin))
			{
				return v_int(0);
			}
			char *end = NULL;
			long val = strtol(buf, &end, 10);
			return v_int((int)val);
		}

		/* readstr(): read a line, strip newline, return string */
		case EXPR_READSTR:
		{
			char buf[4096];
			if (!fgets(buf, sizeof buf, stdin))
			{
				return v_string(strdup(""));
			}

			size_t n = strlen(buf);
			while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r'))
			{
				buf[n-1] = '\0';
				n--;
			}
			return v_string(strdup(buf));
		}

		/* rand(m): random integer in [0, m-1] */
		case EXPR_RAND:
		{
			value_t val = eval_expr(env, e->l);
			if (!is_number(val))
			{
				v_release(val);
				RAISE_RUNTIME_ERROR(RT_TYPE_MISMATCH, .op = "rand", .detail = "expects number");
			}

			int m = val.u.ival;
			v_release(val);

			if (m <= 0)
			{
				RAISE_RUNTIME_ERROR(RT_TYPE_MISMATCH, .op = "rand", .detail = "argument must be positive");
			}

			int r = rand() % m;
			return v_int(r);
		}
		
		/* function call: create child env, bind params, execute body */
		case EXPR_CALL:
		{
			func_def_t *f = func_lookup(e->callee);
			if (f == NULL)
			{
				RAISE_RUNTIME_ERROR(RT_UNDEFINED_FUNCTION, .name = e->callee);
			}

			/* new scope chained to caller environment */
			env_t *local = env_new_child(env);

			/* bind arguments to parameters */
			ast_id_list_t *p = f->params;
			ast_expr_list_t *a = e->args;

			while (p != NULL && a != NULL)
			{
				value_t arg_val = eval_expr(env, a->expr);
				env_set(local, p->name, arg_val);

				/* env_set clones/stores; release our local */
				v_release(arg_val);

				p = p->next;
				a = a->next;
			}

			/* mismatch if still leftover params or args */
			if (p != NULL || a != NULL)
			{
				env_free(local);
				RAISE_RUNTIME_ERROR(RT_ARG_COUNT_MISMATCH, .name = e->callee);
			}

			/* execute body and return value if any */
			exec_res_t r = exec_stmt(local, f->body);
			value_t ret_val = r.has_return ? r.r_value : v_int(0);

			env_free(local);
			return ret_val;
		}

		/* push(array, value): append a clone of value to array */
		case EXPR_PUSH:
		{
			value_t arr = eval_expr(env, e->l);
			value_t val = eval_expr(env, e->r);

			if (arr.kind != VAL_ARRAY)
			{
				v_release(arr);
				v_release(val);
				RAISE_RUNTIME_ERROR(RT_TYPE_MISMATCH, .op = "push", .detail = "expects array");
			}

			array_obj_t *a = arr.u.arr;

			a->items = realloc(a->items, sizeof(value_t) * (a->len + 1));
			a->items[a->len] = v_clone(val);
			a->len++;

			v_release(val);
			v_release(arr);

			return v_int(0);
		}

		/* remove(array, index): delete element at index and shift left */
		case EXPR_REMOVE:
		{
			value_t arr = eval_expr(env, e->l);
			value_t index = eval_expr(env, e->r);

			if (arr.kind != VAL_ARRAY || index.kind != VAL_INT)
			{
				v_release(arr);
				v_release(index);
				RAISE_RUNTIME_ERROR(RT_TYPE_MISMATCH, .op = "remove",
				                   .detail = "expects array and integer index");
			}

			int idx = index.u.ival;
			array_obj_t *a = arr.u.arr;

			if (idx < 0 || idx >= a->len)
			{
				v_release(arr);
				v_release(index);
				RAISE_RUNTIME_ERROR(RT_INDEX_OUT_OF_BOUNDS, .index = idx, .len = a->len);
			}

			/* release removed element */
			v_release(a->items[idx]);

			/* shift items left */
			for (int i = idx; i < a->len - 1; i++)
			{
				a->items[i] = a->items[i + 1];
			}

			a->len--;
			a->items = realloc(a->items, sizeof(value_t) * a->len);

			v_release(index);
			v_release(arr);

			return v_int(0);
		}

		/* chars(string): convert string into array of 1-char strings */
		case EXPR_CHARS:
		{
			value_t v = eval_expr(env, e->l);

			if (v.kind != VAL_STRING)
			{
				v_release(v);
				RAISE_RUNTIME_ERROR(RT_TYPE_MISMATCH, .op = "chars", .detail = "expects string");
			}

			const char *s = v.u.sval ? v.u.sval : "";
			int n = (int)strlen(s);

			value_t arr = v_array(n);

			for (int i = 0; i < n; i++)
			{
				char *one = (char *)malloc(2);
				one[0] = s[i];
				one[1] = '\0';
				arr.u.arr->items[i] = v_string(one);
			}

			v_release(v);
			return arr;
		}
	}

	/* default fallback */
	return v_int(0);
}


/* ============================================================
 *  Assignment Helpers
 * ============================================================ */

/*
 * Resolve an indexed lvalue (base[index]) into a writable cell pointer.
 *
 * Important: this returns a pointer to a cell inside an array object.
 * To prevent the array from being freed while we still hold the pointer,
 * we increment owner->refcount and later decrement it in assign_lvalue().
 */
static cell_ref_t get_index_cell(env_t *env, ast_expr_t *lv)
{
	if (!lv || lv->kind != EXPR_INDEX)
	{
		RAISE_RUNTIME_ERROR(RT_INVALID_ASSIGN_TARGET);
	}

	value_t base;

	/*
	 * Optimization / safety:
	 * - if base is a variable, fetch it directly from env
	 * - otherwise evaluate base expression normally
	 */
	if (lv->base->kind == EXPR_VAR)
	{
		if (!env_get(env, lv->base->name, &base))
		{
			RAISE_RUNTIME_ERROR(RT_UNDEFINED_VARIABLE, .name = lv->base->name);
		}
	}
	else
	{
		base = eval_expr(env, lv->base);
	}

	if (base.kind != VAL_ARRAY)
	{
		v_release(base);
		RAISE_RUNTIME_ERROR(RT_INDEX_NON_ARRAY);
	}

	/* evaluate index and validate it is an integer-like value */
	value_t index = eval_expr(env, lv->index);
	if (!is_number(index))
	{
		v_release(base);
		v_release(index);
		RAISE_RUNTIME_ERROR(RT_INDEX_NOT_INT);
	}

	int idx = index.u.ival;
	v_release(index);

	/* bounds check */
	if (idx < 0 || idx >= base.u.arr->len)
	{
		int n = base.u.arr->len;
		v_release(base);
		RAISE_RUNTIME_ERROR(RT_INDEX_OUT_OF_BOUNDS, .index = idx, .len = n);
	}

	/* build reference to the underlying cell */
	cell_ref_t ref;
	ref.owner = base.u.arr;

	/* keep array alive while we keep a pointer into its items[] */
	ref.owner->refcount++;

	ref.cell = &base.u.arr->items[idx];

	/* release our local reference to base; refcount above keeps it alive */
	v_release(base);
	return ref;
}

/*
 * Assign into an lvalue target:
 * - variable assignment: env_set(name, rhs)
 * - indexed assignment: resolve cell pointer then replace element
 */
static void assign_lvalue(env_t *env, ast_expr_t *lv, value_t rhs)
{
	/* simple variable assignment */
	if (lv->kind == EXPR_VAR)
	{
		env_set(env, lv->name, rhs);
		return;
	}

	/* indexed assignment: a[i] = rhs */
	cell_ref_t ref = get_index_cell(env, lv);

	/* release old value in the cell, then store clone of rhs */
	v_release(*ref.cell);
	*ref.cell = v_clone(rhs);

	/* drop the temporary keep-alive reference */
	ref.owner->refcount--;
}
