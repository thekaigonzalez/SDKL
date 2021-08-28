/*
** $Id: lstring.h $
** String table (keep all strings handled by SDKL)
** See Copyright Notice in sdkl.h
*/

#ifndef lstring_h
#define lstring_h

#include "lgc.h"
#include "lobject.h"
#include "lstate.h"


/*
** Memory-allocation error message must be preallocated (it cannot
** be created after memory is exhausted)
*/
#define MEMERRMSG       "not enough memory"


/*
** Size of a TString: Size of the header plus space for the string
** itself (including final '\0').
*/
#define sizelstring(l)  (offsetof(TString, contents) + ((l) + 1) * sizeof(char))

#define sdklS_newliteral(L, s)	(sdklS_newlstr(L, "" s, \
                                 (sizeof(s)/sizeof(char))-1))


/*
** test whether a string is a reserved word
*/
#define isreserved(s)	((s)->tt == SDKL_VSHRSTR && (s)->extra > 0)


/*
** equality for short strings, which are always internalized
*/
#define eqshrstr(a,b)	check_exp((a)->tt == SDKL_VSHRSTR, (a) == (b))


SDKLI_FUNC unsigned int sdklS_hash (const char *str, size_t l, unsigned int seed);
SDKLI_FUNC unsigned int sdklS_hashlongstr (TString *ts);
SDKLI_FUNC int sdklS_eqlngstr (TString *a, TString *b);
SDKLI_FUNC void sdklS_resize (sdkl_State *L, int newsize);
SDKLI_FUNC void sdklS_clearcache (global_State *g);
SDKLI_FUNC void sdklS_init (sdkl_State *L);
SDKLI_FUNC void sdklS_remove (sdkl_State *L, TString *ts);
SDKLI_FUNC Udata *sdklS_newudata (sdkl_State *L, size_t s, int nuvalue);
SDKLI_FUNC TString *sdklS_newlstr (sdkl_State *L, const char *str, size_t l);
SDKLI_FUNC TString *sdklS_new (sdkl_State *L, const char *str);
SDKLI_FUNC TString *sdklS_createlngstrobj (sdkl_State *L, size_t l);


#endif
