#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "../includes/ast.h"

/*===================== Evaluation =====================*/

// Initialize an abstract syntax tree node with given type, value, left and right nodes
AST* ast_init(NodeType type, AST *left, AST *right){
	AST *ast = (AST*)malloc(sizeof(AST)); 
	ast->type = type; 
	ast->left = left; 
	ast->right = right;
	return ast; 
}

//create a node with type of int
AST* make_int_node(int val){
	AST *node = ast_init(NODE_INT, NULL, NULL);
	node->value.i_value = val;
	return node;
}
//create a node with type of float
AST* make_float_node(int val){
	AST *node = ast_init(NODE_FLOAT, NULL, NULL);
	node->value.f_value = val;
	return node;
}

AST* make_bool_node(bool val){
	AST *node = ast_init(NODE_BOOL, NULL, NULL);
	node->value.b_value = val;
	return node;
}

AST* make_object_node(Dictionary *properties){
	AST *node = ast_init(NODE_OBJECT, NULL, NULL);
	node->value.properties = properties;
	return node;
}
AST* make_block_node(List *statements){
	AST *node = ast_init(NODE_BLOCK, NULL, NULL);
	node->value.statements = statements;
	return node;
}

AST* make_binop_node(NodeType type, AST *left, AST *right){
	AST *binop = ast_init(type, left, right);
	return binop;
}

AST* make_if_node(AST *condition, AST *if_case, AST *else_case){
	AST *ifnode = ast_init(NODE_IF_ELSE, if_case, else_case);
	ifnode->value.condition = condition;
	return ifnode;
}

AST* make_func_node(char *fname, AST *fbody, List *parameters, Enviroment *env){
	AST *func_node = ast_init(NODE_FUNCTION, NULL, NULL);
	func_node->value.fn.fbody = fbody;
	func_node->value.fn.fname = fname;
	func_node->value.fn.parameters = parameters;
	func_node->value.fn.env = env;
	return func_node;
}

AST* make_var_node(char *vname){
	AST *varnode = ast_init(NODE_VARIABLE, NULL, NULL);
	varnode->value.var.vname = vname;
	return varnode;
}

AST* make_assign_node(char *vname, AST *expr){
	AST *assign = ast_init(NODE_ASSIGN, NULL, NULL);
	assign->value.var.vname = vname;
	assign->value.var.expr = expr;
	return assign;
}

AST* make_return_node(AST *expr){
	AST *ret = ast_init(NODE_RETURN, NULL, NULL);
	ret->value.return_expr = expr;
	return ret;
}

AST *make_call_node(char *caller, List *arguments){
	AST *call = ast_init(NODE_CALL, NULL, NULL);
	call->value.call_expr.caller  = caller;
	call->value.call_expr.arguments = arguments;

	return call;
}

AST* make_operator_node(NodeType type, List *arguments){
	AST *operator = ast_init(type, NULL, NULL);
	operator->value.arguments = arguments;
	return operator;
}

// evaluate the expresion represented by the abstract syntax tree and return the result
RuntimeVal eval_expr(AST *root, Enviroment* env){

	RuntimeVal result; result.type = RESULT_NONE;
	result.retval = false;
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
	else if(root->type == NODE_BOOL){
		result.type = RESULT_BOOL;
		result.value.b_value = root->value.b_value;
	}
	else if(root->type == NODE_BLOCK){
		Node *statement = root->value.statements->head;
		RuntimeVal stmnt; stmnt.type = RESULT_NONE; stmnt.retval = false;
		while(statement != NULL){
			stmnt = eval_expr(statement->value, env);
			if(stmnt.retval) return stmnt;
			statement = statement->next;
		}
		return stmnt;
	}
	else if(root->type == NODE_IF_ELSE){
		RuntimeVal result;
		result.type = RESULT_INT;
		RuntimeVal condition = eval_boolean_expr(root->value.condition, env);
		if(is_error(condition)) return condition;
		if(condition.type != RESULT_BOOL)
			return make_error(RESULT_ERROR_VALUE, "Expected a boolean condition after if statement");

		if(condition.value.b_value) return eval_expr(root->left, env);

		return eval_expr(root->right, env);

	} 			
	else if(root->type == NODE_ASSIGN){
		RuntimeVal result; result.type = RESULT_NONE;
		result = eval_expr(root->value.var.expr, env);
		AST *value = NULL;
		if(result.type  == RESULT_INT) 				value = 	make_int_node(result.value.i_value);
		else if(result.type == RESULT_FLOAT) 		value = 	make_int_node(result.value.f_value);
		else make_error(RESULT_ERROR_UNDEFINED, "Undefined assignment of function to variable");

		env_assign_var(env, root->value.var.vname, value);
		return result;
	}
	else if(root->type == NODE_FUNCTION){
		if(!root->value.fn.fbody) 
			return make_error(RESULT_ERROR_VALUE, "Cannot evaluate function");
		char *fname = root->value.fn.fname;
		env_define_func(env, fname, root);
		result.type = RESULT_FUNCTION;
	}
	else if(root->type == NODE_RETURN){
		result = eval_expr(root->value.return_expr, env);
		result.retval = true;
	}

	return result;
}

