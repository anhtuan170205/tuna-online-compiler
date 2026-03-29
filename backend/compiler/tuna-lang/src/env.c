#include "env.h"
#include <stdlib.h>
#include <string.h>

/*
 * Create a new, empty environment.
 * This environment initially has:
 *  - no variable bindings (head = NULL)
 *  - no parent (used for the global environment)
 */
env_t *env_new(void)
{
	env_t *env = (env_t *)malloc(sizeof(env_t));
	env->head = NULL;
	env->parent = NULL;
	return env;
}

/*
 * Create a new child environment with a given parent.
 * This is used to implement lexical scoping, for example:
 *  - when entering a function call
 *  - when introducing a new local scope
 *
 * Variable lookups will first search this environment,
 * then recursively search the parent environments.
 */
env_t *env_new_child(env_t *parent)
{
	env_t *env = env_new();
	env->parent = parent;
	return env;
}

/*
 * Free an environment and all variable bindings stored in it.
 * Each variable entry contains:
 *  - a dynamically allocated variable name
 *  - a runtime value (value_t) that must be released properly
 *
 * This function does NOT free the parent environment.
 */
void env_free(env_t *env)
{
	env_entry_t *cur = env->head;
	while (cur)
	{
		env_entry_t *next = cur->next;

		/* Free variable name */
		free(cur->name);

		/* Release the stored runtime value */
		v_release(cur->value);

		/* Free the entry itself */
		free(cur);

		cur = next;
	}
	free(env);
}

/*
 * Look up a variable by name in the environment chain.
 *
 * The lookup starts from the current environment and
 * continues upward through parent environments.
 *
 * If the variable is found:
 *  - its value is cloned into out_value
 *  - the function returns 1
 *
 * If the variable is not found:
 *  - the function returns 0
 */
int env_get(env_t *env, const char *name, value_t *out_value)
{
	for (env_t *cur = env; cur != NULL; cur = cur->parent)
	{
		for (env_entry_t *e = cur->head; e != NULL; e = e->next)
		{
			if (strcmp(e->name, name) == 0)
			{
				/* Clone value to avoid shared ownership issues */
				*out_value = v_clone(e->value);
				return 1;
			}
		}
	}
	return 0;
}

/*
 * Set or update a variable in the current environment.
 *
 * If the variable already exists in the current environment:
 *  - its old value is released
 *  - the new value is cloned and stored
 *
 * If the variable does not exist:
 *  - a new entry is created
 *  - the variable name is duplicated
 *  - the value is cloned and stored
 *
 * Note: this function does NOT modify parent environments.
 */
void env_set(env_t *env, const char *name, value_t value)
{
	/* Try to update an existing variable */
	for (env_entry_t *e = env->head; e != NULL; e = e->next)
	{
		if (strcmp(e->name, name) == 0)
		{
			v_release(e->value);
			e->value = v_clone(value);
			return;
		}
	}

	/* Otherwise, create a new variable entry */
	env_entry_t *e = (env_entry_t *)malloc(sizeof(env_entry_t));
	e->name = strdup(name);
	e->value = v_clone(value);
	e->next = env->head;
	env->head = e;
}
