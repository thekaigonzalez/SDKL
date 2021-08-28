/*
** $Id: loadlib.c $
** Dynamic library loader for SDKL
** See Copyright Notice in sdkl.h
**
** This module contains an implementation of loadlib for Unix systems
** that have dlfcn, an implementation for Windows, and a stub for other
** systems.
*/

#define loadlib_c
#define SDKL_LIB

#include "lprefix.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sdkl.h"

#include "sdklauxillary.h"
#include "sdkllib.h"


/*
** SDKL_IGMARK is a mark to ignore all before it when building the
** sdklopen_ function name.
*/
#if !defined (SDKL_IGMARK)
#define SDKL_IGMARK		"-"
#endif


/*
** SDKL_CSUBSEP is the character that replaces dots in submodule names
** when searching for a C loader.
** SDKL_LSUBSEP is the character that replaces dots in submodule names
** when searching for a SDKL loader.
*/
#if !defined(SDKL_CSUBSEP)
#define SDKL_CSUBSEP		SDKL_DIRSEP
#endif

#if !defined(SDKL_LSUBSEP)
#define SDKL_LSUBSEP		SDKL_DIRSEP
#endif


/* prefix for open functions in C libraries */
#define SDKL_POF		"sdklopen_"

/* separator for open functions in C libraries */
#define SDKL_OFSEP	"_"


/*
** key for table in the registry that keeps handles
** for all loaded C libraries
*/
static const char *const CLIBS = "_CLIBS";

#define LIB_FAIL	"open"


#define setprogdir(L)           ((void)0)


/*
** Special type equivalent to '(void*)' for functions in gcc
** (to suppress warnings when converting function pointers)
*/
typedef void (*voidf)(void);


/*
** system-dependent functions
*/

/*
** unload library 'lib'
*/
static void lsys_unloadlib (void *lib);

/*
** load C library in file 'path'. If 'seeglb', load with all names in
** the library global.
** Returns the library; in case of error, returns NULL plus an
** error string in the stack.
*/
static void *lsys_load (sdkl_State *L, const char *path, int seeglb);

/*
** Try to find a function named 'sym' in library 'lib'.
** Returns the function; in case of error, returns NULL plus an
** error string in the stack.
*/
static sdkl_CFunction lsys_sym (sdkl_State *L, void *lib, const char *sym);




#if defined(SDKL_USE_DLOPEN)	/* { */
/*
** {========================================================================
** This is an implementation of loadlib based on the dlfcn interface.
** The dlfcn interface is available in Linux, SunOS, Solaris, IRIX, FreeBSD,
** NetBSD, AIX 4.2, HPUX 11, and  probably most other Unix flavors, at least
** as an emulation layer on top of native functions.
** =========================================================================
*/

#include <dlfcn.h>

/*
** Macro to convert pointer-to-void* to pointer-to-function. This cast
** is undefined according to ISO C, but POSIX assumes that it works.
** (The '__extension__' in gnu compilers is only to avoid warnings.)
*/
#if defined(__GNUC__)
#define cast_func(p) (__extension__ (sdkl_CFunction)(p))
#else
#define cast_func(p) ((sdkl_CFunction)(p))
#endif



static void lsys_unloadlib (void *lib) {
  dlclose(lib);
}


static void *lsys_load (sdkl_State *L, const char *path, int seeglb) {
  void *lib = dlopen(path, RTLD_NOW | (seeglb ? RTLD_GLOBAL : RTLD_LOCAL));
  if (l_unlikely(lib == NULL))
    sdkl_pushstring(L, dlerror());
  return lib;
}


static sdkl_CFunction lsys_sym (sdkl_State *L, void *lib, const char *sym) {
  sdkl_CFunction f = cast_func(dlsym(lib, sym));
  if (l_unlikely(f == NULL))
    sdkl_pushstring(L, dlerror());
  return f;
}

/* }====================================================== */



