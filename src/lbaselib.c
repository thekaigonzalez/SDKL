/*
** $Id: lbaselib.c $
** Basic library
** See Copyright Notice in sdkl.h
*/

#define lbaselib_c
#define LUA_LIB

#include "lprefix.h"


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __unix__
# include <unistd.h>
#endif
#if defined _WIN32
# include <windows.h>
# define sleep(x) Sleep(1000 * (x))
#endif

# include "sdkl.h"

# include "sdklauxillary.h"
# include "sdkllib.h"
static int help(sdkl_State* L) /* use L */
{
 fprintf(stdout,
"help: Please read the following below before using. \n"
"SDKL is licensed under the MIT license, no warranty is given for this Software.\n"
"\n"
"SDKL is a Programming language inspired off of the SDKL Programming language. you can use everything in lua like SDKL.\n"
"Here's some mods to check out\n"
"- os: Operating System. Mastere de ways!\n"
"- io: IO. Stream functionalitieees\n");
sdkl_pushinteger(L, 1);
return 1;
}


static int sdklB_print (sdkl_State *L) {
  int n = sdkl_gettop(L);  /* number of arguments */
  int i;
  for (i = 1; i <= n; i++) {  /* for each argument */
    size_t l;
    const char *s = sdklL_tolstring(L, i, &l);  /* convert it to string */
    if (i > 1)  /* not the first element? */
      sdkl_writestring("\t", 1);  /* add a tab before it */
    sdkl_writestring(s, l);  /* print it */
    sdkl_pop(L, 1);  /* pop result */
  }
  sdkl_writeline();
  return 0;
}


/*
** Creates a warning with all given arguments.
** Check first for errors; otherwise an error may interrupt
** the composition of a warning, leaving it unfinished.
*/
static int sdklB_warn (sdkl_State *L) {
  int n = sdkl_gettop(L);  /* number of arguments */
  int i;
  sdklL_checkstring(L, 1);  /* at least one argument */
  for (i = 2; i <= n; i++)
    sdklL_checkstring(L, i);  /* make sure all arguments are strings */
  for (i = 1; i < n; i++)  /* compose warning */
    sdkl_warning(L, sdkl_tostring(L, i), 1);
  sdkl_warning(L, sdkl_tostring(L, n), 0);  /* close warning */
  return 0;
}


#define SPACECHARS	" \f\n\r\t\v"

static const char *b_str2int (const char *s, int base, sdkl_Integer *pn) {
  sdkl_Unsigned n = 0;
  int neg = 0;
  s += strspn(s, SPACECHARS);  /* skip initial spaces */
  if (*s == '-') { s++; neg = 1; }  /* handle sign */
  else if (*s == '+') s++;
  if (!isalnum((unsigned char)*s))  /* no digit? */
    return NULL;
  do {
    int digit = (isdigit((unsigned char)*s)) ? *s - '0'
                   : (toupper((unsigned char)*s) - 'A') + 10;
    if (digit >= base) return NULL;  /* invalid numeral */
    n = n * base + digit;
    s++;
  } while (isalnum((unsigned char)*s));
  s += strspn(s, SPACECHARS);  /* skip trailing spaces */
  *pn = (sdkl_Integer)((neg) ? (0u - n) : n);
  return s;
}


static int sdklB_tonumber (sdkl_State *L) {
  if (sdkl_isnoneornil(L, 2)) {  /* standard conversion? */
    if (sdkl_type(L, 1) == LUA_TNUMBER) {  /* already a number? */
      sdkl_settop(L, 1);  /* yes; return it */
      return 1;
    }
    else {
      size_t l;
      const char *s = sdkl_tolstring(L, 1, &l);
      if (s != NULL && sdkl_stringtonumber(L, s) == l + 1)
        return 1;  /* successful conversion to number */
      /* else not a number */
      sdklL_checkany(L, 1);  /* (but there must be some parameter) */
    }
  }
  else {
    size_t l;
    const char *s;
    sdkl_Integer n = 0;  /* to avoid warnings */
    sdkl_Integer base = sdklL_checkinteger(L, 2);
    sdklL_checktype(L, 1, LUA_TSTRING);  /* no numbers as strings */
    s = sdkl_tolstring(L, 1, &l);
    sdklL_argcheck(L, 2 <= base && base <= 36, 2, "base out of range");
    if (b_str2int(s, (int)base, &n) == s + l) {
      sdkl_pushinteger(L, n);
      return 1;
    }  /* else not a number */
  }  /* else not a number */
  sdklL_pushfail(L);  /* not a number */
  return 1;
}


