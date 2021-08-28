/*
** $Id: loslib.c $
** Standard Operating System library
** See Copyright Notice in sdkl.h
*/

#define loslib_c
#define SDKL_LIB

#include "lprefix.h"


#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef __unix__
# include <unistd.h>
#endif
#if defined _WIN32
# include <windows.h>
#define sleep(x) Sleep(1000 * (x))
#endif

#include "sdkl.h"

#include "sdklauxillary.h"
#include "sdkllib.h"


/*
** {==================================================================
** List of valid conversion specifiers for the 'strftime' function;
** options are grouped by length; group of length 2 start with '||'.
** ===================================================================
*/
#if !defined(SDKL_STRFTIMEOPTIONS)	/* { */

/* options for ANSI C 89 (only 1-char options) */
#define L_STRFTIMEC89		"aAbBcdHIjmMpSUwWxXyYZ%"

/* options for ISO C 99 and POSIX */
#define L_STRFTIMEC99 "aAbBcCdDeFgGhHIjmMnprRStTuUVwWxXyYzZ%" \
    "||" "EcECExEXEyEY" "OdOeOHOIOmOMOSOuOUOVOwOWOy"  /* two-char options */

/* options for Windows */
#define L_STRFTIMEWIN "aAbBcdHIjmMpSUwWxXyYzZ%" \
    "||" "#c#x#d#H#I#j#m#M#S#U#w#W#y#Y"  /* two-char options */

#if defined(SDKL_USE_WINDOWS)
#define SDKL_STRFTIMEOPTIONS	L_STRFTIMEWIN
#elif defined(SDKL_USE_C89)
#define SDKL_STRFTIMEOPTIONS	L_STRFTIMEC89
#else  /* C99 specification */
#define SDKL_STRFTIMEOPTIONS	L_STRFTIMEC99
#endif

#endif					/* } */
/* }================================================================== */


/*
** {==================================================================
** Configuration for time-related stuff
** ===================================================================
*/

/*
** type to represent time_t in SDKL
*/
#if !defined(SDKL_NUMTIME)	/* { */

#define l_timet			sdkl_Integer
#define l_pushtime(L,t)		sdkl_pushinteger(L,(sdkl_Integer)(t))
#define l_gettime(L,arg)	sdklL_checkinteger(L, arg)

#else				/* }{ */

#define l_timet			sdkl_Number
#define l_pushtime(L,t)		sdkl_pushnumber(L,(sdkl_Number)(t))
#define l_gettime(L,arg)	sdklL_checknumber(L, arg)

#endif				/* } */


#if !defined(l_gmtime)		/* { */
/*
** By default, SDKL uses gmtime/localtime, except when POSIX is available,
** where it uses gmtime_r/localtime_r
*/

#if defined(SDKL_USE_POSIX)	/* { */

#define l_gmtime(t,r)		gmtime_r(t,r)
#define l_localtime(t,r)	localtime_r(t,r)

#else				/* }{ */

/* ISO C definitions */
#define l_gmtime(t,r)		((void)(r)->tm_sec, gmtime(t))
#define l_localtime(t,r)	((void)(r)->tm_sec, localtime(t))

#endif				/* } */

#endif				/* } */

/* }================================================================== */


/*
** {==================================================================
** Configuration for 'tmpnam':
** By default, SDKL uses tmpnam except when POSIX is available, where
** it uses mkstemp.
** ===================================================================
*/
#if !defined(sdkl_tmpnam)	/* { */

#if defined(SDKL_USE_POSIX)	/* { */

#include <unistd.h>

#define SDKL_TMPNAMBUFSIZE	32

#if !defined(SDKL_TMPNAMTEMPLATE)
#define SDKL_TMPNAMTEMPLATE	"/tmp/sdkl_XXXXXX"
#endif

#define sdkl_tmpnam(b,e) { \
        strcpy(b, SDKL_TMPNAMTEMPLATE); \
        e = mkstemp(b); \
        if (e != -1) close(e); \
        e = (e == -1); }

