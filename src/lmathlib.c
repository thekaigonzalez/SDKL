/*
** $Id: lmathlib.c $
** Standard mathematical library
** See Copyright Notice in sdkl.h
*/

#define lmathlib_c
#define SDKL_LIB

#include "lprefix.h"


#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "sdkl.h"

#include "sdklauxillary.h"
#include "sdkllib.h"


#undef PI
#define PI	(l_mathop(3.141592653589793238462643383279502884))


static int math_abs (sdkl_State *L) {
  if (sdkl_isinteger(L, 1)) {
    sdkl_Integer n = sdkl_tointeger(L, 1);
    if (n < 0) n = (sdkl_Integer)(0u - (sdkl_Unsigned)n);
    sdkl_pushinteger(L, n);
  }
  else
    sdkl_pushnumber(L, l_mathop(fabs)(sdklL_checknumber(L, 1)));
  return 1;
}

static int math_sin (sdkl_State *L) {
  sdkl_pushnumber(L, l_mathop(sin)(sdklL_checknumber(L, 1)));
  return 1;
}

static int math_cos (sdkl_State *L) {
  sdkl_pushnumber(L, l_mathop(cos)(sdklL_checknumber(L, 1)));
  return 1;
}

static int math_tan (sdkl_State *L) {
  sdkl_pushnumber(L, l_mathop(tan)(sdklL_checknumber(L, 1)));
  return 1;
}

static int math_asin (sdkl_State *L) {
  sdkl_pushnumber(L, l_mathop(asin)(sdklL_checknumber(L, 1)));
  return 1;
}

static int math_acos (sdkl_State *L) {
  sdkl_pushnumber(L, l_mathop(acos)(sdklL_checknumber(L, 1)));
  return 1;
}

static int math_atan (sdkl_State *L) {
  sdkl_Number y = sdklL_checknumber(L, 1);
  sdkl_Number x = sdklL_optnumber(L, 2, 1);
  sdkl_pushnumber(L, l_mathop(atan2)(y, x));
  return 1;
}


static int math_toint (sdkl_State *L) {
  int valid;
  sdkl_Integer n = sdkl_tointegerx(L, 1, &valid);
  if (l_likely(valid))
    sdkl_pushinteger(L, n);
  else {
    sdklL_checkany(L, 1);
    sdklL_pushfail(L);  /* value is not convertible to integer */
  }
  return 1;
}


static void pushnumint (sdkl_State *L, sdkl_Number d) {
  sdkl_Integer n;
  if (sdkl_numbertointeger(d, &n))  /* does 'd' fit in an integer? */
    sdkl_pushinteger(L, n);  /* result is integer */
  else
    sdkl_pushnumber(L, d);  /* result is float */
}


static int math_floor (sdkl_State *L) {
  if (sdkl_isinteger(L, 1))
    sdkl_settop(L, 1);  /* integer is its own floor */
  else {
    sdkl_Number d = l_mathop(floor)(sdklL_checknumber(L, 1));
    pushnumint(L, d);
  }
  return 1;
}


static int math_ceil (sdkl_State *L) {
  if (sdkl_isinteger(L, 1))
    sdkl_settop(L, 1);  /* integer is its own ceil */
  else {
    sdkl_Number d = l_mathop(ceil)(sdklL_checknumber(L, 1));
    pushnumint(L, d);
  }
  return 1;
}


static int math_fmod (sdkl_State *L) {
  if (sdkl_isinteger(L, 1) && sdkl_isinteger(L, 2)) {
    sdkl_Integer d = sdkl_tointeger(L, 2);
    if ((sdkl_Unsigned)d + 1u <= 1u) {  /* special cases: -1 or 0 */
      sdklL_argcheck(L, d != 0, 2, "zero");
      sdkl_pushinteger(L, 0);  /* avoid overflow with 0x80000... / -1 */
    }
    else
      sdkl_pushinteger(L, sdkl_tointeger(L, 1) % d);
  }
  else
    sdkl_pushnumber(L, l_mathop(fmod)(sdklL_checknumber(L, 1),
                                     sdklL_checknumber(L, 2)));
  return 1;
}


