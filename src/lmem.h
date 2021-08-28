/*
** $Id: lmem.h $
** Interface to Memory Manager
** See Copyright Notice in sdkl.h
*/

#ifndef lmem_h
#define lmem_h


#include <stddef.h>

#include "llimits.h"
#include "sdkl.h"


#define sdklM_error(L)	sdklD_throw(L, SDKL_ERRMEM)


/*
** This macro tests whether it is safe to multiply 'n' by the size of
** type 't' without overflows. Because 'e' is always constant, it avoids
** the runtime division MAX_SIZET/(e).
** (The macro is somewhat complex to avoid warnings:  The 'sizeof'
** comparison avoids a runtime comparison when overflow cannot occur.
** The compiler should be able to optimize the real test by itself, but
** when it does it, it may give a warning about "comparison is always
** false due to limited range of data type"; the +1 tricks the compiler,
** avoiding this warning but also this optimization.)
*/
#define sdklM_testsize(n,e)  \
	(sizeof(n) >= sizeof(size_t) && cast_sizet((n)) + 1 > MAX_SIZET/(e))

#define sdklM_checksize(L,n,e)  \
	(sdklM_testsize(n,e) ? sdklM_toobig(L) : cast_void(0))


/*
** Computes the minimum between 'n' and 'MAX_SIZET/sizeof(t)', so that
** the result is not larger than 'n' and cannot overflow a 'size_t'
** when multiplied by the size of type 't'. (Assumes that 'n' is an
** 'int' or 'unsigned int' and that 'int' is not larger than 'size_t'.)
*/
#define sdklM_limitN(n,t)  \
  ((cast_sizet(n) <= MAX_SIZET/sizeof(t)) ? (n) :  \
     cast_uint((MAX_SIZET/sizeof(t))))


/*
** Arrays of chars do not need any test
*/
#define sdklM_reallocvchar(L,b,on,n)  \
  cast_charp(sdklM_saferealloc_(L, (b), (on)*sizeof(char), (n)*sizeof(char)))

#define sdklM_freemem(L, b, s)	sdklM_free_(L, (b), (s))
#define sdklM_free(L, b)		sdklM_free_(L, (b), sizeof(*(b)))
#define sdklM_freearray(L, b, n)   sdklM_free_(L, (b), (n)*sizeof(*(b)))

#define sdklM_new(L,t)		cast(t*, sdklM_malloc_(L, sizeof(t), 0))
#define sdklM_newvector(L,n,t)	cast(t*, sdklM_malloc_(L, (n)*sizeof(t), 0))
#define sdklM_newvectorchecked(L,n,t) \
  (sdklM_checksize(L,n,sizeof(t)), sdklM_newvector(L,n,t))

#define sdklM_newobject(L,tag,s)	sdklM_malloc_(L, (s), tag)

#define sdklM_growvector(L,v,nelems,size,t,limit,e) \
	((v)=cast(t *, sdklM_growaux_(L,v,nelems,&(size),sizeof(t), \
                         sdklM_limitN(limit,t),e)))

#define sdklM_reallocvector(L, v,oldn,n,t) \
   (cast(t *, sdklM_realloc_(L, v, cast_sizet(oldn) * sizeof(t), \
                                  cast_sizet(n) * sizeof(t))))

#define sdklM_shrinkvector(L,v,size,fs,t) \
   ((v)=cast(t *, sdklM_shrinkvector_(L, v, &(size), fs, sizeof(t))))

SDKLI_FUNC l_noret sdklM_toobig (sdkl_State *L);

/* not to be called directly */
SDKLI_FUNC void *sdklM_realloc_ (sdkl_State *L, void *block, size_t oldsize,
                                                          size_t size);
SDKLI_FUNC void *sdklM_saferealloc_ (sdkl_State *L, void *block, size_t oldsize,
                                                              size_t size);
SDKLI_FUNC void sdklM_free_ (sdkl_State *L, void *block, size_t osize);
SDKLI_FUNC void *sdklM_growaux_ (sdkl_State *L, void *block, int nelems,
                               int *size, int size_elem, int limit,
                               const char *what);
SDKLI_FUNC void *sdklM_shrinkvector_ (sdkl_State *L, void *block, int *nelem,
                                    int final_n, int size_elem);
SDKLI_FUNC void *sdklM_malloc_ (sdkl_State *L, size_t size, int tag);

#endif

