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


#define LUA_VERSION_MAJOR	"2"
#define LUA_VERSION_MINOR	"8"
#define LUA_VERSION_RELEASE	"9"

#define LUA_VERSION_NUM			504
#define LUA_VERSION_RELEASE_NUM		(LUA_VERSION_NUM * 100 + 0)

#define LUA_VERSION	"SDKL " LUA_VERSION_MAJOR "." LUA_VERSION_MINOR
#define LUA_RELEASE	LUA_VERSION "." LUA_VERSION_RELEASE
#define LUA_COPYRIGHT	LUA_RELEASE "  Copyright (C) 2021 Kai Gonzalez & Naruto_The9tailedFox#8496"
#define LUA_AUTHORS	"Kai D. Gonzalez"


/* mark for precompiled code ('<esc>Lua') */
#define LUA_SIGNATURE	"\x1bLua"

/* option for multiple returns in 'sdkl_pcall' and 'sdkl_call' */
#define LUA_MULTRET	(-1)


/*
** Pseudo-indices
** (-LUAI_MAXSTACK is the minimum valid index; we keep some free empty
** space after that to help overflow detection)
*/
#define LUA_REGISTRYINDEX	(-LUAI_MAXSTACK - 1000)
#define sdkl_upvalueindex(i)	(LUA_REGISTRYINDEX - (i))


/* thread status */
#define LUA_OK		0
#define LUA_YIELD	1
#define LUA_ERRRUN	2
#define LUA_ERRSYNTAX	3
#define LUA_ERRMEM	4
#define LUA_ERRERR	5


typedef struct sdkl_State sdkl_State;


/*
** basic types
*/
#define LUA_TNONE		(-1)

#define LUA_TNIL		0
#define LUA_TBOOLEAN		1
#define LUA_TLIGHTUSERDATA	2
#define LUA_TNUMBER		3
#define LUA_TSTRING		4
#define LUA_TTABLE		5
#define LUA_TFUNCTION		6
#define LUA_TUSERDATA		7
#define LUA_TTHREAD		8

#define LUA_NUMTYPES		9



/* minimum Lua stack available to a C function */
#define LUA_MINSTACK	20


/* predefined values in the registry */
#define LUA_RIDX_MAINTHREAD	1
#define LUA_RIDX_GLOBALS	2
#define LUA_RIDX_LAST		LUA_RIDX_GLOBALS


/* type of numbers in Lua */
typedef LUA_NUMBER sdkl_Number;


/* type for integer functions */
typedef LUA_INTEGER sdkl_Integer;

/* unsigned integer type */
typedef LUA_UNSIGNED sdkl_Unsigned;

/* type for continuation-function contexts */
typedef LUA_KCONTEXT sdkl_KContext;


/*
** Type for C functions registered with Lua
*/
typedef int (*sdkl_CFunction) (sdkl_State *L);

/*
** Type for continuation functions
*/
typedef int (*sdkl_KFunction) (sdkl_State *L, int status, sdkl_KContext ctx);


/*
** Type for functions that read/write blocks when loading/dumping Lua chunks
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
#if defined(LUA_USER_H)
#include LUA_USER_H
#endif


/*
** RCS ident string
*/
extern const char sdkl_ident[];


/*
** state manipulation
*/
LUA_API sdkl_State *(sdkl_newstate) (sdkl_Alloc f, void *ud);
LUA_API void       (sdkl_close) (sdkl_State *L);
LUA_API sdkl_State *(sdkl_newthread) (sdkl_State *L);
LUA_API int        (sdkl_resetthread) (sdkl_State *L);

LUA_API sdkl_CFunction (sdkl_atpanic) (sdkl_State *L, sdkl_CFunction panicf);


LUA_API sdkl_Number (sdkl_version) (sdkl_State *L);


/*
** basic stack manipulation
*/
LUA_API int   (sdkl_absindex) (sdkl_State *L, int idx);
LUA_API int   (sdkl_gettop) (sdkl_State *L);
LUA_API void  (sdkl_settop) (sdkl_State *L, int idx);
LUA_API void  (sdkl_pushvalue) (sdkl_State *L, int idx);
LUA_API void  (sdkl_rotate) (sdkl_State *L, int idx, int n);
LUA_API void  (sdkl_copy) (sdkl_State *L, int fromidx, int toidx);
LUA_API int   (sdkl_checkstack) (sdkl_State *L, int n);

LUA_API void  (sdkl_xmove) (sdkl_State *from, sdkl_State *to, int n);


/*
** access functions (stack -> C)
*/

