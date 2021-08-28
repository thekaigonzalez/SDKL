/*
** $Id: sdkl.h $
** SDKL - A Scripting Language
** thekaigonzalez.github.io/mirrors/SDKL
** See Copyright Notice at the end of this file
*/


#ifndef sdkl_h
#define sdkl_h

#include <stdarg.h>
#include <stddef.h>


#include "sdklconf.h"


#define SDKL_VERSION_MAJOR	"3"
#define SDKL_VERSION_MINOR	"0"
#define SDKL_VERSION_RELEASE	"11"

#define SDKL_VERSION_NUM			504
#define SDKL_VERSION_RELEASE_NUM		(SDKL_VERSION_NUM * 100 + 0)

#define SDKL_VERSION	"SDKL " SDKL_VERSION_MAJOR "." SDKL_VERSION_MINOR
#define SDKL_RELEASE	SDKL_VERSION "." SDKL_VERSION_RELEASE
#define SDKL_COPYRIGHT	SDKL_RELEASE "  Copyright (C) SDKL Team"
#define SDKL_AUTHORS	"Kai D. Gonzalez"


/* mark for precompiled code ('<esc>SDKL') */
#define SDKL_SIGNATURE	"\x1bSDKL"

/* option for multiple returns in 'sdkl_pcall' and 'sdkl_call' */
#define SDKL_MULTRET	(-1)


/*
** Pseudo-indices
** (-SDKLI_MAXSTACK is the minimum valid index; we keep some free empty
** space after that to help overflow detection)
*/
#define SDKL_REGISTRYINDEX	(-SDKLI_MAXSTACK - 1000)
#define sdkl_upvalueindex(i)	(SDKL_REGISTRYINDEX - (i))


/* thread status */
#define SDKL_OK		0
#define SDKL_YIELD	1
#define SDKL_ERRRUN	2
#define SDKL_ERRSYNTAX	3
#define SDKL_ERRMEM	4
#define SDKL_ERRERR	5


typedef struct sdkl_State sdkl_State;


/*
** basic types
*/
#define SDKL_TNONE		(-1)

#define SDKL_TNIL		0
#define SDKL_TBOOLEAN		1
#define SDKL_TLIGHTUSERDATA	2
#define SDKL_TNUMBER		3
#define SDKL_TSTRING		4
#define SDKL_TTABLE		5
#define SDKL_TFUNCTION		6
#define SDKL_TUSERDATA		7
#define SDKL_TTHREAD		8

#define SDKL_NUMTYPES		9



/* minimum SDKL stack available to a C function */
#define SDKL_MINSTACK	20


/* predefined values in the registry */
#define SDKL_RIDX_MAINTHREAD	1
#define SDKL_RIDX_GLOBALS	2
#define SDKL_RIDX_LAST		SDKL_RIDX_GLOBALS


/* type of numbers in SDKL */
typedef SDKL_NUMBER sdkl_Number;


/* type for integer functions */
typedef SDKL_INTEGER sdkl_Integer;

/* unsigned integer type */
typedef SDKL_UNSIGNED sdkl_Unsigned;

/* type for continuation-function contexts */
typedef SDKL_KCONTEXT sdkl_KContext;


/*
** Type for C functions registered with SDKL
*/
typedef int (*sdkl_CFunction) (sdkl_State *L);

/*
** Type for continuation functions
*/
typedef int (*sdkl_KFunction) (sdkl_State *L, int status, sdkl_KContext ctx);


/*
** Type for functions that read/write blocks when loading/dumping SDKL chunks
*/
typedef const char * (*sdkl_Reader) (sdkl_State *L, void *ud, size_t *sz);

typedef int (*sdkl_Writer) (sdkl_State *L, const void *p, size_t sz, void *ud);


/*
** Type for memory-allocation functions
*/
typedef void * (*sdkl_Alloc) (void *ud, void *ptr, size_t osize, size_t nsize);


/*
** Type for warning functions
*/
typedef void (*sdkl_WarnFunction) (void *ud, const char *msg, int tocont);




/*
** generic extra include file
*/
#if defined(SDKL_USER_H)
#include SDKL_USER_H
#endif


/*
** RCS ident string
*/
extern const char sdkl_ident[];


