#ifndef _reader_h_
#define _reader_h_

#include "misc.h"
#include "builtin.h"
#include <stdio.h>

value_t readline	(FILE* fp);
value_t read_str	(value_t s);
value_t read		(FILE* fp);

#endif // _reader_h_
