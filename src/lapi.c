/*
** $Id: lapi.c $
** SDKL API
** See Copyright Notice in sdkl.h
*/

#define lapi_c
#define SDKL_CORE

#include "lprefix.h"


#include <limits.h>
#include <stdarg.h>
#include <string.h>

#include "sdkl.h"

#include "lapi.h"
#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "lgc.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "ltm.h"
#include "lundump.h"
#include "lvm.h"



const char sdkl_ident[] =
  "$SDKLVersion: " SDKL_COPYRIGHT " $"
  "$SDKLAuthors: " SDKL_AUTHORS " $";



/*
** Test for a valid index (one that is not the 'nilvalue').
** '!ttisnil(o)' implies 'o != &G(L)->nilvalue', so it is not needed.
** However, it covers the most common cases in a faster way.
*/
#define isvalid(L, o)	(!ttisnil(o) || o != &G(L)->nilvalue)


/* test for pseudo index */
#define ispseudo(i)		((i) <= SDKL_REGISTRYINDEX)

/* test for upvalue */
#define isupvalue(i)		((i) < SDKL_REGISTRYINDEX)


static TValue *index2value (sdkl_State *L, int idx) {
  CallInfo *ci = L->ci;
  if (idx > 0) {
    StkId o = ci->func + idx;
    api_check(L, idx <= L->ci->top - (ci->func + 1), "unacceptable index");
    if (o >= L->top) return &G(L)->nilvalue;
    else return s2v(o);
  }
  else if (!ispseudo(idx)) {  /* negative index */
    api_check(L, idx != 0 && -idx <= L->top - (ci->func + 1), "invalid index");
    return s2v(L->top + idx);
  }
  else if (idx == SDKL_REGISTRYINDEX)
    return &G(L)->l_registry;
  else {  /* upvalues */
    idx = SDKL_REGISTRYINDEX - idx;
    api_check(L, idx <= MAXUPVAL + 1, "upvalue index too large");
    if (ttislcf(s2v(ci->func)))  /* light C function? */
      return &G(L)->nilvalue;  /* it has no upvalues */
    else {
      CClosure *func = clCvalue(s2v(ci->func));
      return (idx <= func->nupvalues) ? &func->upvalue[idx-1]
                                      : &G(L)->nilvalue;
    }
  }
}


static StkId index2stack (sdkl_State *L, int idx) {
  CallInfo *ci = L->ci;
  if (idx > 0) {
    StkId o = ci->func + idx;
    api_check(L, o < L->top, "unacceptable index");
    return o;
  }
  else {    /* non-positive index */
    api_check(L, idx != 0 && -idx <= L->top - (ci->func + 1), "invalid index");
    api_check(L, !ispseudo(idx), "invalid index");
    return L->top + idx;
  }
}


SDKL_API int sdkl_checkstack (sdkl_State *L, int n) {
  int res;
  CallInfo *ci;
  sdkl_lock(L);
  ci = L->ci;
  api_check(L, n >= 0, "negative 'n'");
  if (L->stack_last - L->top > n)  /* stack large enough? */
    res = 1;  /* yes; check is OK */
  else {  /* no; need to grow stack */
    int inuse = cast_int(L->top - L->stack) + EXTRA_STACK;
    if (inuse > SDKLI_MAXSTACK - n)  /* can grow without overflow? */
      res = 0;  /* no */
    else  /* try to grow stack */
      res = sdklD_growstack(L, n, 0);
  }
  if (res && ci->top < L->top + n)
    ci->top = L->top + n;  /* adjust frame top */
  sdkl_unlock(L);
  return res;
}


SDKL_API void sdkl_xmove (sdkl_State *from, sdkl_State *to, int n) {
  int i;
  if (from == to) return;
  sdkl_lock(to);
  api_checknelems(from, n);
  api_check(from, G(from) == G(to), "moving among independent states");
  api_check(from, to->ci->top - to->top >= n, "stack overflow");
  from->top -= n;
  for (i = 0; i < n; i++) {
    setobjs2s(to, to->top, from->top + i);
    to->top++;  /* stack already checked by previous 'api_check' */
  }
  sdkl_unlock(to);
}


SDKL_API sdkl_CFunction sdkl_atpanic (sdkl_State *L, sdkl_CFunction panicf) {
  sdkl_CFunction old;
  sdkl_lock(L);
  old = G(L)->panic;
  G(L)->panic = panicf;
  sdkl_unlock(L);
  return old;
}


SDKL_API sdkl_Number sdkl_version (sdkl_State *L) {
  UNUSED(L);
  return SDKL_VERSION_NUM;
}



/*
** basic stack manipulation
*/


/*
** convert an acceptable stack index into an absolute index
*/
SDKL_API int sdkl_absindex (sdkl_State *L, int idx) {
  return (idx > 0 || ispseudo(idx))
         ? idx
         : cast_int(L->top - L->ci->func) + idx;
}


SDKL_API int sdkl_gettop (sdkl_State *L) {
  return cast_int(L->top - (L->ci->func + 1));
}


SDKL_API void sdkl_settop (sdkl_State *L, int idx) {
  CallInfo *ci;
  StkId func, newtop;
  ptrdiff_t diff;  /* difference for new top */
  sdkl_lock(L);
  ci = L->ci;
  func = ci->func;
  if (idx >= 0) {
    api_check(L, idx <= ci->top - (func + 1), "new top too large");
    diff = ((func + 1) + idx) - L->top;
    for (; diff > 0; diff--)
      setnilvalue(s2v(L->top++));  /* clear new slots */
  }
  else {
    api_check(L, -(idx+1) <= (L->top - (func + 1)), "invalid new top");
    diff = idx + 1;  /* will "subtract" index (as it is negative) */
  }
  api_check(L, L->tbclist < L->top, "previous pop of an unclosed slot");
  newtop = L->top + diff;
  if (diff < 0 && L->tbclist >= newtop) {
    sdkl_assert(hastocloseCfunc(ci->nresults));
    sdklF_close(L, newtop, CLOSEKTOP, 0);
  }
  L->top = newtop;  /* correct top only after closing any upvalue */
  sdkl_unlock(L);
}