#elif defined(SDKL_DL_DLL)	/* }{ */
/*
** {======================================================================
** This is an implementation of loadlib for Windows using native functions.
** =======================================================================
*/

#include <windows.h>


/*
** optional flags for LoadLibraryEx
*/
#if !defined(SDKL_LLE_FLAGS)
#define SDKL_LLE_FLAGS	0
#endif


#undef setprogdir


/*
** Replace in the path (on the top of the stack) any occurrence
** of SDKL_EXEC_DIR with the executable's path.
*/
static void setprogdir (sdkl_State *L) {
  char buff[MAX_PATH + 1];
  char *lb;
  DWORD nsize = sizeof(buff)/sizeof(char);
  DWORD n = GetModuleFileNameA(NULL, buff, nsize);  /* get exec. name */
  if (n == 0 || n == nsize || (lb = strrchr(buff, '\\')) == NULL)
    sdklL_error(L, "unable to get ModuleFileName");
  else {
    *lb = '\0';  /* cut name on the last '\\' to get the path */
    sdklL_gsub(L, sdkl_tostring(L, -1), SDKL_EXEC_DIR, buff);
    sdkl_remove(L, -2);  /* remove original string */
  }
}




static void pusherror (sdkl_State *L) {
  int error = GetLastError();
  char buffer[128];
  if (FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
      NULL, error, 0, buffer, sizeof(buffer)/sizeof(char), NULL))
    sdkl_pushstring(L, buffer);
  else
    sdkl_pushfstring(L, "system error %d\n", error);
}

static void lsys_unloadlib (void *lib) {
  FreeLibrary((HMODULE)lib);
}


static void *lsys_load (sdkl_State *L, const char *path, int seeglb) {
  HMODULE lib = LoadLibraryExA(path, NULL, SDKL_LLE_FLAGS);
  (void)(seeglb);  /* not used: symbols are 'global' by default */
  if (lib == NULL) pusherror(L);
  return lib;
}


static sdkl_CFunction lsys_sym (sdkl_State *L, void *lib, const char *sym) {
  sdkl_CFunction f = (sdkl_CFunction)(voidf)GetProcAddress((HMODULE)lib, sym);
  if (f == NULL) pusherror(L);
  return f;
}

/* }====================================================== */


#else				/* }{ */
/*
** {======================================================
** Fallback for other systems
** =======================================================
*/

#undef LIB_FAIL
#define LIB_FAIL	"absent"


#define DLMSG	"dynamic libraries not enabled; check your SDKL installation"


static void lsys_unloadlib (void *lib) {
  (void)(lib);  /* not used */
}


static void *lsys_load (sdkl_State *L, const char *path, int seeglb) {
  (void)(path); (void)(seeglb);  /* not used */
  sdkl_pushliteral(L, DLMSG);
  return NULL;
}


static sdkl_CFunction lsys_sym (sdkl_State *L, void *lib, const char *sym) {
  (void)(lib); (void)(sym);  /* not used */
  sdkl_pushliteral(L, DLMSG);
  return NULL;
}

/* }====================================================== */
#endif				/* } */


/*
** {==================================================================
** Set Paths
** ===================================================================
*/

/*
** SDKL_PATH_VAR and SDKL_CPATH_VAR are the names of the environment
** variables that SDKL check to set its paths.
*/
#if !defined(SDKL_PATH_VAR)
#define SDKL_PATH_VAR    "SDKL_PATH"
#endif

#if !defined(SDKL_CPATH_VAR)
#define SDKL_CPATH_VAR   "SDKL_CPATH"
#endif



/*
** return registry.SDKL_NOENV as a boolean
*/
static int noenv (sdkl_State *L) {
  int b;
  sdkl_getfield(L, SDKL_REGISTRYINDEX, "SDKL_NOENV");
  b = sdkl_toboolean(L, -1);
  sdkl_pop(L, 1);  /* remove value */
  return b;
}