static int sdklB_error (sdkl_State *L) {
  int level = (int)sdklL_optinteger(L, 2, 1);
  sdkl_settop(L, 1);
  if (sdkl_type(L, 1) == LUA_TSTRING && level > 0) {
    sdklL_where(L, level);   /* add extra information */
    sdkl_pushvalue(L, 1);
    sdkl_concat(L, 2);
  }
  return sdkl_error(L);
}


static int sdklB_getmetatable (sdkl_State *L) {
  sdklL_checkany(L, 1);
  if (!sdkl_getmetatable(L, 1)) {
    sdkl_pushnil(L);
    return 1;  /* no metatable */
  }
  sdklL_getmetafield(L, 1, "__metatable");
  return 1;  /* returns either __metatable field (if present) or metatable */
}


static int sdklB_setmetatable (sdkl_State *L) {
  int t = sdkl_type(L, 2);
  sdklL_checktype(L, 1, LUA_TTABLE);
  sdklL_argexpected(L, t == LUA_TNIL || t == LUA_TTABLE, 2, "nil or table");
  if (l_unlikely(sdklL_getmetafield(L, 1, "__metatable") != LUA_TNIL))
    return sdklL_error(L, "cannot change a protected metatable");
  sdkl_settop(L, 2);
  sdkl_setmetatable(L, 1);
  return 1;
}


static int sdklB_rawequal (sdkl_State *L) {
  sdklL_checkany(L, 1);
  sdklL_checkany(L, 2);
  sdkl_pushboolean(L, sdkl_rawequal(L, 1, 2));
  return 1;
}


static int sdklB_rawlen (sdkl_State *L) {
  int t = sdkl_type(L, 1);
  sdklL_argexpected(L, t == LUA_TTABLE || t == LUA_TSTRING, 1,
                      "table or string");
  sdkl_pushinteger(L, sdkl_rawlen(L, 1));
  return 1;
}


static int sdklB_rawget (sdkl_State *L) {
  sdklL_checktype(L, 1, LUA_TTABLE);
  sdklL_checkany(L, 2);
  sdkl_settop(L, 2);
  sdkl_rawget(L, 1);
  return 1;
}

static int sdklB_rawset (sdkl_State *L) {
  sdklL_checktype(L, 1, LUA_TTABLE);
  sdklL_checkany(L, 2);
  sdklL_checkany(L, 3);
  sdkl_settop(L, 3);
  sdkl_rawset(L, 1);
  return 1;
}


static int pushmode (sdkl_State *L, int oldmode) {
  sdkl_pushstring(L, (oldmode == LUA_GCINC) ? "incremental"
                                           : "generational");
  return 1;
}