SDKL_API void sdkl_closeslot (sdkl_State *L, int idx) {
  StkId level;
  sdkl_lock(L);
  level = index2stack(L, idx);
  api_check(L, hastocloseCfunc(L->ci->nresults) && L->tbclist == level,
     "no variable to close at given level");
  sdklF_close(L, level, CLOSEKTOP, 0);
  level = index2stack(L, idx);  /* stack may be moved */
  setnilvalue(s2v(level));
  sdkl_unlock(L);
}


/*
** Reverse the stack segment from 'from' to 'to'
** (auxiliary to 'sdkl_rotate')
** Note that we move(copy) only the value inside the stack.
** (We do not move additional fields that may exist.)
*/
static void reverse (sdkl_State *L, StkId from, StkId to) {
  for (; from < to; from++, to--) {
    TValue temp;
    setobj(L, &temp, s2v(from));
    setobjs2s(L, from, to);
    setobj2s(L, to, &temp);
  }
}


/*
** Let x = AB, where A is a prefix of length 'n'. Then,
** rotate x n == BA. But BA == (A^r . B^r)^r.
*/
SDKL_API void sdkl_rotate (sdkl_State *L, int idx, int n) {
  StkId p, t, m;
  sdkl_lock(L);
  t = L->top - 1;  /* end of stack segment being rotated */
  p = index2stack(L, idx);  /* start of segment */
  api_check(L, (n >= 0 ? n : -n) <= (t - p + 1), "invalid 'n'");
  m = (n >= 0 ? t - n : p - n - 1);  /* end of prefix */
  reverse(L, p, m);  /* reverse the prefix with length 'n' */
  reverse(L, m + 1, t);  /* reverse the suffix */
  reverse(L, p, t);  /* reverse the entire segment */
  sdkl_unlock(L);
}


SDKL_API void sdkl_copy (sdkl_State *L, int fromidx, int toidx) {
  TValue *fr, *to;
  sdkl_lock(L);
  fr = index2value(L, fromidx);
  to = index2value(L, toidx);
  api_check(L, isvalid(L, to), "invalid index");
  setobj(L, to, fr);
  if (isupvalue(toidx))  /* function upvalue? */
    sdklC_barrier(L, clCvalue(s2v(L->ci->func)), fr);
  /* SDKL_REGISTRYINDEX does not need gc barrier
     (collector revisits it before finishing collection) */
  sdkl_unlock(L);
}


SDKL_API void sdkl_pushvalue (sdkl_State *L, int idx) {
  sdkl_lock(L);
  setobj2s(L, L->top, index2value(L, idx));
  api_incr_top(L);
  sdkl_unlock(L);
}



/*
** access functions (stack -> C)
*/


SDKL_API int sdkl_type (sdkl_State *L, int idx) {
  const TValue *o = index2value(L, idx);
  return (isvalid(L, o) ? ttype(o) : SDKL_TNONE);
}


SDKL_API const char *sdkl_typename (sdkl_State *L, int t) {
  UNUSED(L);
  api_check(L, SDKL_TNONE <= t && t < SDKL_NUMTYPES, "invalid type");
  return ttypename(t);
}


SDKL_API int sdkl_iscfunction (sdkl_State *L, int idx) {
  const TValue *o = index2value(L, idx);
  return (ttislcf(o) || (ttisCclosure(o)));
}


SDKL_API int sdkl_isinteger (sdkl_State *L, int idx) {
  const TValue *o = index2value(L, idx);
  return ttisinteger(o);
}


SDKL_API int sdkl_isnumber (sdkl_State *L, int idx) {
  sdkl_Number n;
  const TValue *o = index2value(L, idx);
  return tonumber(o, &n);
}


SDKL_API int sdkl_isstring (sdkl_State *L, int idx) {
  const TValue *o = index2value(L, idx);
  return (ttisstring(o) || cvt2str(o));
}


SDKL_API int sdkl_isuserdata (sdkl_State *L, int idx) {
  const TValue *o = index2value(L, idx);
  return (ttisfulluserdata(o) || ttislightuserdata(o));
}


SDKL_API int sdkl_rawequal (sdkl_State *L, int index1, int index2) {
  const TValue *o1 = index2value(L, index1);
  const TValue *o2 = index2value(L, index2);
  return (isvalid(L, o1) && isvalid(L, o2)) ? sdklV_rawequalobj(o1, o2) : 0;
}


SDKL_API void sdkl_arith (sdkl_State *L, int op) {
  sdkl_lock(L);
  if (op != SDKL_OPUNM && op != SDKL_OPBNOT)
    api_checknelems(L, 2);  /* all other operations expect two operands */
  else {  /* for unary operations, add fake 2nd operand */
    api_checknelems(L, 1);
    setobjs2s(L, L->top, L->top - 1);
    api_incr_top(L);
  }
  /* first operand at top - 2, second at top - 1; result go to top - 2 */
  sdklO_arith(L, op, s2v(L->top - 2), s2v(L->top - 1), L->top - 2);
  L->top--;  /* remove second operand */
  sdkl_unlock(L);
}


SDKL_API int sdkl_compare (sdkl_State *L, int index1, int index2, int op) {
  const TValue *o1;
  const TValue *o2;
  int i = 0;
  sdkl_lock(L);  /* may call tag method */
  o1 = index2value(L, index1);
  o2 = index2value(L, index2);
  if (isvalid(L, o1) && isvalid(L, o2)) {
    switch (op) {
      case SDKL_OPEQ: i = sdklV_equalobj(L, o1, o2); break;
      case SDKL_OPLT: i = sdklV_lessthan(L, o1, o2); break;
      case SDKL_OPLE: i = sdklV_lessequal(L, o1, o2); break;
      default: api_check(L, 0, "invalid option");
    }
  }
  sdkl_unlock(L);
  return i;
}


