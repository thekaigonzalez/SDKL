/*
** $Id: sdklauxillary.h $
** Auxiliary functions for building SDKL libraries
** See Copyright Notice in sdkl.h
*/


#ifndef sdklauxillary_h
#define sdklauxillary_h


#include <stddef.h>
#include <stdio.h>

#include "sdklconf.h"
#include "sdkl.h"


/* global table */
#define SDKL_GNAME	"_G"


typedef struct sdklL_Buffer sdklL_Buffer;


/* extra error code for 'sdklL_loadfilex' */
#define SDKL_ERRFILE     (SDKL_ERRERR+1)


/* key, in the registry, for table of loaded modules */
#define SDKL_LOADED_TABLE	"_LOADED"


/* key, in the registry, for table of preloaded loaders */
#define SDKL_PRELOAD_TABLE	"_PRELOAD"


typedef struct sdklL_Reg {
  const char *name;
  sdkl_CFunction func;
} sdklL_Reg;


#define SDKLL_NUMSIZES	(sizeof(sdkl_Integer)*16 + sizeof(sdkl_Number))

SDKLLIB_API void (sdklL_checkversion_) (sdkl_State *L, sdkl_Number ver, size_t sz);
#define sdklL_checkversion(L)  \
	  sdklL_checkversion_(L, SDKL_VERSION_NUM, SDKLL_NUMSIZES)

SDKLLIB_API int (sdklL_getmetafield) (sdkl_State *L, int obj, const char *e);
SDKLLIB_API int (sdklL_callmeta) (sdkl_State *L, int obj, const char *e);
SDKLLIB_API const char *(sdklL_tolstring) (sdkl_State *L, int idx, size_t *len);
SDKLLIB_API int (sdklL_argerror) (sdkl_State *L, int arg, const char *extramsg);
SDKLLIB_API int (sdklL_typeerror) (sdkl_State *L, int arg, const char *tname);
SDKLLIB_API const char *(sdklL_checklstring) (sdkl_State *L, int arg,
                                                          size_t *l);
SDKLLIB_API const char *(sdklL_optlstring) (sdkl_State *L, int arg,
                                          const char *def, size_t *l);
SDKLLIB_API sdkl_Number (sdklL_checknumber) (sdkl_State *L, int arg);
SDKLLIB_API sdkl_Number (sdklL_optnumber) (sdkl_State *L, int arg, sdkl_Number def);

SDKLLIB_API sdkl_Integer (sdklL_checkinteger) (sdkl_State *L, int arg);
SDKLLIB_API sdkl_Integer (sdklL_optinteger) (sdkl_State *L, int arg,
                                          sdkl_Integer def);

SDKLLIB_API void (sdklL_checkstack) (sdkl_State *L, int sz, const char *msg);
SDKLLIB_API void (sdklL_checktype) (sdkl_State *L, int arg, int t);
SDKLLIB_API void (sdklL_checkany) (sdkl_State *L, int arg);

SDKLLIB_API int   (sdklL_newmetatable) (sdkl_State *L, const char *tname);
SDKLLIB_API void  (sdklL_setmetatable) (sdkl_State *L, const char *tname);
SDKLLIB_API void *(sdklL_testudata) (sdkl_State *L, int ud, const char *tname);
SDKLLIB_API void *(sdklL_checkudata) (sdkl_State *L, int ud, const char *tname);

SDKLLIB_API void (sdklL_where) (sdkl_State *L, int lvl);
SDKLLIB_API int (sdklL_error) (sdkl_State *L, const char *fmt, ...);

SDKLLIB_API int (sdklL_checkoption) (sdkl_State *L, int arg, const char *def,
                                   const char *const lst[]);

SDKLLIB_API int (sdklL_fileresult) (sdkl_State *L, int stat, const char *fname);
SDKLLIB_API int (sdklL_execresult) (sdkl_State *L, int stat);