/*
** next function does not use 'modf', avoiding problems with 'double*'
** (which is not compatible with 'float*') when sdkl_Number is not
** 'double'.
*/
static int math_modf (sdkl_State *L) {
  if (sdkl_isinteger(L ,1)) {
    sdkl_settop(L, 1);  /* number is its own integer part */
    sdkl_pushnumber(L, 0);  /* no fractional part */
  }
  else {
    sdkl_Number n = sdklL_checknumber(L, 1);
    /* integer part (rounds toward zero) */
    sdkl_Number ip = (n < 0) ? l_mathop(ceil)(n) : l_mathop(floor)(n);
    pushnumint(L, ip);
    /* fractional part (test needed for inf/-inf) */
    sdkl_pushnumber(L, (n == ip) ? l_mathop(0.0) : (n - ip));
  }
  return 2;
}


static int math_sqrt (sdkl_State *L) {
  sdkl_pushnumber(L, l_mathop(sqrt)(sdklL_checknumber(L, 1)));
  return 1;
}


static int math_ult (sdkl_State *L) {
  sdkl_Integer a = sdklL_checkinteger(L, 1);
  sdkl_Integer b = sdklL_checkinteger(L, 2);
  sdkl_pushboolean(L, (sdkl_Unsigned)a < (sdkl_Unsigned)b);
  return 1;
}

static int math_log (sdkl_State *L) {
  sdkl_Number x = sdklL_checknumber(L, 1);
  sdkl_Number res;
  if (sdkl_isnoneornil(L, 2))
    res = l_mathop(log)(x);
  else {
    sdkl_Number base = sdklL_checknumber(L, 2);
#if !defined(SDKL_USE_C89)
    if (base == l_mathop(2.0))
      res = l_mathop(log2)(x);
    else
#endif
    if (base == l_mathop(10.0))
      res = l_mathop(log10)(x);
    else
      res = l_mathop(log)(x)/l_mathop(log)(base);
  }
  sdkl_pushnumber(L, res);
  return 1;
}

static int math_exp (sdkl_State *L) {
  sdkl_pushnumber(L, l_mathop(exp)(sdklL_checknumber(L, 1)));
  return 1;
}

static int math_deg (sdkl_State *L) {
  sdkl_pushnumber(L, sdklL_checknumber(L, 1) * (l_mathop(180.0) / PI));
  return 1;
}

static int math_rad (sdkl_State *L) {
  sdkl_pushnumber(L, sdklL_checknumber(L, 1) * (PI / l_mathop(180.0)));
  return 1;
}


static int math_min (sdkl_State *L) {
  int n = sdkl_gettop(L);  /* number of arguments */
  int imin = 1;  /* index of current minimum value */
  int i;
  sdklL_argcheck(L, n >= 1, 1, "value expected");
  for (i = 2; i <= n; i++) {
    if (sdkl_compare(L, i, imin, SDKL_OPLT))
      imin = i;
  }
  sdkl_pushvalue(L, imin);
  return 1;
}


static int math_max (sdkl_State *L) {
  int n = sdkl_gettop(L);  /* number of arguments */
  int imax = 1;  /* index of current maximum value */
  int i;
  sdklL_argcheck(L, n >= 1, 1, "value expected");
  for (i = 2; i <= n; i++) {
    if (sdkl_compare(L, imax, i, SDKL_OPLT))
      imax = i;
  }
  sdkl_pushvalue(L, imax);
  return 1;
}


static int math_type (sdkl_State *L) {
  if (sdkl_type(L, 1) == SDKL_TNUMBER)
    sdkl_pushstring(L, (sdkl_isinteger(L, 1)) ? "integer" : "float");
  else {
    sdklL_checkany(L, 1);
    sdklL_pushfail(L);
  }
  return 1;
}



/*
** {==================================================================
** Pseudo-Random Number Generator based on 'xoshiro256**'.
** ===================================================================
*/

/* number of binary digits in the mantissa of a float */
#define FIGS	l_floatatt(MANT_DIG)

#if FIGS > 64
/* there are only 64 random bits; use them all */
#undef FIGS
#define FIGS	64
#endif


/*
** SDKL_RAND32 forces the use of 32-bit integers in the implementation
** of the PRN generator (mainly for testing).
*/
#if !defined(SDKL_RAND32) && !defined(Rand64)