#else				/* }{ */

/* ISO C definitions */
#define SDKL_TMPNAMBUFSIZE	L_tmpnam
#define sdkl_tmpnam(b,e)		{ e = (tmpnam(b) == NULL); }

#endif				/* } */

#endif				/* } */
/* }================================================================== */



static int os_execute (sdkl_State *L) {
  const char *cmd = sdklL_optstring(L, 1, NULL);
  int stat;
  errno = 0;
  stat = system(cmd);
  if (cmd != NULL)
    return sdklL_execresult(L, stat);
  else {
    sdkl_pushboolean(L, stat);  /* true if there is a shell */
    return 1;
  }
}

static int os_code(sdkl_State* L)
{
  int stat;
  stat = system(sdklL_checkstring(L, 1));
  sdkl_pushinteger(L, stat);
  return 1;
}

static int os_remove (sdkl_State *L) {
  const char *filename = sdklL_checkstring(L, 1);
  return sdklL_fileresult(L, remove(filename) == 0, filename);
}


static int os_rename (sdkl_State *L) {
  const char *fromname = sdklL_checkstring(L, 1);
  const char *toname = sdklL_checkstring(L, 2);
  return sdklL_fileresult(L, rename(fromname, toname) == 0, NULL);
}


static int os_tmpname (sdkl_State *L) {
  char buff[SDKL_TMPNAMBUFSIZE];
  int err;
  sdkl_tmpnam(buff, err);
  if (l_unlikely(err))
    return sdklL_error(L, "unable to generate a unique filename");
  sdkl_pushstring(L, buff);
  return 1;
}


static int os_getenv (sdkl_State *L) {
  sdkl_pushstring(L, getenv(sdklL_checkstring(L, 1)));  /* if NULL push nil */
  return 1;
}


static int os_clock (sdkl_State *L) {
  sdkl_pushnumber(L, ((sdkl_Number)clock())/(sdkl_Number)CLOCKS_PER_SEC);
  return 1;
}


/*
** {======================================================
** Time/Date operations
** { year=%Y, month=%m, day=%d, hour=%H, min=%M, sec=%S,
**   wday=%w+1, yday=%j, isdst=? }
** =======================================================
*/

/*
** About the overflow check: an overflow cannot occur when time
** is represented by a sdkl_Integer, because either sdkl_Integer is
** large enough to represent all int fields or it is not large enough
** to represent a time that cause a field to overflow.  However, if
** times are represented as doubles and sdkl_Integer is int, then the
** time 0x1.e1853b0d184f6p+55 would cause an overflow when adding 1900
** to compute the year.
*/
static void setfield (sdkl_State *L, const char *key, int value, int delta) {
  #if (defined(SDKL_NUMTIME) && SDKL_MAXINTEGER <= INT_MAX)
    if (l_unlikely(value > SDKL_MAXINTEGER - delta))
      sdklL_error(L, "field '%s' is out-of-bound", key);
  #endif
  sdkl_pushinteger(L, (sdkl_Integer)value + delta);
  sdkl_setfield(L, -2, key);
}


static void setboolfield (sdkl_State *L, const char *key, int value) {
  if (value < 0)  /* undefined? */
    return;  /* does not set field */
  sdkl_pushboolean(L, value);
  sdkl_setfield(L, -2, key);
}


/*
** Set all fields from structure 'tm' in the table on top of the stack
*/
static void setallfields (sdkl_State *L, struct tm *stm) {
  setfield(L, "year", stm->tm_year, 1900);
  setfield(L, "month", stm->tm_mon, 1);
  setfield(L, "day", stm->tm_mday, 0);
  setfield(L, "hour", stm->tm_hour, 0);
  setfield(L, "min", stm->tm_min, 0);
  setfield(L, "sec", stm->tm_sec, 0);
  setfield(L, "yday", stm->tm_yday, 1);
  setfield(L, "wday", stm->tm_wday, 1);
  setboolfield(L, "isdst", stm->tm_isdst);
}


