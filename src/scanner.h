#ifndef _scanner_h_
#define _scanner_h_

#include "misc.h"
#include "builtin.h"

typedef struct
{
	value_t		cur;
	value_t		token;
} scan_t;

scan_t	scan_init(value_t str);
value_t	scan_next(scan_t *s);
value_t	scan_peek(scan_t *s);

#endif // _scanner_h_
