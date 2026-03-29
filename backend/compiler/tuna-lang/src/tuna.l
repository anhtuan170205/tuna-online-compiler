%{
#include "ast.h"
#include "tuna.tab.h"
#include <stdlib.h>
#include <string.h>
%}

%option noyywrap nounput noinput yylineno

ID      [a-zA-Z_][a-zA-Z0-9_]*
INT     [0-9]+
STRING  \"([^\"\\\n]|\\[ntr\"\\])*\"

%%

"#".*					;

\r?\n+					{ return NEWLINE; }
[ \t\r]+				;

"set"					{ return SET; }
"print"					{ return PRINT; }
"if"					{ return IF; }
"then"					{ return THEN; }
"else"					{ return ELSE; }
"end"					{ return END; }

"while"					{ return WHILE; }
"do"					{ return DO; }

"true"					{ yylval.num = 1; return BOOL; }
"false"					{ yylval.num = 0; return BOOL; }

"and"					{ return AND; }
"or"					{ return OR; }
"not"					{ return NOT; }

"len"					{ return LEN; }
"readint"				{ return READINT; }
"readstr"				{ return READSTR; }
"rand"					{ return RAND; }

"func"					{ return FUNC; }
"return"				{ return RETURN; }

"push"					{ return PUSH; }
"remove"				{ return REMOVE; }

"chars"					{ return CHARS; }

":="					{ return ASSIGN; }

"=="					{ return EQ; }
"!=" 					{ return NEQ; }
"<="					{ return LE; }
">="					{ return GE; }
"<"						{ return LT; }
">"						{ return GT; }

"+"						{ return '+'; }
"-"						{ return '-'; }
"*"						{ return '*'; }
"/"						{ return '/'; }
"%"						{ return '%'; }

"[" 					{ return '['; }
"]"						{ return ']'; }
","						{ return ','; }
"("						{ return '('; }
")"						{ return ')'; }

{INT}					{ yylval.num = atoi(yytext); return NUM; }

{STRING}				{
							/* strip quotes */
							size_t n = strlen(yytext);
							char *s = (char*)malloc(n - 1);
							memcpy(s, yytext + 1, n - 2);
							s[n - 2] = '\0';
							yylval.str = s;
							return STRING;
						}

{ID}					{ yylval.id = strdup(yytext); return ID; }

<<EOF>>					{
							static int eof_sent = 0;
							if (!eof_sent) { eof_sent = 1; return NEWLINE; }
							return 0;
						}

.						{ return ERROR_CHAR; }

%%