LUA_API int             (sdkl_isnumber) (sdkl_State *L, int idx);
LUA_API int             (sdkl_isstring) (sdkl_State *L, int idx);
LUA_API int             (sdkl_iscfunction) (sdkl_State *L, int idx);
LUA_API int             (sdkl_isinteger) (sdkl_State *L, int idx);
LUA_API int             (sdkl_isuserdata) (sdkl_State *L, int idx);
LUA_API int             (sdkl_type) (sdkl_State *L, int idx);
LUA_API const char     *(sdkl_typename) (sdkl_State *L, int tp);

LUA_API sdkl_Number      (sdkl_tonumberx) (sdkl_State *L, int idx, int *isnum);
LUA_API sdkl_Integer     (sdkl_tointegerx) (sdkl_State *L, int idx, int *isnum);
LUA_API int             (sdkl_toboolean) (sdkl_State *L, int idx);
LUA_API const char     *(sdkl_tolstring) (sdkl_State *L, int idx, size_t *len);
LUA_API sdkl_Unsigned    (sdkl_rawlen) (sdkl_State *L, int idx);
LUA_API sdkl_CFunction   (sdkl_tocfunction) (sdkl_State *L, int idx);
LUA_API void	       *(sdkl_touserdata) (sdkl_State *L, int idx);
LUA_API sdkl_State      *(sdkl_tothread) (sdkl_State *L, int idx);
LUA_API const void     *(sdkl_topointer) (sdkl_State *L, int idx);


/*
** Comparison and arithmetic functions
*/

#define LUA_OPADD	0	/* ORDER TM, ORDER OP */
#define LUA_OPSUB	1
#define LUA_OPMUL	2
#define LUA_OPMOD	3
#define LUA_OPPOW	4
#define LUA_OPDIV	5
#define LUA_OPIDIV	6
#define LUA_OPBAND	7
#define LUA_OPBOR	8
#define LUA_OPBXOR	9
#define LUA_OPSHL	10
#define LUA_OPSHR	11
#define LUA_OPUNM	12
#define LUA_OPBNOT	13

LUA_API void  (sdkl_arith) (sdkl_State *L, int op);

#define LUA_OPEQ	0
#define LUA_OPLT	1
#define LUA_OPLE	2

LUA_API int   (sdkl_rawequal) (sdkl_State *L, int idx1, int idx2);
LUA_API int   (sdkl_compare) (sdkl_State *L, int idx1, int idx2, int op);


/*
** push functions (C -> stack)
*/
LUA_API void        (sdkl_pushnil) (sdkl_State *L);
LUA_API void        (sdkl_pushnumber) (sdkl_State *L, sdkl_Number n);
LUA_API void        (sdkl_pushinteger) (sdkl_State *L, sdkl_Integer n);
LUA_API const char *(sdkl_pushlstring) (sdkl_State *L, const char *s, size_t len);
LUA_API const char *(sdkl_pushstring) (sdkl_State *L, const char *s);
LUA_API const char *(sdkl_pushvfstring) (sdkl_State *L, const char *fmt,
                                                      va_list argp);
LUA_API const char *(sdkl_pushfstring) (sdkl_State *L, const char *fmt, ...);
LUA_API void  (sdkl_pushcclosure) (sdkl_State *L, sdkl_CFunction fn, int n);
LUA_API void  (sdkl_pushboolean) (sdkl_State *L, int b);
LUA_API void  (sdkl_pushlightuserdata) (sdkl_State *L, void *p);
LUA_API int   (sdkl_pushthread) (sdkl_State *L);


/*
** get functions (Lua -> stack)
*/
LUA_API int (sdkl_getglobal) (sdkl_State *L, const char *name);
LUA_API int (sdkl_gettable) (sdkl_State *L, int idx);
LUA_API int (sdkl_getfield) (sdkl_State *L, int idx, const char *k);
LUA_API int (sdkl_geti) (sdkl_State *L, int idx, sdkl_Integer n);
LUA_API int (sdkl_rawget) (sdkl_State *L, int idx);
LUA_API int (sdkl_rawgeti) (sdkl_State *L, int idx, sdkl_Integer n);
LUA_API int (sdkl_rawgetp) (sdkl_State *L, int idx, const void *p);

LUA_API void  (sdkl_createtable) (sdkl_State *L, int narr, int nrec);
LUA_API void *(sdkl_newuserdatauv) (sdkl_State *L, size_t sz, int nuvalue);
LUA_API int   (sdkl_getmetatable) (sdkl_State *L, int objindex);
LUA_API int  (sdkl_getiuservalue) (sdkl_State *L, int idx, int n);