/* try to find an integer type with at least 64 bits */

#if (ULONG_MAX >> 31 >> 31) >= 3

/* 'long' has at least 64 bits */
#define Rand64		unsigned long

#elif !defined(SDKL_USE_C89) && defined(LLONG_MAX)

/* there is a 'long long' type (which must have at least 64 bits) */
#define Rand64		unsigned long long

#elif (SDKL_MAXUNSIGNED >> 31 >> 31) >= 3

/* 'sdkl_Integer' has at least 64 bits */
#define Rand64		sdkl_Unsigned

#endif

#endif


#if defined(Rand64)  /* { */

/*
** Standard implementation, using 64-bit integers.
** If 'Rand64' has more than 64 bits, the extra bits do not interfere
** with the 64 initial bits, except in a right shift. Moreover, the
** final result has to discard the extra bits.
*/

/* avoid using extra bits when needed */
#define trim64(x)	((x) & 0xffffffffffffffffu)


/* rotate left 'x' by 'n' bits */
static Rand64 rotl (Rand64 x, int n) {
  return (x << n) | (trim64(x) >> (64 - n));
}

static Rand64 nextrand (Rand64 *state) {
  Rand64 state0 = state[0];
  Rand64 state1 = state[1];
  Rand64 state2 = state[2] ^ state0;
  Rand64 state3 = state[3] ^ state1;
  Rand64 res = rotl(state1 * 5, 7) * 9;
  state[0] = state0 ^ state3;
  state[1] = state1 ^ state2;
  state[2] = state2 ^ (state1 << 17);
  state[3] = rotl(state3, 45);
  return res;
}


/* must take care to not shift stuff by more than 63 slots */


/*
** Convert bits from a random integer into a float in the
** interval [0,1), getting the higher FIG bits from the
** random unsigned integer and converting that to a float.
*/

/* must throw out the extra (64 - FIGS) bits */
#define shift64_FIG	(64 - FIGS)

/* to scale to [0, 1), multiply by scaleFIG = 2^(-FIGS) */
#define scaleFIG	(l_mathop(0.5) / ((Rand64)1 << (FIGS - 1)))

static sdkl_Number I2d (Rand64 x) {
  return (sdkl_Number)(trim64(x) >> shift64_FIG) * scaleFIG;
}

/* convert a 'Rand64' to a 'sdkl_Unsigned' */
#define I2UInt(x)	((sdkl_Unsigned)trim64(x))

/* convert a 'sdkl_Unsigned' to a 'Rand64' */
#define Int2I(x)	((Rand64)(x))


#else	/* no 'Rand64'   }{ */

/* get an integer with at least 32 bits */
#if SDKLI_IS32INT
typedef unsigned int lu_int32;
#else
typedef unsigned long lu_int32;
#endif


/*
** Use two 32-bit integers to represent a 64-bit quantity.
*/
typedef struct Rand64 {
  lu_int32 h;  /* higher half */
  lu_int32 l;  /* lower half */
} Rand64;


/*
** If 'lu_int32' has more than 32 bits, the extra bits do not interfere
** with the 32 initial bits, except in a right shift and comparisons.
** Moreover, the final result has to discard the extra bits.
*/

/* avoid using extra bits when needed */
#define trim32(x)	((x) & 0xffffffffu)


/*
** basic operations on 'Rand64' values
*/

/* build a new Rand64 value */
static Rand64 packI (lu_int32 h, lu_int32 l) {
  Rand64 result;
  result.h = h;
  result.l = l;
  return result;
}

/* return i << n */
static Rand64 Ishl (Rand64 i, int n) {
  sdkl_assert(n > 0 && n < 32);
  return packI((i.h << n) | (trim32(i.l) >> (32 - n)), i.l << n);
}

/* i1 ^= i2 */
static void Ixor (Rand64 *i1, Rand64 i2) {
  i1->h ^= i2.h;
  i1->l ^= i2.l;
}

/* return i1 + i2 */
static Rand64 Iadd (Rand64 i1, Rand64 i2) {
  Rand64 result = packI(i1.h + i2.h, i1.l + i2.l);
  if (trim32(result.l) < trim32(i1.l))  /* carry? */
    result.h++;
  return result;
}