RuntimeVal	eval_number(AST *root, Enviroment* env){
	RuntimeVal result;
	result.type = RESULT_ERROR;
	if(root == NULL || (root->type != NODE_FLOAT && root->type != NODE_INT))
		return make_error(RESULT_ERROR_VALUE, "call to eval_number must be FLOAT | INT");

	if(root->type == NODE_FLOAT){
		result.value.f_value = root->value.f_value;
		result.type = RESULT_FLOAT;
		return result;
	}

	result.value.i_value = root->value.i_value;
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
	if((!is_number(left) || !is_number(right)) && (!is_boolean(left) || !is_boolean(right)))
		return make_error(RESULT_ERROR_VALUE, "Unsupported operation on operands");

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
	else if(root->type == NODE_OR){
		if(is_boolean(left) && is_boolean(right))
			result.value.b_value = left.value.b_value || right.value.b_value;
		else if(is_number(left) || is_number(right))
			result.value.b_value = coerce_to_int(left).value.i_value ||  coerce_to_int(right).value.i_value;
	}
	else if(root->type == NODE_AND){
		if(is_boolean(left) && is_boolean(right))
			result.value.b_value = left.value.b_value && right.value.b_value;
		else if(is_number(left) && is_number(right))
			result.value.b_value = coerce_to_int(left).value.i_value && coerce_to_int(right).value.i_value;
	}

	return result;
}

RuntimeVal eval_variable(AST *root, Enviroment* env){
	AST *variable = env_get_var(env, root->value.var.vname);
	if(variable) 
		return eval_expr(variable, env);

	variable = env_get_function(env, root->value.var.vname);
	if(variable) 
		return eval_call_expr(root, env);

	return make_error(RESULT_ERROR_UNDEFINED, (char*)root->value.var.vname);
}

RuntimeVal 	eval_call_expr(AST *root, Enviroment *env){
	RuntimeVal result;
	result.type = RESULT_ERROR_UNDEFINED;
	if(root == NULL) return result;
	AST *function = env_get_function(env, root->value.call_expr.caller);
	List *arguments = root->value.call_expr.arguments;
	List *parameters = function->value.fn.parameters;

	Node *arguments_iter = arguments->head;
	Node *parameters_iter = parameters->head;
	Enviroment *scope = function->value.fn.env; //initializing the functions enviroment
	//defining the functions definion env as the functions parent scope
	scope->parent = env_get(env, root->value.call_expr.caller); 

	if(arguments->size < parameters->size)
		return make_error(RESULT_ERROR_VALUE, "Missing arguments");
	else if(arguments->size > parameters->size)
		return make_error(RESULT_ERROR_VALUE, "Too many arguments provided");

	//evaluting the arguments and assigning them into the enviroment
	while(arguments_iter && parameters_iter){
		RuntimeVal evaluatedArg = eval_expr(arguments_iter->value, env);
		AST *node = NULL;
		if(evaluatedArg.type == RESULT_INT){
			node = make_int_node(evaluatedArg.value.i_value);
		}
		else if(evaluatedArg.type == RESULT_FLOAT){
			node = make_float_node(evaluatedArg.value.f_value);
		}
		else return make_error(RESULT_ERROR_UNDEFINED, "nothing type value given");
		env_assign_var(scope, (char*)parameters_iter->value, node);

		arguments_iter = arguments_iter->next;
		parameters_iter = parameters_iter->next;
	}

	RuntimeVal returnedVal = eval_expr(function->value.fn.fbody, scope); //evaluating the functions body
	if(returnedVal.retval) return returnedVal;
	returnedVal.type = RESULT_NONE;
	return returnedVal;
}

