#ifndef _reader_h_
#define _reader_h_

#include "misc.h"
#include "builtin.h"
#include <stdio.h>

#define USE_LINENOISE
#define RUDEL_INPUT_HISTORY	".history.rd"

void			init_linenoise(void);
value_t read_str	(value_t s);
#ifdef USE_LINENOISE
value_t READ		(const char* prompt, FILE* fp);
#else // USE_LINENOISE
value_t READ		(const wchar_t* prompt, FILE* fp);
#endif // USE_LINENOISE

#endif // _reader_h_