/*
** Set a path
*/
static void setpath (sdkl_State *L, const char *fieldname,
                                   const char *envname,
                                   const char *dft) {
  const char *dftmark;
  const char *nver = sdkl_pushfstring(L, "%s%s", envname, SDKL_VERSUFFIX);
  const char *path = getenv(nver);  /* try versioned name */
  if (path == NULL)  /* no versioned environment variable? */
    path = getenv(envname);  /* try unversioned name */
  if (path == NULL || noenv(L))  /* no environment variable? */
    sdkl_pushstring(L, dft);  /* use default */
  else if ((dftmark = strstr(path, SDKL_PATH_SEP SDKL_PATH_SEP)) == NULL)
    sdkl_pushstring(L, path);  /* nothing to change */
  else {  /* path contains a ";;": insert default path in its place */
    size_t len = strlen(path);
    sdklL_Buffer b;
    sdklL_buffinit(L, &b);
    if (path < dftmark) {  /* is there a prefix before ';;'? */
      sdklL_addlstring(&b, path, dftmark - path);  /* add it */
      sdklL_addchar(&b, *SDKL_PATH_SEP);
    }
    sdklL_addstring(&b, dft);  /* add default */
    if (dftmark < path + len - 2) {  /* is there a suffix after ';;'? */
      sdklL_addchar(&b, *SDKL_PATH_SEP);
      sdklL_addlstring(&b, dftmark + 2, (path + len - 2) - dftmark);
    }
    sdklL_pushresult(&b);
  }
  setprogdir(L);
  sdkl_setfield(L, -3, fieldname);  /* package[fieldname] = path value */
  sdkl_pop(L, 1);  /* pop versioned variable name ('nver') */
}

/* }================================================================== */


/*
** return registry.CLIBS[path]
*/
static void *checkclib (sdkl_State *L, const char *path) {
  void *plib;
  sdkl_getfield(L, SDKL_REGISTRYINDEX, CLIBS);
  sdkl_getfield(L, -1, path);
  plib = sdkl_touserdata(L, -1);  /* plib = CLIBS[path] */
  sdkl_pop(L, 2);  /* pop CLIBS table and 'plib' */
  return plib;
}


/*
** registry.CLIBS[path] = plib        -- for queries
** registry.CLIBS[#CLIBS + 1] = plib  -- also keep a list of all libraries
*/
static void addtoclib (sdkl_State *L, const char *path, void *plib) {
  sdkl_getfield(L, SDKL_REGISTRYINDEX, CLIBS);
  sdkl_pushlightuserdata(L, plib);
  sdkl_pushvalue(L, -1);
  sdkl_setfield(L, -3, path);  /* CLIBS[path] = plib */
  sdkl_rawseti(L, -2, sdklL_len(L, -2) + 1);  /* CLIBS[#CLIBS + 1] = plib */
  sdkl_pop(L, 1);  /* pop CLIBS table */
}


/*
** __gc tag method for CLIBS table: calls 'lsys_unloadlib' for all lib
** handles in list CLIBS
*/
static int gctm (sdkl_State *L) {
  sdkl_Integer n = sdklL_len(L, 1);
  for (; n >= 1; n--) {  /* for each handle, in reverse order */
    sdkl_rawgeti(L, 1, n);  /* get handle CLIBS[n] */
    lsys_unloadlib(sdkl_touserdata(L, -1));
    sdkl_pop(L, 1);  /* pop handle */
  }
  return 0;
}



/* error codes for 'lookforfunc' */
#define ERRLIB		1
#define ERRFUNC		2

