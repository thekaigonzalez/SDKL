/*
** $Id: lstate.c $
** Global State
** See Copyright Notice in sdkl.h
*/

#define lstate_c
#define SDKL_CORE

#include "lprefix.h"


#include <stddef.h>
#include <string.h>

#include "sdkl.h"

#include "lapi.h"
#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "lgc.h"
#include "llex.h"
#include "lmem.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "ltm.h"



/*
** thread state + extra space
*/
typedef struct LX {
  lu_byte extra_[SDKL_EXTRASPACE];
  sdkl_State l;
} LX;


/*
** Main thread combines a thread state and the global state
*/
typedef struct LG {
  LX l;
  global_State g;
} LG;



#define fromstate(L)	(cast(LX *, cast(lu_byte *, (L)) - offsetof(LX, l)))


/*
** A macro to create a "random" seed when a state is created;
** the seed is used to randomize string hashes.
*/
#if !defined(sdkli_makeseed)

#include <time.h>

/*
** Compute an initial seed with some level of randomness.
** Rely on Address Space Layout Randomization (if present) and
** current time.
*/
#define addbuff(b,p,e) \
  { size_t t = cast_sizet(e); \
    memcpy(b + p, &t, sizeof(t)); p += sizeof(t); }

static unsigned int sdkli_makeseed (sdkl_State *L) {
  char buff[3 * sizeof(size_t)];
  unsigned int h = cast_uint(time(NULL));
  int p = 0;
  addbuff(buff, p, L);  /* heap variable */
  addbuff(buff, p, &h);  /* local variable */
  addbuff(buff, p, &sdkl_newstate);  /* public function */
  sdkl_assert(p == sizeof(buff));
  return sdklS_hash(buff, p, h);
}

#endif


/*
** set GCdebt to a new value keeping the value (totalbytes + GCdebt)
** invariant (and avoiding underflows in 'totalbytes')
*/
void sdklE_setdebt (global_State *g, l_mem debt) {
  l_mem tb = gettotalbytes(g);
  sdkl_assert(tb > 0);
  if (debt < tb - MAX_LMEM)
    debt = tb - MAX_LMEM;  /* will make 'totalbytes == MAX_LMEM' */
  g->totalbytes = tb - debt;
  g->GCdebt = debt;
}


SDKL_API int sdkl_setcstacklimit (sdkl_State *L, unsigned int limit) {
  UNUSED(L); UNUSED(limit);
  return SDKLI_MAXCCALLS;  /* warning?? */
}


CallInfo *sdklE_extendCI (sdkl_State *L) {
  CallInfo *ci;
  sdkl_assert(L->ci->next == NULL);
  ci = sdklM_new(L, CallInfo);
  sdkl_assert(L->ci->next == NULL);
  L->ci->next = ci;
  ci->previous = L->ci;
  ci->next = NULL;
  ci->u.l.trap = 0;
  L->nci++;
  return ci;
}


/*
** free all CallInfo structures not in use by a thread
*/
void sdklE_freeCI (sdkl_State *L) {
  CallInfo *ci = L->ci;
  CallInfo *next = ci->next;
  ci->next = NULL;
  while ((ci = next) != NULL) {
    next = ci->next;
    sdklM_free(L, ci);
    L->nci--;
  }
}


/*
** free half of the CallInfo structures not in use by a thread,
** keeping the first one.
*/
void sdklE_shrinkCI (sdkl_State *L) {
  CallInfo *ci = L->ci->next;  /* first free CallInfo */
  CallInfo *next;
  if (ci == NULL)
    return;  /* no extra elements */
  while ((next = ci->next) != NULL) {  /* two extra elements? */
    CallInfo *next2 = next->next;  /* next's next */
    ci->next = next2;  /* remove next from the list */
    L->nci--;
    sdklM_free(L, next);  /* free next */
    if (next2 == NULL)
      break;  /* no more elements */
    else {
      next2->previous = ci;
      ci = next2;  /* continue */
    }
  }
}


/*
** Called when 'getCcalls(L)' larger or equal to SDKLI_MAXCCALLS.
** If equal, raises an overflow error. If value is larger than
** SDKLI_MAXCCALLS (which means it is handling an overflow) but
** not much larger, does not report an error (to allow overflow
** handling to work).
*/
void sdklE_checkcstack (sdkl_State *L) {
  if (getCcalls(L) == SDKLI_MAXCCALLS)
    sdklG_runerror(L, "C stack overflow");
  else if (getCcalls(L) >= (SDKLI_MAXCCALLS / 10 * 11))
    sdklD_throw(L, SDKL_ERRERR);  /* error while handing stack error */
}


SDKLI_FUNC void sdklE_incCstack (sdkl_State *L) {
  L->nCcalls++;
  if (l_unlikely(getCcalls(L) >= SDKLI_MAXCCALLS))
    sdklE_checkcstack(L);
}


