#ifndef _printer_h_
#define _printer_h_

#include "misc.h"
#include "builtin.h"
#include <stdio.h>

void    printline	(value_t s, FILE* fp);
value_t pr_str		(value_t s, value_t pkg, value_t cyclic, bool print_readably);
void	print		(value_t s, value_t pkg, FILE* fp);

#endif // _printer_h_
