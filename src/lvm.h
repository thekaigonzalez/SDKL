/*
** $Id: lvm.h $
** SDKL virtual machine
** See Copyright Notice in sdkl.h
*/

#ifndef lvm_h
#define lvm_h


#include "ldo.h"
#include "lobject.h"
#include "ltm.h"


#if !defined(SDKL_NOCVTN2S)
#define cvt2str(o)	ttisnumber(o)
#else
#define cvt2str(o)	0	/* no conversion from numbers to strings */
#endif


#if !defined(SDKL_NOCVTS2N)
#define cvt2num(o)	ttisstring(o)
#else
#define cvt2num(o)	0	/* no conversion from strings to numbers */
#endif


/*
** You can define SDKL_FLOORN2I if you want to convert floats to integers
** by flooring them (instead of raising an error if they are not
** integral values)
*/
#if !defined(SDKL_FLOORN2I)
#define SDKL_FLOORN2I		F2Ieq
#endif


/*
** Rounding modes for float->integer coercion
 */
typedef enum {
  F2Ieq,     /* no rounding; accepts only integral values */
  F2Ifloor,  /* takes the floor of the number */
  F2Iceil    /* takes the ceil of the number */
} F2Imod;


/* convert an object to a float (including string coercion) */
#define tonumber(o,n) \
	(ttisfloat(o) ? (*(n) = fltvalue(o), 1) : sdklV_tonumber_(o,n))


/* convert an object to a float (without string coercion) */
#define tonumberns(o,n) \
	(ttisfloat(o) ? ((n) = fltvalue(o), 1) : \
	(ttisinteger(o) ? ((n) = cast_num(ivalue(o)), 1) : 0))


/* convert an object to an integer (including string coercion) */
#define tointeger(o,i) \
  (l_likely(ttisinteger(o)) ? (*(i) = ivalue(o), 1) \
                          : sdklV_tointeger(o,i,SDKL_FLOORN2I))


/* convert an object to an integer (without string coercion) */
#define tointegerns(o,i) \
  (l_likely(ttisinteger(o)) ? (*(i) = ivalue(o), 1) \
                          : sdklV_tointegerns(o,i,SDKL_FLOORN2I))


#define intop(op,v1,v2) l_castU2S(l_castS2U(v1) op l_castS2U(v2))

#define sdklV_rawequalobj(t1,t2)		sdklV_equalobj(NULL,t1,t2)


/*
** fast track for 'gettable': if 't' is a table and 't[k]' is present,
** return 1 with 'slot' pointing to 't[k]' (position of final result).
** Otherwise, return 0 (meaning it will have to check metamethod)
** with 'slot' pointing to an empty 't[k]' (if 't' is a table) or NULL
** (otherwise). 'f' is the raw get function to use.
*/
#define sdklV_fastget(L,t,k,slot,f) \
  (!ttistable(t)  \
   ? (slot = NULL, 0)  /* not a table; 'slot' is NULL and result is 0 */  \
   : (slot = f(hvalue(t), k),  /* else, do raw access */  \
      !isempty(slot)))  /* result not empty? */


/*
** Special case of 'sdklV_fastget' for integers, inlining the fast case
** of 'sdklH_getint'.
*/
#define sdklV_fastgeti(L,t,k,slot) \
  (!ttistable(t)  \
   ? (slot = NULL, 0)  /* not a table; 'slot' is NULL and result is 0 */  \
   : (slot = (l_castS2U(k) - 1u < hvalue(t)->alimit) \
              ? &hvalue(t)->array[k - 1] : sdklH_getint(hvalue(t), k), \
      !isempty(slot)))  /* result not empty? */


/*
** Finish a fast set operation (when fast get succeeds). In that case,
** 'slot' points to the place to put the value.
*/
#define sdklV_finishfastset(L,t,slot,v) \
    { setobj2t(L, cast(TValue *,slot), v); \
      sdklC_barrierback(L, gcvalue(t), v); }




SDKLI_FUNC int sdklV_equalobj (sdkl_State *L, const TValue *t1, const TValue *t2);
SDKLI_FUNC int sdklV_lessthan (sdkl_State *L, const TValue *l, const TValue *r);
SDKLI_FUNC int sdklV_lessequal (sdkl_State *L, const TValue *l, const TValue *r);
SDKLI_FUNC int sdklV_tonumber_ (const TValue *obj, sdkl_Number *n);
SDKLI_FUNC int sdklV_tointeger (const TValue *obj, sdkl_Integer *p, F2Imod mode);
SDKLI_FUNC int sdklV_tointegerns (const TValue *obj, sdkl_Integer *p,
                                F2Imod mode);
SDKLI_FUNC int sdklV_flttointeger (sdkl_Number n, sdkl_Integer *p, F2Imod mode);
SDKLI_FUNC void sdklV_finishget (sdkl_State *L, const TValue *t, TValue *key,
                               StkId val, const TValue *slot);
SDKLI_FUNC void sdklV_finishset (sdkl_State *L, const TValue *t, TValue *key,
                               TValue *val, const TValue *slot);
SDKLI_FUNC void sdklV_finishOp (sdkl_State *L);
SDKLI_FUNC void sdklV_execute (sdkl_State *L, CallInfo *ci);
SDKLI_FUNC void sdklV_concat (sdkl_State *L, int total);
SDKLI_FUNC sdkl_Integer sdklV_idiv (sdkl_State *L, sdkl_Integer x, sdkl_Integer y);
SDKLI_FUNC sdkl_Integer sdklV_mod (sdkl_State *L, sdkl_Integer x, sdkl_Integer y);
SDKLI_FUNC sdkl_Number sdklV_modf (sdkl_State *L, sdkl_Number x, sdkl_Number y);
SDKLI_FUNC sdkl_Integer sdklV_shiftl (sdkl_Integer x, sdkl_Integer y);
SDKLI_FUNC void sdklV_objlen (sdkl_State *L, StkId ra, const TValue *rb);

#endif