static void stack_init (sdkl_State *L1, sdkl_State *L) {
  int i; CallInfo *ci;
  /* initialize stack array */
  L1->stack = sdklM_newvector(L, BASIC_STACK_SIZE + EXTRA_STACK, StackValue);
  L1->tbclist = L1->stack;
  for (i = 0; i < BASIC_STACK_SIZE + EXTRA_STACK; i++)
    setnilvalue(s2v(L1->stack + i));  /* erase new stack */
  L1->top = L1->stack;
  L1->stack_last = L1->stack + BASIC_STACK_SIZE;
  /* initialize first ci */
  ci = &L1->base_ci;
  ci->next = ci->previous = NULL;
  ci->callstatus = CIST_C;
  ci->func = L1->top;
  ci->u.c.k = NULL;
  ci->nresults = 0;
  setnilvalue(s2v(L1->top));  /* 'function' entry for this 'ci' */
  L1->top++;
  ci->top = L1->top + SDKL_MINSTACK;
  L1->ci = ci;
}


static void freestack (sdkl_State *L) {
  if (L->stack == NULL)
    return;  /* stack not completely built yet */
  L->ci = &L->base_ci;  /* free the entire 'ci' list */
  sdklE_freeCI(L);
  sdkl_assert(L->nci == 0);
  sdklM_freearray(L, L->stack, stacksize(L) + EXTRA_STACK);  /* free stack */
}


/*
** Create registry table and its predefined values
*/
static void init_registry (sdkl_State *L, global_State *g) {
  /* create registry */
  Table *registry = sdklH_new(L);
  sethvalue(L, &g->l_registry, registry);
  sdklH_resize(L, registry, SDKL_RIDX_LAST, 0);
  /* registry[SDKL_RIDX_MAINTHREAD] = L */
  setthvalue(L, &registry->array[SDKL_RIDX_MAINTHREAD - 1], L);
  /* registry[SDKL_RIDX_GLOBALS] = new table (table of globals) */
  sethvalue(L, &registry->array[SDKL_RIDX_GLOBALS - 1], sdklH_new(L));
}


/*
** open parts of the state that may cause memory-allocation errors.
*/
static void f_sdklopen (sdkl_State *L, void *ud) {
  global_State *g = G(L);
  UNUSED(ud);
  stack_init(L, L);  /* init stack */
  init_registry(L, g);
  sdklS_init(L);
  sdklT_init(L);
  sdklX_init(L);
  g->gcrunning = 1;  /* allow gc */
  setnilvalue(&g->nilvalue);  /* now state is complete */
  sdkli_userstateopen(L);
}


/*
** preinitialize a thread with consistent values without allocating
** any memory (to avoid errors)
*/
static void preinit_thread (sdkl_State *L, global_State *g) {
  G(L) = g;
  L->stack = NULL;
  L->ci = NULL;
  L->nci = 0;
  L->twups = L;  /* thread has no upvalues */
  L->nCcalls = 0;
  L->errorJmp = NULL;
  L->hook = NULL;
  L->hookmask = 0;
  L->basehookcount = 0;
  L->allowhook = 1;
  resethookcount(L);
  L->openupval = NULL;
  L->status = SDKL_OK;
  L->errfunc = 0;
  L->oldpc = 0;
}


static void close_state (sdkl_State *L) {
  global_State *g = G(L);
  if (!completestate(g))  /* closing a partially built state? */
    sdklC_freeallobjects(L);  /* jucst collect its objects */
  else {  /* closing a fully built state */
    sdklD_closeprotected(L, 1, SDKL_OK);  /* close all upvalues */
    sdklC_freeallobjects(L);  /* collect all objects */
    sdkli_userstateclose(L);
  }
  sdklM_freearray(L, G(L)->strt.hash, G(L)->strt.size);
  freestack(L);
  sdkl_assert(gettotalbytes(g) == sizeof(LG));
  (*g->frealloc)(g->ud, fromstate(L), sizeof(LG), 0);  /* free main block */
}


SDKL_API sdkl_State *sdkl_newthread (sdkl_State *L) {
  global_State *g;
  sdkl_State *L1;
  sdkl_lock(L);
  g = G(L);
  sdklC_checkGC(L);
  /* create new thread */
  L1 = &cast(LX *, sdklM_newobject(L, SDKL_TTHREAD, sizeof(LX)))->l;
  L1->marked = sdklC_white(g);
  L1->tt = SDKL_VTHREAD;
  /* link it on list 'allgc' */
  L1->next = g->allgc;
  g->allgc = obj2gco(L1);
  /* anchor it on L stack */
  setthvalue2s(L, L->top, L1);
  api_incr_top(L);
  preinit_thread(L1, g);
  L1->hookmask = L->hookmask;
  L1->basehookcount = L->basehookcount;
  L1->hook = L->hook;
  resethookcount(L1);
  /* initialize L1 extra space */
  memcpy(sdkl_getextraspace(L1), sdkl_getextraspace(g->mainthread),
         SDKL_EXTRASPACE);
  sdkli_userstatethread(L, L1);
  stack_init(L1, L);  /* init stack */
  sdkl_unlock(L);
  return L1;
}


