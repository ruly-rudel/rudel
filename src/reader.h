#ifndef _reader_h_
#define _reader_h_

#include "misc.h"
#include "builtin.h"
#include <stdio.h>

#define USE_LINENOISE
#define RUDEL_INPUT_HISTORY	".history.rd"

void			init_linenoise(void);
value_t read_str	(value_t s);
value_t READ		(const char* prompt, FILE* fp);

#endif // _reader_h_
