/*
** $Id: lundump.h $
** load precompiled SDKL chunks
** See Copyright Notice in sdkl.h
*/

#ifndef lundump_h
#define lundump_h

#include "llimits.h"
#include "lobject.h"
#include "lzio.h"


/* data to catch conversion errors */
#define SDKLC_DATA	"\x19\x93\r\n\x1a\n"

#define SDKLC_INT	0x5678
#define SDKLC_NUM	cast_num(370.5)

/*
** Encode major-minor version in one byte, one nibble for each
*/
#define MYINT(s)	(s[0]-'0')  /* assume one-digit numerals */
#define SDKLC_VERSION	(MYINT(SDKL_VERSION_MAJOR)*16+MYINT(SDKL_VERSION_MINOR))

#define SDKLC_FORMAT	0	/* this is the official format */

/* load one chunk; from lundump.c */
SDKLI_FUNC LClosure* sdklU_undump (sdkl_State* L, ZIO* Z, const char* name);

/* dump one chunk; from ldump.c */
SDKLI_FUNC int sdklU_dump (sdkl_State* L, const Proto* f, sdkl_Writer w,
                         void* data, int strip);

#endif