static int sdklB_collectgarbage (sdkl_State *L) {
  static const char *const opts[] = {"stop", "restart", "collect",
    "count", "step", "setpause", "setstepmul",
    "isrunning", "generational", "incremental", NULL};
  static const int optsnum[] = {LUA_GCSTOP, LUA_GCRESTART, LUA_GCCOLLECT,
    LUA_GCCOUNT, LUA_GCSTEP, LUA_GCSETPAUSE, LUA_GCSETSTEPMUL,
    LUA_GCISRUNNING, LUA_GCGEN, LUA_GCINC};
  int o = optsnum[sdklL_checkoption(L, 1, "collect", opts)];
  switch (o) {
    case LUA_GCCOUNT: {
      int k = sdkl_gc(L, o);
      int b = sdkl_gc(L, LUA_GCCOUNTB);
      sdkl_pushnumber(L, (sdkl_Number)k + ((sdkl_Number)b/1024));
      return 1;
    }
    case LUA_GCSTEP: {
      int step = (int)sdklL_optinteger(L, 2, 0);
      int res = sdkl_gc(L, o, step);
      sdkl_pushboolean(L, res);
      return 1;
    }
    case LUA_GCSETPAUSE:
    case LUA_GCSETSTEPMUL: {
      int p = (int)sdklL_optinteger(L, 2, 0);
      int previous = sdkl_gc(L, o, p);
      sdkl_pushinteger(L, previous);
      return 1;
    }
    case LUA_GCISRUNNING: {
      int res = sdkl_gc(L, o);
      sdkl_pushboolean(L, res);
      return 1;
    }
    case LUA_GCGEN: {
      int minormul = (int)sdklL_optinteger(L, 2, 0);
      int majormul = (int)sdklL_optinteger(L, 3, 0);
      return pushmode(L, sdkl_gc(L, o, minormul, majormul));
    }
    case LUA_GCINC: {
      int pause = (int)sdklL_optinteger(L, 2, 0);
      int stepmul = (int)sdklL_optinteger(L, 3, 0);
      int stepsize = (int)sdklL_optinteger(L, 4, 0);
      return pushmode(L, sdkl_gc(L, o, pause, stepmul, stepsize));
    }
    default: {
      int res = sdkl_gc(L, o);
      sdkl_pushinteger(L, res);
      return 1;
    }
  }
}


static int sdklB_type (sdkl_State *L) {
  int t = sdkl_type(L, 1);
  sdklL_argcheck(L, t != LUA_TNONE, 1, "value expected");
  sdkl_pushstring(L, sdkl_typename(L, t));
  return 1;
}


static int sdklB_next (sdkl_State *L) {
  sdklL_checktype(L, 1, LUA_TTABLE);
  sdkl_settop(L, 2);  /* create a 2nd argument if there isn't one */
  if (sdkl_next(L, 1))
    return 2;
  else {
    sdkl_pushnil(L);
    return 1;
  }
}


static int sdklB_pairs (sdkl_State *L) {
  sdklL_checkany(L, 1);
  if (sdklL_getmetafield(L, 1, "__pairs") == LUA_TNIL) {  /* no metamethod? */
    sdkl_pushcfunction(L, sdklB_next);  /* will return generator, */
    sdkl_pushvalue(L, 1);  /* state, */
    sdkl_pushnil(L);  /* and initial value */
  }
  else {
    sdkl_pushvalue(L, 1);  /* argument 'self' to metamethod */
    sdkl_call(L, 1, 3);  /* get 3 values from metamethod */
  }
  return 3;
}


/*
** Traversal function for 'ipairs'
*/
static int ipairsaux (sdkl_State *L) {
  sdkl_Integer i = sdklL_checkinteger(L, 2) + 1;
  sdkl_pushinteger(L, i);
  return (sdkl_geti(L, 1, i) == LUA_TNIL) ? 1 : 2;
}


/*
** 'ipairs' function. Returns 'ipairsaux', given "table", 0.
** (The given "table" may not be a table.)
*/
static int sdklB_ipairs (sdkl_State *L) {
  sdklL_checkany(L, 1);
  sdkl_pushcfunction(L, ipairsaux);  /* iteration function */
  sdkl_pushvalue(L, 1);  /* state */
  sdkl_pushinteger(L, 0);  /* initial value */
  return 3;
}


static int load_aux (sdkl_State *L, int status, int envidx) {
  if (l_likely(status == LUA_OK)) {
    if (envidx != 0) {  /* 'env' parameter? */
      sdkl_pushvalue(L, envidx);  /* environment for loaded function */
      if (!sdkl_setupvalue(L, -2, 1))  /* set it as 1st upvalue */
        sdkl_pop(L, 1);  /* remove 'env' if not used by previous call */
    }
    return 1;
  }
  else {  /* error (message is on top of the stack) */
    sdklL_pushfail(L);
    sdkl_insert(L, -2);  /* put before error message */
    return 2;  /* return fail plus error message */
  }
}


