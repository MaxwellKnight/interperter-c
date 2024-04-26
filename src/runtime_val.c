#include "../includes/runtime_val.h"

// Helper function to coerce the result to a float if necessary
RuntimeVal coerce_to_float(RuntimeVal result) {
    if (result.type == RESULT_INT) {
        result.value.f_value = (float)result.value.i_value; // Convert int to float
        result.type = RESULT_FLOAT;
    }
    return result;
}

// Helper function to coerce the result to a int if necessary
RuntimeVal coerce_to_int(RuntimeVal result) {
    if (result.type == RESULT_FLOAT) {
        result.value.i_value = (int)result.value.f_value; // Convert float to int
        result.type = RESULT_INT;
    }
    return result;
}

bool is_number(RuntimeVal val){ return val.type == RESULT_INT || val.type == RESULT_FLOAT;}
bool is_boolean(RuntimeVal val){ return val.type == RESULT_BOOL; }
bool is_none(RuntimeVal val) { return val.type == RESULT_NONE; }

void print_runtime_val(RuntimeVal result){
	if(is_error(result)){
		printf("%s %s\n", result.error, result.value.msg);
		return;
	}
	printf("{ type: ");
	switch(result.type){
		case RESULT_BOOL:				printf("bool"); 	break;
		case RESULT_INT: 				printf("int"); 	break;
		case RESULT_FLOAT: 			printf("float"); 	break;
		case RESULT_NONE:				printf("null");	break;
		case RESULT_FUNCTION:		printf("function");	break;
		default: printf("not supported yet");
	}
	printf(", value: ");
	switch(result.type){
		case RESULT_BOOL:
			if(result.value.b_value) printf("true");
			else printf("false");
			break;
		case RESULT_INT: 				printf("%d", result.value.i_value); break;
		case RESULT_FLOAT: 			printf("%f", result.value.f_value); break;
		case RESULT_NONE:				printf("nothing");	break;
		case RESULT_FUNCTION:		printf("nothing");	break;
		default: printf("not supported yet");
	}
	
	printf(" }\n");
}

bool is_error(RuntimeVal val){
	return 	val.type == RESULT_ERROR 				||
				val.type == RESULT_ERROR_SYNTAX		||
				val.type == RESULT_ERROR_UNDEFINED	||
				val.type == RESULT_ERROR_VALUE		||
				val.type == RESULT_ERROR_ZERO_DIV;
}

RuntimeVal 	make_error(enum EvalNodeType type, char *msg){
	RuntimeVal err;
	if(type == RESULT_ERROR_SYNTAX){
		err.type = RESULT_ERROR_SYNTAX;
		err.error = "SyntaxError:";
	}
	else if(type == RESULT_ERROR_UNDEFINED){
		err.type = RESULT_ERROR_SYNTAX;
		err.error = "Undefined keyword:";
	}
	else if(type == RESULT_ERROR_ZERO_DIV){
		err.type = RESULT_ERROR_ZERO_DIV;
		err.error = "Zero Division Error:";
	}
	else if(type == RESULT_ERROR_VALUE){
		err.type = RESULT_ERROR_VALUE;
		err.error = "ValueError:";
	}

	err.value.msg = msg;
	return err;
}