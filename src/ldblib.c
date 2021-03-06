/*
** $Id: ldblib.c $
** Interface from SDKL to its debug API
** See Copyright Notice in sdkl.h
*/

#define ldblib_c
#define SDKL_LIB

#include "lprefix.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sdkl.h"

#include "sdklauxillary.h"
#include "sdkllib.h"


/*
** The hook table at registry[HOOKKEY] maps threads to their current
** hook function.
*/
static const char *const HOOKKEY = "_HOOKKEY";


/*
** If L1 != L, L1 can be in any state, and therefore there are no
** guarantees about its stack space; any push in L1 must be
** checked.
*/
static void checkstack (sdkl_State *L, sdkl_State *L1, int n) {
  if (l_unlikely(L != L1 && !sdkl_checkstack(L1, n)))
    sdklL_error(L, "stack overflow");
}


static int db_getregistry (sdkl_State *L) {
  sdkl_pushvalue(L, SDKL_REGISTRYINDEX);
  return 1;
}


static int db_getmetatable (sdkl_State *L) {
  sdklL_checkany(L, 1);
  if (!sdkl_getmetatable(L, 1)) {
    sdkl_pushnil(L);  /* no metatable */
  }
  return 1;
}


static int db_setmetatable (sdkl_State *L) {
  int t = sdkl_type(L, 2);
  sdklL_argexpected(L, t == SDKL_TNIL || t == SDKL_TTABLE, 2, "nil or table");
  sdkl_settop(L, 2);
  sdkl_setmetatable(L, 1);
  return 1;  /* return 1st argument */
}


static int db_getuservalue (sdkl_State *L) {
  int n = (int)sdklL_optinteger(L, 2, 1);
  if (sdkl_type(L, 1) != SDKL_TUSERDATA)
    sdklL_pushfail(L);
  else if (sdkl_getiuservalue(L, 1, n) != SDKL_TNONE) {
    sdkl_pushboolean(L, 1);
    return 2;
  }
  return 1;
}


static int db_setuservalue (sdkl_State *L) {
  int n = (int)sdklL_optinteger(L, 3, 1);
  sdklL_checktype(L, 1, SDKL_TUSERDATA);
  sdklL_checkany(L, 2);
  sdkl_settop(L, 2);
  if (!sdkl_setiuservalue(L, 1, n))
    sdklL_pushfail(L);
  return 1;
}


/*
** Auxiliary function used by several library functions: check for
** an optional thread as function's first argument and set 'arg' with
** 1 if this argument is present (so that functions can skip it to
** access their other arguments)
*/
static sdkl_State *getthread (sdkl_State *L, int *arg) {
  if (sdkl_isthread(L, 1)) {
    *arg = 1;
    return sdkl_tothread(L, 1);
  }
  else {
    *arg = 0;
    return L;  /* function will operate over current thread */
  }
}


/*
** Variations of 'sdkl_settable', used by 'db_getinfo' to put results
** from 'sdkl_getinfo' into result table. Key is always a string;
** value can be a string, an int, or a boolean.
*/
static void settabss (sdkl_State *L, const char *k, const char *v) {
  sdkl_pushstring(L, v);
  sdkl_setfield(L, -2, k);
}

static void settabsi (sdkl_State *L, const char *k, int v) {
  sdkl_pushinteger(L, v);
  sdkl_setfield(L, -2, k);
}

static void settabsb (sdkl_State *L, const char *k, int v) {
  sdkl_pushboolean(L, v);
  sdkl_setfield(L, -2, k);
}


/*
** In function 'db_getinfo', the call to 'sdkl_getinfo' may push
** results on the stack; later it creates the result table to put
** these objects. Function 'treatstackoption' puts the result from
** 'sdkl_getinfo' on top of the result table so that it can call
** 'sdkl_setfield'.
*/
static void treatstackoption (sdkl_State *L, sdkl_State *L1, const char *fname) {
  if (L == L1)
    sdkl_rotate(L, -2, 1);  /* exchange object and table */
  else
    sdkl_xmove(L1, L, 1);  /* move object to the "main" stack */
  sdkl_setfield(L, -2, fname);  /* put object into table */
}


