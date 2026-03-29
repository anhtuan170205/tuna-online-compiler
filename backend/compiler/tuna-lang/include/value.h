#ifndef VALUE_H
#define VALUE_H

#include <stdlib.h>
#include <string.h>

/*
 * Runtime value representation for the Tuna interpreter.
 *
 * This module defines:
 * - the value kinds supported by the language
 * - the value_t structure used at runtime
 * - memory management helpers (retain / release / clone)
 *
 * Values are passed around by value (value_t), but may internally
 * reference heap-allocated data (strings, arrays).
 */


/* ============================================================
 *  Value Kinds
 * ============================================================ */

/*
 * Enumeration of all runtime value types.
 */
typedef enum
{
	VAL_INT,     /* integer value */
	VAL_BOOL,    /* boolean value (stored as 0/1 in ival) */
	VAL_STRING,  /* heap-allocated C string */
	VAL_ARRAY    /* heap-allocated array object */
} value_kind_t;


/* Forward declaration for mutual references */
typedef struct value value_t;


/* ============================================================
 *  Array Object
 * ============================================================ */

/*
 * Heap-allocated array object.
 *
 * Arrays use reference counting because:
 * - they can be shared across variables
 * - indexing assignment needs stable storage
 *
 * refcount : number of value_t objects referencing this array
 * len      : number of elements
 * items    : dynamically allocated array of value_t
 */
typedef struct array_obj
{
	int refcount;
	int len;
	value_t *items;
} array_obj_t;


/* ============================================================
 *  Runtime Value
 * ============================================================ */

/*
 * A runtime value.
 *
 * The 'kind' field determines which member of the union is valid.
 */
struct value
{
	value_kind_t kind;
	union 
	{
		int ival;        /* for VAL_INT and VAL_BOOL */
		char *sval;      /* for VAL_STRING */
		array_obj_t *arr;/* for VAL_ARRAY */
	} u;	
};


/* ============================================================
 *  Value Constructors
 * ============================================================ */

/* Create an integer value */
static inline value_t v_int(int x) 
{
	value_t v;
	v.kind = VAL_INT;
	v.u.ival = x;
	return v;
}

/* Create a boolean value (0 or 1) */
static inline value_t v_bool(int b) 
{
	value_t v;
	v.kind = VAL_BOOL;
	v.u.ival = b;
	return v;
}

/* Create a string value (takes ownership of s) */
static inline value_t v_string(char *s) 
{
	value_t v;
	v.kind = VAL_STRING;
	v.u.sval = s;
	return v;
}

/*
 * Create a new array value of given length.
 * Elements are initialized to zeroed value_t structs.
 */
static inline value_t v_array(int len)
{
	array_obj_t *a = (array_obj_t *)calloc(1, sizeof(array_obj_t));
	a->len = len;
	a->items = (value_t *)calloc((size_t)len, sizeof(value_t));
	a->refcount = 1;

	value_t v;
	v.kind = VAL_ARRAY;
	v.u.arr = a;
	return v;
}


/* ============================================================
 *  Reference Counting Helpers
 * ============================================================ */

/*
 * Increment reference count of a value if it is an array.
 * Used when sharing arrays across values.
 */
static inline void v_retain(value_t v)
{
	if (v.kind == VAL_ARRAY && v.u.arr)
	{
		v.u.arr->refcount++;
	}
}

/*
 * Release a value and free underlying resources if needed.
 *
 * - Strings are always freed immediately.
 * - Arrays are reference-counted and freed only when refcount reaches 0.
 * - Primitive values (int/bool) require no action.
 */
static inline void v_release(value_t v)
{
	if (v.kind == VAL_STRING)
	{
		free(v.u.sval);
		return;
	}

	if (v.kind == VAL_ARRAY && v.u.arr)
	{
		array_obj_t *a = v.u.arr;
		a->refcount--;

		if (a->refcount <= 0)
		{
			/* recursively release all elements */
			for (int i = 0; i < a->len; i++)
			{
				v_release(a->items[i]);
			}
			free(a->items);
			free(a);
		}
		return;
	}
}


/* ============================================================
 *  Value Cloning
 * ============================================================ */

/*
 * Clone a value so that the caller gets its own logical copy.
 *
 * - int / bool : copied by value
 * - string     : deep-copied using strdup
 * - array      : shallow copy with refcount increment
 */
static inline value_t v_clone(value_t v)
{
	switch (v.kind)
	{
		case VAL_INT:
			return v_int(v.u.ival);

		case VAL_BOOL:
			return v_bool(v.u.ival);

		case VAL_STRING:
			return v_string(v.u.sval ? strdup(v.u.sval) : NULL);

		case VAL_ARRAY:
			v_retain(v);
			return v;

		default:
			return v_int(0);
	}
}

#endif /* VALUE_H */
