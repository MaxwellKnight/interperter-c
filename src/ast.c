#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "../includes/ast.h"

/*===================== Evaluation =====================*/

// Initialize an abstract syntax tree node with given type, value, left and right nodes
AST* ast_init(int type, void* value, AST *left, AST *right){
	AST *ast = (AST*)malloc(sizeof(AST)); 
	ast->type = type; 
	ast->left = left; 
	ast->right = right;
	ast->value = value;
	return ast; 
}

//create a node with type of int
AST* make_int_node(int val){
	AST *node = ast_init(NODE_INT, NULL, NULL, NULL);
	node->_value.i_value = val;
	return node;
}
//create a node with type of float
AST* make_float_node(int val){
	AST *node = ast_init(NODE_FLOAT, NULL, NULL, NULL);
	node->_value.f_value = val;
	return node;
}
AST* make_block_node(List *statements){
	AST *node = ast_init(NODE_BLOCK, NULL, NULL, NULL);
	node->_value.statements = statements;
	return node;
}

// evaluate the expresion represented by the abstract syntax tree and return the result
RuntimeVal eval_expr(AST *root, Enviroment* env){
	RuntimeVal result; result.type = RESULT_NONE;
	if(root == NULL) return result;
	if(is_binary_op(root) && root->right == NULL)
		return make_error(RESULT_ERROR_SYNTAX,  "Missing operand in binary operations");

	if(is_binary_op(root)) 							return eval_binary_expr(root, env);
	else if(is_boolean_op(root)) 					return eval_boolean_expr(root, env);
	else if(is_unary_op(root)) 					return eval_unary_expr(root, env);
	else if(is_number_node(root)) 				return eval_number(root, env);
	else if(root->type == NODE_VARIABLE) 		return eval_variable(root, env);
	else if(root->type == NODE_CALL) 			return eval_call_expr(root, env);
	else if(root->type == NODE_FUNCTION_ADD) 	return builtin_function_add(root, env);
	else if(root->type == NODE_FUNCTION_SUB) 	return builtin_function_sub(root, env);
	else if(root->type == NODE_FUNCTION_MUL) 	return builtin_function_mul(root, env);
	else if(root->type == NODE_FUNCTION_DIV) 	return builtin_function_div(root, env);
	else if(root->type == NODE_BLOCK){
		Node *statement = root->_value.statements->head;
		RuntimeVal stmnt; stmnt.type = RESULT_NONE;
		while(statement != NULL){
			stmnt = eval_expr(statement->value, env);
			statement = statement->next;
		}
		return stmnt;
	}
	else if(root->type == NODE_IF){
		RuntimeVal result;
		result.type = RESULT_NONE;
		RuntimeVal condition = eval_boolean_expr(root->left, env);
		if(is_error(condition)) return condition;
		if(condition.type != RESULT_BOOL){
			return make_error(RESULT_ERROR_VALUE, "Expected a boolean condition after if statement");
		}
		if(!condition.value.b_value) return result;

		return eval_expr(root->right, env);

	}  
	else if(root->type == NODE_IF_ELSE){
		RuntimeVal result;
		result.type = RESULT_INT;
		RuntimeVal condition = eval_boolean_expr(root->value, env);
		if(is_error(condition)) return condition;
		if(condition.type != RESULT_BOOL)
			return make_error(RESULT_ERROR_VALUE, "Expected a boolean condition after if statement");

		if(condition.value.b_value) return eval_expr(root->left, env);

		return eval_expr(root->right, env);

	} 			
	else if(root->type == NODE_ASSIGN){
		RuntimeVal result; result.type = RESULT_NONE;
		result = eval_expr(root->right, env);
		AST *value = NULL;
		if(result.type  == RESULT_INT) 				value = 	make_int_node(result.value.i_value);
		else if(result.type == RESULT_FLOAT) 		value = 	make_int_node(result.value.f_value);
		else make_error(RESULT_ERROR_UNDEFINED, "Undefined assignment of function to variable");

		env_assign_var(env, root->left->_value.vname, value);
		return result;
	}
	else if(root->type == NODE_FUNCTION){
		if(!root->left || !root->right) 
			return make_error(RESULT_ERROR_VALUE, "Cannot evaluate function");
		char *fname = root->left->value;
		env_define_func(env, fname, root);
		result.type = RESULT_FUNCTION;
	}

	return result;
}