/*
** state manipulation
*/
SDKL_API sdkl_State *(sdkl_newstate) (sdkl_Alloc f, void *ud);
SDKL_API void       (sdkl_close) (sdkl_State *L);
SDKL_API sdkl_State *(sdkl_newthread) (sdkl_State *L);
SDKL_API int        (sdkl_resetthread) (sdkl_State *L);

SDKL_API sdkl_CFunction (sdkl_atpanic) (sdkl_State *L, sdkl_CFunction panicf);


SDKL_API sdkl_Number (sdkl_version) (sdkl_State *L);


/*
** basic stack manipulation
*/
SDKL_API int   (sdkl_absindex) (sdkl_State *L, int idx);
SDKL_API int   (sdkl_gettop) (sdkl_State *L);
SDKL_API void  (sdkl_settop) (sdkl_State *L, int idx);
SDKL_API void  (sdkl_pushvalue) (sdkl_State *L, int idx);
SDKL_API void  (sdkl_rotate) (sdkl_State *L, int idx, int n);
SDKL_API void  (sdkl_copy) (sdkl_State *L, int fromidx, int toidx);
SDKL_API int   (sdkl_checkstack) (sdkl_State *L, int n);

SDKL_API void  (sdkl_xmove) (sdkl_State *from, sdkl_State *to, int n);


/*
** access functions (stack -> C)
*/

SDKL_API int             (sdkl_isnumber) (sdkl_State *L, int idx);
SDKL_API int             (sdkl_isstring) (sdkl_State *L, int idx);
SDKL_API int             (sdkl_iscfunction) (sdkl_State *L, int idx);
SDKL_API int             (sdkl_isinteger) (sdkl_State *L, int idx);
SDKL_API int             (sdkl_isuserdata) (sdkl_State *L, int idx);
SDKL_API int             (sdkl_type) (sdkl_State *L, int idx);
SDKL_API const char     *(sdkl_typename) (sdkl_State *L, int tp);

SDKL_API sdkl_Number      (sdkl_tonumberx) (sdkl_State *L, int idx, int *isnum);
SDKL_API sdkl_Integer     (sdkl_tointegerx) (sdkl_State *L, int idx, int *isnum);
SDKL_API int             (sdkl_toboolean) (sdkl_State *L, int idx);
SDKL_API const char     *(sdkl_tolstring) (sdkl_State *L, int idx, size_t *len);
SDKL_API sdkl_Unsigned    (sdkl_rawlen) (sdkl_State *L, int idx);
SDKL_API sdkl_CFunction   (sdkl_tocfunction) (sdkl_State *L, int idx);
SDKL_API void	       *(sdkl_touserdata) (sdkl_State *L, int idx);
SDKL_API sdkl_State      *(sdkl_tothread) (sdkl_State *L, int idx);
SDKL_API const void     *(sdkl_topointer) (sdkl_State *L, int idx);


/*
** Comparison and arithmetic functions
*/

#define SDKL_OPADD	0	/* ORDER TM, ORDER OP */
#define SDKL_OPSUB	1
#define SDKL_OPMUL	2
#define SDKL_OPMOD	3
#define SDKL_OPPOW	4
#define SDKL_OPDIV	5
#define SDKL_OPIDIV	6
#define SDKL_OPBAND	7
#define SDKL_OPBOR	8
#define SDKL_OPBXOR	9
#define SDKL_OPSHL	10
#define SDKL_OPSHR	11
#define SDKL_OPUNM	12
#define SDKL_OPBNOT	13

SDKL_API void  (sdkl_arith) (sdkl_State *L, int op);

#define SDKL_OPEQ	0
#define SDKL_OPLT	1
#define SDKL_OPLE	2

SDKL_API int   (sdkl_rawequal) (sdkl_State *L, int idx1, int idx2);
SDKL_API int   (sdkl_compare) (sdkl_State *L, int idx1, int idx2, int op);


