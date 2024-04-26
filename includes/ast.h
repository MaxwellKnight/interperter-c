#ifndef AST_H
#define AST_H
#include <stdbool.h>
#include "./enviroment.h"
#include "./runtime_val.h"
#include "../../data_structures/linked_list/linked_list.h"

typedef enum NodeType {
	NODE_FLOAT,
	NODE_INT,
	NODE_ADD,
	NODE_SUB,
	NODE_MUL,
	NODE_DIV,
	NODE_POW,
	NODE_MODULUS,
	NODE_GT,
	NODE_GTE,
	NODE_LT,
	NODE_LTE,
	NODE_EQUALS,
	NODE_NOT_EQUALS,
	NODE_OR,
	NODE_AND,
	NODE_UNARY_NOT,
	NODE_IF,
	NODE_IF_ELSE,
	NODE_UNARY_PLUS,
	NODE_UNARY_MINUS,
	NODE_FUNCTION_ADD,
	NODE_FUNCTION_SUB,
	NODE_FUNCTION_MUL,
	NODE_FUNCTION_DIV,
	NODE_VARIABLE,
	NODE_ASSIGN,
	NODE_FUNCTION,
	NODE_FUNC_VARIABLE,
	NODE_CALL,
	NODE_BLOCK,
} NodeType;


typedef union {
	int i_value; // INT node
	float f_value; // FLOAT node
	bool b_value; // BOOLEAN node // TODO: add boolean nodes and update the code

	List *statements; //block nodes 
	List *arguments; // operator nodes

	struct AST *condition; // if nodes

	struct AssignNodeVal {
		char *vname;
		struct AST *expr;
	} var;

	struct CallExprNode {
		char *caller;
		List *arguments;
	} call_expr;

	struct FunctionNodeVal {
		char *fname;
		struct AST *fbody; //represented by a root node of type block
		List *parameters; //list of strings
	} fn;

} NodeValue;

typedef struct AST{
	NodeValue value;
	NodeType type;
	struct AST *left;
	struct AST *right;

} AST;

/*===================== AST =====================*/
AST*			ast_init(NodeType type, AST *left, AST *right);
RuntimeVal	eval_expr(AST *root, Enviroment* env);
RuntimeVal	eval_unary_expr(AST *root, Enviroment* env);
RuntimeVal	eval_binary_expr(AST *root, Enviroment* env);
RuntimeVal	eval_boolean_expr(AST *root, Enviroment* env);
RuntimeVal	eval_variable(AST *root, Enviroment* env);
RuntimeVal 	eval_call_expr(AST *root, Enviroment *env);
RuntimeVal	eval_number(AST *root, Enviroment* env);


// node creation function
AST* 			make_int_node(int value);
AST* 			make_float_node(int value);
AST* 			make_block_node(List* statements);
AST*			make_assign_node(char *vname, AST *expr);
AST*			make_binop_node(NodeType type, AST *left, AST *right);
AST*			make_if_node(AST *condition, AST *if_case, AST *else_case);
AST*			make_func_node(char *fname, AST *fbody, List *parameters);
AST*			make_var_node(char *vname);
AST*			make_call_node(char *caller, List *arguments);
AST*			make_operator_node(NodeType type, List *arguments);

//operators add, sub, mul, div
RuntimeVal	builtin_function_add(AST *root, Enviroment *env);
RuntimeVal	builtin_function_sub(AST *root, Enviroment *env);
RuntimeVal	builtin_function_mul(AST *root, Enviroment *env);
RuntimeVal	builtin_function_div(AST *root, Enviroment *env);

//utils functions
bool 			is_binary_op(AST *root);
bool 			is_number_node(AST *root);
bool 			is_unary_op(AST *root);
bool 			is_boolean_op(AST *root);
bool 			is_keyword(int type);
void 			print_ast(AST* root, Enviroment* env, int level);
void 			print_builtin_math_function(AST* root, Enviroment* env, int level);
RuntimeVal 	coerce_to_int(RuntimeVal result);
RuntimeVal 	coerce_to_float(RuntimeVal result);

#endif