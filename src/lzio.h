/*
** $Id: lzio.h $
** Buffered streams
** See Copyright Notice in sdkl.h
*/


#ifndef lzio_h
#define lzio_h

#include "sdkl.h"

#include "lmem.h"


#define EOZ	(-1)			/* end of stream */

typedef struct Zio ZIO;

#define zgetc(z)  (((z)->n--)>0 ?  cast_uchar(*(z)->p++) : sdklZ_fill(z))


typedef struct Mbuffer {
  char *buffer;
  size_t n;
  size_t buffsize;
} Mbuffer;

#define sdklZ_initbuffer(L, buff) ((buff)->buffer = NULL, (buff)->buffsize = 0)

#define sdklZ_buffer(buff)	((buff)->buffer)
#define sdklZ_sizebuffer(buff)	((buff)->buffsize)
#define sdklZ_bufflen(buff)	((buff)->n)

#define sdklZ_buffremove(buff,i)	((buff)->n -= (i))
#define sdklZ_resetbuffer(buff) ((buff)->n = 0)


#define sdklZ_resizebuffer(L, buff, size) \
	((buff)->buffer = sdklM_reallocvchar(L, (buff)->buffer, \
				(buff)->buffsize, size), \
	(buff)->buffsize = size)

#define sdklZ_freebuffer(L, buff)	sdklZ_resizebuffer(L, buff, 0)


LUAI_FUNC void sdklZ_init (sdkl_State *L, ZIO *z, sdkl_Reader reader,
                                        void *data);
LUAI_FUNC size_t sdklZ_read (ZIO* z, void *b, size_t n);	/* read next n bytes */



/* --------- Private Part ------------------ */

struct Zio {
  size_t n;			/* bytes still unread */
  const char *p;		/* current position in buffer */
  sdkl_Reader reader;		/* reader function */
  void *data;			/* additional data */
  sdkl_State *L;			/* Lua state (for reader) */
};


LUAI_FUNC int sdklZ_fill (ZIO *z);

#endif
