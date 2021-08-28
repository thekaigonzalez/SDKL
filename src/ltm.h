/*
** $Id: ltm.h $
** Tag methods
** See Copyright Notice in sdkl.h
*/

#ifndef ltm_h
#define ltm_h


#include "lobject.h"


/*
* WARNING: if you change the order of this enumeration,
* grep "ORDER TM" and "ORDER OP"
*/
typedef enum {
  TM_INDEX,
  TM_NEWINDEX,
  TM_GC,
  TM_MODE,
  TM_LEN,
  TM_EQ,  /* last tag method with fast access */
  TM_ADD,
  TM_SUB,
  TM_MUL,
  TM_MOD,
  TM_POW,
  TM_DIV,
  TM_IDIV,
  TM_BAND,
  TM_BOR,
  TM_BXOR,
  TM_SHL,
  TM_SHR,
  TM_UNM,
  TM_BNOT,
  TM_LT,
  TM_LE,
  TM_CONCAT,
  TM_CALL,
  TM_CLOSE,
  TM_N		/* number of elements in the enum */
} TMS;


/*
** Mask with 1 in all fast-access methods. A 1 in any of these bits
** in the flag of a (meta)table means the metatable does not have the
** corresponding metamethod field. (Bit 7 of the flag is used for
** 'isrealasize'.)
*/
#define maskflags	(~(~0u << (TM_EQ + 1)))


/*
** Test whether there is no tagmethod.
** (Because tagmethods use raw accesses, the result may be an "empty" nil.)
*/
#define notm(tm)	ttisnil(tm)


#define gfasttm(g,et,e) ((et) == NULL ? NULL : \
  ((et)->flags & (1u<<(e))) ? NULL : sdklT_gettm(et, e, (g)->tmname[e]))

#define fasttm(l,et,e)	gfasttm(G(l), et, e)

#define ttypename(x)	sdklT_typenames_[(x) + 1]

SDKLI_DDEC(const char *const sdklT_typenames_[SDKL_TOTALTYPES];)


SDKLI_FUNC const char *sdklT_objtypename (sdkl_State *L, const TValue *o);

SDKLI_FUNC const TValue *sdklT_gettm (Table *events, TMS event, TString *ename);
SDKLI_FUNC const TValue *sdklT_gettmbyobj (sdkl_State *L, const TValue *o,
                                                       TMS event);
SDKLI_FUNC void sdklT_init (sdkl_State *L);

SDKLI_FUNC void sdklT_callTM (sdkl_State *L, const TValue *f, const TValue *p1,
                            const TValue *p2, const TValue *p3);
SDKLI_FUNC void sdklT_callTMres (sdkl_State *L, const TValue *f,
                            const TValue *p1, const TValue *p2, StkId p3);
SDKLI_FUNC void sdklT_trybinTM (sdkl_State *L, const TValue *p1, const TValue *p2,
                              StkId res, TMS event);
SDKLI_FUNC void sdklT_tryconcatTM (sdkl_State *L);
SDKLI_FUNC void sdklT_trybinassocTM (sdkl_State *L, const TValue *p1,
       const TValue *p2, int inv, StkId res, TMS event);
SDKLI_FUNC void sdklT_trybiniTM (sdkl_State *L, const TValue *p1, sdkl_Integer i2,
                               int inv, StkId res, TMS event);
SDKLI_FUNC int sdklT_callorderTM (sdkl_State *L, const TValue *p1,
                                const TValue *p2, TMS event);
SDKLI_FUNC int sdklT_callorderiTM (sdkl_State *L, const TValue *p1, int v2,
                                 int inv, int isfloat, TMS event);

SDKLI_FUNC void sdklT_adjustvarargs (sdkl_State *L, int nfixparams,
                                   struct CallInfo *ci, const Proto *p);
SDKLI_FUNC void sdklT_getvarargs (sdkl_State *L, struct CallInfo *ci,
                                              StkId where, int wanted);


#endif
