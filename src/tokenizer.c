#include "../includes/tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFF_SIZE 64

/*===================== Tokenizer =====================*/

// Initialize a token with given value and type
Token* token_init(char *value, int type){
	Token *token = (Token*)malloc(sizeof(Token)); 
	token->value = value; 
	token->type = type; 
	return token; 
}

// Convert token to string representation
char* token_to_str(Token *token){
	if(!token) return "";
	char* token_str = (char*)calloc(BUFF_SIZE, sizeof(char)); 
	sprintf(token_str, "token(%d, `%s`)", token->type, token->value);
	return token_str;
}

void token_free(void *token){
	Token *t = (Token*)token;
	free(t->value);
	free(t);
}

// Tokenize the input string and return a list of tokens
List* tokenize(char *input){
	List *tokens = createList(); 
	char *iter = input; 

	// Continue tokenizing until end of input string is reached
	while(!isEOF(iter)){ 
		skip_whitespace(&iter); 
		if(is_numeric(*iter) || *iter == '.'){
			int type;
			char *num = tokenize_numeric(iter, &type); 
			list_push(tokens, (void*)token_init(num, type)); 
			iter += strlen(num); // Move iterator by the length of the tokenized numeric value
		}
		else if(is_alpha(*iter)){
			char *keyword = tokenize_keyword(iter); 
			list_push(tokens, (void*)token_init(keyword, TOKEN_KEYWORD));
			iter += strlen(keyword); // Move iterator by the length of the tokenized keyword
		}
		else if(is_op(*iter)){ 
			int type;
			char *op = tokenize_op(iter, &type);
			list_push(tokens, (void*)token_init(op, type));
			iter += strlen(op); //next character
		}
		else if(is_paren(*iter)){
			char *paren = (char*)calloc(1, sizeof(char));
			*paren = *iter;
			list_push(tokens, (void*)token_init(paren, *paren == '(' ? TOKEN_LPAREN : TOKEN_RPAREN));
			iter++; //next character
		}
		else if(is_comma(*iter)){
			char *comma = (char*)calloc(1, sizeof(char)); // Allocate memory for comma token
			*comma = *iter; // Set the comma value
			list_push(tokens, (void*)token_init(comma, TOKEN_COMMA));
			iter++; //next character
		}
		else if(*iter == ':'){
			char *colon = (char*)calloc(1, sizeof(char)); // Allocate memory for comma token
			*colon = *iter; // Set the colon value
			list_push(tokens, (void*)token_init(colon, TOKEN_COLON));
			iter++; //next character
		}
		else if(*iter == '[' || *iter == ']'){
			char *bracket = (char*)calloc(1, sizeof(char)); // Allocate memory for comma token
			*bracket = *iter; // Set the bracket value
			list_push(tokens, (void*)token_init(bracket, *iter == '[' ? TOKEN_RBRACKET : TOKEN_LBRACKET));
			iter++; //next character
		}
		else if(*iter == '{' || *iter == '}'){
			char *brace = (char*)calloc(1, sizeof(char)); // Allocate memory for comma token
			*brace = *iter; // Set the colon value
			list_push(tokens, (void*)token_init(brace, *iter == '{' ? TOKEN_LBRACE : TOKEN_RBRACE));
			iter++; //next character
		}
		else if(*iter == '<'){
			char *eq = (char*)calloc(1, sizeof(char)); // Allocate memory for comma token
			*eq = *iter; // Set the comma value
			int type = TOKEN_LT;
			if(*(iter + 1) && *(iter + 1) == '='){
				eq = realloc(eq, sizeof(char) * 3);
				eq[0] = '<';
				eq[1] = '=';
				eq[2] = '\0';
				type = TOKEN_LTE;
				iter++;
			}
			list_push(tokens, (void*)token_init(eq, type));
			iter++; //next character
		}
		else if(*iter == '>'){
			char *eq = (char*)calloc(1, sizeof(char)); // Allocate memory for comma token
			*eq = *iter; // Set the comma value
			int type = TOKEN_GT;
			if(*(iter + 1) && *(iter + 1) == '='){
				eq = realloc(eq, sizeof(char) * 3);
				eq[0] = '>';
				eq[1] = '=';
				eq[2] = '\0';
				type = TOKEN_GTE;
				iter++;
			}
			list_push(tokens, (void*)token_init(eq, type));
			iter++; //next character
		}
		else if(*iter == '='){
			char *eq = (char*)calloc(1, sizeof(char)); // Allocate memory for comma token
			*eq = *iter; // Set the comma value
			int type = TOKEN_EQ;
			if(*(iter + 1) && (*(iter + 1) == '=' || *(iter + 1) == '>')){
				eq = realloc(eq, sizeof(char) * 3);
				eq[0] = *iter;
				eq[1] = *(iter + 1);
				eq[2] = '\0';
				type = *(iter + 1) == '=' ? TOKEN_EQUALS : TOKEN_RARROW;
				iter++;
			}
			list_push(tokens, (void*)token_init(eq, type));
			iter++; //next character
		}
		else if(*iter == '!'){
			char *eq = (char*)calloc(1, sizeof(char)); // Allocate memory for comma token
			*eq = *iter; // Set the comma value
			int type = TOKEN_NOT;
			if(*(iter + 1) && *(iter + 1) == '='){
				eq = realloc(eq, sizeof(char) * 3);
				eq[0] = '!';
				eq[1] = '=';
				eq[2] = '\0';
				type = TOKEN_NOT_EQUALS;
				iter++;
			}
			list_push(tokens, (void*)token_init(eq, type));
			iter++; //next character
		}
		else if(*iter == '\n'){
			char *newline = (char*)calloc(8, sizeof(char)); // Allocate memory for comma token
			strcpy(newline, "NEWLINE");
			newline[7] = '\0';
			list_push(tokens, (void*)token_init(newline, TOKEN_NEWLINE));
			iter++; //next character
		}
	}
	return tokens;
}

