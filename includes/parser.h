#ifndef PARSER_H
#define PARSER_H
#include "../../data_structures/linked_list/linked_list.h"
#include "./tokenizer.h"
#include "./enviroment.h"
#include "./ast.h"

typedef enum {
	ERR_NONE,
	ERR_ZERO_DIV,
	ERR_SYNTAX,
	ERR_UNKNOWN,
	ERR_UNKNON_VAR
} ErrorType;

typedef struct Error{
	char *err;
	char *message;
	ErrorType type;
} Error;

typedef struct Parser{
	List *tokens; // list of Token*
	Node *curr_token; //current token
	int curr_tok_type;
} Parser;

/*===================== PARSER =====================*/
Parser*	parser_init(List *tokens);
int		is_parser_eof(Parser *parser);
Token*	parser_next(Parser *parser, Error *error);
Token*	parser_peek(Parser *parser, Error *error);
Token*	parser_skip(Parser *parser, int jmp_count, Error *error);
AST*		parse_block(Parser *parser, Enviroment* env, Error *error);
AST*		parse_function(Parser *parser, Enviroment* env, Error *error);
AST*		parse_call_expr(Parser *parser, Enviroment* env, Error *error);
List*		parse_parameters(Parser *parser, Enviroment* env, Error *error);
AST*		parse_assignment_expr(Parser *parser, Enviroment *env, Error *error);
AST*		parse_if_expr(Parser *parser, Enviroment *env, Error *error);
AST*		parse_bool_expr(Parser *parser, Enviroment *env, Error *error);
AST*		parse_logic_expr(Parser *parser, Enviroment *env, Error *error);
AST*		parse_statement(Parser *parser, Enviroment* env, Error *error);
AST*		parse_expr(Parser *parser, Enviroment* env, Error *error);
AST*		parse_term(Parser *parser, Enviroment* env, Error *error);
AST*		parse_factor(Parser *parser, Enviroment* env, Error *error);
AST*		parse_power(Parser *parser, Enviroment* env, Error *error);
List*		parse_arguments(Parser *parser, Enviroment* env, Error *error);
AST*		parse_builtin_operator(Parser *parser, Enviroment* env, Error *error);
void 		parse_error(int errorType, Error **error, char *message);
NodeType builtin_operators(Token *token);
float		str_to_float(char *str);
int		str_to_int(char *str);
bool 		is_boolean_node(int type);
bool 		is_logical(Token* token);
int		logical_operator(Token *token);
#endif