SDKL_API size_t sdkl_stringtonumber (sdkl_State *L, const char *s) {
  size_t sz = sdklO_str2num(s, s2v(L->top));
  if (sz != 0)
    api_incr_top(L);
  return sz;
}


SDKL_API sdkl_Number sdkl_tonumberx (sdkl_State *L, int idx, int *pisnum) {
  sdkl_Number n = 0;
  const TValue *o = index2value(L, idx);
  int isnum = tonumber(o, &n);
  if (pisnum)
    *pisnum = isnum;
  return n;
}


SDKL_API sdkl_Integer sdkl_tointegerx (sdkl_State *L, int idx, int *pisnum) {
  sdkl_Integer res = 0;
  const TValue *o = index2value(L, idx);
  int isnum = tointeger(o, &res);
  if (pisnum)
    *pisnum = isnum;
  return res;
}


SDKL_API int sdkl_toboolean (sdkl_State *L, int idx) {
  const TValue *o = index2value(L, idx);
  return !l_isfalse(o);
}


SDKL_API const char *sdkl_tolstring (sdkl_State *L, int idx, size_t *len) {
  TValue *o;
  sdkl_lock(L);
  o = index2value(L, idx);
  if (!ttisstring(o)) {
    if (!cvt2str(o)) {  /* not convertible? */
      if (len != NULL) *len = 0;
      sdkl_unlock(L);
      return NULL;
    }
    sdklO_tostring(L, o);
    sdklC_checkGC(L);
    o = index2value(L, idx);  /* previous call may reallocate the stack */
  }
  if (len != NULL)
    *len = vslen(o);
  sdkl_unlock(L);
  return svalue(o);
}


SDKL_API sdkl_Unsigned sdkl_rawlen (sdkl_State *L, int idx) {
  const TValue *o = index2value(L, idx);
  switch (ttypetag(o)) {
    case SDKL_VSHRSTR: return tsvalue(o)->shrlen;
    case SDKL_VLNGSTR: return tsvalue(o)->u.lnglen;
    case SDKL_VUSERDATA: return uvalue(o)->len;
    case SDKL_VTABLE: return sdklH_getn(hvalue(o));
    default: return 0;
  }
}


SDKL_API sdkl_CFunction sdkl_tocfunction (sdkl_State *L, int idx) {
  const TValue *o = index2value(L, idx);
  if (ttislcf(o)) return fvalue(o);
  else if (ttisCclosure(o))
    return clCvalue(o)->f;
  else return NULL;  /* not a C function */
}


static void *touserdata (const TValue *o) {
  switch (ttype(o)) {
    case SDKL_TUSERDATA: return getudatamem(uvalue(o));
    case SDKL_TLIGHTUSERDATA: return pvalue(o);
    default: return NULL;
  }
}


SDKL_API void *sdkl_touserdata (sdkl_State *L, int idx) {
  const TValue *o = index2value(L, idx);
  return touserdata(o);
}


SDKL_API sdkl_State *sdkl_tothread (sdkl_State *L, int idx) {
  const TValue *o = index2value(L, idx);
  return (!ttisthread(o)) ? NULL : thvalue(o);
}


/*
** Returns a pointer to the internal representation of an object.
** Note that ANSI C does not allow the conversion of a pointer to
** function to a 'void*', so the conversion here goes through
** a 'size_t'. (As the returned pointer is only informative, this
** conversion should not be a problem.)
*/
SDKL_API const void *sdkl_topointer (sdkl_State *L, int idx) {
  const TValue *o = index2value(L, idx);
  switch (ttypetag(o)) {
    case SDKL_VLCF: return cast_voidp(cast_sizet(fvalue(o)));
    case SDKL_VUSERDATA: case SDKL_VLIGHTUSERDATA:
      return touserdata(o);
    default: {
      if (iscollectable(o))
        return gcvalue(o);
      else
        return NULL;
    }
  }
}



/*
** push functions (C -> stack)
*/


SDKL_API void sdkl_pushnil (sdkl_State *L) {
  sdkl_lock(L);
  setnilvalue(s2v(L->top));
  api_incr_top(L);
  sdkl_unlock(L);
}


SDKL_API void sdkl_pushnumber (sdkl_State *L, sdkl_Number n) {
  sdkl_lock(L);
  setfltvalue(s2v(L->top), n);
  api_incr_top(L);
  sdkl_unlock(L);
}


SDKL_API void sdkl_pushinteger (sdkl_State *L, sdkl_Integer n) {
  sdkl_lock(L);
  setivalue(s2v(L->top), n);
  api_incr_top(L);
  sdkl_unlock(L);
}


/*
** Pushes on the stack a string with given length. Avoid using 's' when
** 'len' == 0 (as 's' can be NULL in that case), due to later use of
** 'memcmp' and 'memcpy'.
*/
SDKL_API const char *sdkl_pushlstring (sdkl_State *L, const char *s, size_t len) {
  TString *ts;
  sdkl_lock(L);
  ts = (len == 0) ? sdklS_new(L, "") : sdklS_newlstr(L, s, len);
  setsvalue2s(L, L->top, ts);
  api_incr_top(L);
  sdklC_checkGC(L);
  sdkl_unlock(L);
  return getstr(ts);
}


SDKL_API const char *sdkl_pushstring (sdkl_State *L, const char *s) {
  sdkl_lock(L);
  if (s == NULL)
    setnilvalue(s2v(L->top));
  else {
    TString *ts;
    ts = sdklS_new(L, s);
    setsvalue2s(L, L->top, ts);
    s = getstr(ts);  /* internal copy's address */
  }
  api_incr_top(L);
  sdklC_checkGC(L);
  sdkl_unlock(L);
  return s;
}


