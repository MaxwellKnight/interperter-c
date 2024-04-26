#ifndef TOKENIZER_H
#define TOKENIZER_H
#include "../../data_structures/linked_list/linked_list.h"

typedef enum {
	TOKEN_FLOAT,
	TOKEN_INT,
	TOKEN_KEYWORD,
	TOKEN_PLUS,
	TOKEN_MINUS,
	TOKEN_MUL,
	TOKEN_DIV,
	TOKEN_POW,
	TOKEN_MODULUS,
	TOKEN_LPAREN,
	TOKEN_RPAREN,
	TOKEN_RBRACE,
	TOKEN_LBRACE,
	TOKEN_RBRACKET,
	TOKEN_LBRACKET,
	TOKEN_COMMA,
	TOKEN_EQ,
	TOKEN_VARIABLE,
	TOKEN_GT,
	TOKEN_GTE,
	TOKEN_LT,
	TOKEN_LTE,
	TOKEN_NOT,
	TOKEN_EQUALS,
	TOKEN_NOT_EQUALS,
	TOKEN_RARROW,
	TOKEN_NEWLINE,
	TOKEN_COLON,
	TOKEN_EOF
} TokenType;

typedef struct Token {
	char* value;
	TokenType type;
} Token;

Token* token_init(char *value, int type);
List* tokenize(char *input);
char* token_to_str(Token *token);

/* ================= BOOLEAN FUNCTIONS =================*/
int isEOF(char *input);
int is_numeric(char c);
int is_alpha(char c);
int is_whitespace(char c);
int is_op(char c);
int is_paren(char c);
int is_comma(char c);
void skip_whitespace(char **iter);
/* ================= TOKENIZATION FUNCTIONS =================*/
char *tokenize_numeric(char *iter, int *type);
char *tokenize_keyword(char *iter);
char *tokenize_op(char *iter, int *type);
void  token_free(void *token);
#endif