static int getboolfield (sdkl_State *L, const char *key) {
  int res;
  res = (sdkl_getfield(L, -1, key) == SDKL_TNIL) ? -1 : sdkl_toboolean(L, -1);
  sdkl_pop(L, 1);
  return res;
}


static int getfield (sdkl_State *L, const char *key, int d, int delta) {
  int isnum;
  int t = sdkl_getfield(L, -1, key);  /* get field and its type */
  sdkl_Integer res = sdkl_tointegerx(L, -1, &isnum);
  if (!isnum) {  /* field is not an integer? */
    if (l_unlikely(t != SDKL_TNIL))  /* some other value? */
      return sdklL_error(L, "field '%s' is not an integer", key);
    else if (l_unlikely(d < 0))  /* absent field; no default? */
      return sdklL_error(L, "field '%s' missing in date table", key);
    res = d;
  }
  else {
    /* unsigned avoids overflow when sdkl_Integer has 32 bits */
    if (!(res >= 0 ? (sdkl_Unsigned)res <= (sdkl_Unsigned)INT_MAX + delta
                   : (sdkl_Integer)INT_MIN + delta <= res))
      return sdklL_error(L, "field '%s' is out-of-bound", key);
    res -= delta;
  }
  sdkl_pop(L, 1);
  return (int)res;
}


static const char *checkoption (sdkl_State *L, const char *conv,
                                ptrdiff_t convlen, char *buff) {
  const char *option = SDKL_STRFTIMEOPTIONS;
  int oplen = 1;  /* length of options being checked */
  for (; *option != '\0' && oplen <= convlen; option += oplen) {
    if (*option == '|')  /* next block? */
      oplen++;  /* will check options with next length (+1) */
    else if (memcmp(conv, option, oplen) == 0) {  /* match? */
      memcpy(buff, conv, oplen);  /* copy valid option to buffer */
      buff[oplen] = '\0';
      return conv + oplen;  /* return next item */
    }
  }
  sdklL_argerror(L, 1,
    sdkl_pushfstring(L, "invalid conversion specifier '%%%s'", conv));
  return conv;  /* to avoid warnings */
}


static time_t l_checktime (sdkl_State *L, int arg) {
  l_timet t = l_gettime(L, arg);
  sdklL_argcheck(L, (time_t)t == t, arg, "time out-of-bounds");
  return (time_t)t;
}


/* maximum size for an individual 'strftime' item */
#define SIZETIMEFMT	250


static int os_date (sdkl_State *L) {
  size_t slen;
  const char *s = sdklL_optlstring(L, 1, "%c", &slen);
  time_t t = sdklL_opt(L, l_checktime, 2, time(NULL));
  const char *se = s + slen;  /* 's' end */
  struct tm tmr, *stm;
  if (*s == '!') {  /* UTC? */
    stm = l_gmtime(&t, &tmr);
    s++;  /* skip '!' */
  }
  else
    stm = l_localtime(&t, &tmr);
  if (stm == NULL)  /* invalid date? */
    return sdklL_error(L,
                 "date result cannot be represented in this installation");
  if (strcmp(s, "*t") == 0) {
    sdkl_createtable(L, 0, 9);  /* 9 = number of fields */
    setallfields(L, stm);
  }
  else {
    char cc[4];  /* buffer for individual conversion specifiers */
    sdklL_Buffer b;
    cc[0] = '%';
    sdklL_buffinit(L, &b);
    while (s < se) {
      if (*s != '%')  /* not a conversion specifier? */
        sdklL_addchar(&b, *s++);
      else {
        size_t reslen;
        char *buff = sdklL_prepbuffsize(&b, SIZETIMEFMT);
        s++;  /* skip '%' */
        s = checkoption(L, s, se - s, cc + 1);  /* copy specifier to 'cc' */
        reslen = strftime(buff, SIZETIMEFMT, cc, stm);
        sdklL_addsize(&b, reslen);
      }
    }
    sdklL_pushresult(&b);
  }
  return 1;
}