/* predefined references */
#define SDKL_NOREF       (-2)
#define SDKL_REFNIL      (-1)

SDKLLIB_API int (sdklL_ref) (sdkl_State *L, int t);
SDKLLIB_API void (sdklL_unref) (sdkl_State *L, int t, int ref);

SDKLLIB_API int (sdklL_loadfilex) (sdkl_State *L, const char *filename,
                                               const char *mode);

#define sdklL_loadfile(L,f)	sdklL_loadfilex(L,f,NULL)

SDKLLIB_API int (sdklL_loadbufferx) (sdkl_State *L, const char *buff, size_t sz,
                                   const char *name, const char *mode);
SDKLLIB_API int (sdklL_loadstring) (sdkl_State *L, const char *s);

SDKLLIB_API sdkl_State *(sdklL_newstate) (void);

SDKLLIB_API sdkl_Integer (sdklL_len) (sdkl_State *L, int idx);

SDKLLIB_API void sdklL_addgsub (sdklL_Buffer *b, const char *s,
                                     const char *p, const char *r);
SDKLLIB_API const char *(sdklL_gsub) (sdkl_State *L, const char *s,
                                    const char *p, const char *r);

SDKLLIB_API void (sdklL_setfuncs) (sdkl_State *L, const sdklL_Reg *l, int nup);

SDKLLIB_API int (sdklL_getsubtable) (sdkl_State *L, int idx, const char *fname);

SDKLLIB_API void (sdklL_traceback) (sdkl_State *L, sdkl_State *L1,
                                  const char *msg, int level);

SDKLLIB_API void (sdklL_requiref) (sdkl_State *L, const char *modname,
                                 sdkl_CFunction openf, int glb);

/*
** ===============================================================
** some useful macros
** ===============================================================
*/


#define sdklL_newlibtable(L,l)	\
  sdkl_createtable(L, 0, sizeof(l)/sizeof((l)[0]) - 1)

#define sdklL_newlib(L,l)  \
  (sdklL_checkversion(L), sdklL_newlibtable(L,l), sdklL_setfuncs(L,l,0))

#define sdklL_argcheck(L, cond,arg,extramsg)	\
	((void)(sdkli_likely(cond) || sdklL_argerror(L, (arg), (extramsg))))

#define sdklL_argexpected(L,cond,arg,tname)	\
	((void)(sdkli_likely(cond) || sdklL_typeerror(L, (arg), (tname))))

#define sdklL_checkstring(L,n)	(sdklL_checklstring(L, (n), NULL))
#define sdklL_optstring(L,n,d)	(sdklL_optlstring(L, (n), (d), NULL))

#define sdklL_typename(L,i)	sdkl_typename(L, sdkl_type(L,(i)))

#define sdklL_dofile(L, fn) \
	(sdklL_loadfile(L, fn) || sdkl_pcall(L, 0, SDKL_MULTRET, 0))

#define sdklL_dostring(L, s) \
	(sdklL_loadstring(L, s) || sdkl_pcall(L, 0, SDKL_MULTRET, 0))

#define sdklL_getmetatable(L,n)	(sdkl_getfield(L, SDKL_REGISTRYINDEX, (n)))

#define sdklL_opt(L,f,n,d)	(sdkl_isnoneornil(L,(n)) ? (d) : f(L,(n)))

#define sdklL_loadbuffer(L,s,sz,n)	sdklL_loadbufferx(L,s,sz,n,NULL)


/* push the value used to represent failure/error */
#define sdklL_pushfail(L)	sdkl_pushnil(L)


/*
** Internal assertions for in-house debugging
*/
#if !defined(sdkl_assert)

#if defined SDKLI_ASSERT
  #include <assert.h>
  #define sdkl_assert(c)		assert(c)
#else
  #define sdkl_assert(c)		((void)0)
#endif

#endif



/*
** {======================================================
** Generic Buffer manipulation
** =======================================================
*/

