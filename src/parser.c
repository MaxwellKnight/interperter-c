#include "../includes/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*

x = 5
y = 7
z = x + y

if x > y => x = y => y = y ** 2

for i in [0,5) => {
	x = x + 1
}

fn fib: n => { 
	x = n + 1
}

a = [1,2]

*/

/*
Grammar rules

if <condition> => <expression>
	<condition> (or | and | not) <condition>

program				:= statement (NEWLINE statement)*
statement			:= assignment 
						| logic_expr 
						| if_expr
						| func_def
						| for-loop

func_statement 	:= statement | return_statement //TODO
return_statement 	:= RETURN (logic_expr | expr) //TODO
				
assignment			:= KEYWORD EQ logic_expr 
for-loop				:= FOR KEYWORD IN SEQUENCE RARRAW BRACE (NEWLINE statement)* LBRACE
func_def				:= fn KEYWORD COLON (KEYWORD? (COMMA KEYWORD)*) RARROW func_body
func_body			:= LBRACE (NEWLINE func_statement)* RBRACE //TODO
func_call			:= KEYWORD LPAREN ((expr (COMMA expr)*)  | expr) RPAREN 

if_expr				:= IF COLON logic_expr RARROW if_expr (ELSE assignment)? 
						| IF COLON logic_expr RARROW LBRACE (NEWLINE statement)* RBRACE ELSE if_expr RARROW LBRACE (NEWLINE statement)* LBRACE
						| logic_expr

logic_expr			:= bool_expr ((or | and |  not) bool_expr)*

bool_expr			:= expr ( LT | LTE | GT | GTE | EQUALS | NOT_EQAULS ) expr
						| expr

expr 					:= term 	( (PLUS | MINUS) term )*
term 					:= unary ( (DIV | MUL | MOD) unary )*

unary					:= PLUS power 
						| MINUS power 
						| 	power

power					:= factor POW factor 
						| factor

factor				:= LPAREN logic_expr RPAREN 
						| 	operator

operator 			:= (add | sub | mul | div) LPAREN ( expr (COMMA expr)* ) RPAREN 
						| func_call
						| 	number

number				:= FLOAT | INT

*/

#define UNKNOWN_KEYWORD -1

/*===================== PARSER =====================*/

// Initialize the parser with a list of tokens
Parser* parser_init(List *tokens){
	Parser *parser = (Parser*)malloc(sizeof(Parser)); 
	parser->tokens = tokens; 
	parser->curr_token = tokens->head;
	return parser; 
}

// Move the iterator to the next token and return it
Token* parser_next(Parser *parser, Error *error){
	if(is_parser_eof(parser)){
		parse_error(ERR_UNKNOWN, &error, "Unexpected end of tokens");
		return NULL; 
	}
	Token *token = parser->curr_token->value; 
	parser->curr_token = parser->curr_token->next; 
	parser->curr_tok_type = parser->curr_token ? ((Token*)parser->curr_token->value)->type : -1;
	return token;
}

// Return the current token without moving the iterator, throw error if end of tokens is reached
Token* parser_peek(Parser *parser, Error *error){
	if(is_parser_eof(parser)){
		parse_error(ERR_SYNTAX, &error, "Unexpected end of tokens.");
		return NULL;
    }

	return parser->curr_token->value; 
}

Token* parser_skip(Parser *parser, int jmp_count, Error *error){
	int i = 0;
	if(is_parser_eof(parser)){
		parse_error(ERR_SYNTAX, &error, "Unexpected end of tokens.");
		return NULL;
	}

	Node *curr = parser->curr_token;
	while(curr != NULL && i < jmp_count){
		i++;
		curr = curr->next;
	}
	return curr->value;
}

// Check if the parser has reached end of tokens
int is_parser_eof(Parser *parser){ return parser->curr_token == NULL; }


