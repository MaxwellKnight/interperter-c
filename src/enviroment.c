#include <stdlib.h>
#include <stdio.h>
#include "../includes/enviroment.h"
#include "../includes/ast.h"

//initialize enviroment
Enviroment* env_init(int env_size){
	Enviroment *env = (Enviroment*)malloc(sizeof(Enviroment));
	env->variables = create_htable(env_size);
	env->fn_defs = create_htable(env_size);
	env->parent = NULL;
	return env;
}

//create the global env with builtin variables and functions
Enviroment*	create_global_env(int env_size){
	Enviroment *env = env_init(env_size);
	env_assign_var(env, "true", make_int_node(1));
	env_assign_var(env, "false", make_int_node(0));
	env_assign_var(env, "null", make_int_node(0));
	return env;
}

//define a variable in an enviroment
void*	env_assign_var(Enviroment *env, char *vname, void* value){
	if(env == NULL || value == NULL) return NULL;

	if(ht_retrieve(env->variables, vname))
		ht_delete(&env->variables, vname);
	
	ht_add(&env->variables, vname, value);
	return ht_retrieve(env->variables, value);
}

//get variable from the enviroment
void*	env_get_var(Enviroment *env, char *vname){
	if(env == NULL) return NULL;

	Enviroment *curr = env;
	while(curr != NULL){
		void *value = ht_retrieve(curr->variables, vname);
		if(value) return value;
		curr = curr->parent;
	}
	return NULL;
}

//define a function in the enviroment
void*	env_define_func(Enviroment *env, char *fname, void* fbody){
	if(env == NULL || fbody == NULL) return NULL;

	if(ht_retrieve(env->fn_defs, fname))
		ht_delete(&env->fn_defs, fname);
	
	ht_add(&env->fn_defs, fname, fbody);
	return ht_retrieve(env->fn_defs, fname);
}

//get function from the enviroment
void* env_get_function(Enviroment *env, char *fname){
	if(env == NULL) return NULL;

	Enviroment *curr = env;
	while(curr != NULL){
		void* value = ht_retrieve(curr->fn_defs, fname);
		if(value) return value;
		curr = curr->parent;
	}
	return NULL;
}

Enviroment*	env_get(Enviroment *env, char *fname){
	if(env == NULL) return NULL;
	void* fn = ht_retrieve(env->fn_defs, fname);
	if(fn != NULL) return env;
	void* var = ht_retrieve(env->variables, fname);
	if(var != NULL) return env;
	return env_get(env->parent, fname);
}