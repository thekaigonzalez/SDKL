/*
** $Id: sdkllib.h $
** SDKL standard libraries
** See Copyright Notice in sdkl.h
*/


#ifndef sdkllib_h
#define sdkllib_h

#include "sdkl.h"


/* version suffix for environment variable names */
#define SDKL_VERSUFFIX          "_" SDKL_VERSION_MAJOR "_" SDKL_VERSION_MINOR


SDKLMOD_API int (sdklopen_base) (sdkl_State *L);

#define SDKL_COLIBNAME	"coroutine"
SDKLMOD_API int (sdklopen_coroutine) (sdkl_State *L);

#define SDKL_TABLIBNAME	"table"
SDKLMOD_API int (sdklopen_table) (sdkl_State *L);

#define SDKL_IOLIBNAME	"io"
SDKLMOD_API int (sdklopen_io) (sdkl_State *L);

#define SDKL_OSLIBNAME	"os"
SDKLMOD_API int (sdklopen_os) (sdkl_State *L);

#define SDKL_STRLIBNAME	"string"
SDKLMOD_API int (sdklopen_string) (sdkl_State *L);

#define SDKL_UTF8LIBNAME	"utf8"
SDKLMOD_API int (sdklopen_utf8) (sdkl_State *L);

#define SDKL_MATHLIBNAME	"math"
SDKLMOD_API int (sdklopen_math) (sdkl_State *L);

#define SDKL_DBLIBNAME	"debug"
SDKLMOD_API int (sdklopen_debug) (sdkl_State *L);

#define SDKL_LOADLIBNAME	"package"
SDKLMOD_API int (sdklopen_package) (sdkl_State *L);


/* open all previous libraries */
SDKLLIB_API void (sdklL_openlibs) (sdkl_State *L);


#endif