AST *parse_block(Parser *parser, Enviroment *env, Error *error) {
	if (is_parser_eof(parser)) return NULL;

	// Skip leading newlines
	while (!is_parser_eof(parser) && parser_peek(parser, error)->type == TOKEN_NEWLINE) 
		parser_next(parser, error);

	List *statements = createList();
	// Parse the first statement and check for errors
	AST *statement = parse_statement(parser, env, error);
	if (error->type != ERR_NONE) return NULL;
	list_push(statements, statement);

    // Ensure there's a newline after the statement
	if (!is_parser_eof(parser) && parser_peek(parser, error)->type != TOKEN_NEWLINE) {
		parse_error(ERR_SYNTAX, &error, "Expected new line after statement");
		return NULL;
	}

	// Parse the rest of the block, separated by newlines
	while (!is_parser_eof(parser) && parser->curr_tok_type == TOKEN_NEWLINE) {
		parser_next(parser, error); // Consume newline
		if (error->type != ERR_NONE) return NULL;
		if (parser->curr_tok_type == TOKEN_RBRACE) break; // End of block

		statement = parse_statement(parser, env, error);
		if (error->type != ERR_NONE) return NULL;
		list_push(statements, statement);
	}

    AST *block = make_block_node(statements);

    // Skip trailing newlines
	while (!is_parser_eof(parser) && parser_peek(parser, error)->type == TOKEN_NEWLINE)
		parser_next(parser, error);

	return block;
}


// parse a single line statement
AST*	parse_statement(Parser *parser, Enviroment* env, Error *error){
	if(is_parser_eof(parser)) return NULL;
	
	Token *token = parser_peek(parser, error);
	if(token->type == TOKEN_KEYWORD && parser->curr_token->next != NULL){
		token = parser_skip(parser, 1, error); //if the next token is '='
		//handle assignment case
		if(token && token->type == TOKEN_EQ)
			return parse_assignment_expr(parser, env, error);
		//handle IF statement
		else if(strcmp(parser_peek(parser, error)->value, "if") == 0)
			return parse_if_expr(parser, env, error);
		else if(strcmp(parser_peek(parser, error)->value, "fn") == 0)
			return parse_function(parser, env, error);
	}
	return parse_logic_expr(parser, env, error);
}
AST* parse_assignment_expr(Parser *parser, Enviroment *env, Error *error){
	if(is_parser_eof(parser)) return NULL;

	char *vname = parser_peek(parser, error)->value;
	parser_next(parser, error); //consume variable name
	parser_next(parser, error); //consume =

	AST *expr = parse_statement(parser, env, error); // parse the variable value
	if(error->type != ERR_NONE) return NULL;
	if(expr == NULL){
		parse_error(ERR_SYNTAX, &error, "Expected expression after assignment.");
		return NULL;
	}
	AST *variable = ast_init(NODE_VARIABLE, NULL, NULL, NULL);
	variable->_value.vname = vname; 
	if(error->type != ERR_NONE) return NULL;
	//return an assign node with left child of variable node and right child of expr node
	return ast_init(NODE_ASSIGN, NULL, variable, expr);
}

AST* parse_if_expr(Parser *parser, Enviroment *env, Error *error){
	if(is_parser_eof(parser)) return NULL;

	parser_next(parser, error); // consume if 
	parser_next(parser, error); //consume :
	if(error->type != ERR_NONE) return NULL;

	AST *condition_expr = parse_logic_expr(parser, env, error);
	AST *if_body = NULL, *if_else = NULL;
	if(parser->curr_tok_type != TOKEN_RARROW){
		parse_error(ERR_SYNTAX, &error, "SynatxError: expected => after condition");
		return NULL;
	}

	parser_next(parser, error); // consume =>
	if(!is_parser_eof(parser) && parser->curr_tok_type == TOKEN_LBRACE){
		parser_next(parser, error); // consume {
		if_body = parse_block(parser, env, error);
		if(error->type != ERR_NONE) return NULL;
		if(is_parser_eof(parser) || parser->curr_tok_type != TOKEN_RBRACE){
			parse_error(ERR_SYNTAX, &error, "Exepcted closing brace } after if block");
			return NULL;
		}
		parser_next(parser, error); //consume }
		if(!is_parser_eof(parser) && strcmp(parser_peek(parser, error)->value, "else") == 0){
			parser_next(parser, error);// consume else
			if(is_parser_eof(parser) || strcmp(parser_peek(parser, error)->value, "if") != 0){
				parse_error(ERR_SYNTAX, &error, "Expected if after else statement.");
				return NULL;
			}
			if_else = parse_if_expr(parser, env, error);
			if(error->type != ERR_NONE) return NULL;
			return ast_init(NODE_IF_ELSE, condition_expr, if_body, if_else);
		}
		else if(!is_parser_eof(parser) && parser->curr_tok_type == TOKEN_RARROW){
			parser_next(parser, error); //consume =>
			if(!is_parser_eof(parser) && parser->curr_tok_type != TOKEN_LBRACE){
				parse_error(ERR_SYNTAX, &error, "Expected { after =>.");
				return NULL;
			}
			parser_next(parser, error); // consume {
			if_else = parse_block(parser, env, error);
			if(error->type != ERR_NONE) return NULL;
			if(!is_parser_eof(parser) && parser->curr_tok_type != TOKEN_RBRACE){
				parse_error(ERR_SYNTAX, &error, "Expected closing brace } after else block");
				return NULL;
			}
			parser_next(parser, error); //consume }
			return ast_init(NODE_IF_ELSE, condition_expr, if_body, if_else);
		}
		return ast_init(NODE_IF, NULL, condition_expr, if_body);
	}
	AST *expr = parse_statement(parser, env, error); // parse the expression to be evaluated

	//checking if there's an else case 
	if(!is_parser_eof(parser) && parser_peek(parser, error)->type == TOKEN_RARROW){
		if(error->type != ERR_NONE) return NULL;
		parser_next(parser, error); //consume else =>

		AST *else_expr = parse_statement(parser, env, error);
		if(error->type != ERR_NONE) return NULL;

		return ast_init(NODE_IF_ELSE, condition_expr, expr, else_expr);
	}
	if(error->type != ERR_NONE) return NULL;
	return ast_init(NODE_IF, NULL, condition_expr, expr);
}

