#ifndef ENV_H
#define ENV_H

#include "value.h"

/*
 * Environment (symbol table) interface for the Tuna interpreter.
 *
 * An environment maps variable names to runtime values and supports
 * lexical scoping through a parent pointer.
 *
 * Each environment corresponds to a scope:
 * - the global scope
 * - a function call scope
 * - (potentially) nested block scopes if extended
 */


/* ============================================================
 *  Environment Entry
 * ============================================================ */

/*
 * One variable binding inside an environment.
 *
 * name  : variable identifier
 * value : runtime value associated with the variable
 * next  : next binding in the same environment (linked list)
 */
typedef struct env_entry
{
	char *name;
	value_t value;
	struct env_entry *next;
} env_entry_t;


/* ============================================================
 *  Environment (Scope)
 * ============================================================ */

/*
 * An environment represents one scope.
 *
 * head   : linked list of variable bindings in this scope
 * parent : enclosing (outer) environment, or NULL for global scope
 */
typedef struct env
{
	env_entry_t *head;
	struct env *parent;
} env_t;


/* ============================================================
 *  Environment API
 * ============================================================ */

/*
 * Create a new, empty environment with no parent.
 * Used to create the global environment.
 */
env_t *env_new(void);

/*
 * Create a new environment whose parent is an existing environment.
 * Used for function calls to implement lexical scoping.
 */
env_t *env_new_child(env_t *parent);

/*
 * Free an environment and all variable bindings it owns.
 * Does NOT free the parent environment.
 */
void env_free(env_t *env);

/*
 * Look up a variable by name.
 *
 * Search order:
 *   - current environment
 *   - parent environments (recursively)
 *
 * If found:
 *   - stores the value in *out_value (usually cloned internally)
 *   - returns non-zero
 *
 * If not found:
 *   - returns 0
 */
int env_get(env_t *env, const char *name, value_t *out_value);

/*
 * Bind or update a variable in the current environment.
 *
 * If the variable already exists in this environment:
 *   - its value is replaced
 *
 * If it does not exist:
 *   - a new binding is created in this environment
 *
 * Note:
 *   This function does NOT search parent environments when setting.
 */
void env_set(env_t *env, const char *name, value_t value);

#endif /* ENV_H */