static int os_time (sdkl_State *L) {
  time_t t;
  if (sdkl_isnoneornil(L, 1))  /* called without args? */
    t = time(NULL);  /* get current time */
  else {
    struct tm ts;
    sdklL_checktype(L, 1, SDKL_TTABLE);
    sdkl_settop(L, 1);  /* make sure table is at the top */
    ts.tm_year = getfield(L, "year", -1, 1900);
    ts.tm_mon = getfield(L, "month", -1, 1);
    ts.tm_mday = getfield(L, "day", -1, 0);
    ts.tm_hour = getfield(L, "hour", 12, 0);
    ts.tm_min = getfield(L, "min", 0, 0);
    ts.tm_sec = getfield(L, "sec", 0, 0);
    ts.tm_isdst = getboolfield(L, "isdst");
    t = mktime(&ts);
    setallfields(L, &ts);  /* update fields with normalized values */
  }
  if (t != (time_t)(l_timet)t || t == (time_t)(-1))
    return sdklL_error(L,
                  "time result cannot be represented in this installation");
  l_pushtime(L, t);
  return 1;
}


static int os_difftime (sdkl_State *L) {
  time_t t1 = l_checktime(L, 1);
  time_t t2 = l_checktime(L, 2);
  sdkl_pushnumber(L, (sdkl_Number)difftime(t1, t2));
  return 1;
}

/* }====================================================== */


static int os_setlocale (sdkl_State *L) {
  static const int cat[] = {LC_ALL, LC_COLLATE, LC_CTYPE, LC_MONETARY,
                      LC_NUMERIC, LC_TIME};
  static const char *const catnames[] = {"all", "collate", "ctype", "monetary",
     "numeric", "time", NULL};
  const char *l = sdklL_optstring(L, 1, NULL);
  int op = sdklL_checkoption(L, 2, "all", catnames);
  sdkl_pushstring(L, setlocale(cat[op], l));
  return 1;
}


static int os_exit (sdkl_State *L) {
  int status;
  if (sdkl_isboolean(L, 1))
    status = (sdkl_toboolean(L, 1) ? EXIT_SUCCESS : EXIT_FAILURE);
  else
    status = (int)sdklL_optinteger(L, 1, EXIT_SUCCESS);
  if (sdkl_toboolean(L, 2))
    sdkl_close(L);
  if (L) exit(status);  /* 'if' to avoid warnings for unreachable 'return' */
  return 0;
}

static int os_name(sdkl_State* L)
{
#ifdef __linux__
  sdkl_pushstring(L, "Linux");
#elif __MACH__
  sdkl_pushstring(L, "Darwin");
#elif BSD
  sdkl_pushstring(L, "BSD");
#elif _WIN32
  sdkl_pushstring(L, "Windows");
#elif __ANDROID__
  sdkl_pushstring(L, "Android");
#endif
  return 1;
}

static int os_sleep(sdkl_State* L)
{
 sleep(sdklL_checkinteger(L, 1));
 return 1;
}

static const sdklL_Reg syslib[] = {
  {"clock",     os_clock},
  {"date",      os_date},
  {"difftime",  os_difftime},
  {"execute",   os_execute},
  {"exit",      os_exit},
  {"code", os_code},
  {"getenv",    os_getenv},
  {"remove",    os_remove},
  {"rename",    os_rename},
  {"setlocale", os_setlocale},
  {"time",      os_time},
  {"tmpname",   os_tmpname},
  {"name", os_name},
  {"sleep", os_sleep},
  {NULL, NULL}
};

/* }====================================================== */



SDKLMOD_API int sdklopen_os (sdkl_State *L) {
  sdklL_newlib(L, syslib);
  return 1;
}