SDKL_API const char *sdkl_pushvfstring (sdkl_State *L, const char *fmt,
                                      va_list argp) {
  const char *ret;
  sdkl_lock(L);
  ret = sdklO_pushvfstring(L, fmt, argp);
  sdklC_checkGC(L);
  sdkl_unlock(L);
  return ret;
}


SDKL_API const char *sdkl_pushfstring (sdkl_State *L, const char *fmt, ...) {
  const char *ret;
  va_list argp;
  sdkl_lock(L);
  va_start(argp, fmt);
  ret = sdklO_pushvfstring(L, fmt, argp);
  va_end(argp);
  sdklC_checkGC(L);
  sdkl_unlock(L);
  return ret;
}


SDKL_API void sdkl_pushcclosure (sdkl_State *L, sdkl_CFunction fn, int n) {
  sdkl_lock(L);
  if (n == 0) {
    setfvalue(s2v(L->top), fn);
    api_incr_top(L);
  }
  else {
    CClosure *cl;
    api_checknelems(L, n);
    api_check(L, n <= MAXUPVAL, "upvalue index too large");
    cl = sdklF_newCclosure(L, n);
    cl->f = fn;
    L->top -= n;
    while (n--) {
      setobj2n(L, &cl->upvalue[n], s2v(L->top + n));
      /* does not need barrier because closure is white */
      sdkl_assert(iswhite(cl));
    }
    setclCvalue(L, s2v(L->top), cl);
    api_incr_top(L);
    sdklC_checkGC(L);
  }
  sdkl_unlock(L);
}


SDKL_API void sdkl_pushboolean (sdkl_State *L, int b) {
  sdkl_lock(L);
  if (b)
    setbtvalue(s2v(L->top));
  else
    setbfvalue(s2v(L->top));
  api_incr_top(L);
  sdkl_unlock(L);
}


SDKL_API void sdkl_pushlightuserdata (sdkl_State *L, void *p) {
  sdkl_lock(L);
  setpvalue(s2v(L->top), p);
  api_incr_top(L);
  sdkl_unlock(L);
}


SDKL_API int sdkl_pushthread (sdkl_State *L) {
  sdkl_lock(L);
  setthvalue(L, s2v(L->top), L);
  api_incr_top(L);
  sdkl_unlock(L);
  return (G(L)->mainthread == L);
}



/*
** get functions (SDKL -> stack)
*/


static int auxgetstr (sdkl_State *L, const TValue *t, const char *k) {
  const TValue *slot;
  TString *str = sdklS_new(L, k);
  if (sdklV_fastget(L, t, str, slot, sdklH_getstr)) {
    setobj2s(L, L->top, slot);
    api_incr_top(L);
  }
  else {
    setsvalue2s(L, L->top, str);
    api_incr_top(L);
    sdklV_finishget(L, t, s2v(L->top - 1), L->top - 1, slot);
  }
  sdkl_unlock(L);
  return ttype(s2v(L->top - 1));
}


/*
** Get the global table in the registry. Since all predefined
** indices in the registry were inserted right when the registry
** was created and never removed, they must always be in the array
** part of the registry.
*/
#define getGtable(L)  \
	(&hvalue(&G(L)->l_registry)->array[SDKL_RIDX_GLOBALS - 1])


SDKL_API int sdkl_getglobal (sdkl_State *L, const char *name) {
  const TValue *G;
  sdkl_lock(L);
  G = getGtable(L);
  return auxgetstr(L, G, name);
}


SDKL_API int sdkl_gettable (sdkl_State *L, int idx) {
  const TValue *slot;
  TValue *t;
  sdkl_lock(L);
  t = index2value(L, idx);
  if (sdklV_fastget(L, t, s2v(L->top - 1), slot, sdklH_get)) {
    setobj2s(L, L->top - 1, slot);
  }
  else
    sdklV_finishget(L, t, s2v(L->top - 1), L->top - 1, slot);
  sdkl_unlock(L);
  return ttype(s2v(L->top - 1));
}


SDKL_API int sdkl_getfield (sdkl_State *L, int idx, const char *k) {
  sdkl_lock(L);
  return auxgetstr(L, index2value(L, idx), k);
}


SDKL_API int sdkl_geti (sdkl_State *L, int idx, sdkl_Integer n) {
  TValue *t;
  const TValue *slot;
  sdkl_lock(L);
  t = index2value(L, idx);
  if (sdklV_fastgeti(L, t, n, slot)) {
    setobj2s(L, L->top, slot);
  }
  else {
    TValue aux;
    setivalue(&aux, n);
    sdklV_finishget(L, t, &aux, L->top, slot);
  }
  api_incr_top(L);
  sdkl_unlock(L);
  return ttype(s2v(L->top - 1));
}


static int finishrawget (sdkl_State *L, const TValue *val) {
  if (isempty(val))  /* avoid copying empty items to the stack */
    setnilvalue(s2v(L->top));
  else
    setobj2s(L, L->top, val);
  api_incr_top(L);
  sdkl_unlock(L);
  return ttype(s2v(L->top - 1));
}


static Table *gettable (sdkl_State *L, int idx) {
  TValue *t = index2value(L, idx);
  api_check(L, ttistable(t), "table expected");
  return hvalue(t);
}


SDKL_API int sdkl_rawget (sdkl_State *L, int idx) {
  Table *t;
  const TValue *val;
  sdkl_lock(L);
  api_checknelems(L, 1);
  t = gettable(L, idx);
  val = sdklH_get(t, s2v(L->top - 1));
  L->top--;  /* remove key */
  return finishrawget(L, val);
}


SDKL_API int sdkl_rawgeti (sdkl_State *L, int idx, sdkl_Integer n) {
  Table *t;
  sdkl_lock(L);
  t = gettable(L, idx);
  return finishrawget(L, sdklH_getint(t, n));
}


SDKL_API int sdkl_rawgetp (sdkl_State *L, int idx, const void *p) {
  Table *t;
  TValue k;
  sdkl_lock(L);
  t = gettable(L, idx);
  setpvalue(&k, cast_voidp(p));
  return finishrawget(L, sdklH_get(t, &k));
}


