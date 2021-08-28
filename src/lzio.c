/*
** $Id: lzio.c $
** Buffered streams
** See Copyright Notice in sdkl.h
*/

#define lzio_c
#define SDKL_CORE

#include "lprefix.h"


#include <string.h>

#include "sdkl.h"

#include "llimits.h"
#include "lmem.h"
#include "lstate.h"
#include "lzio.h"


int sdklZ_fill (ZIO *z) {
  size_t size;
  sdkl_State *L = z->L;
  const char *buff;
  sdkl_unlock(L);
  buff = z->reader(L, z->data, &size);
  sdkl_lock(L);
  if (buff == NULL || size == 0)
    return EOZ;
  z->n = size - 1;  /* discount char being returned */
  z->p = buff;
  return cast_uchar(*(z->p++));
}


void sdklZ_init (sdkl_State *L, ZIO *z, sdkl_Reader reader, void *data) {
  z->L = L;
  z->reader = reader;
  z->data = data;
  z->n = 0;
  z->p = NULL;
}


/* --------------------------------------------------------------- read --- */
size_t sdklZ_read (ZIO *z, void *b, size_t n) {
  while (n) {
    size_t m;
    if (z->n == 0) {  /* no bytes in buffer? */
      if (sdklZ_fill(z) == EOZ)  /* try to read more */
        return n;  /* no more input; return number of missing bytes */
      else {
        z->n++;  /* sdklZ_fill consumed first byte; put it back */
        z->p--;
      }
    }
    m = (n <= z->n) ? n : z->n;  /* min. between n and z->n */
    memcpy(b, z->p, m);
    z->n -= m;
    z->p += m;
    b = (char *)b + m;
    n -= m;
  }
  return 0;
}