void sdklE_freethread (sdkl_State *L, sdkl_State *L1) {
  LX *l = fromstate(L1);
  sdklF_closeupval(L1, L1->stack);  /* close all upvalues */
  sdkl_assert(L1->openupval == NULL);
  sdkli_userstatefree(L, L1);
  freestack(L1);
  sdklM_free(L, l);
}


int sdklE_resetthread (sdkl_State *L, int status) {
  CallInfo *ci = L->ci = &L->base_ci;  /* unwind CallInfo list */
  setnilvalue(s2v(L->stack));  /* 'function' entry for basic 'ci' */
  ci->func = L->stack;
  ci->callstatus = CIST_C;
  if (status == SDKL_YIELD)
    status = SDKL_OK;
  status = sdklD_closeprotected(L, 1, status);
  if (status != SDKL_OK)  /* errors? */
    sdklD_seterrorobj(L, status, L->stack + 1);
  else
    L->top = L->stack + 1;
  ci->top = L->top + SDKL_MINSTACK;
  L->status = cast_byte(status);
  sdklD_reallocstack(L, cast_int(ci->top - L->stack), 0);
  return status;
}


SDKL_API int sdkl_resetthread (sdkl_State *L) {
  int status;
  sdkl_lock(L);
  status = sdklE_resetthread(L, L->status);
  sdkl_unlock(L);
  return status;
}


SDKL_API sdkl_State *sdkl_newstate (sdkl_Alloc f, void *ud) {
  int i;
  sdkl_State *L;
  global_State *g;
  LG *l = cast(LG *, (*f)(ud, NULL, SDKL_TTHREAD, sizeof(LG)));
  if (l == NULL) return NULL;
  L = &l->l.l;
  g = &l->g;
  L->tt = SDKL_VTHREAD;
  g->currentwhite = bitmask(WHITE0BIT);
  L->marked = sdklC_white(g);
  preinit_thread(L, g);
  g->allgc = obj2gco(L);  /* by now, only object is the main thread */
  L->next = NULL;
  incnny(L);  /* main thread is always non yieldable */
  g->frealloc = f;
  g->ud = ud;
  g->warnf = NULL;
  g->ud_warn = NULL;
  g->mainthread = L;
  g->seed = sdkli_makeseed(L);
  g->gcrunning = 0;  /* no GC while building state */
  g->strt.size = g->strt.nuse = 0;
  g->strt.hash = NULL;
  setnilvalue(&g->l_registry);
  g->panic = NULL;
  g->gcstate = GCSpause;
  g->gckind = KGC_INC;
  g->gcstopem = 0;
  g->gcemergency = 0;
  g->finobj = g->tobefnz = g->fixedgc = NULL;
  g->firstold1 = g->survival = g->old1 = g->reallyold = NULL;
  g->finobjsur = g->finobjold1 = g->finobjrold = NULL;
  g->sweepgc = NULL;
  g->gray = g->grayagain = NULL;
  g->weak = g->ephemeron = g->allweak = NULL;
  g->twups = NULL;
  g->totalbytes = sizeof(LG);
  g->GCdebt = 0;
  g->lastatomic = 0;
  setivalue(&g->nilvalue, 0);  /* to signal that state is not yet built */
  setgcparam(g->gcpause, SDKLI_GCPAUSE);
  setgcparam(g->gcstepmul, SDKLI_GCMUL);
  g->gcstepsize = SDKLI_GCSTEPSIZE;
  setgcparam(g->genmajormul, SDKLI_GENMAJORMUL);
  g->genminormul = SDKLI_GENMINORMUL;
  for (i=0; i < SDKL_NUMTAGS; i++) g->mt[i] = NULL;
  if (sdklD_rawrunprotected(L, f_sdklopen, NULL) != SDKL_OK) {
    /* memory allocation error: free partial state */
    close_state(L);
    L = NULL;
  }
  return L;
}


SDKL_API void sdkl_close (sdkl_State *L) {
  sdkl_lock(L);
  L = G(L)->mainthread;  /* only the main thread can be closed */
  close_state(L);
}


void sdklE_warning (sdkl_State *L, const char *msg, int tocont) {
  sdkl_WarnFunction wf = G(L)->warnf;
  if (wf != NULL)
    wf(G(L)->ud_warn, msg, tocont);
}


/*
** Generate a warning from an error message
*/
void sdklE_warnerror (sdkl_State *L, const char *where) {
  TValue *errobj = s2v(L->top - 1);  /* error object */
  const char *msg = (ttisstring(errobj))
                  ? svalue(errobj)
                  : "error object is not a string";
  /* produce warning "error in %s (%s)" (where, msg) */
  sdklE_warning(L, "error in ", 1);
  sdklE_warning(L, where, 1);
  sdklE_warning(L, " (", 1);
  sdklE_warning(L, msg, 1);
  sdklE_warning(L, ")", 0);
}