AST* parse_function(Parser *parser, Enviroment *env, Error *error){
	if(is_parser_eof(parser)) return NULL;

	Token *token = parser_peek(parser, error);
	if(error->type != ERR_NONE) return NULL;
	if(token->type == TOKEN_KEYWORD && strcmp(token->value, "fn") == 0){
		token = parser_next(parser, error); //consume fn
		if(token->type != TOKEN_KEYWORD){ //check for function name
			parse_error(ERR_SYNTAX, &error, "Expected identifer after fn");
			return NULL;
		}
		token = parser_next(parser, error); //consume function name
		char *fname = token->value;
		parser_next(parser, error); //consume semicolon
		if(error->type != ERR_NONE) return NULL;

		List *parameters = parse_parameters(parser, env, error);
		if(error->type != ERR_NONE) return NULL;
		if(parser_peek(parser, error)->type != TOKEN_RARROW && parameters->size > 0){
			parse_error(ERR_SYNTAX, &error, "Expected => after parameters list.");
			return NULL;
		}
		parser_next(parser, error); // consume =>

		//define the function in the enviroment before parsing its body
		AST *function_var = ast_init(NODE_FUNC_VARIABLE, fname, NULL, NULL);
		function_var->fname = fname;
		env_define_func(env, fname, function_var); 

		if(!is_parser_eof(parser) && parser->curr_tok_type == TOKEN_LBRACE){
			parser_next(parser, error); // consume {
			parser_next(parser, error); // consume new line
			AST *fn_body = parse_block(parser, env, error);
			if(error->type != ERR_NONE) return NULL;
			if(parser->curr_tok_type != TOKEN_RBRACE){
				parse_error(ERR_SYNTAX, &error, "Expected } after function declation");
				return NULL;
			}
			parser_next(parser, error); // consume }
			env_define_func(env, fname, fn_body);
			return ast_init(NODE_FUNCTION, parameters, function_var, fn_body);
		}


		AST *fn_body = parse_statement(parser, env, error); // parse the function body (only single line functions for now)
		if(error->type != ERR_NONE) return NULL;
		env_define_func(env, fname, fn_body);
		return ast_init(NODE_FUNCTION, parameters, function_var, fn_body);
	}

	parse_error(ERR_SYNTAX, &error, "Expected a function declaration.");
	return NULL;
}

