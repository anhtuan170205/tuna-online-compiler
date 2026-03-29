#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "interp.h"

/*
 * These symbols come from the parser/lexer (Bison/Flex):
 *
 * yyparse()    : entry point that parses the input stream and builds the AST
 * yyin         : FILE* used by Flex as the input stream
 * tuna_program : global pointer to the parsed AST root (set by the parser)
 */
extern int yyparse(void);
extern FILE *yyin;
extern ast_stmt_t *tuna_program;

/*
 * Built-in test programs shipped with the project.
 * User can run them by passing a test number to the executable.
 */
static const char *test_files[] =
{
	"tests/game_of_life.tuna",
	"tests/run_length_decoder.tuna",
	"tests/black_jack.tuna",
	"tests/language_demo.tuna",
};

/* Number of available tests */
static const int test_count = (int)(sizeof(test_files) / sizeof(test_files[0]));

/*
 * Print CLI usage help and list available test programs.
 */
static void print_usage(const char *prog)
{
	fprintf(stderr,
		"Usage:\n"
		"  %s <source-file>\n"
		"  %s <test-number>\n\n"
		"Available tests:\n",
		prog, prog);

	for (int i = 0; i < test_count; i++)
	{
		fprintf(stderr, "  %d: %s\n", i + 1, test_files[i]);
	}
}

/*
 * Main entry:
 * - accepts either:
 *    (1) a .tuna filename
 *    (2) a test number (1..test_count)
 * - parses the file into an AST
 * - runs the interpreter on the AST
 */
int main(int argc, char **argv)
{
	const char *filename = NULL;

	/* Expect exactly one argument besides program name */
	if (argc != 2)
	{
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	/*
	 * Try interpreting argv[1] as an integer test choice.
	 * If conversion consumes the whole string, it is a number.
	 * Otherwise, treat argv[1] as a filename.
	 */
	char *end = NULL;
	long choice = strtol(argv[1], &end, 10);

	if (end != NULL && *end == '\0')
	{
		/* argv[1] is a valid integer string */
		if (choice < 1 || choice > test_count)
		{
			fprintf(stderr, "Invalid test number: %ld\n", choice);
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
		filename = test_files[choice - 1];
	}
	else
	{
		/* argv[1] is not purely numeric -> treat as path */
		filename = argv[1];
	}

	/* Open source file for the lexer/parser */
	yyin = fopen(filename, "r");
	if (!yyin)
	{
		perror("Cannot open source file");
		return EXIT_FAILURE;
	}

	/*
	 * Parse the file:
	 * - yyparse() returns 0 on success, non-zero on failure
	 * - on success, tuna_program should be set by parser actions
	 */
	if (yyparse() != 0)
	{
		fprintf(stderr, "Parsing failed.\n");
		fclose(yyin);
		return EXIT_FAILURE;
	}

	/* Close input stream (AST is already built in memory) */
	fclose(yyin);

	/* Safety check: ensure parser produced an AST */
	if (!tuna_program)
	{
		fprintf(stderr, "No program to execute.\n");
		return EXIT_FAILURE;
	}

	/* Execute the AST using the interpreter */
	tuna_run(tuna_program);

	return EXIT_SUCCESS;
}
