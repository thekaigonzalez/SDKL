/*
** $Id: lcode.h $
** Code generator for SDKL
** See Copyright Notice in sdkl.h
*/

#ifndef lcode_h
#define lcode_h

#include "llex.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lparser.h"


/*
** Marks the end of a patch list. It is an invalid value both as an absolute
** address, and as a list link (would link an element to itself).
*/
#define NO_JUMP (-1)


/*
** grep "ORDER OPR" if you change these enums  (ORDER OP)
*/
typedef enum BinOpr {
  /* arithmetic operators */
  OPR_ADD, OPR_SUB, OPR_MUL, OPR_MOD, OPR_POW,
  OPR_DIV, OPR_IDIV,
  /* bitwise operators */
  OPR_BAND, OPR_BOR, OPR_BXOR,
  OPR_SHL, OPR_SHR,
  /* string operator */
  OPR_CONCAT,
  /* comparison operators */
  OPR_EQ, OPR_LT, OPR_LE,
  OPR_NE, OPR_GT, OPR_GE,
  /* logical operators */
  OPR_AND, OPR_OR,
  OPR_NOBINOPR
} BinOpr;


/* true if operation is foldable (that is, it is arithmetic or bitwise) */
#define foldbinop(op)	((op) <= OPR_SHR)


#define sdklK_codeABC(fs,o,a,b,c)	sdklK_codeABCk(fs,o,a,b,c,0)


typedef enum UnOpr { OPR_MINUS, OPR_BNOT, OPR_NOT, OPR_LEN, OPR_NOUNOPR } UnOpr;


/* get (pointer to) instruction of given 'expdesc' */
#define getinstruction(fs,e)	((fs)->f->code[(e)->u.info])


#define sdklK_setmultret(fs,e)	sdklK_setreturns(fs, e, SDKL_MULTRET)

#define sdklK_jumpto(fs,t)	sdklK_patchlist(fs, sdklK_jump(fs), t)

SDKLI_FUNC int sdklK_code (FuncState *fs, Instruction i);
SDKLI_FUNC int sdklK_codeABx (FuncState *fs, OpCode o, int A, unsigned int Bx);
SDKLI_FUNC int sdklK_codeAsBx (FuncState *fs, OpCode o, int A, int Bx);
SDKLI_FUNC int sdklK_codeABCk (FuncState *fs, OpCode o, int A,
                                            int B, int C, int k);
SDKLI_FUNC int sdklK_isKint (expdesc *e);
SDKLI_FUNC int sdklK_exp2const (FuncState *fs, const expdesc *e, TValue *v);
SDKLI_FUNC void sdklK_fixline (FuncState *fs, int line);
SDKLI_FUNC void sdklK_nil (FuncState *fs, int from, int n);
SDKLI_FUNC void sdklK_reserveregs (FuncState *fs, int n);
SDKLI_FUNC void sdklK_checkstack (FuncState *fs, int n);
SDKLI_FUNC void sdklK_int (FuncState *fs, int reg, sdkl_Integer n);
SDKLI_FUNC void sdklK_dischargevars (FuncState *fs, expdesc *e);
SDKLI_FUNC int sdklK_exp2anyreg (FuncState *fs, expdesc *e);
SDKLI_FUNC void sdklK_exp2anyregup (FuncState *fs, expdesc *e);
SDKLI_FUNC void sdklK_exp2nextreg (FuncState *fs, expdesc *e);
SDKLI_FUNC void sdklK_exp2val (FuncState *fs, expdesc *e);
SDKLI_FUNC int sdklK_exp2RK (FuncState *fs, expdesc *e);
SDKLI_FUNC void sdklK_self (FuncState *fs, expdesc *e, expdesc *key);
SDKLI_FUNC void sdklK_indexed (FuncState *fs, expdesc *t, expdesc *k);
SDKLI_FUNC void sdklK_goiftrue (FuncState *fs, expdesc *e);
SDKLI_FUNC void sdklK_goiffalse (FuncState *fs, expdesc *e);
SDKLI_FUNC void sdklK_storevar (FuncState *fs, expdesc *var, expdesc *e);
SDKLI_FUNC void sdklK_setreturns (FuncState *fs, expdesc *e, int nresults);
SDKLI_FUNC void sdklK_setoneret (FuncState *fs, expdesc *e);
SDKLI_FUNC int sdklK_jump (FuncState *fs);
SDKLI_FUNC void sdklK_ret (FuncState *fs, int first, int nret);
SDKLI_FUNC void sdklK_patchlist (FuncState *fs, int list, int target);
SDKLI_FUNC void sdklK_patchtohere (FuncState *fs, int list);
SDKLI_FUNC void sdklK_concat (FuncState *fs, int *l1, int l2);
SDKLI_FUNC int sdklK_getlabel (FuncState *fs);
SDKLI_FUNC void sdklK_prefix (FuncState *fs, UnOpr op, expdesc *v, int line);
SDKLI_FUNC void sdklK_infix (FuncState *fs, BinOpr op, expdesc *v);
SDKLI_FUNC void sdklK_posfix (FuncState *fs, BinOpr op, expdesc *v1,
                            expdesc *v2, int line);
SDKLI_FUNC void sdklK_settablesize (FuncState *fs, int pc,
                                  int ra, int asize, int hsize);
SDKLI_FUNC void sdklK_setlist (FuncState *fs, int base, int nelems, int tostore);
SDKLI_FUNC void sdklK_finish (FuncState *fs);
SDKLI_FUNC l_noret sdklK_semerror (LexState *ls, const char *msg);


#endif