//parse logical expressions
AST* parse_logic_expr(Parser *parser, Enviroment *env, Error *error){
	if(is_parser_eof(parser)) return NULL;

	AST *root = parse_bool_expr(parser, env, error); //parse left hand side of logical expr
	if(error->type != ERR_NONE) return NULL;//check for errors

	while(!is_parser_eof(parser) && is_logical(parser_peek(parser, error))){
		int type = logical_operator(parser_peek(parser, error));
		parser_next(parser, error); //consume and | or
		root = ast_init(type, NULL, root, parse_bool_expr(parser, env, error));
		if(error->type != ERR_NONE) return NULL;
	}
	return root;
}
//parse boolean expressions
AST*	parse_bool_expr(Parser *parser, Enviroment *env, Error *error){
	if(is_parser_eof(parser)) return NULL;
	AST *root = parse_expr(parser, env, error); //parse the left hand side expression
	if(error->type != ERR_NONE) return NULL; //check for errors

	if(is_boolean_node(parser->curr_tok_type)){
		Token *token = parser_next(parser, error);
		if(error->type != ERR_NONE) return NULL;
		int type;
		//get the specific type of the boolean expression
		switch (token->type){
			case TOKEN_GT: 			type = NODE_GT; break;
			case TOKEN_GTE: 			type = NODE_GTE; break;
			case TOKEN_LT: 			type = NODE_LT; break;
			case TOKEN_LTE: 			type = NODE_LTE; break;
			case TOKEN_EQUALS:		type = NODE_EQUALS; break;
			case TOKEN_NOT_EQUALS:	type = NODE_NOT_EQUALS; break;
			default: return NULL;
		}
		root = ast_init(type, NULL, root, parse_expr(parser, env, error));
		if(error->type != ERR_NONE) return NULL;
	}

	return root;
}

// Parse an expression
AST* parse_expr(Parser *parser, Enviroment* env, Error *error){
	if(is_parser_eof(parser)) return NULL; 
	AST *root = parse_term(parser, env, error);  // parse left hand side
	if(error->type != ERR_NONE) return NULL;

	if (is_parser_eof(parser) || root == NULL) return root;

	while(!is_parser_eof(parser) && (parser->curr_tok_type == TOKEN_PLUS || 
												parser->curr_tok_type == TOKEN_MINUS) && 
												parser->curr_tok_type != TOKEN_NEWLINE)
	{
		int type = parser->curr_tok_type == TOKEN_PLUS ? NODE_ADD : NODE_SUB; 

		// consume operators + or -
		parser_next(parser, error); 
		//create ast node with parsed right hand side and assign to root
		root	= ast_init(type, NULL, root, parse_term(parser, env, error));
		if(error->type != ERR_NONE) return NULL;
	}
	return root;
}

// Parse a term
AST* parse_term(Parser *parser, Enviroment* env, Error *error){
	if(is_parser_eof(parser)) return NULL; 

	AST *root = parse_power(parser, env, error); // parse left hand side
	if(error->type != ERR_NONE) return NULL;

	if (is_parser_eof(parser) || root == NULL) return root;

	while(!is_parser_eof(parser) && parser->curr_tok_type != TOKEN_NEWLINE && (parser->curr_tok_type == TOKEN_MUL || 
												parser->curr_tok_type == TOKEN_DIV || 
												parser->curr_tok_type == TOKEN_MODULUS))
	{
		//assign appropriate type either * , /, %
		int type = 	parser->curr_tok_type == TOKEN_MUL ? NODE_MUL : 
						parser->curr_tok_type == TOKEN_DIV ? NODE_DIV : NODE_MODULUS;

		// consume operators * or /
		parser_next(parser, error); 
		//create ast node with parsed right hand side and assign to root
		root = ast_init(type, NULL, root, parse_power(parser, env, error));
		if(error->type != ERR_NONE) return NULL;
	}
	return root;
}

AST* parse_power(Parser *parser, Enviroment* env, Error *error) {
	if (is_parser_eof(parser)) return NULL;

	Token *token = parser_peek(parser, error);
	if (error->type != ERR_NONE) return NULL;

	AST *result = NULL;
	if (token->type == TOKEN_MINUS || token->type == TOKEN_PLUS) {
		int unary_type = (token->type == TOKEN_MINUS) ? NODE_UNARY_MINUS : NODE_UNARY_PLUS;
		parser_next(parser, error); //consume operator
		
		AST *operand = (parser->curr_tok_type == TOKEN_LPAREN)
			? parse_expr(parser, env, error)
			: parse_factor(parser, env, error);
		if (error->type != ERR_NONE) return NULL;

		result = ast_init(unary_type, NULL, operand, NULL);
	} else if (token->type == TOKEN_LPAREN) {
		parser_next(parser, error); // consume (

		result = parse_logic_expr(parser, env, error);
		if (error->type != ERR_NONE) return NULL;

		if (parser->curr_tok_type != TOKEN_RPAREN) {
			parse_error(ERR_SYNTAX, &error, "Expected ')'");
			return NULL;
		}
		parser_next(parser, error); // consume )
	} else result = parse_factor(parser, env, error);
	

	if (parser->curr_tok_type == TOKEN_POW) {
		parser_next(parser, error);

		AST *right = parse_factor(parser, env, error);
		if (error->type != ERR_NONE) return NULL;

		result = ast_init(NODE_POW, NULL, result, right);
	}

	return result;
}