RuntimeVal	eval_number(AST *root, Enviroment* env){
	RuntimeVal result;
	result.type = RESULT_ERROR;
	if(root == NULL || (root->type != NODE_FLOAT && root->type != NODE_INT))
		return make_error(RESULT_ERROR_VALUE, "call to eval_number must be FLOAT | INT");

	if(root->type == NODE_FLOAT){
		result.value.f_value = root->_value.f_value;
		result.type = RESULT_FLOAT;
		return result;
	}

	result.value.i_value = root->_value.i_value;
	result.type = RESULT_INT;
	return result;
}

RuntimeVal eval_binary_expr(AST *root, Enviroment *env){
	RuntimeVal result;
	result.type = RESULT_INT;
	RuntimeVal left = eval_expr(root->left, env);
	RuntimeVal right = eval_expr(root->right, env);
	//check for errors
	if(is_error(left) ||is_error(right)) 
		return is_error(left) ? left : right;

	if(left.type == RESULT_FLOAT || right.type == RESULT_FLOAT){
		left = coerce_to_float(left);
		right = coerce_to_float(right);
		result.type = RESULT_FLOAT;
	}

	//check for division by zero error
	if((root->type == NODE_DIV || root->type == NODE_MODULUS))
		if(coerce_to_int(right).value.i_value == 0)
			return make_error(RESULT_ERROR_ZERO_DIV, "division by zero is not allowed.");

	if(root->type == NODE_ADD){
		if(left.type == RESULT_FLOAT && right.type == RESULT_FLOAT)
			result.value.f_value = left.value.f_value + right.value.f_value;
		else if(left.type == RESULT_INT && right.type == RESULT_INT)
			result.value.i_value = left.value.i_value + right.value.i_value;
	}
	else if(root->type == NODE_SUB){
		if(left.type == RESULT_FLOAT && right.type == RESULT_FLOAT)
			result.value.f_value = left.value.f_value - right.value.f_value;
		else if(left.type == RESULT_INT && right.type == RESULT_INT)
			result.value.i_value = left.value.i_value - right.value.i_value;
	}
	else if(root->type == NODE_DIV){
		if(left.type == RESULT_FLOAT && right.type == RESULT_FLOAT)
			result.value.f_value = left.value.f_value / right.value.f_value;
		else if(left.type == RESULT_INT && right.type == RESULT_INT)
			result.value.i_value = left.value.i_value / right.value.i_value;
	}
	else if(root->type == NODE_MUL){
		if(left.type == RESULT_FLOAT && right.type == RESULT_FLOAT)
			result.value.f_value = left.value.f_value * right.value.f_value;
		else if(left.type == RESULT_INT && right.type == RESULT_INT)
			result.value.i_value = left.value.i_value * right.value.i_value;
	}
	else if(root->type == NODE_MODULUS){
		result.value.i_value = coerce_to_int(left).value.i_value % coerce_to_int(right).value.i_value;
		result.type = RESULT_INT;
	}
	else if(root->type == NODE_POW){
		if(left.type == RESULT_FLOAT && right.type == RESULT_FLOAT)
			result.value.f_value = (float)pow(left.value.f_value, right.value.f_value);
		else if(left.type == RESULT_INT && right.type == RESULT_INT)
			result.value.i_value = (int)pow(left.value.i_value, right.value.i_value);
	}

	return result;
}

