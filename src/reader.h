#ifndef _reader_h_
#define _reader_h_

#include "misc.h"
#include "builtin.h"
#include <stdio.h>

#define USE_LINENOISE
#define RUDEL_INPUT_HISTORY	".history.rd"

#ifdef USE_LINENOISE
typedef const char* prompt_t;
#define PROMPT "user> "
#else // USE_LINENOISE
typedef const wchar_t* prompt_t;
#define PROMPT L"user> "
#endif // USE_LINENOISE

void	init_linenoise	(void);
value_t read_line	(FILE* fp);
value_t read_line_prompt(prompt_t prompt, FILE* fp);
value_t read_str	(value_t s);
value_t READ		(prompt_t prompt, FILE* fp);

#endif // _reader_h_