AST*	parse_call_expr(Parser *parser, Enviroment* env, Error *error){
	Token *token = parser_peek(parser, error);
	if(error->type != ERR_NONE) return NULL;

	//if the keyword is a defined function
	AST *variable = (AST*)env_get_function(env, token->value);
	if(!variable) return NULL;
	if(is_parser_eof(parser) || parser_peek(parser, error)->type == TOKEN_NEWLINE)
			return variable;

	parser_next(parser, error); // consume keyword
	Token *current = parser_peek(parser, error);
	if(current->type != TOKEN_LPAREN){
		parse_error(ERR_SYNTAX, &error, "Expected `(` after function name: ");
		return NULL;
	}

	if(current->type == TOKEN_LPAREN){
		parser_next(parser, error); // consume (
		List *args = parse_arguments(parser, env, error);
		if(error->type != ERR_NONE) return NULL;

		if(is_parser_eof(parser) || parser_peek(parser, error)->type != TOKEN_RPAREN){
			parse_error(ERR_SYNTAX, &error, "Expected `)` after arguments list.");
			return NULL;
		}
		parser_next(parser, error); //consume )

		AST *root = ast_init(NODE_CALL, NULL, NULL, NULL);
		root->fname = token->value;
		root->_value.arguments = args;

		return root;
	}
	return NULL;
}
// Parse a factor
AST* parse_factor(Parser *parser, Enviroment* env, Error *error){
	if(is_parser_eof(parser)) return NULL; 
	Token *token = parser_peek(parser, error);
	if(error->type != ERR_NONE) return NULL;

	if(token->type == TOKEN_KEYWORD){

		//if the keyword is a defined variable
		AST *variable = (AST*)env_get_var(env, token->value);
		if(variable) return variable;

		AST *function = parse_call_expr(parser, env, error); //try parsing a function call
		if(error->type != ERR_NONE) return NULL;
		if(function) return function;

		//if the keyword is a known operator ie. add / mul / div 
		if(builtin_operators(token) == UNKNOWN_KEYWORD){
			parser_next(parser, error);
			AST *var =  ast_init(NODE_VARIABLE, NULL, NULL, NULL);
			var->_value.vname = token->value;
			return var;
		}

		return parse_builtin_operator(parser, env, error);
		
	}
	else if(token->type == TOKEN_FLOAT){ 
		parser_next(parser, error);
		return make_int_node(str_to_float(token->value));
	}
	else if(token->type == TOKEN_INT){
		parser_next(parser, error); 
		return make_int_node(str_to_int(token->value));
	}

	return NULL; // Return NULL if token is not recognized
}

AST*	parse_builtin_operator(Parser *parser, Enviroment* env, Error *error){
	Token *token = parser_next(parser, error); 
	if(is_parser_eof(parser)){
		parse_error(ERR_SYNTAX, &error, "Expected operator name");
		return NULL;
	}
	Token *p = parser_peek(parser, error); //consume keyword 
	if(is_parser_eof(parser) || !p || p->type != TOKEN_LPAREN){
		parse_error(ERR_SYNTAX, &error, "Expected ( after operator name.");
		return NULL;
	}
	parser_next(parser, error);

	List *arguments = parse_arguments(parser, env, error);
	if(error->type != ERR_NONE) return NULL;
	// Check if closing parenthesis is missing
	if(!is_parser_eof(parser) && parser->curr_tok_type != TOKEN_RPAREN){
		parse_error(ERR_SYNTAX, &error, "Expected ) after arguments list.");
		return NULL;
	}
	parser_next(parser, error); // consume ')'
	AST *root = ast_init(builtin_operators(token), NULL, NULL, NULL);
	root->_value.arguments = arguments;
	return root;
}

