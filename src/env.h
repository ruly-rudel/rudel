#ifndef _env_h_
#define _env_h_

#include "misc.h"
#include "builtin.h"

value_t	create_env		(value_t key, value_t val, value_t outer);
value_t	set_env			(value_t key, value_t val, value_t env);
value_t	find_env_all		(value_t key, value_t env);
value_t	get_env_value		(value_t key, value_t env);
value_t	get_env_pkg		(value_t env);
value_t	get_env_ref		(value_t key, value_t env);
value_t	get_env_value_ref	(value_t ref, value_t env);
value_t	create_root_env		(value_t pkg);
//value_t	init_root_env		(value_t pkg);

#endif // _env_h_