SDKL_API void sdkl_createtable (sdkl_State *L, int narray, int nrec) {
  Table *t;
  sdkl_lock(L);
  t = sdklH_new(L);
  sethvalue2s(L, L->top, t);
  api_incr_top(L);
  if (narray > 0 || nrec > 0)
    sdklH_resize(L, t, narray, nrec);
  sdklC_checkGC(L);
  sdkl_unlock(L);
}


SDKL_API int sdkl_getmetatable (sdkl_State *L, int objindex) {
  const TValue *obj;
  Table *mt;
  int res = 0;
  sdkl_lock(L);
  obj = index2value(L, objindex);
  switch (ttype(obj)) {
    case SDKL_TTABLE:
      mt = hvalue(obj)->metatable;
      break;
    case SDKL_TUSERDATA:
      mt = uvalue(obj)->metatable;
      break;
    default:
      mt = G(L)->mt[ttype(obj)];
      break;
  }
  if (mt != NULL) {
    sethvalue2s(L, L->top, mt);
    api_incr_top(L);
    res = 1;
  }
  sdkl_unlock(L);
  return res;
}


SDKL_API int sdkl_getiuservalue (sdkl_State *L, int idx, int n) {
  TValue *o;
  int t;
  sdkl_lock(L);
  o = index2value(L, idx);
  api_check(L, ttisfulluserdata(o), "full userdata expected");
  if (n <= 0 || n > uvalue(o)->nuvalue) {
    setnilvalue(s2v(L->top));
    t = SDKL_TNONE;
  }
  else {
    setobj2s(L, L->top, &uvalue(o)->uv[n - 1].uv);
    t = ttype(s2v(L->top));
  }
  api_incr_top(L);
  sdkl_unlock(L);
  return t;
}


/*
** set functions (stack -> SDKL)
*/

/*
** t[k] = value at the top of the stack (where 'k' is a string)
*/
static void auxsetstr (sdkl_State *L, const TValue *t, const char *k) {
  const TValue *slot;
  TString *str = sdklS_new(L, k);
  api_checknelems(L, 1);
  if (sdklV_fastget(L, t, str, slot, sdklH_getstr)) {
    sdklV_finishfastset(L, t, slot, s2v(L->top - 1));
    L->top--;  /* pop value */
  }
  else {
    setsvalue2s(L, L->top, str);  /* push 'str' (to make it a TValue) */
    api_incr_top(L);
    sdklV_finishset(L, t, s2v(L->top - 1), s2v(L->top - 2), slot);
    L->top -= 2;  /* pop value and key */
  }
  sdkl_unlock(L);  /* lock done by caller */
}


SDKL_API void sdkl_setglobal (sdkl_State *L, const char *name) {
  const TValue *G;
  sdkl_lock(L);  /* unlock done in 'auxsetstr' */
  G = getGtable(L);
  auxsetstr(L, G, name);
}


SDKL_API void sdkl_settable (sdkl_State *L, int idx) {
  TValue *t;
  const TValue *slot;
  sdkl_lock(L);
  api_checknelems(L, 2);
  t = index2value(L, idx);
  if (sdklV_fastget(L, t, s2v(L->top - 2), slot, sdklH_get)) {
    sdklV_finishfastset(L, t, slot, s2v(L->top - 1));
  }
  else
    sdklV_finishset(L, t, s2v(L->top - 2), s2v(L->top - 1), slot);
  L->top -= 2;  /* pop index and value */
  sdkl_unlock(L);
}


SDKL_API void sdkl_setfield (sdkl_State *L, int idx, const char *k) {
  sdkl_lock(L);  /* unlock done in 'auxsetstr' */
  auxsetstr(L, index2value(L, idx), k);
}


SDKL_API void sdkl_seti (sdkl_State *L, int idx, sdkl_Integer n) {
  TValue *t;
  const TValue *slot;
  sdkl_lock(L);
  api_checknelems(L, 1);
  t = index2value(L, idx);
  if (sdklV_fastgeti(L, t, n, slot)) {
    sdklV_finishfastset(L, t, slot, s2v(L->top - 1));
  }
  else {
    TValue aux;
    setivalue(&aux, n);
    sdklV_finishset(L, t, &aux, s2v(L->top - 1), slot);
  }
  L->top--;  /* pop value */
  sdkl_unlock(L);
}


static void aux_rawset (sdkl_State *L, int idx, TValue *key, int n) {
  Table *t;
  sdkl_lock(L);
  api_checknelems(L, n);
  t = gettable(L, idx);
  sdklH_set(L, t, key, s2v(L->top - 1));
  invalidateTMcache(t);
  sdklC_barrierback(L, obj2gco(t), s2v(L->top - 1));
  L->top -= n;
  sdkl_unlock(L);
}


SDKL_API void sdkl_rawset (sdkl_State *L, int idx) {
  aux_rawset(L, idx, s2v(L->top - 2), 2);
}


SDKL_API void sdkl_rawsetp (sdkl_State *L, int idx, const void *p) {
  TValue k;
  setpvalue(&k, cast_voidp(p));
  aux_rawset(L, idx, &k, 1);
}


SDKL_API void sdkl_rawseti (sdkl_State *L, int idx, sdkl_Integer n) {
  Table *t;
  sdkl_lock(L);
  api_checknelems(L, 1);
  t = gettable(L, idx);
  sdklH_setint(L, t, n, s2v(L->top - 1));
  sdklC_barrierback(L, obj2gco(t), s2v(L->top - 1));
  L->top--;
  sdkl_unlock(L);
}


