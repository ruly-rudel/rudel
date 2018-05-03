#ifndef _MISC_H_
#define _MISC_H_

#ifdef DEF_EXTERN

	#define EXTERN

#ifdef NO_INLINE
	#define INLINE(NAME, BODY) NAME { return BODY; }
#else  // NO_INLINE
	#define INLINE(NAME, BODY) inline NAME { return BODY; }
#endif // NO_INLINE

#else  // DEF_EXTERN

	#define EXTERN extern

#ifdef NO_INLINE
	#define INLINE(NAME, BODY) NAME;
#else  // NO_INLINE
	#define INLINE(NAME, BODY) inline NAME { return BODY; }
#endif // NO_INLINE

#endif // DEF_EXTERN

#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))

union _value_t;
union _value_t	init_global	(union _value_t pkg);
void		release_global	(void);

#endif  // _MISC_H_