RuntimeVal eval_unary_expr(AST *root, Enviroment* env){
	RuntimeVal result;
	result.type = RESULT_INT;
	result = eval_expr(root->left, env);
	if(is_error(result)) return result;

	if(root->type == NODE_UNARY_MINUS){
		 if(result.type == RESULT_INT) result.value.i_value *= -1;
		 else if(result.type == RESULT_FLOAT) result.value.f_value *= -1;
	}
	else if(root->type == NODE_UNARY_NOT){
		if(result.type == RESULT_BOOL)
			result.value.b_value = !result.value.b_value;
		if(result.type == RESULT_INT)
			result.value.b_value = result.value.i_value == 0 ? false : true;
		if(result.type == RESULT_FLOAT)
			result.value.b_value = coerce_to_int(result).value.i_value == 0 ? false : true;
		result.type = RESULT_BOOL;
	}
	else return make_error(RESULT_ERROR_VALUE, "Unsupported operation on operand");

	return result;
}

RuntimeVal	eval_boolean_expr(AST *root, Enviroment* env){
	RuntimeVal result;
	result.type = RESULT_BOOL;
	RuntimeVal left = eval_expr(root->left, env);
	RuntimeVal right = eval_expr(root->right, env);
	//check for errors
	if(is_error(left) ||is_error(right)) 
			return is_error(left) ? left : right;

	if(root->type == NODE_GT)
		result.value.b_value = coerce_to_float(left).value.f_value > coerce_to_float(right).value.f_value;
	else if(root->type == NODE_GTE)
		result.value.b_value = coerce_to_float(left).value.f_value >= coerce_to_float(right).value.f_value;
	else if(root->type == NODE_LT)
		result.value.b_value = coerce_to_float(left).value.f_value < coerce_to_float(right).value.f_value;
	else if(root->type == NODE_LTE)
		result.value.b_value = coerce_to_float(left).value.f_value <= coerce_to_float(right).value.f_value;
	else if(root->type == NODE_EQUALS)
		result.value.b_value = coerce_to_float(left).value.f_value == coerce_to_float(right).value.f_value;
	else if(root->type == NODE_NOT_EQUALS)
		result.value.b_value = coerce_to_float(left).value.f_value != coerce_to_float(right).value.f_value;
	else if(root->type == NODE_OR)
		result.value.b_value = left.value.b_value || right.value.b_value;
	else if(root->type == NODE_AND)
		result.value.b_value = left.value.b_value && right.value.b_value;
	else return make_error(RESULT_ERROR_VALUE, "Unsupported operation on operands");

	return result;
}

RuntimeVal eval_variable(AST *root, Enviroment* env){
	AST *variable = env_get_var(env, root->_value.vname);
	if(variable) 
		return eval_expr(variable, env);

	variable = env_get_function(env, root->_value.vname);
	if(variable) 
		return eval_call_expr(root, env);

	return make_error(RESULT_ERROR_UNDEFINED, (char*)root->_value.vname);
}

RuntimeVal 	eval_call_expr(AST *root, Enviroment *env){
	RuntimeVal result;
	result.type = RESULT_ERROR_UNDEFINED;
	if(root == NULL) return result;
	AST *function = env_get_function(env, root->fname);
	List *arguments = root->_value.arguments;
	List *parameters = function->value;
	Node *argsCurr = arguments->head;
	Node *paramCurr = parameters->head;
	Enviroment *scope = env_init(DEFAULT_SIZE);
	scope->parent = env_get(env, root->fname);

	if(arguments->size < parameters->size)
		return make_error(RESULT_ERROR_VALUE, "Missing arguments");
	else if(arguments->size > parameters->size)
		return make_error(RESULT_ERROR_VALUE, "Too many arguments provided");

	while(argsCurr && paramCurr){
		RuntimeVal evaluatedArg = eval_expr(argsCurr->value, env);
		AST *node = NULL;
		if(evaluatedArg.type == RESULT_INT){
			node = make_int_node(evaluatedArg.value.i_value);
		}
		else if(evaluatedArg.type == RESULT_FLOAT){
			node = make_float_node(evaluatedArg.value.f_value);
		}
		else return make_error(RESULT_ERROR_UNDEFINED, "nothing type value given");
		env_assign_var(scope, (char*)paramCurr->value, node);

		argsCurr = argsCurr->next;
		paramCurr = paramCurr->next;
	}

	RuntimeVal returnedVal = eval_expr(function->right, scope);
	return returnedVal;
}