SDKL_API int sdkl_setmetatable (sdkl_State *L, int objindex) {
  TValue *obj;
  Table *mt;
  sdkl_lock(L);
  api_checknelems(L, 1);
  obj = index2value(L, objindex);
  if (ttisnil(s2v(L->top - 1)))
    mt = NULL;
  else {
    api_check(L, ttistable(s2v(L->top - 1)), "table expected");
    mt = hvalue(s2v(L->top - 1));
  }
  switch (ttype(obj)) {
    case SDKL_TTABLE: {
      hvalue(obj)->metatable = mt;
      if (mt) {
        sdklC_objbarrier(L, gcvalue(obj), mt);
        sdklC_checkfinalizer(L, gcvalue(obj), mt);
      }
      break;
    }
    case SDKL_TUSERDATA: {
      uvalue(obj)->metatable = mt;
      if (mt) {
        sdklC_objbarrier(L, uvalue(obj), mt);
        sdklC_checkfinalizer(L, gcvalue(obj), mt);
      }
      break;
    }
    default: {
      G(L)->mt[ttype(obj)] = mt;
      break;
    }
  }
  L->top--;
  sdkl_unlock(L);
  return 1;
}


SDKL_API int sdkl_setiuservalue (sdkl_State *L, int idx, int n) {
  TValue *o;
  int res;
  sdkl_lock(L);
  api_checknelems(L, 1);
  o = index2value(L, idx);
  api_check(L, ttisfulluserdata(o), "full userdata expected");
  if (!(cast_uint(n) - 1u < cast_uint(uvalue(o)->nuvalue)))
    res = 0;  /* 'n' not in [1, uvalue(o)->nuvalue] */
  else {
    setobj(L, &uvalue(o)->uv[n - 1].uv, s2v(L->top - 1));
    sdklC_barrierback(L, gcvalue(o), s2v(L->top - 1));
    res = 1;
  }
  L->top--;
  sdkl_unlock(L);
  return res;
}


/*
** 'load' and 'call' functions (run SDKL code)
*/


#define checkresults(L,na,nr) \
     api_check(L, (nr) == SDKL_MULTRET || (L->ci->top - L->top >= (nr) - (na)), \
	"results from function overflow current stack size")


SDKL_API void sdkl_callk (sdkl_State *L, int nargs, int nresults,
                        sdkl_KContext ctx, sdkl_KFunction k) {
  StkId func;
  sdkl_lock(L);
  api_check(L, k == NULL || !isSDKL(L->ci),
    "cannot use continuations inside hooks");
  api_checknelems(L, nargs+1);
  api_check(L, L->status == SDKL_OK, "cannot do calls on non-normal thread");
  checkresults(L, nargs, nresults);
  func = L->top - (nargs+1);
  if (k != NULL && yieldable(L)) {  /* need to prepare continuation? */
    L->ci->u.c.k = k;  /* save continuation */
    L->ci->u.c.ctx = ctx;  /* save context */
    sdklD_call(L, func, nresults);  /* do the call */
  }
  else  /* no continuation or no yieldable */
    sdklD_callnoyield(L, func, nresults);  /* just do the call */
  adjustresults(L, nresults);
  sdkl_unlock(L);
}



/*
** Execute a protected call.
*/
struct CallS {  /* data to 'f_call' */
  StkId func;
  int nresults;
};


static void f_call (sdkl_State *L, void *ud) {
  struct CallS *c = cast(struct CallS *, ud);
  sdklD_callnoyield(L, c->func, c->nresults);
}



SDKL_API int sdkl_pcallk (sdkl_State *L, int nargs, int nresults, int errfunc,
                        sdkl_KContext ctx, sdkl_KFunction k) {
  struct CallS c;
  int status;
  ptrdiff_t func;
  sdkl_lock(L);
  api_check(L, k == NULL || !isSDKL(L->ci),
    "cannot use continuations inside hooks");
  api_checknelems(L, nargs+1);
  api_check(L, L->status == SDKL_OK, "cannot do calls on non-normal thread");
  checkresults(L, nargs, nresults);
  if (errfunc == 0)
    func = 0;
  else {
    StkId o = index2stack(L, errfunc);
    api_check(L, ttisfunction(s2v(o)), "error handler must be a function");
    func = savestack(L, o);
  }
  c.func = L->top - (nargs+1);  /* function to be called */
  if (k == NULL || !yieldable(L)) {  /* no continuation or no yieldable? */
    c.nresults = nresults;  /* do a 'conventional' protected call */
    status = sdklD_pcall(L, f_call, &c, savestack(L, c.func), func);
  }
  else {  /* prepare continuation (call is already protected by 'resume') */
    CallInfo *ci = L->ci;
    ci->u.c.k = k;  /* save continuation */
    ci->u.c.ctx = ctx;  /* save context */
    /* save information for error recovery */
    ci->u2.funcidx = cast_int(savestack(L, c.func));
    ci->u.c.old_errfunc = L->errfunc;
    L->errfunc = func;
    setoah(ci->callstatus, L->allowhook);  /* save value of 'allowhook' */
    ci->callstatus |= CIST_YPCALL;  /* function can do error recovery */
    sdklD_call(L, c.func, nresults);  /* do the call */
    ci->callstatus &= ~CIST_YPCALL;
    L->errfunc = ci->u.c.old_errfunc;
    status = SDKL_OK;  /* if it is here, there were no errors */
  }
  adjustresults(L, nresults);
  sdkl_unlock(L);
  return status;
}


SDKL_API int sdkl_load (sdkl_State *L, sdkl_Reader reader, void *data,
                      const char *chunkname, const char *mode) {
  ZIO z;
  int status;
  sdkl_lock(L);
  if (!chunkname) chunkname = "?";
  sdklZ_init(L, &z, reader, data);
  status = sdklD_protectedparser(L, &z, chunkname, mode);
  if (status == SDKL_OK) {  /* no errors? */
    LClosure *f = clLvalue(s2v(L->top - 1));  /* get newly created function */
    if (f->nupvalues >= 1) {  /* does it have an upvalue? */
      /* get global table from registry */
      const TValue *gt = getGtable(L);
      /* set global table as 1st upvalue of 'f' (may be SDKL_ENV) */
      setobj(L, f->upvals[0]->v, gt);
      sdklC_barrier(L, f->upvals[0], gt);
    }
  }
  sdkl_unlock(L);
  return status;
}


