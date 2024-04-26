#ifndef RUNTIME_VAL
#define RUNTIME_VAL
#include <stdbool.h>
#include <stdio.h>

typedef struct RuntimeVal{
	enum EvalNodeType {
		RESULT_INT,
		RESULT_FLOAT,
		RESULT_BOOL,
		RESULT_NONE,
		RESULT_ERROR,
		RESULT_ERROR_SYNTAX,
		RESULT_ERROR_UNDEFINED,
		RESULT_ERROR_VALUE,
		RESULT_ERROR_ZERO_DIV,
		RESULT_FUNCTION,
	} type;

	union {
		int i_value;
		float f_value;
		bool b_value;
		char *msg;
	} value;

	bool retval; //boolean to hold if the runtime value is a returned expression

	char *error; //error type

} RuntimeVal;

/*===================== RuntimeVal =====================*/
RuntimeVal 	coerce_to_float(RuntimeVal result);
RuntimeVal 	coerce_to_int(RuntimeVal result);
RuntimeVal 	make_error(enum EvalNodeType type, char *msg);
bool 			is_error(RuntimeVal val);
bool 			is_number(RuntimeVal val);
bool 			is_boolean(RuntimeVal val);
bool 			is_none(RuntimeVal val);
void 			print_runtime_val(RuntimeVal result);

#endif 