// Tokenize numeric value from the input iterator
char *tokenize_numeric(char *iter, int *type){
	char *num = (char*)calloc(1, sizeof(char));
	int index = 0;
	*type = TOKEN_INT;
	while(is_numeric(*iter) || *iter == '.'){
		if(*iter == '.') *type = TOKEN_FLOAT;
		num = (char*)realloc(num, 1 + index);
		num[index++] = *(iter++); 
	}
	num[index] = '\0'; 
	return num;
}

char *tokenize_op(char *iter, int *type){
	char *op = (char*)calloc(1, sizeof(char));
	op[0] = *iter; 
	if(*(iter + 1) && *op == '*' && *(iter + 1) == '*') {
		op = realloc(op, 3);
		op[0] = '*';
		op[1] = '*';
		op[2] = '\0';
		*type = TOKEN_POW;
	}
	else if(*op == '*') 		*type = TOKEN_MUL;
	else if(*op == '/') 	*type = TOKEN_DIV;
	else if(*op == '%') 	*type = TOKEN_MODULUS;
	else if(*op == '+')	*type = TOKEN_PLUS;
	else 						*type = TOKEN_MINUS;

	return op;
}

// Tokenize identifier from the input iterator
char *tokenize_keyword(char *iter){
	char *id = (char*)calloc(1, sizeof(char)); 
	int index = 0;

	while(is_alpha(*iter)){ 
		id = (char*)realloc(id, 1 + index); 
		id[index++] = *(iter++); 
	}
	id[index] = '\0'; 
	return id; 
}

// Skip leading whitespace characters in the input iterator
void skip_whitespace(char **iter){
	char *it = *iter; 
	while(is_whitespace(*(it++))) 
	*iter = it;
}

// Check if iterator has reached end of input string
int isEOF(char *iter){ return *iter == '\0'; }

// Check if character is a numeric digit
int is_numeric(char c){ return '0' <= c && c <= '9'; }

// Check if character is an alphabet
int is_alpha(char c){ return ('a' <= c && c <= 'z') || c == '_' || ('A' <= c && c <= 'Z'); }

// Check if character is a whitespace character
int is_whitespace(char c){ return c == ' ' || c == '\t' || c == '\r'; }

// Check if character is an operator
int is_op(char c){ return c == '+' || c == '-' || c == '*' || c == '/' || c == '%'; }

// Check if character is a parenthesis
int is_paren(char c){ return c == '(' || c == ')'; }

// Check if character is a comma
int is_comma(char c){ return c == ','; }
