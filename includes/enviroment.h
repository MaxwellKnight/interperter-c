#ifndef ENV_H
#define ENV_H
#include "../../data_structures/hash_table/hash_table.h"
#include "../../data_structures/linked_list/linked_list.h"

typedef HashTable Dict;

typedef struct Enviroment{
	//key: variable name, value: AST* node
	Dict *variables;
	//key: function name, value: (params, function body)
	Dict *fn_defs;
	struct Enviroment *parent;
} Enviroment;

Enviroment*	env_init(int env_size);
Enviroment*	create_global_env(int env_size);
void*			env_assign_var(Enviroment *env, char *vname, void* value);
void*			env_define_func(Enviroment *env, char *fname, void* fbody);
void*			env_get_var(Enviroment *env, char *vname);
void* 		env_get_function(Enviroment *env, char *fname);
void*			env_call_func(Enviroment *env, char *fname, void* params);
Enviroment*	env_get(Enviroment *env, char *fname);
#endif