/*
** Look for a C function named 'sym' in a dynamically loaded library
** 'path'.
** First, check whether the library is already loaded; if not, try
** to load it.
** Then, if 'sym' is '*', return true (as library has been loaded).
** Otherwise, look for symbol 'sym' in the library and push a
** C function with that symbol.
** Return 0 and 'true' or a function in the stack; in case of
** errors, return an error code and an error message in the stack.
*/
static int lookforfunc (sdkl_State *L, const char *path, const char *sym) {
  void *reg = checkclib(L, path);  /* check loaded C libraries */
  if (reg == NULL) {  /* must load library? */
    reg = lsys_load(L, path, *sym == '*');  /* global symbols if 'sym'=='*' */
    if (reg == NULL) return ERRLIB;  /* unable to load library */
    addtoclib(L, path, reg);
  }
  if (*sym == '*') {  /* loading only library (no function)? */
    sdkl_pushboolean(L, 1);  /* return 'true' */
    return 0;  /* no errors */
  }
  else {
    sdkl_CFunction f = lsys_sym(L, reg, sym);
    if (f == NULL)
      return ERRFUNC;  /* unable to find function */
    sdkl_pushcfunction(L, f);  /* else create new function */
    return 0;  /* no errors */
  }
}


static int ll_loadlib (sdkl_State *L) {
  const char *path = sdklL_checkstring(L, 1);
  const char *init = sdklL_checkstring(L, 2);
  int stat = lookforfunc(L, path, init);
  if (l_likely(stat == 0))  /* no errors? */
    return 1;  /* return the loaded function */
  else {  /* error; error message is on stack top */
    sdklL_pushfail(L);
    sdkl_insert(L, -2);
    sdkl_pushstring(L, (stat == ERRLIB) ?  LIB_FAIL : "init");
    return 3;  /* return fail, error message, and where */
  }
}



/*
** {======================================================
** 'require' function
** =======================================================
*/


static int readable (const char *filename) {
  FILE *f = fopen(filename, "r");  /* try to open file */
  if (f == NULL) return 0;  /* open failed */
  fclose(f);
  return 1;
}


/*
** Get the next name in '*path' = 'name1;name2;name3;...', changing
** the ending ';' to '\0' to create a zero-terminated string. Return
** NULL when list ends.
*/
static const char *getnextfilename (char **path, char *end) {
  char *sep;
  char *name = *path;
  if (name == end)
    return NULL;  /* no more names */
  else if (*name == '\0') {  /* from previous iteration? */
    *name = *SDKL_PATH_SEP;  /* restore separator */
    name++;  /* skip it */
  }
  sep = strchr(name, *SDKL_PATH_SEP);  /* find next separator */
  if (sep == NULL)  /* separator not found? */
    sep = end;  /* name goes until the end */
  *sep = '\0';  /* finish file name */
  *path = sep;  /* will start next search from here */
  return name;
}


/*
** Given a path such as ";blabla.so;blublu.so", pushes the string
**
** no file 'blabla.so'
**	no file 'blublu.so'
*/
static void pusherrornotfound (sdkl_State *L, const char *path) {
  sdklL_Buffer b;
  sdklL_buffinit(L, &b);
  sdklL_addstring(&b, "no file '");
  sdklL_addgsub(&b, path, SDKL_PATH_SEP, "'\n\tno file '");
  sdklL_addstring(&b, "'");
  sdklL_pushresult(&b);
}


static const char *searchpath (sdkl_State *L, const char *name,
                                             const char *path,
                                             const char *sep,
                                             const char *dirsep) {
  sdklL_Buffer buff;
  char *pathname;  /* path with name inserted */
  char *endpathname;  /* its end */
  const char *filename;
  /* separator is non-empty and appears in 'name'? */
  if (*sep != '\0' && strchr(name, *sep) != NULL)
    name = sdklL_gsub(L, name, sep, dirsep);  /* replace it by 'dirsep' */
  sdklL_buffinit(L, &buff);
  /* add path to the buffer, replacing marks ('?') with the file name */
  sdklL_addgsub(&buff, path, SDKL_PATH_MARK, name);
  sdklL_addchar(&buff, '\0');
  pathname = sdklL_buffaddr(&buff);  /* writable list of file names */
  endpathname = pathname + sdklL_bufflen(&buff) - 1;
  while ((filename = getnextfilename(&pathname, endpathname)) != NULL) {
    if (readable(filename))  /* does file exist and is readable? */
      return sdkl_pushstring(L, filename);  /* save and return name */
  }
  sdklL_pushresult(&buff);  /* push path to create error message */
  pusherrornotfound(L, sdkl_tostring(L, -1));  /* create error message */
  return NULL;  /* not found */
}