/* return i * 5 */
static Rand64 times5 (Rand64 i) {
  return Iadd(Ishl(i, 2), i);  /* i * 5 == (i << 2) + i */
}

/* return i * 9 */
static Rand64 times9 (Rand64 i) {
  return Iadd(Ishl(i, 3), i);  /* i * 9 == (i << 3) + i */
}

/* return 'i' rotated left 'n' bits */
static Rand64 rotl (Rand64 i, int n) {
  sdkl_assert(n > 0 && n < 32);
  return packI((i.h << n) | (trim32(i.l) >> (32 - n)),
               (trim32(i.h) >> (32 - n)) | (i.l << n));
}

/* for offsets larger than 32, rotate right by 64 - offset */
static Rand64 rotl1 (Rand64 i, int n) {
  sdkl_assert(n > 32 && n < 64);
  n = 64 - n;
  return packI((trim32(i.h) >> n) | (i.l << (32 - n)),
               (i.h << (32 - n)) | (trim32(i.l) >> n));
}

/*
** implementation of 'xoshiro256**' algorithm on 'Rand64' values
*/
static Rand64 nextrand (Rand64 *state) {
  Rand64 res = times9(rotl(times5(state[1]), 7));
  Rand64 t = Ishl(state[1], 17);
  Ixor(&state[2], state[0]);
  Ixor(&state[3], state[1]);
  Ixor(&state[1], state[2]);
  Ixor(&state[0], state[3]);
  Ixor(&state[2], t);
  state[3] = rotl1(state[3], 45);
  return res;
}


/*
** Converts a 'Rand64' into a float.
*/

/* an unsigned 1 with proper type */
#define UONE		((lu_int32)1)


#if FIGS <= 32

/* 2^(-FIGS) */
#define scaleFIG       (l_mathop(0.5) / (UONE << (FIGS - 1)))

/*
** get up to 32 bits from higher half, shifting right to
** throw out the extra bits.
*/
static sdkl_Number I2d (Rand64 x) {
  sdkl_Number h = (sdkl_Number)(trim32(x.h) >> (32 - FIGS));
  return h * scaleFIG;
}

#else	/* 32 < FIGS <= 64 */

/* must take care to not shift stuff by more than 31 slots */

/* 2^(-FIGS) = 1.0 / 2^30 / 2^3 / 2^(FIGS-33) */
#define scaleFIG  \
	((sdkl_Number)1.0 / (UONE << 30) / 8.0 / (UONE << (FIGS - 33)))

/*
** use FIGS - 32 bits from lower half, throwing out the other
** (32 - (FIGS - 32)) = (64 - FIGS) bits
*/
#define shiftLOW	(64 - FIGS)

/*
** higher 32 bits go after those (FIGS - 32) bits: shiftHI = 2^(FIGS - 32)
*/
#define shiftHI		((sdkl_Number)(UONE << (FIGS - 33)) * 2.0)


static sdkl_Number I2d (Rand64 x) {
  sdkl_Number h = (sdkl_Number)trim32(x.h) * shiftHI;
  sdkl_Number l = (sdkl_Number)(trim32(x.l) >> shiftLOW);
  return (h + l) * scaleFIG;
}

#endif


/* convert a 'Rand64' to a 'sdkl_Unsigned' */
static sdkl_Unsigned I2UInt (Rand64 x) {
  return ((sdkl_Unsigned)trim32(x.h) << 31 << 1) | (sdkl_Unsigned)trim32(x.l);
}

/* convert a 'sdkl_Unsigned' to a 'Rand64' */
static Rand64 Int2I (sdkl_Unsigned n) {
  return packI((lu_int32)(n >> 31 >> 1), (lu_int32)n);
}

#endif  /* } */


/*
** A state uses four 'Rand64' values.
*/
typedef struct {
  Rand64 s[4];
} RanState;


