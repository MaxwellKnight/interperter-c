#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "./includes/tokenizer.h"
#include "./includes/parser.h"
#include "./includes/enviroment.h"
#include "./includes/runtime_val.h"

#define BUFFER 256
#define KEYWORD_SIZE 2
#define SYMBOL_SIZE 100

Error error_init();
void 	print_token(void *t);
void 	run(Enviroment *global_env);
char* read_contents(char *filepath);

int main(int argc, char** argv){
	Enviroment *global_env = create_global_env(SYMBOL_SIZE);
	if(argc < 2){
		run(global_env);
		return 0;
	}
	char *source = read_contents(argv[1]);
	Parser *parser = parser_init(tokenize(source));
	Error error = error_init();

	// list_print(tokenize(source), print_token);
	AST *program = parse_block(parser, global_env, &error);
	if(error.err != NULL){
		printf("%s:[%u] %s\n", error.err, error.type, error.message);
		return 0;
	}

	RuntimeVal runtime_res = eval_expr(program, global_env);
	print_runtime_val(runtime_res);
	print_ast(program, global_env, 0);

	free_list(parser->tokens, token_free);
	free(source);
	return 0;
}


//runtime REPL
void run(Enviroment *global_env){
	char input[BUFFER] = {0};
	while(true){
		printf(">>> ");
		fgets(input, BUFFER, stdin);
		Parser *parser = parser_init(tokenize(input));
		Error error = error_init();
		AST *program = parse_statement(parser, global_env, &error);
		if(error.err != NULL){
			printf("%s [%u] %s\n", error.err, error.type, error.message);
			continue;
		}

		RuntimeVal runtime_res = eval_expr(program, global_env);
		print_runtime_val(runtime_res);
	}
}

//helper function to print tokens
void print_token(void *t){
	Token *token = (Token*)t;
	printf("%s", token_to_str(token));
}

//read contents of the source file
char* read_contents(char *filepath){
	FILE *f = fopen(filepath, "rb");
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET); 

	char *source = malloc(fsize + 1);
	fread(source, fsize, 1, f);
	fclose(f);

	source[fsize] = '\0';
	return source;
}

//intialize error struct for parsing
Error error_init(){
	Error err;
	err.err =NULL;
	err.message = NULL;
	err.type = ERR_NONE;
	return err;
}