SDKL_API int sdkl_dump (sdkl_State *L, sdkl_Writer writer, void *data, int strip) {
  int status;
  TValue *o;
  sdkl_lock(L);
  api_checknelems(L, 1);
  o = s2v(L->top - 1);
  if (isLfunction(o))
    status = sdklU_dump(L, getproto(o), writer, data, strip);
  else
    status = 1;
  sdkl_unlock(L);
  return status;
}


SDKL_API int sdkl_status (sdkl_State *L) {
  return L->status;
}


/*
** Garbage-collection function
*/
SDKL_API int sdkl_gc (sdkl_State *L, int what, ...) {
  va_list argp;
  int res = 0;
  global_State *g;
  sdkl_lock(L);
  g = G(L);
  va_start(argp, what);
  switch (what) {
    case SDKL_GCSTOP: {
      g->gcrunning = 0;
      break;
    }
    case SDKL_GCRESTART: {
      sdklE_setdebt(g, 0);
      g->gcrunning = 1;
      break;
    }
    case SDKL_GCCOLLECT: {
      sdklC_fullgc(L, 0);
      break;
    }
    case SDKL_GCCOUNT: {
      /* GC values are expressed in Kbytes: #bytes/2^10 */
      res = cast_int(gettotalbytes(g) >> 10);
      break;
    }
    case SDKL_GCCOUNTB: {
      res = cast_int(gettotalbytes(g) & 0x3ff);
      break;
    }
    case SDKL_GCSTEP: {
      int data = va_arg(argp, int);
      l_mem debt = 1;  /* =1 to signal that it did an actual step */
      lu_byte oldrunning = g->gcrunning;
      g->gcrunning = 1;  /* allow GC to run */
      if (data == 0) {
        sdklE_setdebt(g, 0);  /* do a basic step */
        sdklC_step(L);
      }
      else {  /* add 'data' to total debt */
        debt = cast(l_mem, data) * 1024 + g->GCdebt;
        sdklE_setdebt(g, debt);
        sdklC_checkGC(L);
      }
      g->gcrunning = oldrunning;  /* restore previous state */
      if (debt > 0 && g->gcstate == GCSpause)  /* end of cycle? */
        res = 1;  /* signal it */
      break;
    }
    case SDKL_GCSETPAUSE: {
      int data = va_arg(argp, int);
      res = getgcparam(g->gcpause);
      setgcparam(g->gcpause, data);
      break;
    }
    case SDKL_GCSETSTEPMUL: {
      int data = va_arg(argp, int);
      res = getgcparam(g->gcstepmul);
      setgcparam(g->gcstepmul, data);
      break;
    }
    case SDKL_GCISRUNNING: {
      res = g->gcrunning;
      break;
    }
    case SDKL_GCGEN: {
      int minormul = va_arg(argp, int);
      int majormul = va_arg(argp, int);
      res = isdecGCmodegen(g) ? SDKL_GCGEN : SDKL_GCINC;
      if (minormul != 0)
        g->genminormul = minormul;
      if (majormul != 0)
        setgcparam(g->genmajormul, majormul);
      sdklC_changemode(L, KGC_GEN);
      break;
    }
    case SDKL_GCINC: {
      int pause = va_arg(argp, int);
      int stepmul = va_arg(argp, int);
      int stepsize = va_arg(argp, int);
      res = isdecGCmodegen(g) ? SDKL_GCGEN : SDKL_GCINC;
      if (pause != 0)
        setgcparam(g->gcpause, pause);
      if (stepmul != 0)
        setgcparam(g->gcstepmul, stepmul);
      if (stepsize != 0)
        g->gcstepsize = stepsize;
      sdklC_changemode(L, KGC_INC);
      break;
    }
    default: res = -1;  /* invalid option */
  }
  va_end(argp);
  sdkl_unlock(L);
  return res;
}



/*
** miscellaneous functions
*/


SDKL_API int sdkl_error (sdkl_State *L) {
  TValue *errobj;
  sdkl_lock(L);
  errobj = s2v(L->top - 1);
  api_checknelems(L, 1);
  /* error object is the memory error message? */
  if (ttisshrstring(errobj) && eqshrstr(tsvalue(errobj), G(L)->memerrmsg))
    sdklM_error(L);  /* raise a memory error */
  else
    sdklG_errormsg(L);  /* raise a regular error */
  /* code unreachable; will unlock when control actually leaves the kernel */
  return 0;  /* to avoid warnings */
}


SDKL_API int sdkl_next (sdkl_State *L, int idx) {
  Table *t;
  int more;
  sdkl_lock(L);
  api_checknelems(L, 1);
  t = gettable(L, idx);
  more = sdklH_next(L, t, L->top - 1);
  if (more) {
    api_incr_top(L);
  }
  else  /* no more elements */
    L->top -= 1;  /* remove key */
  sdkl_unlock(L);
  return more;
}


SDKL_API void sdkl_toclose (sdkl_State *L, int idx) {
  int nresults;
  StkId o;
  sdkl_lock(L);
  o = index2stack(L, idx);
  nresults = L->ci->nresults;
  api_check(L, L->tbclist < o, "given index below or equal a marked one");
  sdklF_newtbcupval(L, o);  /* create new to-be-closed upvalue */
  if (!hastocloseCfunc(nresults))  /* function not marked yet? */
    L->ci->nresults = codeNresults(nresults);  /* mark it */
  sdkl_assert(hastocloseCfunc(L->ci->nresults));
  sdkl_unlock(L);
}