/*
** push functions (C -> stack)
*/
SDKL_API void        (sdkl_pushnil) (sdkl_State *L);
SDKL_API void        (sdkl_pushnumber) (sdkl_State *L, sdkl_Number n);
SDKL_API void        (sdkl_pushinteger) (sdkl_State *L, sdkl_Integer n);
SDKL_API const char *(sdkl_pushlstring) (sdkl_State *L, const char *s, size_t len);
SDKL_API const char *(sdkl_pushstring) (sdkl_State *L, const char *s);
SDKL_API const char *(sdkl_pushvfstring) (sdkl_State *L, const char *fmt,
                                                      va_list argp);
SDKL_API const char *(sdkl_pushfstring) (sdkl_State *L, const char *fmt, ...);
SDKL_API void  (sdkl_pushcclosure) (sdkl_State *L, sdkl_CFunction fn, int n);
SDKL_API void  (sdkl_pushboolean) (sdkl_State *L, int b);
SDKL_API void  (sdkl_pushlightuserdata) (sdkl_State *L, void *p);
SDKL_API int   (sdkl_pushthread) (sdkl_State *L);


/*
** get functions (SDKL -> stack)
*/
SDKL_API int (sdkl_getglobal) (sdkl_State *L, const char *name);
SDKL_API int (sdkl_gettable) (sdkl_State *L, int idx);
SDKL_API int (sdkl_getfield) (sdkl_State *L, int idx, const char *k);
SDKL_API int (sdkl_geti) (sdkl_State *L, int idx, sdkl_Integer n);
SDKL_API int (sdkl_rawget) (sdkl_State *L, int idx);
SDKL_API int (sdkl_rawgeti) (sdkl_State *L, int idx, sdkl_Integer n);
SDKL_API int (sdkl_rawgetp) (sdkl_State *L, int idx, const void *p);

SDKL_API void  (sdkl_createtable) (sdkl_State *L, int narr, int nrec);
SDKL_API void *(sdkl_newuserdatauv) (sdkl_State *L, size_t sz, int nuvalue);
SDKL_API int   (sdkl_getmetatable) (sdkl_State *L, int objindex);
SDKL_API int  (sdkl_getiuservalue) (sdkl_State *L, int idx, int n);


/*
** set functions (stack -> SDKL)
*/
SDKL_API void  (sdkl_setglobal) (sdkl_State *L, const char *name);
SDKL_API void  (sdkl_settable) (sdkl_State *L, int idx);
SDKL_API void  (sdkl_setfield) (sdkl_State *L, int idx, const char *k);
SDKL_API void  (sdkl_seti) (sdkl_State *L, int idx, sdkl_Integer n);
SDKL_API void  (sdkl_rawset) (sdkl_State *L, int idx);
SDKL_API void  (sdkl_rawseti) (sdkl_State *L, int idx, sdkl_Integer n);
SDKL_API void  (sdkl_rawsetp) (sdkl_State *L, int idx, const void *p);
SDKL_API int   (sdkl_setmetatable) (sdkl_State *L, int objindex);
SDKL_API int   (sdkl_setiuservalue) (sdkl_State *L, int idx, int n);


/*
** 'load' and 'call' functions (load and run SDKL code)
*/
SDKL_API void  (sdkl_callk) (sdkl_State *L, int nargs, int nresults,
                           sdkl_KContext ctx, sdkl_KFunction k);
#define sdkl_call(L,n,r)		sdkl_callk(L, (n), (r), 0, NULL)

SDKL_API int   (sdkl_pcallk) (sdkl_State *L, int nargs, int nresults, int errfunc,
                            sdkl_KContext ctx, sdkl_KFunction k);
#define sdkl_pcall(L,n,r,f)	sdkl_pcallk(L, (n), (r), (f), 0, NULL)

SDKL_API int   (sdkl_load) (sdkl_State *L, sdkl_Reader reader, void *dt,
                          const char *chunkname, const char *mode);

SDKL_API int (sdkl_dump) (sdkl_State *L, sdkl_Writer writer, void *data, int strip);


/*
** coroutine functions
*/
SDKL_API int  (sdkl_yieldk)     (sdkl_State *L, int nresults, sdkl_KContext ctx,
                               sdkl_KFunction k);
SDKL_API int  (sdkl_resume)     (sdkl_State *L, sdkl_State *from, int narg,
                               int *nres);
SDKL_API int  (sdkl_status)     (sdkl_State *L);
SDKL_API int (sdkl_isyieldable) (sdkl_State *L);