/*
** Calls 'sdkl_getinfo' and collects all results in a new table.
** L1 needs stack space for an optional input (function) plus
** two optional outputs (function and line table) from function
** 'sdkl_getinfo'.
*/
static int db_getinfo (sdkl_State *L) {
  sdkl_Debug ar;
  int arg;
  sdkl_State *L1 = getthread(L, &arg);
  const char *options = sdklL_optstring(L, arg+2, "flnSrtu");
  checkstack(L, L1, 3);
  sdklL_argcheck(L, options[0] != '>', arg + 2, "invalid option '>'");
  if (sdkl_isfunction(L, arg + 1)) {  /* info about a function? */
    options = sdkl_pushfstring(L, ">%s", options);  /* add '>' to 'options' */
    sdkl_pushvalue(L, arg + 1);  /* move function to 'L1' stack */
    sdkl_xmove(L, L1, 1);
  }
  else {  /* stack level */
    if (!sdkl_getstack(L1, (int)sdklL_checkinteger(L, arg + 1), &ar)) {
      sdklL_pushfail(L);  /* level out of range */
      return 1;
    }
  }
  if (!sdkl_getinfo(L1, options, &ar))
    return sdklL_argerror(L, arg+2, "invalid option");
  sdkl_newtable(L);  /* table to collect results */
  if (strchr(options, 'S')) {
    sdkl_pushlstring(L, ar.source, ar.srclen);
    sdkl_setfield(L, -2, "source");
    settabss(L, "short_src", ar.short_src);
    settabsi(L, "linedefined", ar.linedefined);
    settabsi(L, "lastlinedefined", ar.lastlinedefined);
    settabss(L, "what", ar.what);
  }
  if (strchr(options, 'l'))
    settabsi(L, "currentline", ar.currentline);
  if (strchr(options, 'u')) {
    settabsi(L, "nups", ar.nups);
    settabsi(L, "nparams", ar.nparams);
    settabsb(L, "isvararg", ar.isvararg);
  }
  if (strchr(options, 'n')) {
    settabss(L, "name", ar.name);
    settabss(L, "namewhat", ar.namewhat);
  }
  if (strchr(options, 'r')) {
    settabsi(L, "ftransfer", ar.ftransfer);
    settabsi(L, "ntransfer", ar.ntransfer);
  }
  if (strchr(options, 't'))
    settabsb(L, "istailcall", ar.istailcall);
  if (strchr(options, 'L'))
    treatstackoption(L, L1, "activelines");
  if (strchr(options, 'f'))
    treatstackoption(L, L1, "func");
  return 1;  /* return table */
}


static int db_getlocal (sdkl_State *L) {
  int arg;
  sdkl_State *L1 = getthread(L, &arg);
  int nvar = (int)sdklL_checkinteger(L, arg + 2);  /* local-variable index */
  if (sdkl_isfunction(L, arg + 1)) {  /* function argument? */
    sdkl_pushvalue(L, arg + 1);  /* push function */
    sdkl_pushstring(L, sdkl_getlocal(L, NULL, nvar));  /* push local name */
    return 1;  /* return only name (there is no value) */
  }
  else {  /* stack-level argument */
    sdkl_Debug ar;
    const char *name;
    int level = (int)sdklL_checkinteger(L, arg + 1);
    if (l_unlikely(!sdkl_getstack(L1, level, &ar)))  /* out of range? */
      return sdklL_argerror(L, arg+1, "level out of range");
    checkstack(L, L1, 1);
    name = sdkl_getlocal(L1, &ar, nvar);
    if (name) {
      sdkl_xmove(L1, L, 1);  /* move local value */
      sdkl_pushstring(L, name);  /* push name */
      sdkl_rotate(L, -2, 1);  /* re-order */
      return 2;
    }
    else {
      sdklL_pushfail(L);  /* no name (nor value) */
      return 1;
    }
  }
}


static int db_setlocal (sdkl_State *L) {
  int arg;
  const char *name;
  sdkl_State *L1 = getthread(L, &arg);
  sdkl_Debug ar;
  int level = (int)sdklL_checkinteger(L, arg + 1);
  int nvar = (int)sdklL_checkinteger(L, arg + 2);
  if (l_unlikely(!sdkl_getstack(L1, level, &ar)))  /* out of range? */
    return sdklL_argerror(L, arg+1, "level out of range");
  sdklL_checkany(L, arg+3);
  sdkl_settop(L, arg+3);
  checkstack(L, L1, 1);
  sdkl_xmove(L, L1, 1);
  name = sdkl_setlocal(L1, &ar, nvar);
  if (name == NULL)
    sdkl_pop(L1, 1);  /* pop value (if not popped by 'sdkl_setlocal') */
  sdkl_pushstring(L, name);
  return 1;
}