// Parse operands for function
List* parse_arguments(Parser* parser, Enviroment* env, Error *error){
	List *operands = createList(); 
	while(!is_parser_eof(parser) && parser->curr_tok_type != TOKEN_RPAREN){
		list_push(operands, (void*)parse_expr(parser, env, error));

		if(error->type != ERR_NONE) return NULL;
		if(is_parser_eof(parser)){
			parse_error(ERR_SYNTAX, &error, "Unexpected end of tokens.");
			return NULL;
		}
		if(parser->curr_tok_type == TOKEN_RPAREN || parser->curr_tok_type == TOKEN_RARROW) break;
		if(parser->curr_tok_type != TOKEN_COMMA){
			parse_error(ERR_SYNTAX, &error, "Expected `,` between arguments.");
			return NULL;
		}
		parser_next(parser, error);
	}
	return operands;
}

//parse parameters list
List* parse_parameters(Parser *parser, Enviroment* env, Error *error){
	if(is_parser_eof(parser) || error->type != ERR_NONE) return NULL;

	//create a list to store the parameters and advance to next token
	List *parameters = createList();
	Token *param = parser_next(parser, error);
	if(!param || error->type != ERR_NONE) return NULL;

	//parse comma separated parameters untill `)`
	while(!is_parser_eof(parser) && parser-> curr_tok_type == TOKEN_COMMA && parser->curr_tok_type != TOKEN_RPAREN){
		list_push(parameters, param->value);
		parser_next(parser, error); // consume param
		if(parser_peek(parser, error)->type != TOKEN_KEYWORD){
			parse_error(ERR_SYNTAX, &error, "Expected , betwen parameters list.");
			return NULL;
		}
		param = parser_next(parser, error);
	}
	if(!is_parser_eof(parser) && param->type == TOKEN_KEYWORD)
		list_push(parameters, param->value);
	return parameters;
}

// Determine the type of function based on keyword
int builtin_operators(Token *token){
	if(strcmp(token->value, "add") == 0) 			return NODE_FUNCTION_ADD;
	else if(strcmp(token->value, "sub") == 0)		return NODE_FUNCTION_SUB;
	else if(strcmp(token->value, "mul") == 0)		return NODE_FUNCTION_MUL;
	else if(strcmp(token->value, "div") == 0) 	return NODE_FUNCTION_DIV;
	
	return UNKNOWN_KEYWORD; // Return -1 if keyword is not recognized
}

// Convert string to float
float str_to_float(char *str){
	float num = 0; 
	while(*str != '\0' && *str != '.')
		num = (num)*10.0 + (*(str++) - '0'); // Convert integer part of the number
	
	if(*str != '.') return num;

	str++; // consume '.'
	float floating = 0;
	int count = 1;
	while(*str != '\0' && *str != '.'){
		floating = (floating * 10.0 + (*(str++) - '0')); // Convert fractional part of the number
		count *= 10;
	}
	num += floating / count;
	return num; 
}

// Convert string to float
int str_to_int(char *str){
	int num =  0;
	while(*str != '\0' && *str != '.')
		num = (num)*10.0 + (*(str++) - '0'); // Convert integer part of the number
	return num; 
}

void parse_error(int errorType, Error **err, char *message){
	Error *error = *err;
	switch (errorType){
		case ERR_NONE: error->err = ""; break;
		case ERR_SYNTAX: error->err = "SyntaxError:"; break;
		case ERR_UNKNON_VAR: error->err = "Undefined variable or function:"; break;
		case ERR_UNKNOWN: error->err = "UnknownValue:"; break;
		case ERR_ZERO_DIV: error->err = "Division By Zero:"; break;
		default: break;
	}
	error->type = errorType;
	error->message = message;
}

bool is_boolean_node(int type){
	return 	type == TOKEN_GT 		||
			 	type == TOKEN_GTE 	|| 
				type == TOKEN_LT 		|| 
				type == TOKEN_LTE 	|| 
				type == TOKEN_EQUALS || 
				type == TOKEN_NOT_EQUALS;
}

bool is_logical(Token* token){
	return 	strcmp(token->value, "or") == 0 	|| 
				strcmp(token->value, "and") == 0 ||
				strcmp(token->value, "not") == 0;
}

int logical_operator(Token *token){
	if(strcmp(token->value, "or") == 0)
		return NODE_OR;
	else if(strcmp(token->value, "and") == 0)
		return NODE_AND;
	else if(strcmp(token->value, "not") == 0)
		return NODE_UNARY_NOT;
	
	return UNKNOWN_KEYWORD; // Return -1 if keyword is not recognized
}