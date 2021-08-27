/*
** $Id: lauxlib.c $
** Auxiliary functions for building SDKL libraries
** See Copyright Notice in sdkl.h
*/

#define lauxlib_c
#define LUA_LIB

#include "lprefix.h"


#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
** This file uses only the official API of SDKL.
** Any function declared here could be written as an application function.
*/

#include "sdkl.h"

#include "sdklauxillary.h"


#if !defined(MAX_SIZET)
/* maximum value for size_t */
#define MAX_SIZET	((size_t)(~(size_t)0))
#endif


/*
** {======================================================
** Traceback
** =======================================================
*/


#define LEVELS1	10	/* size of the first part of the stack */
#define LEVELS2	11	/* size of the second part of the stack */



/*
** Search for 'objidx' in table at index -1. ('objidx' must be an
** absolute index.) Return 1 + string at top if it found a good name.
*/
static int findfield (sdkl_State *L, int objidx, int level) {
  if (level == 0 || !sdkl_istable(L, -1))
    return 0;  /* not found */
  sdkl_pushnil(L);  /* start 'next' loop */
  while (sdkl_next(L, -2)) {  /* for each pair in table */
    if (sdkl_type(L, -2) == LUA_TSTRING) {  /* ignore non-string keys */
      if (sdkl_rawequal(L, objidx, -1)) {  /* found object? */
        sdkl_pop(L, 1);  /* remove value (but keep name) */
        return 1;
      }
      else if (findfield(L, objidx, level - 1)) {  /* try recursively */
        /* stack: lib_name, lib_table, field_name (top) */
        sdkl_pushliteral(L, ".");  /* place '.' between the two names */
        sdkl_replace(L, -3);  /* (in the slot occupied by table) */
        sdkl_concat(L, 3);  /* lib_name.field_name */
        return 1;
      }
    }
    sdkl_pop(L, 1);  /* remove value */
  }
  return 0;  /* not found */
}