/*
** get (if 'get' is true) or set an upvalue from a closure
*/
static int auxupvalue (sdkl_State *L, int get) {
  const char *name;
  int n = (int)sdklL_checkinteger(L, 2);  /* upvalue index */
  sdklL_checktype(L, 1, SDKL_TFUNCTION);  /* closure */
  name = get ? sdkl_getupvalue(L, 1, n) : sdkl_setupvalue(L, 1, n);
  if (name == NULL) return 0;
  sdkl_pushstring(L, name);
  sdkl_insert(L, -(get+1));  /* no-op if get is false */
  return get + 1;
}


static int db_getupvalue (sdkl_State *L) {
  return auxupvalue(L, 1);
}


static int db_setupvalue (sdkl_State *L) {
  sdklL_checkany(L, 3);
  return auxupvalue(L, 0);
}


/*
** Check whether a given upvalue from a given closure exists and
** returns its index
*/
static void *checkupval (sdkl_State *L, int argf, int argnup, int *pnup) {
  void *id;
  int nup = (int)sdklL_checkinteger(L, argnup);  /* upvalue index */
  sdklL_checktype(L, argf, SDKL_TFUNCTION);  /* closure */
  id = sdkl_upvalueid(L, argf, nup);
  if (pnup) {
    sdklL_argcheck(L, id != NULL, argnup, "invalid upvalue index");
    *pnup = nup;
  }
  return id;
}


static int db_upvalueid (sdkl_State *L) {
  void *id = checkupval(L, 1, 2, NULL);
  if (id != NULL)
    sdkl_pushlightuserdata(L, id);
  else
    sdklL_pushfail(L);
  return 1;
}


static int db_upvaluejoin (sdkl_State *L) {
  int n1, n2;
  checkupval(L, 1, 2, &n1);
  checkupval(L, 3, 4, &n2);
  sdklL_argcheck(L, !sdkl_iscfunction(L, 1), 1, "SDKL function expected");
  sdklL_argcheck(L, !sdkl_iscfunction(L, 3), 3, "SDKL function expected");
  sdkl_upvaluejoin(L, 1, n1, 3, n2);
  return 0;
}


/*
** Call hook function registered at hook table for the current
** thread (if there is one)
*/
static void hookf (sdkl_State *L, sdkl_Debug *ar) {
  static const char *const hooknames[] =
    {"call", "return", "line", "count", "tail call"};
  sdkl_getfield(L, SDKL_REGISTRYINDEX, HOOKKEY);
  sdkl_pushthread(L);
  if (sdkl_rawget(L, -2) == SDKL_TFUNCTION) {  /* is there a hook function? */
    sdkl_pushstring(L, hooknames[(int)ar->event]);  /* push event name */
    if (ar->currentline >= 0)
      sdkl_pushinteger(L, ar->currentline);  /* push current line */
    else sdkl_pushnil(L);
    sdkl_assert(sdkl_getinfo(L, "lS", ar));
    sdkl_call(L, 2, 0);  /* call hook function */
  }
}


/*
** Convert a string mask (for 'sethook') into a bit mask
*/
static int makemask (const char *smask, int count) {
  int mask = 0;
  if (strchr(smask, 'c')) mask |= SDKL_MASKCALL;
  if (strchr(smask, 'r')) mask |= SDKL_MASKRET;
  if (strchr(smask, 'l')) mask |= SDKL_MASKLINE;
  if (count > 0) mask |= SDKL_MASKCOUNT;
  return mask;
}


/*
** Convert a bit mask (for 'gethook') into a string mask
*/
static char *unmakemask (int mask, char *smask) {
  int i = 0;
  if (mask & SDKL_MASKCALL) smask[i++] = 'c';
  if (mask & SDKL_MASKRET) smask[i++] = 'r';
  if (mask & SDKL_MASKLINE) smask[i++] = 'l';
  smask[i] = '\0';
  return smask;
}