#define sdkl_yield(L,n)		sdkl_yieldk(L, (n), 0, NULL)


/*
** Warning-related functions
*/
SDKL_API void (sdkl_setwarnf) (sdkl_State *L, sdkl_WarnFunction f, void *ud);
SDKL_API void (sdkl_warning)  (sdkl_State *L, const char *msg, int tocont);


/*
** garbage-collection function and options
*/

#define SDKL_GCSTOP		0
#define SDKL_GCRESTART		1
#define SDKL_GCCOLLECT		2
#define SDKL_GCCOUNT		3
#define SDKL_GCCOUNTB		4
#define SDKL_GCSTEP		5
#define SDKL_GCSETPAUSE		6
#define SDKL_GCSETSTEPMUL	7
#define SDKL_GCISRUNNING		9
#define SDKL_GCGEN		10
#define SDKL_GCINC		11

SDKL_API int (sdkl_gc) (sdkl_State *L, int what, ...);


/*
** miscellaneous functions
*/

SDKL_API int   (sdkl_error) (sdkl_State *L);

SDKL_API int   (sdkl_next) (sdkl_State *L, int idx);

SDKL_API void  (sdkl_concat) (sdkl_State *L, int n);
SDKL_API void  (sdkl_len)    (sdkl_State *L, int idx);

SDKL_API size_t   (sdkl_stringtonumber) (sdkl_State *L, const char *s);

SDKL_API sdkl_Alloc (sdkl_getallocf) (sdkl_State *L, void **ud);
SDKL_API void      (sdkl_setallocf) (sdkl_State *L, sdkl_Alloc f, void *ud);

SDKL_API void (sdkl_toclose) (sdkl_State *L, int idx);
SDKL_API void (sdkl_closeslot) (sdkl_State *L, int idx);


/*
** {==============================================================
** some useful macros
** ===============================================================
*/

#define sdkl_getextraspace(L)	((void *)((char *)(L) - SDKL_EXTRASPACE))

#define sdkl_tonumber(L,i)	sdkl_tonumberx(L,(i),NULL)
#define sdkl_tointeger(L,i)	sdkl_tointegerx(L,(i),NULL)

#define sdkl_pop(L,n)		sdkl_settop(L, -(n)-1)

#define sdkl_newtable(L)		sdkl_createtable(L, 0, 0)

#define sdkl_register(L,n,f) (sdkl_pushcfunction(L, (f)), sdkl_setglobal(L, (n)))

#define sdkl_pushcfunction(L,f)	sdkl_pushcclosure(L, (f), 0)

#define sdkl_isfunction(L,n)	(sdkl_type(L, (n)) == SDKL_TFUNCTION)
#define sdkl_istable(L,n)	(sdkl_type(L, (n)) == SDKL_TTABLE)
#define sdkl_islightuserdata(L,n)	(sdkl_type(L, (n)) == SDKL_TLIGHTUSERDATA)
#define sdkl_isnil(L,n)		(sdkl_type(L, (n)) == SDKL_TNIL)
#define sdkl_isboolean(L,n)	(sdkl_type(L, (n)) == SDKL_TBOOLEAN)
#define sdkl_isthread(L,n)	(sdkl_type(L, (n)) == SDKL_TTHREAD)
#define sdkl_isnone(L,n)		(sdkl_type(L, (n)) == SDKL_TNONE)
#define sdkl_isnoneornil(L, n)	(sdkl_type(L, (n)) <= 0)

#define sdkl_pushliteral(L, s)	sdkl_pushstring(L, "" s)

#define sdkl_pushglobaltable(L)  \
	((void)sdkl_rawgeti(L, SDKL_REGISTRYINDEX, SDKL_RIDX_GLOBALS))

#define sdkl_tostring(L,i)	sdkl_tolstring(L, (i), NULL)


#define sdkl_insert(L,idx)	sdkl_rotate(L, (idx), 1)

#define sdkl_remove(L,idx)	(sdkl_rotate(L, (idx), -1), sdkl_pop(L, 1))

#define sdkl_replace(L,idx)	(sdkl_copy(L, -1, (idx)), sdkl_pop(L, 1))

/* }============================================================== */