static int sdklB_loadfile (sdkl_State *L) {
  const char *fname = sdklL_optstring(L, 1, NULL);
  const char *mode = sdklL_optstring(L, 2, NULL);
  int env = (!sdkl_isnone(L, 3) ? 3 : 0);  /* 'env' index or 0 if no 'env' */
  int status = sdklL_loadfilex(L, fname, mode);
  return load_aux(L, status, env);
}


/*
** {======================================================
** Generic Read function
** =======================================================
*/


/*
** reserved slot, above all arguments, to hold a copy of the returned
** string to avoid it being collected while parsed. 'load' has four
** optional arguments (chunk, source name, mode, and environment).
*/
#define RESERVEDSLOT	5


/*
** Reader for generic 'load' function: 'sdkl_load' uses the
** stack for internal stuff, so the reader cannot change the
** stack top. Instead, it keeps its resulting string in a
** reserved slot inside the stack.
*/
static const char *generic_reader (sdkl_State *L, void *ud, size_t *size) {
  (void)(ud);  /* not used */
  sdklL_checkstack(L, 2, "too many nested functions");
  sdkl_pushvalue(L, 1);  /* get function */
  sdkl_call(L, 0, 1);  /* call it */
  if (sdkl_isnil(L, -1)) {
    sdkl_pop(L, 1);  /* pop result */
    *size = 0;
    return NULL;
  }
  else if (l_unlikely(!sdkl_isstring(L, -1)))
    sdklL_error(L, "reader function must return a string");
  sdkl_replace(L, RESERVEDSLOT);  /* save string in reserved slot */
  return sdkl_tolstring(L, RESERVEDSLOT, size);
}


static int sdklB_load (sdkl_State *L) {
  int status;
  size_t l;
  const char *s = sdkl_tolstring(L, 1, &l);
  const char *mode = sdklL_optstring(L, 3, "bt");
  int env = (!sdkl_isnone(L, 4) ? 4 : 0);  /* 'env' index or 0 if no 'env' */
  if (s != NULL) {  /* loading a string? */
    const char *chunkname = sdklL_optstring(L, 2, s);
    status = sdklL_loadbufferx(L, s, l, chunkname, mode);
  }
  else {  /* loading from a reader function */
    const char *chunkname = sdklL_optstring(L, 2, "=(load)");
    sdklL_checktype(L, 1, LUA_TFUNCTION);
    sdkl_settop(L, RESERVEDSLOT);  /* create reserved slot */
    status = sdkl_load(L, generic_reader, NULL, chunkname, mode);
  }
  return load_aux(L, status, env);
}

/* }====================================================== */


static int dofilecont (sdkl_State *L, int d1, sdkl_KContext d2) {
  (void)d1;  (void)d2;  /* only to match 'sdkl_Kfunction' prototype */
  return sdkl_gettop(L) - 1;
}


static int sdklB_dofile (sdkl_State *L) {
  const char *fname = sdklL_optstring(L, 1, NULL);
  sdkl_settop(L, 1);
  if (l_unlikely(sdklL_loadfile(L, fname) != LUA_OK))
    return sdkl_error(L);
  sdkl_callk(L, 0, LUA_MULTRET, 0, dofilecont);
  return dofilecont(L, 0, 0);
}


static int sdklB_assert (sdkl_State *L) {
  if (l_likely(sdkl_toboolean(L, 1)))  /* condition is true? */
    return sdkl_gettop(L);  /* return all arguments */
  else {  /* error */
    sdklL_checkany(L, 1);  /* there must be a condition */
    sdkl_remove(L, 1);  /* remove it */
    sdkl_pushliteral(L, "assertion failed!");  /* default message */
    sdkl_settop(L, 1);  /* leave only message (default if no other one) */
    return sdklB_error(L);  /* call 'error' */
  }
}


static int sdklB_select (sdkl_State *L) {
  int n = sdkl_gettop(L);
  if (sdkl_type(L, 1) == LUA_TSTRING && *sdkl_tostring(L, 1) == '#') {
    sdkl_pushinteger(L, n-1);
    return 1;
  }
  else {
    sdkl_Integer i = sdklL_checkinteger(L, 1);
    if (i < 0) i = n + i;
    else if (i > n) i = n;
    sdklL_argcheck(L, 1 <= i, 1, "index out of range");
    return n - (int)i;
  }
}