static int ll_searchpath (sdkl_State *L) {
  const char *f = searchpath(L, sdklL_checkstring(L, 1),
                                sdklL_checkstring(L, 2),
                                sdklL_optstring(L, 3, "."),
                                sdklL_optstring(L, 4, SDKL_DIRSEP));
  if (f != NULL) return 1;
  else {  /* error message is on top of the stack */
    sdklL_pushfail(L);
    sdkl_insert(L, -2);
    return 2;  /* return fail + error message */
  }
}


static const char *findfile (sdkl_State *L, const char *name,
                                           const char *pname,
                                           const char *dirsep) {
  const char *path;
  sdkl_getfield(L, sdkl_upvalueindex(1), pname);
  path = sdkl_tostring(L, -1);
  if (l_unlikely(path == NULL))
    sdklL_error(L, "'package.%s' must be a string", pname);
  return searchpath(L, name, path, ".", dirsep);
}


static int checkload (sdkl_State *L, int stat, const char *filename) {
  if (l_likely(stat)) {  /* module loaded successfully? */
    sdkl_pushstring(L, filename);  /* will be 2nd argument to module */
    return 2;  /* return open function and file name */
  }
  else
    return sdklL_error(L, "error loading module '%s' from file '%s':\n\t%s",
                          sdkl_tostring(L, 1), filename, sdkl_tostring(L, -1));
}


static int searcher_SDKL (sdkl_State *L) {
  const char *filename;
  const char *name = sdklL_checkstring(L, 1);
  filename = findfile(L, name, "path", SDKL_LSUBSEP);
  if (filename == NULL) return 1;  /* module not found in this path */
  return checkload(L, (sdklL_loadfile(L, filename) == SDKL_OK), filename);
}


/*
** Try to find a load function for module 'modname' at file 'filename'.
** First, change '.' to '_' in 'modname'; then, if 'modname' has
** the form X-Y (that is, it has an "ignore mark"), build a function
** name "sdklopen_X" and look for it. (For compatibility, if that
** fails, it also tries "sdklopen_Y".) If there is no ignore mark,
** look for a function named "sdklopen_modname".
*/
static int loadfunc (sdkl_State *L, const char *filename, const char *modname) {
  const char *openfunc;
  const char *mark;
  modname = sdklL_gsub(L, modname, ".", SDKL_OFSEP);
  mark = strchr(modname, *SDKL_IGMARK);
  if (mark) {
    int stat;
    openfunc = sdkl_pushlstring(L, modname, mark - modname);
    openfunc = sdkl_pushfstring(L, SDKL_POF"%s", openfunc);
    stat = lookforfunc(L, filename, openfunc);
    if (stat != ERRFUNC) return stat;
    modname = mark + 1;  /* else go ahead and try old-style name */
  }
  openfunc = sdkl_pushfstring(L, SDKL_POF"%s", modname);
  return lookforfunc(L, filename, openfunc);
}


static int searcher_C (sdkl_State *L) {
  const char *name = sdklL_checkstring(L, 1);
  const char *filename = findfile(L, name, "cpath", SDKL_CSUBSEP);
  if (filename == NULL) return 1;  /* module not found in this path */
  return checkload(L, (loadfunc(L, filename, name) == 0), filename);
}