SDKL_API void sdkl_concat (sdkl_State *L, int n) {
  sdkl_lock(L);
  api_checknelems(L, n);
  if (n > 0)
    sdklV_concat(L, n);
  else {  /* nothing to concatenate */
    setsvalue2s(L, L->top, sdklS_newlstr(L, "", 0));  /* push empty string */
    api_incr_top(L);
  }
  sdklC_checkGC(L);
  sdkl_unlock(L);
}


SDKL_API void sdkl_len (sdkl_State *L, int idx) {
  TValue *t;
  sdkl_lock(L);
  t = index2value(L, idx);
  sdklV_objlen(L, L->top, t);
  api_incr_top(L);
  sdkl_unlock(L);
}


SDKL_API sdkl_Alloc sdkl_getallocf (sdkl_State *L, void **ud) {
  sdkl_Alloc f;
  sdkl_lock(L);
  if (ud) *ud = G(L)->ud;
  f = G(L)->frealloc;
  sdkl_unlock(L);
  return f;
}


SDKL_API void sdkl_setallocf (sdkl_State *L, sdkl_Alloc f, void *ud) {
  sdkl_lock(L);
  G(L)->ud = ud;
  G(L)->frealloc = f;
  sdkl_unlock(L);
}


void sdkl_setwarnf (sdkl_State *L, sdkl_WarnFunction f, void *ud) {
  sdkl_lock(L);
  G(L)->ud_warn = ud;
  G(L)->warnf = f;
  sdkl_unlock(L);
}


void sdkl_warning (sdkl_State *L, const char *msg, int tocont) {
  sdkl_lock(L);
  sdklE_warning(L, msg, tocont);
  sdkl_unlock(L);
}



SDKL_API void *sdkl_newuserdatauv (sdkl_State *L, size_t size, int nuvalue) {
  Udata *u;
  sdkl_lock(L);
  api_check(L, 0 <= nuvalue && nuvalue < USHRT_MAX, "invalid value");
  u = sdklS_newudata(L, size, nuvalue);
  setuvalue(L, s2v(L->top), u);
  api_incr_top(L);
  sdklC_checkGC(L);
  sdkl_unlock(L);
  return getudatamem(u);
}



static const char *aux_upvalue (TValue *fi, int n, TValue **val,
                                GCObject **owner) {
  switch (ttypetag(fi)) {
    case SDKL_VCCL: {  /* C closure */
      CClosure *f = clCvalue(fi);
      if (!(cast_uint(n) - 1u < cast_uint(f->nupvalues)))
        return NULL;  /* 'n' not in [1, f->nupvalues] */
      *val = &f->upvalue[n-1];
      if (owner) *owner = obj2gco(f);
      return "";
    }
    case SDKL_VLCL: {  /* SDKL closure */
      LClosure *f = clLvalue(fi);
      TString *name;
      Proto *p = f->p;
      if (!(cast_uint(n) - 1u  < cast_uint(p->sizeupvalues)))
        return NULL;  /* 'n' not in [1, p->sizeupvalues] */
      *val = f->upvals[n-1]->v;
      if (owner) *owner = obj2gco(f->upvals[n - 1]);
      name = p->upvalues[n-1].name;
      return (name == NULL) ? "(no name)" : getstr(name);
    }
    default: return NULL;  /* not a closure */
  }
}


SDKL_API const char *sdkl_getupvalue (sdkl_State *L, int funcindex, int n) {
  const char *name;
  TValue *val = NULL;  /* to avoid warnings */
  sdkl_lock(L);
  name = aux_upvalue(index2value(L, funcindex), n, &val, NULL);
  if (name) {
    setobj2s(L, L->top, val);
    api_incr_top(L);
  }
  sdkl_unlock(L);
  return name;
}


SDKL_API const char *sdkl_setupvalue (sdkl_State *L, int funcindex, int n) {
  const char *name;
  TValue *val = NULL;  /* to avoid warnings */
  GCObject *owner = NULL;  /* to avoid warnings */
  TValue *fi;
  sdkl_lock(L);
  fi = index2value(L, funcindex);
  api_checknelems(L, 1);
  name = aux_upvalue(fi, n, &val, &owner);
  if (name) {
    L->top--;
    setobj(L, val, s2v(L->top));
    sdklC_barrier(L, owner, val);
  }
  sdkl_unlock(L);
  return name;
}


static UpVal **getupvalref (sdkl_State *L, int fidx, int n, LClosure **pf) {
  static const UpVal *const nullup = NULL;
  LClosure *f;
  TValue *fi = index2value(L, fidx);
  api_check(L, ttisLclosure(fi), "SDKL function expected");
  f = clLvalue(fi);
  if (pf) *pf = f;
  if (1 <= n && n <= f->p->sizeupvalues)
    return &f->upvals[n - 1];  /* get its upvalue pointer */
  else
    return (UpVal**)&nullup;
}


SDKL_API void *sdkl_upvalueid (sdkl_State *L, int fidx, int n) {
  TValue *fi = index2value(L, fidx);
  switch (ttypetag(fi)) {
    case SDKL_VLCL: {  /* sdkl closure */
      return *getupvalref(L, fidx, n, NULL);
    }
    case SDKL_VCCL: {  /* C closure */
      CClosure *f = clCvalue(fi);
      if (1 <= n && n <= f->nupvalues)
        return &f->upvalue[n - 1];
      /* else */
    }  /* FALLTHROUGH */
    case SDKL_VLCF:
      return NULL;  /* light C functions have no upvalues */
    default: {
      api_check(L, 0, "function expected");
      return NULL;
    }
  }
}


SDKL_API void sdkl_upvaluejoin (sdkl_State *L, int fidx1, int n1,
                                            int fidx2, int n2) {
  LClosure *f1;
  UpVal **up1 = getupvalref(L, fidx1, n1, &f1);
  UpVal **up2 = getupvalref(L, fidx2, n2, NULL);
  api_check(L, *up1 != NULL && *up2 != NULL, "invalid upvalue index");
  *up1 = *up2;
  sdklC_objbarrier(L, f1, *up1);
}


