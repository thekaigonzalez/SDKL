/*
** $Id: ltable.h $
** Lua tables (hash)
** See Copyright Notice in sdkl.h
*/

#ifndef ltable_h
#define ltable_h

#include "lobject.h"


#define gnode(t,i)	(&(t)->node[i])
#define gval(n)		(&(n)->i_val)
#define gnext(n)	((n)->u.next)


/*
** Clear all bits of fast-access metamethods, which means that the table
** may have any of these metamethods. (First access that fails after the
** clearing will set the bit again.)
*/
#define invalidateTMcache(t)	((t)->flags &= ~maskflags)


/* true when 't' is using 'dummynode' as its hash part */
#define isdummy(t)		((t)->lastfree == NULL)


/* allocated size for hash nodes */
#define allocsizenode(t)	(isdummy(t) ? 0 : sizenode(t))


/* returns the Node, given the value of a table entry */
#define nodefromval(v)	cast(Node *, (v))


LUAI_FUNC const TValue *sdklH_getint (Table *t, sdkl_Integer key);
LUAI_FUNC void sdklH_setint (sdkl_State *L, Table *t, sdkl_Integer key,
                                                    TValue *value);
LUAI_FUNC const TValue *sdklH_getshortstr (Table *t, TString *key);
LUAI_FUNC const TValue *sdklH_getstr (Table *t, TString *key);
LUAI_FUNC const TValue *sdklH_get (Table *t, const TValue *key);
LUAI_FUNC void sdklH_newkey (sdkl_State *L, Table *t, const TValue *key,
                                                    TValue *value);
LUAI_FUNC void sdklH_set (sdkl_State *L, Table *t, const TValue *key,
                                                 TValue *value);
LUAI_FUNC void sdklH_finishset (sdkl_State *L, Table *t, const TValue *key,
                                       const TValue *slot, TValue *value);
LUAI_FUNC Table *sdklH_new (sdkl_State *L);
LUAI_FUNC void sdklH_resize (sdkl_State *L, Table *t, unsigned int nasize,
                                                    unsigned int nhsize);
LUAI_FUNC void sdklH_resizearray (sdkl_State *L, Table *t, unsigned int nasize);
LUAI_FUNC void sdklH_free (sdkl_State *L, Table *t);
LUAI_FUNC int sdklH_next (sdkl_State *L, Table *t, StkId key);
LUAI_FUNC sdkl_Unsigned sdklH_getn (Table *t);
LUAI_FUNC unsigned int sdklH_realasize (const Table *t);


#if defined(LUA_DEBUG)
LUAI_FUNC Node *sdklH_mainposition (const Table *t, const TValue *key);
LUAI_FUNC int sdklH_isdummy (const Table *t);
#endif


#endif