static int searcher_Croot (sdkl_State *L) {
  const char *filename;
  const char *name = sdklL_checkstring(L, 1);
  const char *p = strchr(name, '.');
  int stat;
  if (p == NULL) return 0;  /* is root */
  sdkl_pushlstring(L, name, p - name);
  filename = findfile(L, sdkl_tostring(L, -1), "cpath", SDKL_CSUBSEP);
  if (filename == NULL) return 1;  /* root not found */
  if ((stat = loadfunc(L, filename, name)) != 0) {
    if (stat != ERRFUNC)
      return checkload(L, 0, filename);  /* real error */
    else {  /* open function not found */
      sdkl_pushfstring(L, "no module '%s' in file '%s'", name, filename);
      return 1;
    }
  }
  sdkl_pushstring(L, filename);  /* will be 2nd argument to module */
  return 2;
}


static int searcher_preload (sdkl_State *L) {
  const char *name = sdklL_checkstring(L, 1);
  sdkl_getfield(L, SDKL_REGISTRYINDEX, SDKL_PRELOAD_TABLE);
  if (sdkl_getfield(L, -1, name) == SDKL_TNIL) {  /* not found? */
    sdkl_pushfstring(L, "no field package.preload['%s']", name);
    return 1;
  }
  else {
    sdkl_pushliteral(L, ":preload:");
    return 2;
  }
}


static void findloader (sdkl_State *L, const char *name) {
  int i;
  sdklL_Buffer msg;  /* to build error message */
  /* push 'package.searchers' to index 3 in the stack */
  if (l_unlikely(sdkl_getfield(L, sdkl_upvalueindex(1), "searchers")
                 != SDKL_TTABLE))
    sdklL_error(L, "'package.searchers' must be a table");
  sdklL_buffinit(L, &msg);
  /*  iterate over available searchers to find a loader */
  for (i = 1; ; i++) {
    sdklL_addstring(&msg, "\n\t");  /* error-message prefix */
    if (l_unlikely(sdkl_rawgeti(L, 3, i) == SDKL_TNIL)) {  /* no more searchers? */
      sdkl_pop(L, 1);  /* remove nil */
      sdklL_buffsub(&msg, 2);  /* remove prefix */
      sdklL_pushresult(&msg);  /* create error message */
      sdklL_error(L, "module '%s' not found:%s", name, sdkl_tostring(L, -1));
    }
    sdkl_pushstring(L, name);
    sdkl_call(L, 1, 2);  /* call it */
    if (sdkl_isfunction(L, -2))  /* did it find a loader? */
      return;  /* module loader found */
    else if (sdkl_isstring(L, -2)) {  /* searcher returned error message? */
      sdkl_pop(L, 1);  /* remove extra return */
      sdklL_addvalue(&msg);  /* concatenate error message */
    }
    else {  /* no error message */
      sdkl_pop(L, 2);  /* remove both returns */
      sdklL_buffsub(&msg, 2);  /* remove prefix */
    }
  }
}


static int ll_require (sdkl_State *L) {
  const char *name = sdklL_checkstring(L, 1);
  sdkl_settop(L, 1);  /* LOADED table will be at index 2 */
  sdkl_getfield(L, SDKL_REGISTRYINDEX, SDKL_LOADED_TABLE);
  sdkl_getfield(L, 2, name);  /* LOADED[name] */
  if (sdkl_toboolean(L, -1))  /* is it there? */
    return 1;  /* package is already loaded */
  /* else must load package */
  sdkl_pop(L, 1);  /* remove 'getfield' result */
  findloader(L, name);
  sdkl_rotate(L, -2, 1);  /* function <-> loader data */
  sdkl_pushvalue(L, 1);  /* name is 1st argument to module loader */
  sdkl_pushvalue(L, -3);  /* loader data is 2nd argument */
  /* stack: ...; loader data; loader function; mod. name; loader data */
  sdkl_call(L, 2, 1);  /* run loader to load module */
  /* stack: ...; loader data; result from loader */
  if (!sdkl_isnil(L, -1))  /* non-nil return? */
    sdkl_setfield(L, 2, name);  /* LOADED[name] = returned value */
  else
    sdkl_pop(L, 1);  /* pop nil */
  if (sdkl_getfield(L, 2, name) == SDKL_TNIL) {   /* module set no value? */
    sdkl_pushboolean(L, 1);  /* use true as result */
    sdkl_copy(L, -1, -2);  /* replace loader result */
    sdkl_setfield(L, 2, name);  /* LOADED[name] = true */
  }
  sdkl_rotate(L, -2, 1);  /* loader data <-> module result  */
  return 2;  /* return module result and loader data */
}

