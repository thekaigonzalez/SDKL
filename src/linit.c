/*
** $Id: linit.c $
** Initialization of libraries for sdkl.c and other clients
** See Copyright Notice in sdkl.h
*/


#define linit_c
#define SDKL_LIB

/*
** If you embed SDKL in your program and need to open the standard
** libraries, call sdklL_openlibs in your program. If you need a
** different set of libraries, copy this file to your project and edit
** it to suit your needs.
**
** You can also *preload* libraries, so that a later 'require' can
** open the library, which is already linked to the application.
** For that, do the following code:
**
**  sdklL_getsubtable(L, SDKL_REGISTRYINDEX, SDKL_PRELOAD_TABLE);
**  sdkl_pushcfunction(L, sdklopen_modname);
**  sdkl_setfield(L, -2, modname);
**  sdkl_pop(L, 1);  // remove PRELOAD table
*/

#include "lprefix.h"


#include <stddef.h>

#include "sdkl.h"

#include "sdkllib.h"
#include "sdklauxillary.h"


/*
** these libs are loaded by sdkl.c and are readily available to any SDKL
** program
*/
static const sdklL_Reg loadedlibs[] = {
  {SDKL_GNAME, sdklopen_base},
  {SDKL_LOADLIBNAME, sdklopen_package},
  {SDKL_COLIBNAME, sdklopen_coroutine},
  {SDKL_TABLIBNAME, sdklopen_table},
  {SDKL_IOLIBNAME, sdklopen_io},
  {SDKL_OSLIBNAME, sdklopen_os},
  {SDKL_STRLIBNAME, sdklopen_string},
  {SDKL_MATHLIBNAME, sdklopen_math},
  {SDKL_UTF8LIBNAME, sdklopen_utf8},
  {SDKL_DBLIBNAME, sdklopen_debug},
  {NULL, NULL}
};


SDKLLIB_API void sdklL_openlibs (sdkl_State *L) {
  const sdklL_Reg *lib;
  /* "require" functions from 'loadedlibs' and set results to global table */
  for (lib = loadedlibs; lib->func; lib++) {
    sdklL_requiref(L, lib->name, lib->func, 1);
    sdkl_pop(L, 1);  /* remove lib */
  }
}