struct sdklL_Buffer {
  char *b;  /* buffer address */
  size_t size;  /* buffer size */
  size_t n;  /* number of characters in buffer */
  sdkl_State *L;
  union {
    SDKLI_MAXALIGN;  /* ensure maximum alignment for buffer */
    char b[SDKLL_BUFFERSIZE];  /* initial buffer */
  } init;
};


#define sdklL_bufflen(bf)	((bf)->n)
#define sdklL_buffaddr(bf)	((bf)->b)


#define sdklL_addchar(B,c) \
  ((void)((B)->n < (B)->size || sdklL_prepbuffsize((B), 1)), \
   ((B)->b[(B)->n++] = (c)))

#define sdklL_addsize(B,s)	((B)->n += (s))

#define sdklL_buffsub(B,s)	((B)->n -= (s))

SDKLLIB_API void (sdklL_buffinit) (sdkl_State *L, sdklL_Buffer *B);
SDKLLIB_API char *(sdklL_prepbuffsize) (sdklL_Buffer *B, size_t sz);
SDKLLIB_API void (sdklL_addlstring) (sdklL_Buffer *B, const char *s, size_t l);
SDKLLIB_API void (sdklL_addstring) (sdklL_Buffer *B, const char *s);
SDKLLIB_API void (sdklL_addvalue) (sdklL_Buffer *B);
SDKLLIB_API void (sdklL_pushresult) (sdklL_Buffer *B);
SDKLLIB_API void (sdklL_pushresultsize) (sdklL_Buffer *B, size_t sz);
SDKLLIB_API char *(sdklL_buffinitsize) (sdkl_State *L, sdklL_Buffer *B, size_t sz);

#define sdklL_prepbuffer(B)	sdklL_prepbuffsize(B, SDKLL_BUFFERSIZE)

/* }====================================================== */



/*
** {======================================================
** File handles for IO library
** =======================================================
*/

/*
** A file handle is a userdata with metatable 'SDKL_FILEHANDLE' and
** initial structure 'sdklL_Stream' (it may contain other fields
** after that initial structure).
*/

#define SDKL_FILEHANDLE          "io.file"


typedef struct sdklL_Stream {
  FILE *f;  /* stream (NULL for incompletely created streams) */
  sdkl_CFunction closef;  /* to close stream (NULL for closed streams) */
} sdklL_Stream;

/* }====================================================== */

/*
** {==================================================================
** "Abstraction Layer" for basic report of messages and errors
** ===================================================================
*/

/* print a string */
#if !defined(sdkl_writestring)
#define sdkl_writestring(s,l)   fwrite((s), sizeof(char), (l), stdout)
#endif

/* print a newline and flush the output */
#if !defined(sdkl_writeline)
#define sdkl_writeline()        (sdkl_writestring("\n", 1), fflush(stdout))
#endif

/* print an error message */
#if !defined(sdkl_writestringerror)
#define sdkl_writestringerror(s,p) \
        (fprintf(stderr, (s), (p)), fflush(stderr))
#endif

/* }================================================================== */


/*
** {============================================================
** Compatibility with deprecated conversions
** =============================================================
*/
#if defined(SDKL_COMPAT_APIINTCASTS)

#define sdklL_checkunsigned(L,a)	((sdkl_Unsigned)sdklL_checkinteger(L,a))
#define sdklL_optunsigned(L,a,d)	\
	((sdkl_Unsigned)sdklL_optinteger(L,a,(sdkl_Integer)(d)))

#define sdklL_checkint(L,n)	((int)sdklL_checkinteger(L, (n)))
#define sdklL_optint(L,n,d)	((int)sdklL_optinteger(L, (n), (d)))

#define sdklL_checklong(L,n)	((long)sdklL_checkinteger(L, (n)))
#define sdklL_optlong(L,n,d)	((long)sdklL_optinteger(L, (n), (d)))

#endif
/* }============================================================ */



#endif