/*
** set functions (stack -> Lua)
*/
LUA_API void  (sdkl_setglobal) (sdkl_State *L, const char *name);
LUA_API void  (sdkl_settable) (sdkl_State *L, int idx);
LUA_API void  (sdkl_setfield) (sdkl_State *L, int idx, const char *k);
LUA_API void  (sdkl_seti) (sdkl_State *L, int idx, sdkl_Integer n);
LUA_API void  (sdkl_rawset) (sdkl_State *L, int idx);
LUA_API void  (sdkl_rawseti) (sdkl_State *L, int idx, sdkl_Integer n);
LUA_API void  (sdkl_rawsetp) (sdkl_State *L, int idx, const void *p);
LUA_API int   (sdkl_setmetatable) (sdkl_State *L, int objindex);
LUA_API int   (sdkl_setiuservalue) (sdkl_State *L, int idx, int n);


/*
** 'load' and 'call' functions (load and run Lua code)
*/
LUA_API void  (sdkl_callk) (sdkl_State *L, int nargs, int nresults,
                           sdkl_KContext ctx, sdkl_KFunction k);
#define sdkl_call(L,n,r)		sdkl_callk(L, (n), (r), 0, NULL)

LUA_API int   (sdkl_pcallk) (sdkl_State *L, int nargs, int nresults, int errfunc,
                            sdkl_KContext ctx, sdkl_KFunction k);
#define sdkl_pcall(L,n,r,f)	sdkl_pcallk(L, (n), (r), (f), 0, NULL)

LUA_API int   (sdkl_load) (sdkl_State *L, sdkl_Reader reader, void *dt,
                          const char *chunkname, const char *mode);

LUA_API int (sdkl_dump) (sdkl_State *L, sdkl_Writer writer, void *data, int strip);


/*
** coroutine functions
*/
LUA_API int  (sdkl_yieldk)     (sdkl_State *L, int nresults, sdkl_KContext ctx,
                               sdkl_KFunction k);
LUA_API int  (sdkl_resume)     (sdkl_State *L, sdkl_State *from, int narg,
                               int *nres);
LUA_API int  (sdkl_status)     (sdkl_State *L);
LUA_API int (sdkl_isyieldable) (sdkl_State *L);

#define sdkl_yield(L,n)		sdkl_yieldk(L, (n), 0, NULL)


/*
** Warning-related functions
*/
LUA_API void (sdkl_setwarnf) (sdkl_State *L, sdkl_WarnFunction f, void *ud);
LUA_API void (sdkl_warning)  (sdkl_State *L, const char *msg, int tocont);


/*
** garbage-collection function and options
*/

#define LUA_GCSTOP		0
#define LUA_GCRESTART		1
#define LUA_GCCOLLECT		2
#define LUA_GCCOUNT		3
#define LUA_GCCOUNTB		4
#define LUA_GCSTEP		5
#define LUA_GCSETPAUSE		6
#define LUA_GCSETSTEPMUL	7
#define LUA_GCISRUNNING		9
#define LUA_GCGEN		10
#define LUA_GCINC		11

LUA_API int (sdkl_gc) (sdkl_State *L, int what, ...);


/*
** miscellaneous functions
*/

LUA_API int   (sdkl_error) (sdkl_State *L);

LUA_API int   (sdkl_next) (sdkl_State *L, int idx);

LUA_API void  (sdkl_concat) (sdkl_State *L, int n);
LUA_API void  (sdkl_len)    (sdkl_State *L, int idx);

LUA_API size_t   (sdkl_stringtonumber) (sdkl_State *L, const char *s);

LUA_API sdkl_Alloc (sdkl_getallocf) (sdkl_State *L, void **ud);
LUA_API void      (sdkl_setallocf) (sdkl_State *L, sdkl_Alloc f, void *ud);

LUA_API void (sdkl_toclose) (sdkl_State *L, int idx);
LUA_API void (sdkl_closeslot) (sdkl_State *L, int idx);


/*
** {==============================================================
** some useful macros
** ===============================================================
*/

#define sdkl_getextraspace(L)	((void *)((char *)(L) - LUA_EXTRASPACE))

#define sdkl_tonumber(L,i)	sdkl_tonumberx(L,(i),NULL)
#define sdkl_tointeger(L,i)	sdkl_tointegerx(L,(i),NULL)

#define sdkl_pop(L,n)		sdkl_settop(L, -(n)-1)

#define sdkl_newtable(L)		sdkl_createtable(L, 0, 0)

#define sdkl_register(L,n,f) (sdkl_pushcfunction(L, (f)), sdkl_setglobal(L, (n)))

#define sdkl_pushcfunction(L,f)	sdkl_pushcclosure(L, (f), 0)