/*
** Continuation function for 'pcall' and 'xpcall'. Both functions
** already pushed a 'true' before doing the call, so in case of success
** 'finishpcall' only has to return everything in the stack minus
** 'extra' values (where 'extra' is exactly the number of items to be
** ignored).
*/
static int finishpcall (sdkl_State *L, int status, sdkl_KContext extra) {
  if (l_unlikely(status != LUA_OK && status != LUA_YIELD)) {  /* error? */
    sdkl_pushboolean(L, 0);  /* first result (false) */
    sdkl_pushvalue(L, -2);  /* error message */
    return 2;  /* return false, msg */
  }
  else
    return sdkl_gettop(L) - (int)extra;  /* return all results */
}


static int sdklB_pcall (sdkl_State *L) {
  int status;
  sdklL_checkany(L, 1);
  sdkl_pushboolean(L, 1);  /* first result if no errors */
  sdkl_insert(L, 1);  /* put it in place */
  status = sdkl_pcallk(L, sdkl_gettop(L) - 2, LUA_MULTRET, 0, 0, finishpcall);
  return finishpcall(L, status, 0);
}


/*
** Do a protected call with error handling. After 'sdkl_rotate', the
** stack will have <f, err, true, f, [args...]>; so, the function passes
** 2 to 'finishpcall' to skip the 2 first values when returning results.
*/
static int sdklB_xpcall (sdkl_State *L) {
  int status;
  int n = sdkl_gettop(L);
  sdklL_checktype(L, 2, LUA_TFUNCTION);  /* check error function */
  sdkl_pushboolean(L, 1);  /* first result */
  sdkl_pushvalue(L, 1);  /* function */
  sdkl_rotate(L, 3, 2);  /* move them below function's arguments */
  status = sdkl_pcallk(L, n - 2, LUA_MULTRET, 2, 2, finishpcall);
  return finishpcall(L, status, 2);
}


static int sdklB_tostring (sdkl_State *L) {
  sdklL_checkany(L, 1);
  sdklL_tolstring(L, 1, NULL);
  return 1;
}

static int sdklB_wait(sdkl_State* L)
{
  int time = sdklL_checkinteger(L, 1);
  sleep(time);
  return 1;
}

static const sdklL_Reg base_funcs[] = {
  {"assert", sdklB_assert},
  {"collectgarbage", sdklB_collectgarbage},
  {"dofile", sdklB_dofile},
  {"error", sdklB_error},
  {"getmetatable", sdklB_getmetatable},
  {"ipairs", sdklB_ipairs},
  {"loadfile", sdklB_loadfile},
  {"load", sdklB_load},
  {"next", sdklB_next},
  {"pairs", sdklB_pairs},
  {"pcall", sdklB_pcall},
  {"print", sdklB_print},
  {"warn", sdklB_warn},
  {"rawequal", sdklB_rawequal},
  {"rawlen", sdklB_rawlen},
  {"println", sdklB_print},
  {"exec", sdklB_load},
  {"execfile", sdklB_dofile},
  {"rawget", sdklB_rawget},
  {"rawset", sdklB_rawset},
  {"select", sdklB_select},
  {"wait", sdklB_wait},
  {"help", help},
  {"setmetatable", sdklB_setmetatable},
  {"tonumber", sdklB_tonumber},
  {"tostring", sdklB_tostring},
  {"type", sdklB_type},
  {"xpcall", sdklB_xpcall},
  /* placeholders */
  {LUA_GNAME, NULL},
  {"_VERSION", NULL},
  {NULL, NULL}
};


LUAMOD_API int sdklopen_base (sdkl_State *L) {
  /* open lib into global table */
  sdkl_pushglobaltable(L);
  sdklL_setfuncs(L, base_funcs, 0);
  /* set global _G */
  sdkl_pushvalue(L, -1);
  sdkl_setfield(L, -2, LUA_GNAME);
  /* set global _VERSION */
  sdkl_pushliteral(L, LUA_VERSION);
  sdkl_setfield(L, -2, "_VERSION");
  return 1;
}