RuntimeVal builtin_function_sub(AST *root, Enviroment* env){
	List *operands = root->value.arguments;
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
	List *operands = root->value.arguments;
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
	List *operands = root->value.arguments;
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
	List *operands = root->value.arguments;
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

// Function to print appropriate indentation
void print_indent(int level) {
	for (int i = 0; i < level; i++) {
		printf("  ");  // Double space indentation
	}
}

// Function to print node type in a readable format
void print_node_type(AST *root) {
	if(!root) return;
	switch (root->type) {
		case NODE_ADD: 				printf("add:"); 													break;
		case NODE_SUB: 				printf("sub:"); 													break;
		case NODE_MUL: 				printf("mul:"); 													break;
		case NODE_DIV: 				printf("div:"); 													break;
		case NODE_GT: 					printf("GT(`>`):"); 												break;
		case NODE_GTE: 				printf("GTE(`>=`):"); 											break;
		case NODE_LT: 					printf("LT(`<`):"); 												break;
		case NODE_LTE: 				printf("LTE(`<=`):");											break;
		case NODE_EQUALS: 			printf("EQUALS(`==`):"); 										break;
		case NODE_NOT_EQUALS: 		printf("NOT-EQUALS(`!=`):"); 									break;
		case NODE_POW: 				printf("pow:"); 													break;
		case NODE_MODULUS: 			printf("modulus:"); 												break;
		case NODE_FLOAT: 				printf("float(%.2f)", root->value.f_value); 				break;
		case NODE_INT: 				printf("int(%d)", root->value.i_value); 					break;
		case NODE_OBJECT: 			printf("{"); 														break;
		case NODE_VARIABLE: 			printf("var(`%s`)", root->value.var.vname); 				break;
		case NODE_ASSIGN: 			printf("assign `%s`:", root->value.var.vname);			break;
		case NODE_RETURN: 			printf("return:");												break;
		case NODE_BLOCK: 				printf("block:"); 												break;
		case NODE_UNARY_PLUS: 		printf("plus:"); 													break;
		case NODE_UNARY_MINUS: 		printf("minus:"); 												break;
		case NODE_IF_ELSE: 			printf("if-else-stmnt:"); 										break;
		case NODE_AND: 				printf("and-stmnt:"); 											break;
		case NODE_OR: 					printf("or-stmnt:"); 											break;
		case NODE_UNARY_NOT: 		printf("not-stmnt:"); 											break;
		case NODE_FUNCTION: 			 																		return;
		case NODE_FUNC_VARIABLE: 	printf("fname(`%s`)", root->value.fn.fname); 			break;
		case NODE_CALL: 				printf("<function_call %s>", root->value.fn.fname); 	break;
		default: 						 																		break;
	}
}

// Function to handle built-in math functions
void print_builtin_math_function(AST* root, Enviroment* env, int level) {
	List* operands = root->value.arguments;
	print_indent(level);  // Proper indentation
	printf("%s(\n", root->type == NODE_FUNCTION_ADD ? "f_add"
		: root->type == NODE_FUNCTION_SUB ? "f_sub"
		: root->type == NODE_FUNCTION_MUL ? "f_mul"
		: "f_div");  // Switch for function type
	
	Node* curr = operands->head;
	while (curr) {
		print_ast(curr->value, env, level + 1);  // Recursive call
		curr = curr->next;
	}
	
	print_indent(level);  // Indentation for closing parenthesis
	printf(")\n");
}

// Function to print a list of parameters
void print_parameters(List* parameters) {
	Node* curr = parameters->head;
	while (curr) {
		printf("%s", (char*)curr->value);
		if (curr->next) {
			printf(", ");
		}
		curr = curr->next;
	}
}

// Function to print function nodes
void print_function_node(AST* root, Enviroment* env, int level) {
	print_indent(level);
	printf("function %s(", root->value.fn.fname);
	
	// Print function parameters
	print_parameters(root->value.fn.parameters);
	
	printf(") {\n");
	
	// Print the function body, which is a block node
	if (root->value.fn.fbody) {
		print_ast(root->value.fn.fbody, env, level + 1);  // Recurse to print the function body
	}

	print_indent(level);  // Indentation for closing block
	printf("}\n");
}
void print_object_node(AST *root, Enviroment *env, int level) {
	if (!root || root->type != NODE_OBJECT) return;

	// Start object with opening brace and newline

	List *keys = ht_keys(root->value.properties);
	Node *curr = keys->head;
	while (curr) {
		// Print each key with indentation
		print_indent(level + 1);
		printf("\"%s\": ", (char*)curr->value);  // Output key with quotes

		// Retrieve and print the value associated with the key
		AST *value = ht_retrieve(root->value.properties, curr->value);
		if(value && value->type != NODE_OBJECT){
			print_node_type(value);  // Recursive call for nested objects
			if(curr->next) printf(", ");
		}else {
			printf("\n");
			print_ast(value, env, level + 2);
		}
		printf("\n");

		curr = curr->next;
	}

	// Closing brace with proper indentation
	print_indent(level);
	printf("}\n");
}
// Recursive function to print AST
void print_ast(AST* root, Enviroment* env, int level) {
	if (!root) return;  // Base case to exit recursion

	print_indent(level);  // Ensure proper indentation
	print_node_type(root);  // Print the node type
	printf("\n");
	if(root->type == NODE_FUNCTION) 	print_function_node(root, env, level + 1);
	if(root->type == NODE_RETURN) 	print_ast(root->value.return_expr, env, level + 1);
	
	if (root->type == NODE_BLOCK) {
		print_indent(level);  // Ensure proper indentation
		printf("{\n");
		Node* curr = root->value.statements->head;
		while (curr) {
			print_ast(curr->value, env, level);  // Recursive call for each statement
			curr = curr->next;
		}
		print_indent(level);
		printf("}\n");
		return;
	}

	if (root->type == NODE_VARIABLE) {
		AST* value = env_get_var(env, root->value.var.vname);
		if (value) {
			print_ast(value, env, level + 1);  // Recurse for variable's value
			printf("\n");
		}
		return;
	}

	if (root->type == NODE_CALL) {
		print_indent(level);
		printf("arguments:\n");
		Node* curr = root->value.call_expr.arguments->head;
		while (curr) {
			print_ast(curr->value, env, level + 2);  // Recurse for each parameter
			curr = curr->next;
		}
		printf("\n");
		return;
	}

	// For unary operators, recursive printing with appropriate indentation
	if (root->type == NODE_FUNCTION_ADD || root->type == NODE_FUNCTION_SUB ||
		root->type == NODE_FUNCTION_MUL || root->type == NODE_FUNCTION_DIV) {
		print_builtin_math_function(root, env, level);  // Delegate to specific function
		printf("\n");
		return;
	}

	if(root->type == NODE_ASSIGN) print_ast(root->value.var.expr, env, level + 1);

	if(root->type == NODE_OBJECT){
		print_object_node(root, env, level);
	}

	// Recurse for left and right subtrees
	if (root->left) print_ast(root->left, env, level + 1);
	if (root->right) print_ast(root->right, env, level + 1);
}

void free_ast_list(List *nodes) {
	if(!nodes) return;

	Node *curr = nodes->head, *prev = NULL;
	while(curr != NULL){
		prev = curr;
		curr = curr->next;
		ast_free(prev->value);
	}
	list_free(nodes);
}
bool is_builtin_operator(AST *root){
	if(!root) return false;
	return 	root->type == NODE_FUNCTION_ADD ||
				root->type == NODE_FUNCTION_SUB ||
				root->type == NODE_FUNCTION_MUL ||
				root->type == NODE_FUNCTION_DIV;
}

void ast_free(AST *root){
	if(!root) return;
	//free child nodes before freeing the root
	ast_free(root->left); ast_free(root->right);

	if(is_builtin_operator(root))			free_ast_list(root->value.arguments);
	else if(root->type == NODE_BLOCK) 	free_ast_list(root->value.statements);
	else if(root->type == NODE_IF_ELSE)	ast_free(root->value.condition);
	else if(root->type == NODE_RETURN)	ast_free(root->value.return_expr);
	else if(root->type == NODE_ASSIGN)	ast_free(root->value.var.expr);
	else if(root->type == NODE_CALL)		free_ast_list(root->value.call_expr.arguments);
	else if(root->type == NODE_FUNCTION){
		ast_free(root->value.fn.fbody);
		list_free(root->value.fn.parameters);
	}

	free(root);
}