#define sdkl_isfunction(L,n)	(sdkl_type(L, (n)) == LUA_TFUNCTION)
#define sdkl_istable(L,n)	(sdkl_type(L, (n)) == LUA_TTABLE)
#define sdkl_islightuserdata(L,n)	(sdkl_type(L, (n)) == LUA_TLIGHTUSERDATA)
#define sdkl_isnil(L,n)		(sdkl_type(L, (n)) == LUA_TNIL)
#define sdkl_isboolean(L,n)	(sdkl_type(L, (n)) == LUA_TBOOLEAN)
#define sdkl_isthread(L,n)	(sdkl_type(L, (n)) == LUA_TTHREAD)
#define sdkl_isnone(L,n)		(sdkl_type(L, (n)) == LUA_TNONE)
#define sdkl_isnoneornil(L, n)	(sdkl_type(L, (n)) <= 0)

#define sdkl_pushliteral(L, s)	sdkl_pushstring(L, "" s)

#define sdkl_pushglobaltable(L)  \
	((void)sdkl_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS))

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
#if defined(LUA_COMPAT_APIINTCASTS)

#define sdkl_pushunsigned(L,n)	sdkl_pushinteger(L, (sdkl_Integer)(n))
#define sdkl_tounsignedx(L,i,is)	((sdkl_Unsigned)sdkl_tointegerx(L,i,is))
#define sdkl_tounsigned(L,i)	sdkl_tounsignedx(L,(i),NULL)

#endif

#define sdkl_newuserdata(L,s)	sdkl_newuserdatauv(L,s,1)
#define sdkl_getuservalue(L,idx)	sdkl_getiuservalue(L,idx,1)
#define sdkl_setuservalue(L,idx)	sdkl_setiuservalue(L,idx,1)

#define LUA_NUMTAGS		LUA_NUMTYPES

/* }============================================================== */

/*
** {======================================================================
** Debug API
** =======================================================================
*/


/*
** Event codes
*/
#define LUA_HOOKCALL	0
#define LUA_HOOKRET	1
#define LUA_HOOKLINE	2
#define LUA_HOOKCOUNT	3
#define LUA_HOOKTAILCALL 4


/*
** Event masks
*/
#define LUA_MASKCALL	(1 << LUA_HOOKCALL)
#define LUA_MASKRET	(1 << LUA_HOOKRET)
#define LUA_MASKLINE	(1 << LUA_HOOKLINE)
#define LUA_MASKCOUNT	(1 << LUA_HOOKCOUNT)

typedef struct sdkl_Debug sdkl_Debug;  /* activation record */


/* Functions to be called by the debugger in specific events */
typedef void (*sdkl_Hook) (sdkl_State *L, sdkl_Debug *ar);


LUA_API int (sdkl_getstack) (sdkl_State *L, int level, sdkl_Debug *ar);
LUA_API int (sdkl_getinfo) (sdkl_State *L, const char *what, sdkl_Debug *ar);
LUA_API const char *(sdkl_getlocal) (sdkl_State *L, const sdkl_Debug *ar, int n);
LUA_API const char *(sdkl_setlocal) (sdkl_State *L, const sdkl_Debug *ar, int n);
LUA_API const char *(sdkl_getupvalue) (sdkl_State *L, int funcindex, int n);
LUA_API const char *(sdkl_setupvalue) (sdkl_State *L, int funcindex, int n);

LUA_API void *(sdkl_upvalueid) (sdkl_State *L, int fidx, int n);
LUA_API void  (sdkl_upvaluejoin) (sdkl_State *L, int fidx1, int n1,
                                               int fidx2, int n2);

LUA_API void (sdkl_sethook) (sdkl_State *L, sdkl_Hook func, int mask, int count);
LUA_API sdkl_Hook (sdkl_gethook) (sdkl_State *L);
LUA_API int (sdkl_gethookmask) (sdkl_State *L);
LUA_API int (sdkl_gethookcount) (sdkl_State *L);

LUA_API int (sdkl_setcstacklimit) (sdkl_State *L, unsigned int limit);

struct sdkl_Debug {
  int event;
  const char *name;	/* (n) */
  const char *namewhat;	/* (n) 'global', 'local', 'field', 'method' */
  const char *what;	/* (S) 'Lua', 'C', 'main', 'tail' */
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
  char short_src[LUA_IDSIZE]; /* (S) */
  /* private part */
  struct CallInfo *i_ci;  /* active function */
};

/* }====================================================================== */


/******************************************************************************
* Copyright (C) 1994-2021 Lua.org, PUC-Rio.
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