/*
** Search for a name for a function in all loaded modules
*/
static int pushglobalfuncname (sdkl_State *L, sdkl_Debug *ar) {
  int top = sdkl_gettop(L);
  sdkl_getinfo(L, "f", ar);  /* push function */
  sdkl_getfield(L, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
  if (findfield(L, top + 1, 2)) {
    const char *name = sdkl_tostring(L, -1);
    if (strncmp(name, LUA_GNAME ".", 3) == 0) {  /* name start with '_G.'? */
      sdkl_pushstring(L, name + 3);  /* push name without prefix */
      sdkl_remove(L, -2);  /* remove original name */
    }
    sdkl_copy(L, -1, top + 1);  /* copy name to proper place */
    sdkl_settop(L, top + 1);  /* remove table "loaded" and name copy */
    return 1;
  }
  else {
    sdkl_settop(L, top);  /* remove function and global table */
    return 0;
  }
}


static void pushfuncname (sdkl_State *L, sdkl_Debug *ar) {
  if (pushglobalfuncname(L, ar)) {  /* try first a global name */
    sdkl_pushfstring(L, "function '%s'", sdkl_tostring(L, -1));
    sdkl_remove(L, -2);  /* remove name */
  }
  else if (*ar->namewhat != '\0')  /* is there a name from code? */
    sdkl_pushfstring(L, "%s '%s'", ar->namewhat, ar->name);  /* use it */
  else if (*ar->what == 'm')  /* main? */
      sdkl_pushliteral(L, "main chunk");
  else if (*ar->what != 'C')  /* for SDKL functions, use <file:line> */
    sdkl_pushfstring(L, "function <%s:%d>", ar->short_src, ar->linedefined);
  else  /* nothing left... */
    sdkl_pushliteral(L, "?");
}


static int lastlevel (sdkl_State *L) {
  sdkl_Debug ar;
  int li = 1, le = 1;
  /* find an upper bound */
  while (sdkl_getstack(L, le, &ar)) { li = le; le *= 2; }
  /* do a binary search */
  while (li < le) {
    int m = (li + le)/2;
    if (sdkl_getstack(L, m, &ar)) li = m + 1;
    else le = m;
  }
  return le - 1;
}


LUALIB_API void sdklL_traceback (sdkl_State *L, sdkl_State *L1,
                                const char *msg, int level) {
  sdklL_Buffer b;
  sdkl_Debug ar;
  int last = lastlevel(L1);
  int limit2show = (last - level > LEVELS1 + LEVELS2) ? LEVELS1 : -1;
  sdklL_buffinit(L, &b);
  if (msg) {
    sdklL_addstring(&b, msg);
    sdklL_addchar(&b, '\n');
  }
  sdklL_addstring(&b, "stack traceback:");
  while (sdkl_getstack(L1, level++, &ar)) {
    if (limit2show-- == 0) {  /* too many levels? */
      int n = last - level - LEVELS2 + 1;  /* number of levels to skip */
      sdkl_pushfstring(L, "\n\t...\t(skipping %d levels)", n);
      sdklL_addvalue(&b);  /* add warning about skip */
      level += n;  /* and skip to last levels */
    }
    else {
      sdkl_getinfo(L1, "Slnt", &ar);
      if (ar.currentline <= 0)
        sdkl_pushfstring(L, "\n\t%s: in ", ar.short_src);
      else
        sdkl_pushfstring(L, "\n\t%s:%d: in ", ar.short_src, ar.currentline);
      sdklL_addvalue(&b);
      pushfuncname(L, &ar);
      sdklL_addvalue(&b);
      if (ar.istailcall)
        sdklL_addstring(&b, "\n\t(...tail calls...)");
    }
  }
  sdklL_pushresult(&b);
}

/* }====================================================== */


/*
** {======================================================
** Error-report functions
** =======================================================
*/

LUALIB_API int sdklL_argerror (sdkl_State *L, int arg, const char *extramsg) {
  sdkl_Debug ar;
  if (!sdkl_getstack(L, 0, &ar))  /* no stack frame? */
    return sdklL_error(L, "bad argument #%d (%s)", arg, extramsg);
  sdkl_getinfo(L, "n", &ar);
  if (strcmp(ar.namewhat, "method") == 0) {
    arg--;  /* do not count 'self' */
    if (arg == 0)  /* error is in the self argument itself? */
      return sdklL_error(L, "calling '%s' on bad self (%s)",
                           ar.name, extramsg);
  }
  if (ar.name == NULL)
    ar.name = (pushglobalfuncname(L, &ar)) ? sdkl_tostring(L, -1) : "?";
  return sdklL_error(L, "bad argument #%d to '%s' (%s)",
                        arg, ar.name, extramsg);
}


LUALIB_API int sdklL_typeerror (sdkl_State *L, int arg, const char *tname) {
  const char *msg;
  const char *typearg;  /* name for the type of the actual argument */
  if (sdklL_getmetafield(L, arg, "__name") == LUA_TSTRING)
    typearg = sdkl_tostring(L, -1);  /* use the given type name */
  else if (sdkl_type(L, arg) == LUA_TLIGHTUSERDATA)
    typearg = "light userdata";  /* special name for messages */
  else
    typearg = sdklL_typename(L, arg);  /* standard name */
  msg = sdkl_pushfstring(L, "%s expected, got %s", tname, typearg);
  return sdklL_argerror(L, arg, msg);
}


static void tag_error (sdkl_State *L, int arg, int tag) {
  sdklL_typeerror(L, arg, sdkl_typename(L, tag));
}


/*
** The use of 'sdkl_pushfstring' ensures this function does not
** need reserved stack space when called.
*/
LUALIB_API void sdklL_where (sdkl_State *L, int level) {
  sdkl_Debug ar;
  if (sdkl_getstack(L, level, &ar)) {  /* check function at level */
    sdkl_getinfo(L, "Sl", &ar);  /* get info about it */
    if (ar.currentline > 0) {  /* is there info? */
      sdkl_pushfstring(L, "%s:%d: ", ar.short_src, ar.currentline);
      return;
    }
  }
  sdkl_pushfstring(L, "");  /* else, no information available... */
}


/*
** Again, the use of 'sdkl_pushvfstring' ensures this function does
** not need reserved stack space when called. (At worst, it generates
** an error with "stack overflow" instead of the given message.)
*/
LUALIB_API int sdklL_error (sdkl_State *L, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  sdklL_where(L, 1);
  sdkl_pushvfstring(L, fmt, argp);
  va_end(argp);
  sdkl_concat(L, 2);
  return sdkl_error(L);
}


LUALIB_API int sdklL_fileresult (sdkl_State *L, int stat, const char *fname) {
  int en = errno;  /* calls to SDKL API may change this value */
  if (stat) {
    sdkl_pushboolean(L, 1);
    return 1;
  }
  else {
    sdklL_pushfail(L);
    if (fname)
      sdkl_pushfstring(L, "%s: %s", fname, strerror(en));
    else
      sdkl_pushstring(L, strerror(en));
    sdkl_pushinteger(L, en);
    return 3;
  }
}


#if !defined(l_inspectstat)	/* { */

#if defined(LUA_USE_POSIX)

#include <sys/wait.h>

/*
** use appropriate macros to interpret 'pclose' return status
*/
#define l_inspectstat(stat,what)  \
   if (WIFEXITED(stat)) { stat = WEXITSTATUS(stat); } \
   else if (WIFSIGNALED(stat)) { stat = WTERMSIG(stat); what = "signal"; }

#else

#define l_inspectstat(stat,what)  /* no op */

#endif

#endif				/* } */


LUALIB_API int sdklL_execresult (sdkl_State *L, int stat) {
  if (stat != 0 && errno != 0)  /* error with an 'errno'? */
    return sdklL_fileresult(L, 0, NULL);
  else {
    const char *what = "exit";  /* type of termination */
    l_inspectstat(stat, what);  /* interpret result */
    if (*what == 'e' && stat == 0)  /* successful termination? */
      sdkl_pushboolean(L, 1);
    else
      sdklL_pushfail(L);
    sdkl_pushstring(L, what);
    sdkl_pushinteger(L, stat);
    return 3;  /* return true/fail,what,code */
  }
}

/* }====================================================== */



/*
** {======================================================
** Userdata's metatable manipulation
** =======================================================
*/

LUALIB_API int sdklL_newmetatable (sdkl_State *L, const char *tname) {
  if (sdklL_getmetatable(L, tname) != LUA_TNIL)  /* name already in use? */
    return 0;  /* leave previous value on top, but return 0 */
  sdkl_pop(L, 1);
  sdkl_createtable(L, 0, 2);  /* create metatable */
  sdkl_pushstring(L, tname);
  sdkl_setfield(L, -2, "__name");  /* metatable.__name = tname */
  sdkl_pushvalue(L, -1);
  sdkl_setfield(L, LUA_REGISTRYINDEX, tname);  /* registry.name = metatable */
  return 1;
}


LUALIB_API void sdklL_setmetatable (sdkl_State *L, const char *tname) {
  sdklL_getmetatable(L, tname);
  sdkl_setmetatable(L, -2);
}


LUALIB_API void *sdklL_testudata (sdkl_State *L, int ud, const char *tname) {
  void *p = sdkl_touserdata(L, ud);
  if (p != NULL) {  /* value is a userdata? */
    if (sdkl_getmetatable(L, ud)) {  /* does it have a metatable? */
      sdklL_getmetatable(L, tname);  /* get correct metatable */
      if (!sdkl_rawequal(L, -1, -2))  /* not the same? */
        p = NULL;  /* value is a userdata with wrong metatable */
      sdkl_pop(L, 2);  /* remove both metatables */
      return p;
    }
  }
  return NULL;  /* value is not a userdata with a metatable */
}


LUALIB_API void *sdklL_checkudata (sdkl_State *L, int ud, const char *tname) {
  void *p = sdklL_testudata(L, ud, tname);
  sdklL_argexpected(L, p != NULL, ud, tname);
  return p;
}

/* }====================================================== */


/*
** {======================================================
** Argument check functions
** =======================================================
*/

LUALIB_API int sdklL_checkoption (sdkl_State *L, int arg, const char *def,
                                 const char *const lst[]) {
  const char *name = (def) ? sdklL_optstring(L, arg, def) :
                             sdklL_checkstring(L, arg);
  int i;
  for (i=0; lst[i]; i++)
    if (strcmp(lst[i], name) == 0)
      return i;
  return sdklL_argerror(L, arg,
                       sdkl_pushfstring(L, "invalid option '%s'", name));
}


/*
** Ensures the stack has at least 'space' extra slots, raising an error
** if it cannot fulfill the request. (The error handling needs a few
** extra slots to format the error message. In case of an error without
** this extra space, SDKL will generate the same 'stack overflow' error,
** but without 'msg'.)
*/
LUALIB_API void sdklL_checkstack (sdkl_State *L, int space, const char *msg) {
  if (l_unlikely(!sdkl_checkstack(L, space))) {
    if (msg)
      sdklL_error(L, "stack overflow (%s)", msg);
    else
      sdklL_error(L, "stack overflow");
  }
}


LUALIB_API void sdklL_checktype (sdkl_State *L, int arg, int t) {
  if (l_unlikely(sdkl_type(L, arg) != t))
    tag_error(L, arg, t);
}


LUALIB_API void sdklL_checkany (sdkl_State *L, int arg) {
  if (l_unlikely(sdkl_type(L, arg) == LUA_TNONE))
    sdklL_argerror(L, arg, "value expected");
}


LUALIB_API const char *sdklL_checklstring (sdkl_State *L, int arg, size_t *len) {
  const char *s = sdkl_tolstring(L, arg, len);
  if (l_unlikely(!s)) tag_error(L, arg, LUA_TSTRING);
  return s;
}


LUALIB_API const char *sdklL_optlstring (sdkl_State *L, int arg,
                                        const char *def, size_t *len) {
  if (sdkl_isnoneornil(L, arg)) {
    if (len)
      *len = (def ? strlen(def) : 0);
    return def;
  }
  else return sdklL_checklstring(L, arg, len);
}


LUALIB_API sdkl_Number sdklL_checknumber (sdkl_State *L, int arg) {
  int isnum;
  sdkl_Number d = sdkl_tonumberx(L, arg, &isnum);
  if (l_unlikely(!isnum))
    tag_error(L, arg, LUA_TNUMBER);
  return d;
}


LUALIB_API sdkl_Number sdklL_optnumber (sdkl_State *L, int arg, sdkl_Number def) {
  return sdklL_opt(L, sdklL_checknumber, arg, def);
}


static void interror (sdkl_State *L, int arg) {
  if (sdkl_isnumber(L, arg))
    sdklL_argerror(L, arg, "number has no integer representation");
  else
    tag_error(L, arg, LUA_TNUMBER);
}


LUALIB_API sdkl_Integer sdklL_checkinteger (sdkl_State *L, int arg) {
  int isnum;
  sdkl_Integer d = sdkl_tointegerx(L, arg, &isnum);
  if (l_unlikely(!isnum)) {
    interror(L, arg);
  }
  return d;
}


LUALIB_API sdkl_Integer sdklL_optinteger (sdkl_State *L, int arg,
                                                      sdkl_Integer def) {
  return sdklL_opt(L, sdklL_checkinteger, arg, def);
}

/* }====================================================== */


/*
** {======================================================
** Generic Buffer manipulation
** =======================================================
*/

/* userdata to box arbitrary data */
typedef struct UBox {
  void *box;
  size_t bsize;
} UBox;


static void *resizebox (sdkl_State *L, int idx, size_t newsize) {
  void *ud;
  sdkl_Alloc allocf = sdkl_getallocf(L, &ud);
  UBox *box = (UBox *)sdkl_touserdata(L, idx);
  void *temp = allocf(ud, box->box, box->bsize, newsize);
  if (l_unlikely(temp == NULL && newsize > 0)) {  /* allocation error? */
    sdkl_pushliteral(L, "not enough memory");
    sdkl_error(L);  /* raise a memory error */
  }
  box->box = temp;
  box->bsize = newsize;
  return temp;
}


static int boxgc (sdkl_State *L) {
  resizebox(L, 1, 0);
  return 0;
}


static const sdklL_Reg boxmt[] = {  /* box metamethods */
  {"__gc", boxgc},
  {"__close", boxgc},
  {NULL, NULL}
};


static void newbox (sdkl_State *L) {
  UBox *box = (UBox *)sdkl_newuserdatauv(L, sizeof(UBox), 0);
  box->box = NULL;
  box->bsize = 0;
  if (sdklL_newmetatable(L, "_UBOX*"))  /* creating metatable? */
    sdklL_setfuncs(L, boxmt, 0);  /* set its metamethods */
  sdkl_setmetatable(L, -2);
}


/*
** check whether buffer is using a userdata on the stack as a temporary
** buffer
*/
#define buffonstack(B)	((B)->b != (B)->init.b)


/*
** Whenever buffer is accessed, slot 'idx' must either be a box (which
** cannot be NULL) or it is a placeholder for the buffer.
*/
#define checkbufferlevel(B,idx)  \
  sdkl_assert(buffonstack(B) ? sdkl_touserdata(B->L, idx) != NULL  \
                            : sdkl_touserdata(B->L, idx) == (void*)B)


/*
** Compute new size for buffer 'B', enough to accommodate extra 'sz'
** bytes.
*/
static size_t newbuffsize (sdklL_Buffer *B, size_t sz) {
  size_t newsize = B->size * 2;  /* double buffer size */
  if (l_unlikely(MAX_SIZET - sz < B->n))  /* overflow in (B->n + sz)? */
    return sdklL_error(B->L, "buffer too large");
  if (newsize < B->n + sz)  /* double is not big enough? */
    newsize = B->n + sz;
  return newsize;
}


/*
** Returns a pointer to a free area with at least 'sz' bytes in buffer
** 'B'. 'boxidx' is the relative position in the stack where is the
** buffer's box or its placeholder.
*/
static char *prepbuffsize (sdklL_Buffer *B, size_t sz, int boxidx) {
  checkbufferlevel(B, boxidx);
  if (B->size - B->n >= sz)  /* enough space? */
    return B->b + B->n;
  else {
    sdkl_State *L = B->L;
    char *newbuff;
    size_t newsize = newbuffsize(B, sz);
    /* create larger buffer */
    if (buffonstack(B))  /* buffer already has a box? */
      newbuff = (char *)resizebox(L, boxidx, newsize);  /* resize it */
    else {  /* no box yet */
      sdkl_remove(L, boxidx);  /* remove placeholder */
      newbox(L);  /* create a new box */
      sdkl_insert(L, boxidx);  /* move box to its intended position */
      sdkl_toclose(L, boxidx);
      newbuff = (char *)resizebox(L, boxidx, newsize);
      memcpy(newbuff, B->b, B->n * sizeof(char));  /* copy original content */
    }
    B->b = newbuff;
    B->size = newsize;
    return newbuff + B->n;
  }
}

/*
** returns a pointer to a free area with at least 'sz' bytes
*/
LUALIB_API char *sdklL_prepbuffsize (sdklL_Buffer *B, size_t sz) {
  return prepbuffsize(B, sz, -1);
}


LUALIB_API void sdklL_addlstring (sdklL_Buffer *B, const char *s, size_t l) {
  if (l > 0) {  /* avoid 'memcpy' when 's' can be NULL */
    char *b = prepbuffsize(B, l, -1);
    memcpy(b, s, l * sizeof(char));
    sdklL_addsize(B, l);
  }
}


LUALIB_API void sdklL_addstring (sdklL_Buffer *B, const char *s) {
  sdklL_addlstring(B, s, strlen(s));
}


LUALIB_API void sdklL_pushresult (sdklL_Buffer *B) {
  sdkl_State *L = B->L;
  checkbufferlevel(B, -1);
  sdkl_pushlstring(L, B->b, B->n);
  if (buffonstack(B))
    sdkl_closeslot(L, -2);  /* close the box */
  sdkl_remove(L, -2);  /* remove box or placeholder from the stack */
}


LUALIB_API void sdklL_pushresultsize (sdklL_Buffer *B, size_t sz) {
  sdklL_addsize(B, sz);
  sdklL_pushresult(B);
}


/*
** 'sdklL_addvalue' is the only function in the Buffer system where the
** box (if existent) is not on the top of the stack. So, instead of
** calling 'sdklL_addlstring', it replicates the code using -2 as the
** last argument to 'prepbuffsize', signaling that the box is (or will
** be) bellow the string being added to the buffer. (Box creation can
** trigger an emergency GC, so we should not remove the string from the
** stack before we have the space guaranteed.)
*/
LUALIB_API void sdklL_addvalue (sdklL_Buffer *B) {
  sdkl_State *L = B->L;
  size_t len;
  const char *s = sdkl_tolstring(L, -1, &len);
  char *b = prepbuffsize(B, len, -2);
  memcpy(b, s, len * sizeof(char));
  sdklL_addsize(B, len);
  sdkl_pop(L, 1);  /* pop string */
}


LUALIB_API void sdklL_buffinit (sdkl_State *L, sdklL_Buffer *B) {
  B->L = L;
  B->b = B->init.b;
  B->n = 0;
  B->size = LUAL_BUFFERSIZE;
  sdkl_pushlightuserdata(L, (void*)B);  /* push placeholder */
}


LUALIB_API char *sdklL_buffinitsize (sdkl_State *L, sdklL_Buffer *B, size_t sz) {
  sdklL_buffinit(L, B);
  return prepbuffsize(B, sz, -1);
}

/* }====================================================== */


/*
** {======================================================
** Reference system
** =======================================================
*/

/* index of free-list header (after the predefined values) */
#define freelist	(LUA_RIDX_LAST + 1)

/*
** The previously freed references form a linked list:
** t[freelist] is the index of a first free index, or zero if list is
** empty; t[t[freelist]] is the index of the second element; etc.
*/
LUALIB_API int sdklL_ref (sdkl_State *L, int t) {
  int ref;
  if (sdkl_isnil(L, -1)) {
    sdkl_pop(L, 1);  /* remove from stack */
    return LUA_REFNIL;  /* 'nil' has a unique fixed reference */
  }
  t = sdkl_absindex(L, t);
  if (sdkl_rawgeti(L, t, freelist) == LUA_TNIL) {  /* first access? */
    ref = 0;  /* list is empty */
    sdkl_pushinteger(L, 0);  /* initialize as an empty list */
    sdkl_rawseti(L, t, freelist);  /* ref = t[freelist] = 0 */
  }
  else {  /* already initialized */
    sdkl_assert(sdkl_isinteger(L, -1));
    ref = (int)sdkl_tointeger(L, -1);  /* ref = t[freelist] */
  }
  sdkl_pop(L, 1);  /* remove element from stack */
  if (ref != 0) {  /* any free element? */
    sdkl_rawgeti(L, t, ref);  /* remove it from list */
    sdkl_rawseti(L, t, freelist);  /* (t[freelist] = t[ref]) */
  }
  else  /* no free elements */
    ref = (int)sdkl_rawlen(L, t) + 1;  /* get a new reference */
  sdkl_rawseti(L, t, ref);
  return ref;
}


LUALIB_API void sdklL_unref (sdkl_State *L, int t, int ref) {
  if (ref >= 0) {
    t = sdkl_absindex(L, t);
    sdkl_rawgeti(L, t, freelist);
    sdkl_assert(sdkl_isinteger(L, -1));
    sdkl_rawseti(L, t, ref);  /* t[ref] = t[freelist] */
    sdkl_pushinteger(L, ref);
    sdkl_rawseti(L, t, freelist);  /* t[freelist] = ref */
  }
}

/* }====================================================== */


/*
** {======================================================
** Load functions
** =======================================================
*/

typedef struct LoadF {
  int n;  /* number of pre-read characters */
  FILE *f;  /* file being read */
  char buff[BUFSIZ];  /* area for reading file */
} LoadF;


static const char *getF (sdkl_State *L, void *ud, size_t *size) {
  LoadF *lf = (LoadF *)ud;
  (void)L;  /* not used */
  if (lf->n > 0) {  /* are there pre-read characters to be read? */
    *size = lf->n;  /* return them (chars already in buffer) */
    lf->n = 0;  /* no more pre-read characters */
  }
  else {  /* read a block from file */
    /* 'fread' can return > 0 *and* set the EOF flag. If next call to
       'getF' called 'fread', it might still wait for user input.
       The next check avoids this problem. */
    if (feof(lf->f)) return NULL;
    *size = fread(lf->buff, 1, sizeof(lf->buff), lf->f);  /* read block */
  }
  return lf->buff;
}


static int errfile (sdkl_State *L, const char *what, int fnameindex) {
  const char *serr = strerror(errno);
  const char *filename = sdkl_tostring(L, fnameindex) + 1;
  sdkl_pushfstring(L, "cannot %s %s: %s", what, filename, serr);
  sdkl_remove(L, fnameindex);
  return LUA_ERRFILE;
}


static int skipBOM (LoadF *lf) {
  const char *p = "\xEF\xBB\xBF";  /* UTF-8 BOM mark */
  int c;
  lf->n = 0;
  do {
    c = getc(lf->f);
    if (c == EOF || c != *(const unsigned char *)p++) return c;
    lf->buff[lf->n++] = c;  /* to be read by the parser */
  } while (*p != '\0');
  lf->n = 0;  /* prefix matched; discard it */
  return getc(lf->f);  /* return next character */
}


/*
** reads the first character of file 'f' and skips an optional BOM mark
** in its beginning plus its first line if it starts with '#'. Returns
** true if it skipped the first line.  In any case, '*cp' has the
** first "valid" character of the file (after the optional BOM and
** a first-line comment).
*/
static int skipcomment (LoadF *lf, int *cp) {
  int c = *cp = skipBOM(lf);
  if (c == '#') {  /* first line is a comment (Unix exec. file)? */
    do {  /* skip first line */
      c = getc(lf->f);
    } while (c != EOF && c != '\n');
    *cp = getc(lf->f);  /* skip end-of-line, if present */
    return 1;  /* there was a comment */
  }
  else return 0;  /* no comment */
}


LUALIB_API int sdklL_loadfilex (sdkl_State *L, const char *filename,
                                             const char *mode) {
  LoadF lf;
  int status, readstatus;
  int c;
  int fnameindex = sdkl_gettop(L) + 1;  /* index of filename on the stack */
  if (filename == NULL) {
    sdkl_pushliteral(L, "=stdin");
    lf.f = stdin;
  }
  else {
    sdkl_pushfstring(L, "@%s", filename);
    lf.f = fopen(filename, "r");
    if (lf.f == NULL) return errfile(L, "open", fnameindex);
  }
  if (skipcomment(&lf, &c))  /* read initial portion */
    lf.buff[lf.n++] = '\n';  /* add line to correct line numbers */
  if (c == LUA_SIGNATURE[0] && filename) {  /* binary file? */
    lf.f = freopen(filename, "rb", lf.f);  /* reopen in binary mode */
    if (lf.f == NULL) return errfile(L, "reopen", fnameindex);
    skipcomment(&lf, &c);  /* re-read initial portion */
  }
  if (c != EOF)
    lf.buff[lf.n++] = c;  /* 'c' is the first character of the stream */
  status = sdkl_load(L, getF, &lf, sdkl_tostring(L, -1), mode);
  readstatus = ferror(lf.f);
  if (filename) fclose(lf.f);  /* close file (even in case of errors) */
  if (readstatus) {
    sdkl_settop(L, fnameindex);  /* ignore results from 'sdkl_load' */
    return errfile(L, "read", fnameindex);
  }
  sdkl_remove(L, fnameindex);
  return status;
}


typedef struct LoadS {
  const char *s;
  size_t size;
} LoadS;


static const char *getS (sdkl_State *L, void *ud, size_t *size) {
  LoadS *ls = (LoadS *)ud;
  (void)L;  /* not used */
  if (ls->size == 0) return NULL;
  *size = ls->size;
  ls->size = 0;
  return ls->s;
}


LUALIB_API int sdklL_loadbufferx (sdkl_State *L, const char *buff, size_t size,
                                 const char *name, const char *mode) {
  LoadS ls;
  ls.s = buff;
  ls.size = size;
  return sdkl_load(L, getS, &ls, name, mode);
}


LUALIB_API int sdklL_loadstring (sdkl_State *L, const char *s) {
  return sdklL_loadbuffer(L, s, strlen(s), s);
}

/* }====================================================== */



LUALIB_API int sdklL_getmetafield (sdkl_State *L, int obj, const char *event) {
  if (!sdkl_getmetatable(L, obj))  /* no metatable? */
    return LUA_TNIL;
  else {
    int tt;
    sdkl_pushstring(L, event);
    tt = sdkl_rawget(L, -2);
    if (tt == LUA_TNIL)  /* is metafield nil? */
      sdkl_pop(L, 2);  /* remove metatable and metafield */
    else
      sdkl_remove(L, -2);  /* remove only metatable */
    return tt;  /* return metafield type */
  }
}


LUALIB_API int sdklL_callmeta (sdkl_State *L, int obj, const char *event) {
  obj = sdkl_absindex(L, obj);
  if (sdklL_getmetafield(L, obj, event) == LUA_TNIL)  /* no metafield? */
    return 0;
  sdkl_pushvalue(L, obj);
  sdkl_call(L, 1, 1);
  return 1;
}


LUALIB_API sdkl_Integer sdklL_len (sdkl_State *L, int idx) {
  sdkl_Integer l;
  int isnum;
  sdkl_len(L, idx);
  l = sdkl_tointegerx(L, -1, &isnum);
  if (l_unlikely(!isnum))
    sdklL_error(L, "object length is not an integer");
  sdkl_pop(L, 1);  /* remove object */
  return l;
}


LUALIB_API const char *sdklL_tolstring (sdkl_State *L, int idx, size_t *len) {
  if (sdklL_callmeta(L, idx, "__tostring")) {  /* metafield? */
    if (!sdkl_isstring(L, -1))
      sdklL_error(L, "'__tostring' must return a string");
  }
  else {
    switch (sdkl_type(L, idx)) {
      case LUA_TNUMBER: {
        if (sdkl_isinteger(L, idx))
          sdkl_pushfstring(L, "%I", (LUAI_UACINT)sdkl_tointeger(L, idx));
        else
          sdkl_pushfstring(L, "%f", (LUAI_UACNUMBER)sdkl_tonumber(L, idx));
        break;
      }
      case LUA_TSTRING:
        sdkl_pushvalue(L, idx);
        break;
      case LUA_TBOOLEAN:
        sdkl_pushstring(L, (sdkl_toboolean(L, idx) ? "true" : "false"));
        break;
      case LUA_TNIL:
        sdkl_pushliteral(L, "nil");
        break;
      default: {
        int tt = sdklL_getmetafield(L, idx, "__name");  /* try name */
        const char *kind = (tt == LUA_TSTRING) ? sdkl_tostring(L, -1) :
                                                 sdklL_typename(L, idx);
        sdkl_pushfstring(L, "%s: %p", kind, sdkl_topointer(L, idx));
        if (tt != LUA_TNIL)
          sdkl_remove(L, -2);  /* remove '__name' */
        break;
      }
    }
  }
  return sdkl_tolstring(L, -1, len);
}


/*
** set functions from list 'l' into table at top - 'nup'; each
** function gets the 'nup' elements at the top as upvalues.
** Returns with only the table at the stack.
*/
LUALIB_API void sdklL_setfuncs (sdkl_State *L, const sdklL_Reg *l, int nup) {
  sdklL_checkstack(L, nup, "too many upvalues");
  for (; l->name != NULL; l++) {  /* fill the table with given functions */
    if (l->func == NULL)  /* place holder? */
      sdkl_pushboolean(L, 0);
    else {
      int i;
      for (i = 0; i < nup; i++)  /* copy upvalues to the top */
        sdkl_pushvalue(L, -nup);
      sdkl_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
    }
    sdkl_setfield(L, -(nup + 2), l->name);
  }
  sdkl_pop(L, nup);  /* remove upvalues */
}


/*
** ensure that stack[idx][fname] has a table and push that table
** into the stack
*/
LUALIB_API int sdklL_getsubtable (sdkl_State *L, int idx, const char *fname) {
  if (sdkl_getfield(L, idx, fname) == LUA_TTABLE)
    return 1;  /* table already there */
  else {
    sdkl_pop(L, 1);  /* remove previous result */
    idx = sdkl_absindex(L, idx);
    sdkl_newtable(L);
    sdkl_pushvalue(L, -1);  /* copy to be left at top */
    sdkl_setfield(L, idx, fname);  /* assign new table to field */
    return 0;  /* false, because did not find table there */
  }
}


/*
** Stripped-down 'require': After checking "loaded" table, calls 'openf'
** to open a module, registers the result in 'package.loaded' table and,
** if 'glb' is true, also registers the result in the global table.
** Leaves resulting module on the top.
*/
LUALIB_API void sdklL_requiref (sdkl_State *L, const char *modname,
                               sdkl_CFunction openf, int glb) {
  sdklL_getsubtable(L, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
  sdkl_getfield(L, -1, modname);  /* LOADED[modname] */
  if (!sdkl_toboolean(L, -1)) {  /* package not already loaded? */
    sdkl_pop(L, 1);  /* remove field */
    sdkl_pushcfunction(L, openf);
    sdkl_pushstring(L, modname);  /* argument to open function */
    sdkl_call(L, 1, 1);  /* call 'openf' to open module */
    sdkl_pushvalue(L, -1);  /* make copy of module (call result) */
    sdkl_setfield(L, -3, modname);  /* LOADED[modname] = module */
  }
  sdkl_remove(L, -2);  /* remove LOADED table */
  if (glb) {
    sdkl_pushvalue(L, -1);  /* copy of module */
    sdkl_setglobal(L, modname);  /* _G[modname] = module */
  }
}


LUALIB_API void sdklL_addgsub (sdklL_Buffer *b, const char *s,
                                     const char *p, const char *r) {
  const char *wild;
  size_t l = strlen(p);
  while ((wild = strstr(s, p)) != NULL) {
    sdklL_addlstring(b, s, wild - s);  /* push prefix */
    sdklL_addstring(b, r);  /* push replacement in place of pattern */
    s = wild + l;  /* continue after 'p' */
  }
  sdklL_addstring(b, s);  /* push last suffix */
}


LUALIB_API const char *sdklL_gsub (sdkl_State *L, const char *s,
                                  const char *p, const char *r) {
  sdklL_Buffer b;
  sdklL_buffinit(L, &b);
  sdklL_addgsub(&b, s, p, r);
  sdklL_pushresult(&b);
  return sdkl_tostring(L, -1);
}


static void *l_alloc (void *ud, void *ptr, size_t osize, size_t nsize) {
  (void)ud; (void)osize;  /* not used */
  if (nsize == 0) {
    free(ptr);
    return NULL;
  }
  else
    return realloc(ptr, nsize);
}


static int panic (sdkl_State *L) {
  const char *msg = sdkl_tostring(L, -1);
  if (msg == NULL) msg = "error object is not a string";
  sdkl_writestringerror("PANIC: unprotected error in call to SDKL API (%s)\n",
                        msg);
  return 0;  /* return to SDKL to abort */
}


/*
** Warning functions:
** warnfoff: warning system is off
** warnfon: ready to start a new message
** warnfcont: previous message is to be continued
*/
static void warnfoff (void *ud, const char *message, int tocont);
static void warnfon (void *ud, const char *message, int tocont);
static void warnfcont (void *ud, const char *message, int tocont);


/*
** Check whether message is a control message. If so, execute the
** control or ignore it if unknown.
*/
static int checkcontrol (sdkl_State *L, const char *message, int tocont) {
  if (tocont || *(message++) != '@')  /* not a control message? */
    return 0;
  else {
    if (strcmp(message, "off") == 0)
      sdkl_setwarnf(L, warnfoff, L);  /* turn warnings off */
    else if (strcmp(message, "on") == 0)
      sdkl_setwarnf(L, warnfon, L);   /* turn warnings on */
    return 1;  /* it was a control message */
  }
}


static void warnfoff (void *ud, const char *message, int tocont) {
  checkcontrol((sdkl_State *)ud, message, tocont);
}


/*
** Writes the message and handle 'tocont', finishing the message
** if needed and setting the next warn function.
*/
static void warnfcont (void *ud, const char *message, int tocont) {
  sdkl_State *L = (sdkl_State *)ud;
  sdkl_writestringerror("%s", message);  /* write message */
  if (tocont)  /* not the last part? */
    sdkl_setwarnf(L, warnfcont, L);  /* to be continued */
  else {  /* last part */
    sdkl_writestringerror("%s", "\n");  /* finish message with end-of-line */
    sdkl_setwarnf(L, warnfon, L);  /* next call is a new message */
  }
}


static void warnfon (void *ud, const char *message, int tocont) {
  if (checkcontrol((sdkl_State *)ud, message, tocont))  /* control message? */
    return;  /* nothing else to be done */
  sdkl_writestringerror("%s", "SDKL warning: ");  /* start a new warning */
  warnfcont(ud, message, tocont);  /* finish processing */
}


LUALIB_API sdkl_State *sdklL_newstate (void) {
  sdkl_State *L = sdkl_newstate(l_alloc, NULL);
  if (l_likely(L)) {
    sdkl_atpanic(L, &panic);
    sdkl_setwarnf(L, warnfoff, L);  /* default is warnings off */
  }
  return L;
}


LUALIB_API void sdklL_checkversion_ (sdkl_State *L, sdkl_Number ver, size_t sz) {
  sdkl_Number v = sdkl_version(L);
  if (sz != LUAL_NUMSIZES)  /* check numeric types */
    sdklL_error(L, "core and library have incompatible numeric types");
  else if (v != ver)
    sdklL_error(L, "version mismatch: app. needs %f, SDKL core provides %f",
                  (LUAI_UACNUMBER)ver, (LUAI_UACNUMBER)v);
}

