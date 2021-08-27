/*
** $Id: ldebug.h $
** Auxiliary functions from Debug Interface module
** See Copyright Notice in sdkl.h
*/

#ifndef ldebug_h
#define ldebug_h


#include "lstate.h"


#define pcRel(pc, p)	(cast_int((pc) - (p)->code) - 1)


/* Active SDKL function (given call info) */
#define ci_func(ci)		(clLvalue(s2v((ci)->func)))


#define resethookcount(L)	(L->hookcount = L->basehookcount)

/*
** mark for entries in 'lineinfo' array that has absolute information in
** 'abslineinfo' array
*/
#define ABSLINEINFO	(-0x80)


/*
** MAXimum number of successive Instructions WiTHout ABSolute line
** information. (A power of two allows fast divisions.)
*/
#if !defined(MAXIWTHABS)
#define MAXIWTHABS	128
#endif


LUAI_FUNC int sdklG_getfuncline (const Proto *f, int pc);
LUAI_FUNC const char *sdklG_findlocal (sdkl_State *L, CallInfo *ci, int n,
                                                    StkId *pos);
LUAI_FUNC l_noret sdklG_typeerror (sdkl_State *L, const TValue *o,
                                                const char *opname);
LUAI_FUNC l_noret sdklG_callerror (sdkl_State *L, const TValue *o);
LUAI_FUNC l_noret sdklG_forerror (sdkl_State *L, const TValue *o,
                                               const char *what);
LUAI_FUNC l_noret sdklG_concaterror (sdkl_State *L, const TValue *p1,
                                                  const TValue *p2);
LUAI_FUNC l_noret sdklG_opinterror (sdkl_State *L, const TValue *p1,
                                                 const TValue *p2,
                                                 const char *msg);
LUAI_FUNC l_noret sdklG_tointerror (sdkl_State *L, const TValue *p1,
                                                 const TValue *p2);
LUAI_FUNC l_noret sdklG_ordererror (sdkl_State *L, const TValue *p1,
                                                 const TValue *p2);
LUAI_FUNC l_noret sdklG_runerror (sdkl_State *L, const char *fmt, ...);
LUAI_FUNC const char *sdklG_addinfo (sdkl_State *L, const char *msg,
                                                  TString *src, int line);
LUAI_FUNC l_noret sdklG_errormsg (sdkl_State *L);
LUAI_FUNC int sdklG_traceexec (sdkl_State *L, const Instruction *pc);


#endif
