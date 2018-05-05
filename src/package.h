#ifndef _PACKAGE_H_
#define _PACKAGE_H_

#include "misc.h"
#include "builtin.h"

void	clear_symtbl		(void);
value_t	register_sym		(value_t s, value_t pkg);
value_t	create_package		(value_t name);
value_t in_package		(value_t name);

//INLINE(value_t get_pkg_env(value_t pkg),  UNSAFE_CAR(UNSAFE_CDR(pkg)))

#endif // _PACKAGE_H_