/*
** Project the random integer 'ran' into the interval [0, n].
** Because 'ran' has 2^B possible values, the projection can only be
** uniform when the size of the interval is a power of 2 (exact
** division). Otherwise, to get a uniform projection into [0, n], we
** first compute 'lim', the smallest Mersenne number not smaller than
** 'n'. We then project 'ran' into the interval [0, lim].  If the result
** is inside [0, n], we are done. Otherwise, we try with another 'ran',
** until we have a result inside the interval.
*/
static sdkl_Unsigned project (sdkl_Unsigned ran, sdkl_Unsigned n,
                             RanState *state) {
  if ((n & (n + 1)) == 0)  /* is 'n + 1' a power of 2? */
    return ran & n;  /* no bias */
  else {
    sdkl_Unsigned lim = n;
    /* compute the smallest (2^b - 1) not smaller than 'n' */
    lim |= (lim >> 1);
    lim |= (lim >> 2);
    lim |= (lim >> 4);
    lim |= (lim >> 8);
    lim |= (lim >> 16);
#if (SDKL_MAXUNSIGNED >> 31) >= 3
    lim |= (lim >> 32);  /* integer type has more than 32 bits */
#endif
    sdkl_assert((lim & (lim + 1)) == 0  /* 'lim + 1' is a power of 2, */
      && lim >= n  /* not smaller than 'n', */
      && (lim >> 1) < n);  /* and it is the smallest one */
    while ((ran &= lim) > n)  /* project 'ran' into [0..lim] */
      ran = I2UInt(nextrand(state->s));  /* not inside [0..n]? try again */
    return ran;
  }
}


static int math_random (sdkl_State *L) {
  sdkl_Integer low, up;
  sdkl_Unsigned p;
  RanState *state = (RanState *)sdkl_touserdata(L, sdkl_upvalueindex(1));
  Rand64 rv = nextrand(state->s);  /* next pseudo-random value */
  switch (sdkl_gettop(L)) {  /* check number of arguments */
    case 0: {  /* no arguments */
      sdkl_pushnumber(L, I2d(rv));  /* float between 0 and 1 */
      return 1;
    }
    case 1: {  /* only upper limit */
      low = 1;
      up = sdklL_checkinteger(L, 1);
      if (up == 0) {  /* single 0 as argument? */
        sdkl_pushinteger(L, I2UInt(rv));  /* full random integer */
        return 1;
      }
      break;
    }
    case 2: {  /* lower and upper limits */
      low = sdklL_checkinteger(L, 1);
      up = sdklL_checkinteger(L, 2);
      break;
    }
    default: return sdklL_error(L, "wrong number of arguments");
  }
  /* random integer in the interval [low, up] */
  sdklL_argcheck(L, low <= up, 1, "interval is empty");
  /* project random integer into the interval [0, up - low] */
  p = project(I2UInt(rv), (sdkl_Unsigned)up - (sdkl_Unsigned)low, state);
  sdkl_pushinteger(L, p + (sdkl_Unsigned)low);
  return 1;
}


static void setseed (sdkl_State *L, Rand64 *state,
                     sdkl_Unsigned n1, sdkl_Unsigned n2) {
  int i;
  state[0] = Int2I(n1);
  state[1] = Int2I(0xff);  /* avoid a zero state */
  state[2] = Int2I(n2);
  state[3] = Int2I(0);
  for (i = 0; i < 16; i++)
    nextrand(state);  /* discard initial values to "spread" seed */
  sdkl_pushinteger(L, n1);
  sdkl_pushinteger(L, n2);
}


/*
** Set a "random" seed. To get some randomness, use the current time
** and the address of 'L' (in case the machine does address space layout
** randomization).
*/
static void randseed (sdkl_State *L, RanState *state) {
  sdkl_Unsigned seed1 = (sdkl_Unsigned)time(NULL);
  sdkl_Unsigned seed2 = (sdkl_Unsigned)(size_t)L;
  setseed(L, state->s, seed1, seed2);
}


static int math_randomseed (sdkl_State *L) {
  RanState *state = (RanState *)sdkl_touserdata(L, sdkl_upvalueindex(1));
  if (sdkl_isnone(L, 1)) {
    randseed(L, state);
  }
  else {
    sdkl_Integer n1 = sdklL_checkinteger(L, 1);
    sdkl_Integer n2 = sdklL_optinteger(L, 2, 0);
    setseed(L, state->s, n1, n2);
  }
  return 2;  /* return seeds */
}


