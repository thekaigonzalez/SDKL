/*
** $Id: sdkllib.h $
** SDKL standard libraries
** See Copyright Notice in sdkl.h
*/


#ifndef sdkllib_h
#define sdkllib_h

#include "sdkl.h"


/* version suffix for environment variable names */
#define LUA_VERSUFFIX          "_" LUA_VERSION_MAJOR "_" LUA_VERSION_MINOR


LUAMOD_API int (sdklopen_base) (sdkl_State *L);

#define LUA_COLIBNAME	"coroutine"
LUAMOD_API int (sdklopen_coroutine) (sdkl_State *L);

#define LUA_TABLIBNAME	"table"
LUAMOD_API int (sdklopen_table) (sdkl_State *L);

#define LUA_IOLIBNAME	"io"
LUAMOD_API int (sdklopen_io) (sdkl_State *L);

#define LUA_OSLIBNAME	"os"
LUAMOD_API int (sdklopen_os) (sdkl_State *L);

#define LUA_STRLIBNAME	"string"
LUAMOD_API int (sdklopen_string) (sdkl_State *L);

#define LUA_UTF8LIBNAME	"utf8"
LUAMOD_API int (sdklopen_utf8) (sdkl_State *L);

#define LUA_MATHLIBNAME	"math"
LUAMOD_API int (sdklopen_math) (sdkl_State *L);

#define LUA_DBLIBNAME	"debug"
LUAMOD_API int (sdklopen_debug) (sdkl_State *L);

#define LUA_LOADLIBNAME	"package"
LUAMOD_API int (sdklopen_package) (sdkl_State *L);


/* open all previous libraries */
LUALIB_API void (sdklL_openlibs) (sdkl_State *L);


#endif
