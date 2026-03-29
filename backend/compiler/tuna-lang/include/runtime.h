#ifndef RUNTIME_H
#define RUNTIME_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/*
 * Runtime error handling for the Tuna interpreter.
 *
 * This module defines:
 * - the set of runtime error kinds
 * - a structured error object (runtime_error_t)
 * - a helper to print a descriptive error message and abort execution
 *
 * Errors are raised from anywhere in the interpreter using
 * the RAISE_RUNTIME_ERROR(...) macro.
 */

/*
 * Current line number in the source file.
 * Provided by the lexer (Flex) via yylineno.
 */
extern int yylineno;


/* ============================================================
 *  Runtime Error Kinds
 * ============================================================ */

/*
 * Enumeration of all runtime error categories supported by Tuna.
 * Each corresponds to a specific invalid runtime situation.
 */
typedef enum
{
	RT_UNDEFINED_VARIABLE,     /* use of undeclared variable */
	RT_TYPE_MISMATCH,          /* incompatible operand types */
	RT_INDEX_NON_ARRAY,        /* indexing a non-array value */
	RT_INDEX_NOT_INT,          /* array index is not integer */
	RT_INDEX_OUT_OF_BOUNDS,    /* index < 0 or >= array length */
	RT_DIVISION_BY_ZERO,       /* division by zero */
	RT_MODULO_BY_ZERO,         /* modulo by zero */
	RT_LEN_BAD_ARG,            /* len() called on invalid type */
	RT_INVALID_ASSIGN_TARGET,  /* invalid lvalue in assignment */
	RT_UNDEFINED_FUNCTION,     /* call to undefined function */
	RT_ARG_COUNT_MISMATCH      /* wrong number of function arguments */
} runtime_error_kind_t;


/* ============================================================
 *  Runtime Error Object
 * ============================================================ */

/*
 * Structured runtime error information.
 *
 * Not all fields are used for every error kind; only the relevant
 * fields are populated by the caller.
 */
typedef struct
{
	runtime_error_kind_t kind; /* error category */
	int line;                  /* source line number */

	/* Optional contextual information */
	const char *name;   /* variable or function name */
	int index;          /* array index */
	int len;            /* array length */
	const char *op;     /* operator symbol */
	const char *detail; /* additional explanation */
} runtime_error_t;


/* ============================================================
 *  Runtime Error Handler
 * ============================================================ */

/*
 * Print a human-readable runtime error message and terminate execution.
 *
 * This function does not return.
 * It is typically invoked via the RAISE_RUNTIME_ERROR macro.
 */
static inline void runtime_error(runtime_error_t e)
{
	fprintf(stderr, "Runtime error at line %d: ", e.line);

	switch (e.kind)
	{
		case RT_UNDEFINED_VARIABLE:
			fprintf(stderr, "Undefined variable '%s'", e.name);
			break;
		
		case RT_TYPE_MISMATCH:
			fprintf(stderr, "Type mismatch");
			if (e.op)     fprintf(stderr, " for operator '%s'", e.op);
			if (e.detail) fprintf(stderr, " (%s)", e.detail);
			break;
		
		case RT_INDEX_NON_ARRAY:
			fprintf(stderr, "Indexing non-array value");
			break;

		case RT_INDEX_NOT_INT:
			fprintf(stderr, "Array index must be integer");
			break;
		
		case RT_INDEX_OUT_OF_BOUNDS:
			fprintf(stderr,
			        "Array index out of bounds (index = %d, len = %d)",
			        e.index, e.len);
			break;

		case RT_DIVISION_BY_ZERO:
			fprintf(stderr, "Division by zero");
			break;
		
		case RT_MODULO_BY_ZERO:
			fprintf(stderr, "Modulo by zero");
			break;

		case RT_LEN_BAD_ARG:
			fprintf(stderr, "len() expects array or string");
			break;
		
		case RT_INVALID_ASSIGN_TARGET:
			fprintf(stderr, "Invalid assignment target");
			break;

		case RT_UNDEFINED_FUNCTION:
			fprintf(stderr, "Undefined function '%s'", e.name);
			break;
		
		case RT_ARG_COUNT_MISMATCH:
			fprintf(stderr,
			        "Argument count mismatch in call to function '%s'",
			        e.name);
			break;
	}

	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}


/* ============================================================
 *  Error Raising Macro
 * ============================================================ */

/*
 * Raise a runtime error with the given kind and optional fields.
 *
 * Automatically attaches the current source line number (yylineno).
 *
 * Example:
 *   RAISE_RUNTIME_ERROR(RT_UNDEFINED_VARIABLE, .name = "x");
 */
#define RAISE_RUNTIME_ERROR(KIND, ...) \
    runtime_error((runtime_error_t){ .kind = (KIND), .line = yylineno, ##__VA_ARGS__ })

#endif /* RUNTIME_H */