static int db_sethook (sdkl_State *L) {
  int arg, mask, count;
  sdkl_Hook func;
  sdkl_State *L1 = getthread(L, &arg);
  if (sdkl_isnoneornil(L, arg+1)) {  /* no hook? */
    sdkl_settop(L, arg+1);
    func = NULL; mask = 0; count = 0;  /* turn off hooks */
  }
  else {
    const char *smask = sdklL_checkstring(L, arg+2);
    sdklL_checktype(L, arg+1, SDKL_TFUNCTION);
    count = (int)sdklL_optinteger(L, arg + 3, 0);
    func = hookf; mask = makemask(smask, count);
  }
  if (!sdklL_getsubtable(L, SDKL_REGISTRYINDEX, HOOKKEY)) {
    /* table just created; initialize it */
    sdkl_pushliteral(L, "k");
    sdkl_setfield(L, -2, "__mode");  /** hooktable.__mode = "k" */
    sdkl_pushvalue(L, -1);
    sdkl_setmetatable(L, -2);  /* metatable(hooktable) = hooktable */
  }
  checkstack(L, L1, 1);
  sdkl_pushthread(L1); sdkl_xmove(L1, L, 1);  /* key (thread) */
  sdkl_pushvalue(L, arg + 1);  /* value (hook function) */
  sdkl_rawset(L, -3);  /* hooktable[L1] = new SDKL hook */
  sdkl_sethook(L1, func, mask, count);
  return 0;
}


static int db_gethook (sdkl_State *L) {
  int arg;
  sdkl_State *L1 = getthread(L, &arg);
  char buff[5];
  int mask = sdkl_gethookmask(L1);
  sdkl_Hook hook = sdkl_gethook(L1);
  if (hook == NULL) {  /* no hook? */
    sdklL_pushfail(L);
    return 1;
  }
  else if (hook != hookf)  /* external hook? */
    sdkl_pushliteral(L, "external hook");
  else {  /* hook table must exist */
    sdkl_getfield(L, SDKL_REGISTRYINDEX, HOOKKEY);
    checkstack(L, L1, 1);
    sdkl_pushthread(L1); sdkl_xmove(L1, L, 1);
    sdkl_rawget(L, -2);   /* 1st result = hooktable[L1] */
    sdkl_remove(L, -2);  /* remove hook table */
  }
  sdkl_pushstring(L, unmakemask(mask, buff));  /* 2nd result = mask */
  sdkl_pushinteger(L, sdkl_gethookcount(L1));  /* 3rd result = count */
  return 3;
}


static int db_debug (sdkl_State *L) {
  for (;;) {
    char buffer[250];
    sdkl_writestringerror("%s", "sdkl_debug> ");
    if (fgets(buffer, sizeof(buffer), stdin) == NULL ||
        strcmp(buffer, "cont\n") == 0)
      return 0;
    if (sdklL_loadbuffer(L, buffer, strlen(buffer), "=(debug command)") ||
        sdkl_pcall(L, 0, 0, 0))
      sdkl_writestringerror("%s\n", sdklL_tolstring(L, -1, NULL));
    sdkl_settop(L, 0);  /* remove eventual returns */
  }
}


static int db_traceback (sdkl_State *L) {
  int arg;
  sdkl_State *L1 = getthread(L, &arg);
  const char *msg = sdkl_tostring(L, arg + 1);
  if (msg == NULL && !sdkl_isnoneornil(L, arg + 1))  /* non-string 'msg'? */
    sdkl_pushvalue(L, arg + 1);  /* return it untouched */
  else {
    int level = (int)sdklL_optinteger(L, arg + 2, (L == L1) ? 1 : 0);
    sdklL_traceback(L, L1, msg, level);
  }
  return 1;
}


static int db_setcstacklimit (sdkl_State *L) {
  int limit = (int)sdklL_checkinteger(L, 1);
  int res = sdkl_setcstacklimit(L, limit);
  sdkl_pushinteger(L, res);
  return 1;
}


static const sdklL_Reg dblib[] = {
  {"debug", db_debug},
  {"getuservalue", db_getuservalue},
  {"gethook", db_gethook},
  {"getinfo", db_getinfo},
  {"getlocal", db_getlocal},
  {"getregistry", db_getregistry},
  {"getmetatable", db_getmetatable},
  {"getupvalue", db_getupvalue},
  {"upvaluejoin", db_upvaluejoin},
  {"upvalueid", db_upvalueid},
  {"setuservalue", db_setuservalue},
  {"sethook", db_sethook},
  {"setlocal", db_setlocal},
  {"setmetatable", db_setmetatable},
  {"setupvalue", db_setupvalue},
  {"traceback", db_traceback},
  {"setcstacklimit", db_setcstacklimit},
  {NULL, NULL}
};


SDKLMOD_API int sdklopen_debug (sdkl_State *L) {
  sdklL_newlib(L, dblib);
  return 1;
}