/*
** {==============================================================
** compatibility macros
** ===============================================================
*/
#if defined(SDKL_COMPAT_APIINTCASTS)

#define sdkl_pushunsigned(L,n)	sdkl_pushinteger(L, (sdkl_Integer)(n))
#define sdkl_tounsignedx(L,i,is)	((sdkl_Unsigned)sdkl_tointegerx(L,i,is))
#define sdkl_tounsigned(L,i)	sdkl_tounsignedx(L,(i),NULL)

#endif

#define sdkl_newuserdata(L,s)	sdkl_newuserdatauv(L,s,1)
#define sdkl_getuservalue(L,idx)	sdkl_getiuservalue(L,idx,1)
#define sdkl_setuservalue(L,idx)	sdkl_setiuservalue(L,idx,1)

#define SDKL_NUMTAGS		SDKL_NUMTYPES

/* }============================================================== */

/*
** {======================================================================
** Debug API
** =======================================================================
*/


/*
** Event codes
*/
#define SDKL_HOOKCALL	0
#define SDKL_HOOKRET	1
#define SDKL_HOOKLINE	2
#define SDKL_HOOKCOUNT	3
#define SDKL_HOOKTAILCALL 4


/*
** Event masks
*/
#define SDKL_MASKCALL	(1 << SDKL_HOOKCALL)
#define SDKL_MASKRET	(1 << SDKL_HOOKRET)
#define SDKL_MASKLINE	(1 << SDKL_HOOKLINE)
#define SDKL_MASKCOUNT	(1 << SDKL_HOOKCOUNT)

typedef struct sdkl_Debug sdkl_Debug;  /* activation record */


/* Functions to be called by the debugger in specific events */
typedef void (*sdkl_Hook) (sdkl_State *L, sdkl_Debug *ar);


SDKL_API int (sdkl_getstack) (sdkl_State *L, int level, sdkl_Debug *ar);
SDKL_API int (sdkl_getinfo) (sdkl_State *L, const char *what, sdkl_Debug *ar);
SDKL_API const char *(sdkl_getlocal) (sdkl_State *L, const sdkl_Debug *ar, int n);
SDKL_API const char *(sdkl_setlocal) (sdkl_State *L, const sdkl_Debug *ar, int n);
SDKL_API const char *(sdkl_getupvalue) (sdkl_State *L, int funcindex, int n);
SDKL_API const char *(sdkl_setupvalue) (sdkl_State *L, int funcindex, int n);

SDKL_API void *(sdkl_upvalueid) (sdkl_State *L, int fidx, int n);
SDKL_API void  (sdkl_upvaluejoin) (sdkl_State *L, int fidx1, int n1,
                                               int fidx2, int n2);

SDKL_API void (sdkl_sethook) (sdkl_State *L, sdkl_Hook func, int mask, int count);
SDKL_API sdkl_Hook (sdkl_gethook) (sdkl_State *L);
SDKL_API int (sdkl_gethookmask) (sdkl_State *L);
SDKL_API int (sdkl_gethookcount) (sdkl_State *L);

SDKL_API int (sdkl_setcstacklimit) (sdkl_State *L, unsigned int limit);

struct sdkl_Debug {
  int event;
  const char *name;	/* (n) */
  const char *namewhat;	/* (n) 'global', 'local', 'field', 'method' */
  const char *what;	/* (S) 'SDKL', 'C', 'main', 'tail' */
  const char *source;	/* (S) */
  size_t srclen;	/* (S) */
  int currentline;	/* (l) */
  int linedefined;	/* (S) */
  int lastlinedefined;	/* (S) */
  unsigned char nups;	/* (u) number of upvalues */
  unsigned char nparams;/* (u) number of parameters */
  char isvararg;        /* (u) */
  char istailcall;	/* (t) */
  unsigned short ftransfer;   /* (r) index of first value transferred */
  unsigned short ntransfer;   /* (r) number of transferred values */
  char short_src[SDKL_IDSIZE]; /* (S) */
  /* private part */
  struct CallInfo *i_ci;  /* active function */
};

/* }====================================================================== */


/******************************************************************************
* Copyright (C) 1994-2021 SDKL.org, PUC-Rio.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/


#endif
