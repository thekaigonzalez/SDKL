/*
** $Id: ldo.h $
** Stack and Call structure of SDKL
** See Copyright Notice in sdkl.h
*/

#ifndef ldo_h
#define ldo_h


#include "lobject.h"
#include "lstate.h"
#include "lzio.h"


/*
** Macro to check stack size and grow stack if needed.  Parameters
** 'pre'/'pos' allow the macro to preserve a pointer into the
** stack across reallocations, doing the work only when needed.
** It also allows the running of one GC step when the stack is
** reallocated.
** 'condmovestack' is used in heavy tests to force a stack reallocation
** at every check.
*/
#define sdklD_checkstackaux(L,n,pre,pos)  \
	if (l_unlikely(L->stack_last - L->top <= (n))) \
	  { pre; sdklD_growstack(L, n, 1); pos; } \
        else { condmovestack(L,pre,pos); }

/* In general, 'pre'/'pos' are empty (nothing to save) */
#define sdklD_checkstack(L,n)	sdklD_checkstackaux(L,n,(void)0,(void)0)



#define savestack(L,p)		((char *)(p) - (char *)L->stack)
#define restorestack(L,n)	((StkId)((char *)L->stack + (n)))


/* macro to check stack size, preserving 'p' */
#define checkstackGCp(L,n,p)  \
  sdklD_checkstackaux(L, n, \
    ptrdiff_t t__ = savestack(L, p);  /* save 'p' */ \
    sdklC_checkGC(L),  /* stack grow uses memory */ \
    p = restorestack(L, t__))  /* 'pos' part: restore 'p' */


/* macro to check stack size and GC */
#define checkstackGC(L,fsize)  \
	sdklD_checkstackaux(L, (fsize), sdklC_checkGC(L), (void)0)


/* type of protected functions, to be ran by 'runprotected' */
typedef void (*Pfunc) (sdkl_State *L, void *ud);

SDKLI_FUNC void sdklD_seterrorobj (sdkl_State *L, int errcode, StkId oldtop);
SDKLI_FUNC int sdklD_protectedparser (sdkl_State *L, ZIO *z, const char *name,
                                                  const char *mode);
SDKLI_FUNC void sdklD_hook (sdkl_State *L, int event, int line,
                                        int fTransfer, int nTransfer);
SDKLI_FUNC void sdklD_hookcall (sdkl_State *L, CallInfo *ci);
SDKLI_FUNC void sdklD_pretailcall (sdkl_State *L, CallInfo *ci, StkId func, int n);
SDKLI_FUNC CallInfo *sdklD_precall (sdkl_State *L, StkId func, int nResults);
SDKLI_FUNC void sdklD_call (sdkl_State *L, StkId func, int nResults);
SDKLI_FUNC void sdklD_callnoyield (sdkl_State *L, StkId func, int nResults);
SDKLI_FUNC void sdklD_tryfuncTM (sdkl_State *L, StkId func);
SDKLI_FUNC int sdklD_closeprotected (sdkl_State *L, ptrdiff_t level, int status);
SDKLI_FUNC int sdklD_pcall (sdkl_State *L, Pfunc func, void *u,
                                        ptrdiff_t oldtop, ptrdiff_t ef);
SDKLI_FUNC void sdklD_poscall (sdkl_State *L, CallInfo *ci, int nres);
SDKLI_FUNC int sdklD_reallocstack (sdkl_State *L, int newsize, int raiseerror);
SDKLI_FUNC int sdklD_growstack (sdkl_State *L, int n, int raiseerror);
SDKLI_FUNC void sdklD_shrinkstack (sdkl_State *L);
SDKLI_FUNC void sdklD_inctop (sdkl_State *L);

SDKLI_FUNC l_noret sdklD_throw (sdkl_State *L, int errcode);
SDKLI_FUNC int sdklD_rawrunprotected (sdkl_State *L, Pfunc f, void *ud);

#endif

