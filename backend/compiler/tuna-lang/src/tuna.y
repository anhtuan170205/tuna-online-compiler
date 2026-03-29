%{
#include <stdio.h>
#include "ast.h"

/* lexer */
extern int yylex(void);
extern int yylineno;
void yyerror(const char *s);

/* exported program AST */
ast_stmt_t *tuna_program = NULL;
%}

%define parse.error verbose

%union 
{
	int num;
	char *id;
	char *str;
	ast_expr_t *expr;
	ast_stmt_t *stmt;
	ast_expr_list_t *elist;
	ast_id_list_t *idlist;
}

/* ------- Tokens from lexer ------- */
%token SET PRINT IF THEN ELSE END WHILE DO
%token ASSIGN
%token AND OR NOT
%token EQ NEQ LT LE GT GE
%token NEWLINE
%token ERROR_CHAR
%token LEN
%token READINT READSTR
%token RAND
%token FUNC RETURN
%token PUSH REMOVE
%token CHARS

%token <num> NUM BOOL
%token <id>  ID
%token <str> STRING

/* ----- Precedence ----- */
%left OR
%left AND
%nonassoc EQ NEQ LT LE GT GE
%left '+' '-'
%left '*' '/' '%'
%right NOT UMINUS

/* ----- Nonterminal ----- */
%type <stmt> program decl_list decl stmt_list stmt
%type <stmt> assign_stmt print_stmt if_stmt block_stmt else_stmt while_stmt
%type <stmt> opt_newlines
%type <stmt> func_decl return_stmt
%type <stmt> expr_stmt
%type <expr> expr andExpr cmpExpr addExpr mulExpr unaryExpr primary indexExpr lvalue callExpr
%type <num> cmpOp opt_nl
%type <elist> array_elems opt_array_elems opt_args args
%type <idlist> opt_params params

%start program

%%

program
	: opt_newlines decl_list stmt_list opt_newlines
		{ tuna_program = mk_program($2, mk_block($3)); $$ = tuna_program; }
	;

opt_newlines
	: opt_newlines NEWLINE
	| /* empty */
	;

decl_list
	: decl_list decl	{ $$ = stmt_append($1, $2); }
	| /* empty */		{ $$ = NULL; }
	;

decl
	: func_decl
	| NEWLINE			{ $$ = NULL; }
	;

stmt_list
	: stmt_list stmt	{ $$ = stmt_append($1, $2); }
	| /* empty */		{ $$ = NULL; }
	;

stmt
	: NEWLINE			{ $$ = NULL; } /* blank line */
	| assign_stmt
	| print_stmt
	| if_stmt
	| while_stmt
	| return_stmt
	| expr_stmt
	| error NEWLINE		{ yyerrok; $$ = NULL; }
	;

assign_stmt
	: SET lvalue ASSIGN expr NEWLINE
		{ $$ = mk_set($2, $4); }
	;

lvalue
	: ID					{ $$ = mk_var($1); }
	| lvalue '[' expr ']'	{ $$ = mk_index($1, $3); }
	;

print_stmt
	: PRINT expr NEWLINE	{ $$ = mk_print($2); }
	;

if_stmt
	: IF expr THEN NEWLINE block_stmt else_stmt END opt_newlines
		{ $$ = mk_if($2, $5, $6); }
	;

block_stmt
	: stmt_list				{ $$ = mk_block($1); }
	;

else_stmt
	: ELSE NEWLINE block_stmt	{ $$ = $3; }
	| /* empty */				{ $$ = mk_block(NULL); }
	;

while_stmt
	: WHILE expr DO NEWLINE block_stmt END opt_newlines
		{ $$ = mk_while($2, $5); }
	;

func_decl
	: FUNC ID '(' opt_params ')' NEWLINE block_stmt END opt_newlines
		{ $$ = mk_func($2, $4, $7); }
	;

opt_params
	: /* empty */			{ $$ = NULL; }
	| params				{ $$ = $1; }
	;

params
	: ID					{ $$ = id_append(NULL, $1); }
	| params ',' ID			{ $$ = id_append($1, $3); }
	;