RuntimeVal builtin_function_sub(AST *root, Enviroment* env){
	List *operands = root->_value.arguments;
	Node *curr = operands->head;
	RuntimeVal result = coerce_to_float(eval_expr(curr->value, env)); // Initialize sum with the first operand
	if(is_error(result)) return result;

	result.type = RESULT_FLOAT;
	curr = curr->next;
	while(curr != NULL){ // Iterate through remaining operands
		RuntimeVal op = coerce_to_float(eval_expr(curr->value, env));
		if(is_error(op)) return op;
		result.value.f_value -= op.value.f_value;
		curr = curr->next;
	}
	return result;
}

RuntimeVal builtin_function_add(AST *root, Enviroment* env){
	RuntimeVal result;
	List *operands = root->_value.arguments;
	Node *curr = operands->head;
	while(curr != NULL){ 
		RuntimeVal op = coerce_to_float(eval_expr(curr->value, env));
		if(is_error(op)) return op;
		result.value.f_value += op.value.f_value;
		curr = curr->next;
	}
	result.type = RESULT_FLOAT;
	return result; 
}

RuntimeVal builtin_function_mul(AST *root, Enviroment* env){
	RuntimeVal result;
	result.value.f_value = 1;
	List *operands = root->_value.arguments;
	Node *curr = operands->head;
	while(curr != NULL){ 
		RuntimeVal op = coerce_to_float(eval_expr(curr->value, env));
		if(is_error(op)) return op;
		result.value.f_value *= op.value.f_value;
		curr = curr->next;
	}
	result.type = RESULT_FLOAT;
	return result; 
}

RuntimeVal builtin_function_div(AST *root, Enviroment* env){
	List *operands = root->_value.arguments;
	RuntimeVal result;
	if(operands->size != 2)
		return make_error(RESULT_ERROR_SYNTAX, "SyntaxError: `div` accepts exactly two arguments.");

	RuntimeVal left = coerce_to_float(eval_expr(operands->head->value, env));
	RuntimeVal right = coerce_to_float(eval_expr(operands->head->next->value, env));
	if(right.value.f_value == 0)
		return make_error(RESULT_ERROR_ZERO_DIV, "Division by zero is not allowed.");

	result.value.f_value = left.value.f_value / right.value.f_value;
	result.type = RESULT_FLOAT;
	return result;
}

// Check if node type is a keyword
bool is_keyword(int type){
	return type == NODE_FUNCTION_ADD ||
			 type == NODE_FUNCTION_SUB || 
			 type == NODE_FUNCTION_MUL || 
			 type == NODE_FUNCTION_DIV;
}

bool is_binary_op(AST *root){
	return 	root->type == NODE_ADD 		|| 
				root->type == NODE_SUB 		|| 
				root->type == NODE_MUL 		|| 
				root->type == NODE_DIV 		|| 
				root->type == NODE_MODULUS	|| 
				root->type == NODE_POW;
}

bool 	is_unary_op(AST *root){
	return 	root->type == NODE_UNARY_MINUS 	|| 
				root->type == NODE_UNARY_PLUS 	|| 
				root->type == NODE_UNARY_NOT;
}
bool 	is_boolean_op(AST *root){
	return 	root->type == NODE_GT 			||
				root->type == NODE_GTE 			||
				root->type == NODE_LT			||
				root->type == NODE_LTE			||
				root->type == NODE_EQUALS		||
				root->type == NODE_NOT_EQUALS ||
				root->type == NODE_AND 			||
				root->type == NODE_OR;
}

bool is_number_node(AST *root){
	return root->type == NODE_FLOAT || root->type == NODE_INT;
}