/* }====================================================== */




static const sdklL_Reg pk_funcs[] = {
  {"loadlib", ll_loadlib},
  {"searchpath", ll_searchpath},
  /* placeholders */
  {"preload", NULL},
  {"cpath", NULL},
  {"path", NULL},
  {"searchers", NULL},
  {"loaded", NULL},
  {NULL, NULL}
};


static const sdklL_Reg ll_funcs[] = {
  {"include", ll_require},
  {NULL, NULL}
};


static void createsearcherstable (sdkl_State *L) {
  static const sdkl_CFunction searchers[] =
    {searcher_preload, searcher_SDKL, searcher_C, searcher_Croot, NULL};
  int i;
  /* create 'searchers' table */
  sdkl_createtable(L, sizeof(searchers)/sizeof(searchers[0]) - 1, 0);
  /* fill it with predefined searchers */
  for (i=0; searchers[i] != NULL; i++) {
    sdkl_pushvalue(L, -2);  /* set 'package' as upvalue for all searchers */
    sdkl_pushcclosure(L, searchers[i], 1);
    sdkl_rawseti(L, -2, i+1);
  }
  sdkl_setfield(L, -2, "searchers");  /* put it in field 'searchers' */
}


/*
** create table CLIBS to keep track of loaded C libraries,
** setting a finalizer to close all libraries when closing state.
*/
static void createclibstable (sdkl_State *L) {
  sdklL_getsubtable(L, SDKL_REGISTRYINDEX, CLIBS);  /* create CLIBS table */
  sdkl_createtable(L, 0, 1);  /* create metatable for CLIBS */
  sdkl_pushcfunction(L, gctm);
  sdkl_setfield(L, -2, "__gc");  /* set finalizer for CLIBS table */
  sdkl_setmetatable(L, -2);
}


SDKLMOD_API int sdklopen_package (sdkl_State *L) {
  createclibstable(L);
  sdklL_newlib(L, pk_funcs);  /* create 'package' table */
  createsearcherstable(L);
  /* set paths */
  setpath(L, "path", SDKL_PATH_VAR, SDKL_PATH_DEFAULT);
  setpath(L, "cpath", SDKL_CPATH_VAR, SDKL_CPATH_DEFAULT);
  /* store config information */
  sdkl_pushliteral(L, SDKL_DIRSEP "\n" SDKL_PATH_SEP "\n" SDKL_PATH_MARK "\n"
                     SDKL_EXEC_DIR "\n" SDKL_IGMARK "\n");
  sdkl_setfield(L, -2, "config");
  /* set field 'loaded' */
  sdklL_getsubtable(L, SDKL_REGISTRYINDEX, SDKL_LOADED_TABLE);
  sdkl_setfield(L, -2, "loaded");
  /* set field 'preload' */
  sdklL_getsubtable(L, SDKL_REGISTRYINDEX, SDKL_PRELOAD_TABLE);
  sdkl_setfield(L, -2, "preload");
  sdkl_pushglobaltable(L);
  sdkl_pushvalue(L, -2);  /* set 'package' as upvalue for next lib */
  sdklL_setfuncs(L, ll_funcs, 1);  /* open lib into global table */
  sdkl_pop(L, 1);  /* pop global table */
  return 1;  /* return 'package' table */
}