static const sdklL_Reg randfuncs[] = {
  {"random", math_random},
  {"randomseed", math_randomseed},
  {NULL, NULL}
};


/*
** Register the random functions and initialize their state.
*/
static void setrandfunc (sdkl_State *L) {
  RanState *state = (RanState *)sdkl_newuserdatauv(L, sizeof(RanState), 0);
  randseed(L, state);  /* initialize with a "random" seed */
  sdkl_pop(L, 2);  /* remove pushed seeds */
  sdklL_setfuncs(L, randfuncs, 1);
}

/* }================================================================== */


/*
** {==================================================================
** Deprecated functions (for compatibility only)
** ===================================================================
*/
#if defined(SDKL_COMPAT_MATHLIB)

static int math_cosh (sdkl_State *L) {
  sdkl_pushnumber(L, l_mathop(cosh)(sdklL_checknumber(L, 1)));
  return 1;
}

static int math_sinh (sdkl_State *L) {
  sdkl_pushnumber(L, l_mathop(sinh)(sdklL_checknumber(L, 1)));
  return 1;
}

static int math_tanh (sdkl_State *L) {
  sdkl_pushnumber(L, l_mathop(tanh)(sdklL_checknumber(L, 1)));
  return 1;
}

static int math_pow (sdkl_State *L) {
  sdkl_Number x = sdklL_checknumber(L, 1);
  sdkl_Number y = sdklL_checknumber(L, 2);
  sdkl_pushnumber(L, l_mathop(pow)(x, y));
  return 1;
}

static int math_frexp (sdkl_State *L) {
  int e;
  sdkl_pushnumber(L, l_mathop(frexp)(sdklL_checknumber(L, 1), &e));
  sdkl_pushinteger(L, e);
  return 2;
}

static int math_ldexp (sdkl_State *L) {
  sdkl_Number x = sdklL_checknumber(L, 1);
  int ep = (int)sdklL_checkinteger(L, 2);
  sdkl_pushnumber(L, l_mathop(ldexp)(x, ep));
  return 1;
}

static int math_log10 (sdkl_State *L) {
  sdkl_pushnumber(L, l_mathop(log10)(sdklL_checknumber(L, 1)));
  return 1;
}

#endif
/* }================================================================== */



static const sdklL_Reg mathlib[] = {
  {"abs",   math_abs},
  {"acos",  math_acos},
  {"asin",  math_asin},
  {"atan",  math_atan},
  {"ceil",  math_ceil},
  {"cos",   math_cos},
  {"deg",   math_deg},
  {"exp",   math_exp},
  {"tointeger", math_toint},
  {"floor", math_floor},
  {"fmod",   math_fmod},
  {"ult",   math_ult},
  {"log",   math_log},
  {"max",   math_max},
  {"min",   math_min},
  {"modf",   math_modf},
  {"rad",   math_rad},
  {"sin",   math_sin},
  {"sqrt",  math_sqrt},
  {"tan",   math_tan},
  {"type", math_type},
#if defined(SDKL_COMPAT_MATHLIB)
  {"atan2", math_atan},
  {"cosh",   math_cosh},
  {"sinh",   math_sinh},
  {"tanh",   math_tanh},
  {"pow",   math_pow},
  {"frexp", math_frexp},
  {"ldexp", math_ldexp},
  {"log10", math_log10},
#endif
  /* placeholders */
  {"random", NULL},
  {"randomseed", NULL},
  {"pi", NULL},
  {"huge", NULL},
  {"maxinteger", NULL},
  {"mininteger", NULL},
  {NULL, NULL}
};


/*
** Open math library
*/
SDKLMOD_API int sdklopen_math (sdkl_State *L) {
  sdklL_newlib(L, mathlib);
  sdkl_pushnumber(L, PI);
  sdkl_setfield(L, -2, "pi");
  sdkl_pushnumber(L, (sdkl_Number)HUGE_VAL);
  sdkl_setfield(L, -2, "huge");
  sdkl_pushinteger(L, SDKL_MAXINTEGER);
  sdkl_setfield(L, -2, "maxinteger");
  sdkl_pushinteger(L, SDKL_MININTEGER);
  sdkl_setfield(L, -2, "mininteger");
  setrandfunc(L);
  return 1;
}

