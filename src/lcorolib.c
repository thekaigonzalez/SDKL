/*
** $Id: lcorolib.c $
** Coroutine Library
** See Copyright Notice in sdkl.h
*/

#define lcorolib_c
#define LUA_LIB

#include "lprefix.h"


#include <stdlib.h>

#include "sdkl.h"

#include "sdklauxillary.h"
#include "sdkllib.h"


static sdkl_State *getco (sdkl_State *L) {
  sdkl_State *co = sdkl_tothread(L, 1);
  sdklL_argexpected(L, co, 1, "thread");
  return co;
}


/*
** Resumes a coroutine. Returns the number of results for non-error
** cases or -1 for errors.
*/
static int auxresume (sdkl_State *L, sdkl_State *co, int narg) {
  int status, nres;
  if (l_unlikely(!sdkl_checkstack(co, narg))) {
    sdkl_pushliteral(L, "too many arguments to resume");
    return -1;  /* error flag */
  }
  sdkl_xmove(L, co, narg);
  status = sdkl_resume(co, L, narg, &nres);
  if (l_likely(status == LUA_OK || status == LUA_YIELD)) {
    if (l_unlikely(!sdkl_checkstack(L, nres + 1))) {
      sdkl_pop(co, nres);  /* remove results anyway */
      sdkl_pushliteral(L, "too many results to resume");
      return -1;  /* error flag */
    }
    sdkl_xmove(co, L, nres);  /* move yielded values */
    return nres;
  }
  else {
    sdkl_xmove(co, L, 1);  /* move error message */
    return -1;  /* error flag */
  }
}


static int sdklB_coresume (sdkl_State *L) {
  sdkl_State *co = getco(L);
  int r;
  r = auxresume(L, co, sdkl_gettop(L) - 1);
  if (l_unlikely(r < 0)) {
    sdkl_pushboolean(L, 0);
    sdkl_insert(L, -2);
    return 2;  /* return false + error message */
  }
  else {
    sdkl_pushboolean(L, 1);
    sdkl_insert(L, -(r + 1));
    return r + 1;  /* return true + 'resume' returns */
  }
}


static int sdklB_auxwrap (sdkl_State *L) {
  sdkl_State *co = sdkl_tothread(L, sdkl_upvalueindex(1));
  int r = auxresume(L, co, sdkl_gettop(L));
  if (l_unlikely(r < 0)) {  /* error? */
    int stat = sdkl_status(co);
    if (stat != LUA_OK && stat != LUA_YIELD) {  /* error in the coroutine? */
      stat = sdkl_resetthread(co);  /* close its tbc variables */
      sdkl_assert(stat != LUA_OK);
      sdkl_xmove(co, L, 1);  /* copy error message */
    }
    if (stat != LUA_ERRMEM &&  /* not a memory error and ... */
        sdkl_type(L, -1) == LUA_TSTRING) {  /* ... error object is a string? */
      sdklL_where(L, 1);  /* add extra info, if available */
      sdkl_insert(L, -2);
      sdkl_concat(L, 2);
    }
    return sdkl_error(L);  /* propagate error */
  }
  return r;
}


static int sdklB_cocreate (sdkl_State *L) {
  sdkl_State *NL;
  sdklL_checktype(L, 1, LUA_TFUNCTION);
  NL = sdkl_newthread(L);
  sdkl_pushvalue(L, 1);  /* move function to top */
  sdkl_xmove(L, NL, 1);  /* move function from L to NL */
  return 1;
}


static int sdklB_cowrap (sdkl_State *L) {
  sdklB_cocreate(L);
  sdkl_pushcclosure(L, sdklB_auxwrap, 1);
  return 1;
}


static int sdklB_yield (sdkl_State *L) {
  return sdkl_yield(L, sdkl_gettop(L));
}


#define COS_RUN		0
#define COS_DEAD	1
#define COS_YIELD	2
#define COS_NORM	3


static const char *const statname[] =
  {"running", "dead", "suspended", "normal"};


static int auxstatus (sdkl_State *L, sdkl_State *co) {
  if (L == co) return COS_RUN;
  else {
    switch (sdkl_status(co)) {
      case LUA_YIELD:
        return COS_YIELD;
      case LUA_OK: {
        sdkl_Debug ar;
        if (sdkl_getstack(co, 0, &ar))  /* does it have frames? */
          return COS_NORM;  /* it is running */
        else if (sdkl_gettop(co) == 0)
            return COS_DEAD;
        else
          return COS_YIELD;  /* initial state */
      }
      default:  /* some error occurred */
        return COS_DEAD;
    }
  }
}


static int sdklB_costatus (sdkl_State *L) {
  sdkl_State *co = getco(L);
  sdkl_pushstring(L, statname[auxstatus(L, co)]);
  return 1;
}


static int sdklB_yieldable (sdkl_State *L) {
  sdkl_State *co = sdkl_isnone(L, 1) ? L : getco(L);
  sdkl_pushboolean(L, sdkl_isyieldable(co));
  return 1;
}


static int sdklB_corunning (sdkl_State *L) {
  int ismain = sdkl_pushthread(L);
  sdkl_pushboolean(L, ismain);
  return 2;
}


static int sdklB_close (sdkl_State *L) {
  sdkl_State *co = getco(L);
  int status = auxstatus(L, co);
  switch (status) {
    case COS_DEAD: case COS_YIELD: {
      status = sdkl_resetthread(co);
      if (status == LUA_OK) {
        sdkl_pushboolean(L, 1);
        return 1;
      }
      else {
        sdkl_pushboolean(L, 0);
        sdkl_xmove(co, L, 1);  /* copy error message */
        return 2;
      }
    }
    default:  /* normal or running coroutine */
      return sdklL_error(L, "cannot close a %s coroutine", statname[status]);
  }
}


static const sdklL_Reg co_funcs[] = {
  {"create", sdklB_cocreate},
  {"resume", sdklB_coresume},
  {"running", sdklB_corunning},
  {"status", sdklB_costatus},
  {"wrap", sdklB_cowrap},
  {"yield", sdklB_yield},
  {"isyieldable", sdklB_yieldable},
  {"close", sdklB_close},
  {NULL, NULL}
};



LUAMOD_API int sdklopen_coroutine (sdkl_State *L) {
  sdklL_newlib(L, co_funcs);
  return 1;
}