// Print the abstract syntax tree
void print_ast(AST* root, Enviroment* env, int level) {
	if (root == NULL) return;

	for (int i = 0; i < level; i++) printf("  ");

	switch (root->type) {
		case NODE_ADD: 			printf("add:"); 												break;
		case NODE_GT: 				printf("GT(`>`):"); 											break;
		case NODE_GTE: 			printf("GTE(`>=`):"); 										break;
		case NODE_LT: 				printf("LT(`<`):"); 											break;
		case NODE_LTE: 			printf("LTE(`<=`):"); 										break;
		case NODE_EQUALS: 		printf("EQUALS(`==`):"); 									break;
		case NODE_NOT_EQUALS: 	printf("NOT-EQUALS(`!=`):"); 								break;
		case NODE_SUB: 			printf("sub:");												break;
		case NODE_MUL: 			printf("mul:"); 												break;
		case NODE_DIV: 			printf("div:");												break;
		case NODE_POW: 			printf("pow:");												break;
		case NODE_MODULUS: 		printf("modulus:");											break;
		case NODE_FLOAT:			printf("float(%.2lf)\n", root->_value.f_value); 	return; 
		case NODE_INT:				printf("int(%d)\n", root->_value.i_value); 			return; 
		case NODE_VARIABLE:		printf("var(`%s`)", root->_value.vname); 				break; 
		case NODE_ASSIGN: 		printf("assign:");											break;
		case NODE_BLOCK: 			printf("block:");												break;
		case NODE_UNARY_PLUS: 	printf("plus:");												break;
		case NODE_UNARY_MINUS: 	printf("minus:");												break;
		case NODE_IF: 				printf("if-stmnt:");											break;
		case NODE_IF_ELSE: 		printf("if-else-stmnt:");									break;
		case NODE_AND: 			printf("and-stmnt:");										break;
		case NODE_OR: 				printf("or-stmnt:");											break;
		case NODE_UNARY_NOT: 	printf("not-stmnt:");										break;
		case NODE_FUNCTION	: 	printf("function:");											break;
		case NODE_FUNC_VARIABLE:printf("fname(`%s`)", root->fname);						break;
		case NODE_CALL:			printf("<function_call %s>", root->fname);			break;
		case NODE_FUNCTION_ADD: print_builtin_math_function(root, env, level); 		return;
		case NODE_FUNCTION_SUB: print_builtin_math_function(root, env, level); 		return;
		case NODE_FUNCTION_MUL:	print_builtin_math_function(root, env, level); 		return;
		case NODE_FUNCTION_DIV: print_builtin_math_function(root, env, level); 		return;
	}
	printf("\n");

	if(root->type == NODE_VARIABLE){
		AST *value = env_get_var(env, root->_value.vname);
		print_ast(value, env, level + 1);
		return;
	}


	print_ast(root->left, env, level + 1); // Print left subtree
	if(root->type == NODE_FUNCTION){
		for (int i = 0; i < level + 1; i++) printf("  ");
		printf("parameters: ");
		list_print(root->value, print_string);
	}
	if(root->type == NODE_CALL){
		for (int i = 0; i < level + 1; i++) printf("  ");
		printf("parameters: \n");
		Node *curr = root->_value.statements->head;
		while(curr != NULL){
			print_ast(curr->value, env, level + 2);
			curr = curr->next;
		}
	}
	if(root->type == NODE_BLOCK){
		for (int i = 0; i < level + 1; i++) printf("  ");
		printf("statements:{\n");
		Node *curr = root->_value.statements->head;
		while(curr != NULL){
			print_ast(curr->value, env, level + 2);
			curr = curr->next;
		}
		for (int i = 0; i < level + 1; i++) printf("  ");
		printf("}\n");
	}
	print_ast(root->right, env, level + 1); // Print right subtree

}

void print_builtin_math_function(AST *root, Enviroment* env, int level){
	List *operands = root->_value.arguments;
	Node *curr = operands->head;

	switch (root->type){
		case NODE_FUNCTION_ADD: printf("f_add(\n"); break;
		case NODE_FUNCTION_SUB:	printf("f_sub(\n"); break;
		case NODE_FUNCTION_MUL:	printf("f_mul(\n"); break;
		case NODE_FUNCTION_DIV:	printf("f_div(\n"); break;
		default: break;
	}
	while(curr != NULL){
		print_ast(curr->value,env, level + 1);
		curr = curr->next;
	}
	for(int i = 0; i < level; i++) printf("  ");
	printf(")\n");
}