return_stmt
	: RETURN expr NEWLINE	{ $$ = mk_return($2); }
	| RETURN NEWLINE		{ $$ = mk_return(NULL); }
	;

expr_stmt
	: expr NEWLINE			{ $$ = mk_expr_stmt($1); }
	;

expr
	: expr OR andExpr		{ $$ = mk_bin(OR, $1, $3); }
	| andExpr				{ $$ = $1; }
	;

andExpr
	: andExpr AND cmpExpr	{ $$ = mk_bin(AND, $1, $3); }
	| cmpExpr				{ $$ = $1; }
	;

cmpExpr
	: addExpr cmpOp addExpr	{ $$ = mk_bin($2, $1, $3); }
	| addExpr				{ $$ = $1; }
	;

cmpOp
	: EQ	{ $$ = EQ; 	}
	| NEQ	{ $$ = NEQ;	}
	| LT	{ $$ = LT;	}
	| LE	{ $$ = LE;	}
	| GT	{ $$ = GT;	}
	| GE	{ $$ = GE;	}
	;

addExpr
	: addExpr '+' mulExpr	{ $$ = mk_bin('+', $1, $3); }
	| addExpr '-' mulExpr	{ $$ = mk_bin('-', $1, $3);	}
	| mulExpr				{ $$ = $1; }
	;

mulExpr
	: mulExpr '*' unaryExpr	{ $$ = mk_bin('*', $1, $3); }
	| mulExpr '/' unaryExpr	{ $$ = mk_bin('/', $1, $3);	}
	| mulExpr '%' unaryExpr { $$ = mk_bin('%', $1, $3); }
	| unaryExpr				{ $$ = $1; }
	;

unaryExpr
	: NOT unaryExpr			{ $$ = mk_un(NOT, $2); }
	| '-' unaryExpr			{ $$ = mk_un(UMINUS, $2); }
	| indexExpr				{ $$ = $1; }
	;

indexExpr
	: primary					{ $$ = $1; }
	| indexExpr '[' expr ']'	{ $$ = mk_index($1, $3); }
	;

callExpr
	: ID '(' opt_args ')'		{ $$ = mk_call($1, $3); }
	;

opt_args
	: /* empty */				{ $$ = NULL; }
	| args						{ $$ = $1; }
	;

args
	: expr						{ $$ = expr_append(NULL, $1); }
	| args ',' expr				{ $$ = expr_append($1, $3); }
	;

primary
	: NUM						{ $$ = mk_int($1); }
	| BOOL						{ $$ = mk_bool($1); }
	| STRING					{ $$ = mk_string($1); }
	| callExpr					{ $$ = $1; }
	| ID						{ $$ = mk_var($1); }
	| '(' expr ')'				{ $$ = $2; }
	| '[' opt_nl opt_array_elems opt_nl ']'  { $$ = mk_array($3); }
	| LEN '(' expr ')'			{ $$ = mk_len($3); }
	| READINT '(' ')'			{ $$ = mk_readint(); }
	| READSTR '(' ')'			{ $$ = mk_readstr(); }
	| RAND '(' expr ')'			{ $$ = mk_rand($3); }
	| PUSH '(' expr ',' expr ')'	{ $$ = mk_push($3, $5); }
	| REMOVE '(' expr ',' expr ')'	{ $$ = mk_remove($3, $5); }
	| CHARS '(' expr ')'		{ $$ = mk_chars($3); }
	;

opt_array_elems
	: /* empty */				{ $$ = NULL; }
	| array_elems				{ $$ = $1; }
	;

array_elems
	: expr 						{ $$ = expr_append(NULL, $1); }
	| array_elems opt_nl ',' opt_nl expr   	{ $$ = expr_append($1, $5); }
	;

opt_nl
	: opt_nl NEWLINE			{ $$ = 0; }
	| /* empty */				{ $$ = 0; }
	;

%%

void yyerror(const char *s)
{
	fprintf(stderr, "Parse error at line %d: %s\n", yylineno, s);
}
