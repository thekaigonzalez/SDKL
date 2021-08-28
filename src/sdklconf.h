/*
** $Id: sdklconf.h $
** Configuration file for SDKL
** See Copyright Notice in sdkl.h
*/


#ifndef sdklconf_h
#define sdklconf_h

#include <limits.h>
#include <stddef.h>


/*
** ===================================================================
** General Configuration File for SDKL
**
** Some definitions here can be changed externally, through the compiler
** (e.g., with '-D' options): They are commented out or protected
** by '#if !defined' guards. However, several other definitions
** should be changed directly here, either because they affect the
** SDKL ABI (by making the changes here, you ensure that all software
** connected to SDKL, such as C libraries, will be compiled with the same
** configuration); or because they are seldom changed.
**
** Search for "@@" to find all configurable definitions.
** ===================================================================
*/


/*
** {====================================================================
** System Configuration: macros to adapt (if needed) SDKL to some
** particular platform, for instance restricting it to C89.
** =====================================================================
*/

/*
@@ SDKL_USE_C89 controls the use of non-ISO-C89 features.
** Define it if you want SDKL to avoid the use of a few C99 features
** or Windows-specific features on Windows.
*/
/* #define SDKL_USE_C89 */


/*
** By default, SDKL on Windows use (some) specific Windows features
*/
#if !defined(SDKL_USE_C89) && defined(_WIN32) && !defined(_WIN32_WCE)
#define SDKL_USE_WINDOWS  /* enable goodies for regular Windows */
#endif


#if defined(SDKL_USE_WINDOWS)
#define SDKL_DL_DLL	/* enable support for DLL */
#define SDKL_USE_C89	/* broadly, Windows is C89 */
#endif


#if defined(SDKL_USE_LINUX)
#define SDKL_USE_POSIX
#define SDKL_USE_DLOPEN		/* needs an extra library: -ldl */
#endif


#if defined(SDKL_USE_MACOSX)
#define SDKL_USE_POSIX
#define SDKL_USE_DLOPEN		/* MacOS does not need -ldl */
#endif


/*
@@ SDKLI_IS32INT is true iff 'int' has (at least) 32 bits.
*/
#define SDKLI_IS32INT	((UINT_MAX >> 30) >= 3)

/* }================================================================== */



/*
** {==================================================================
** Configuration for Number types. These options should not be
** set externally, because any other code connected to SDKL must
** use the same configuration.
** ===================================================================
*/

/*
@@ SDKL_INT_TYPE defines the type for SDKL integers.
@@ SDKL_FLOAT_TYPE defines the type for SDKL floats.
** SDKL should work fine with any mix of these options supported
** by your C compiler. The usual configurations are 64-bit integers
** and 'double' (the default), 32-bit integers and 'float' (for
** restricted platforms), and 'long'/'double' (for C compilers not
** compliant with C99, which may not have support for 'long long').
*/

/* predefined options for SDKL_INT_TYPE */
#define SDKL_INT_INT		1
#define SDKL_INT_LONG		2
#define SDKL_INT_LONGLONG	3

/* predefined options for SDKL_FLOAT_TYPE */
#define SDKL_FLOAT_FLOAT		1
#define SDKL_FLOAT_DOUBLE	2
#define SDKL_FLOAT_LONGDOUBLE	3


/* Default configuration ('long long' and 'double', for 64-bit SDKL) */
#define SDKL_INT_DEFAULT		SDKL_INT_LONGLONG
#define SDKL_FLOAT_DEFAULT	SDKL_FLOAT_DOUBLE


/*
@@ SDKL_32BITS enables SDKL with 32-bit integers and 32-bit floats.
*/
#define SDKL_32BITS	0


/*
@@ SDKL_C89_NUMBERS ensures that SDKL uses the largest types available for
** C89 ('long' and 'double'); Windows always has '__int64', so it does
** not need to use this case.
*/
#if defined(SDKL_USE_C89) && !defined(SDKL_USE_WINDOWS)
#define SDKL_C89_NUMBERS		1
#else
#define SDKL_C89_NUMBERS		0
#endif


#if SDKL_32BITS		/* { */
/*
** 32-bit integers and 'float'
*/
#if SDKLI_IS32INT  /* use 'int' if big enough */
#define SDKL_INT_TYPE	SDKL_INT_INT
#else  /* otherwise use 'long' */
#define SDKL_INT_TYPE	SDKL_INT_LONG
#endif
#define SDKL_FLOAT_TYPE	SDKL_FLOAT_FLOAT

#elif SDKL_C89_NUMBERS	/* }{ */
/*
** largest types available for C89 ('long' and 'double')
*/
#define SDKL_INT_TYPE	SDKL_INT_LONG
#define SDKL_FLOAT_TYPE	SDKL_FLOAT_DOUBLE

#else		/* }{ */
/* use defaults */

#define SDKL_INT_TYPE	SDKL_INT_DEFAULT
#define SDKL_FLOAT_TYPE	SDKL_FLOAT_DEFAULT

#endif				/* } */


/* }================================================================== */



/*
** {==================================================================
** Configuration for Paths.
** ===================================================================
*/

/*
** SDKL_PATH_SEP is the character that separates templates in a path.
** SDKL_PATH_MARK is the string that marks the substitution points in a
** template.
** SDKL_EXEC_DIR in a Windows path is replaced by the executable's
** directory.
*/
#define SDKL_PATH_SEP            ";"
#define SDKL_PATH_MARK           "?"
#define SDKL_EXEC_DIR            "!"


/*
@@ SDKL_PATH_DEFAULT is the default path that SDKL uses to look for
** SDKL libraries.
@@ SDKL_CPATH_DEFAULT is the default path that SDKL uses to look for
** C libraries.
** CHANGE them if your machine has a non-conventional directory
** hierarchy or if you want to install your libraries in
** non-conventional directories.
*/

#define SDKL_VDIR	SDKL_VERSION_MAJOR "." SDKL_VERSION_MINOR
#if defined(_WIN32)	/* { */
/*
** In Windows, any exclamation mark ('!') in the path is replaced by the
** path of the directory of the executable file of the current process.
*/
#define SDKL_LDIR	"!\\sdkl\\"
#define SDKL_CDIR	"!\\"
#define SDKL_SHRDIR	"!\\..\\share\\sdkl\\" SDKL_VDIR "\\"

#if !defined(SDKL_PATH_DEFAULT)
#define SDKL_PATH_DEFAULT  \
		SDKL_LDIR"?.sdkl;"  SDKL_LDIR"?\\init.sdkl;" \
		SDKL_CDIR"?.sdkl;"  SDKL_CDIR"?\\init.sdkl;" \
		SDKL_SHRDIR"?.sdkl;" SDKL_SHRDIR"?\\init.sdkl;" \
		".\\?.sdkl;" ".\\?\\init.sdkl"
#endif

#if !defined(SDKL_CPATH_DEFAULT)
#define SDKL_CPATH_DEFAULT \
		SDKL_CDIR"?.dll;" \
		SDKL_CDIR"..\\lib\\sdkl\\" SDKL_VDIR "\\?.dll;" \
		SDKL_CDIR"loadall.dll;" ".\\?.dll"
#endif

#else			/* }{ */

#define SDKL_ROOT	"/usr/local/"
#define SDKL_LDIR	SDKL_ROOT "share/sdkl/" SDKL_VDIR "/"
#define SDKL_CDIR	SDKL_ROOT "lib/sdkl/" SDKL_VDIR "/"

#if !defined(SDKL_PATH_DEFAULT)
#define SDKL_PATH_DEFAULT  \
		SDKL_LDIR"?.sdkl;"  SDKL_LDIR"?/init.sdkl;" \
		SDKL_CDIR"?.sdkl;"  SDKL_CDIR"?/init.sdkl;" \
		"./?.sdkl;" "./?/sdkl.sdkl"
#endif

#if !defined(SDKL_CPATH_DEFAULT)
#define SDKL_CPATH_DEFAULT \
		SDKL_CDIR"?.so;" SDKL_CDIR"loadall.so;" "./?.so"
#endif

#endif			/* } */


/*
@@ SDKL_DIRSEP is the directory separator (for submodules).
** CHANGE it if your machine does not use "/" as the directory separator
** and is not Windows. (On Windows SDKL automatically uses "\".)
*/
#if !defined(SDKL_DIRSEP)

#if defined(_WIN32)
#define SDKL_DIRSEP	"\\"
#else
#define SDKL_DIRSEP	"/"
#endif

#endif

/* }================================================================== */


/*
** {==================================================================
** Marks for exported symbols in the C code
** ===================================================================
*/

/*
@@ SDKL_API is a mark for all core API functions.
@@ SDKLLIB_API is a mark for all auxiliary library functions.
@@ SDKLMOD_API is a mark for all standard library opening functions.
** CHANGE them if you need to define those functions in some special way.
** For instance, if you want to create one Windows DLL with the core and
** the libraries, you may want to use the following definition (define
** SDKL_BUILD_AS_DLL to get it).
*/
#if defined(SDKL_BUILD_AS_DLL)	/* { */

#if defined(SDKL_CORE) || defined(SDKL_LIB)	/* { */
#define SDKL_API __declspec(dllexport)
#else						/* }{ */
#define SDKL_API __declspec(dllimport)
#endif						/* } */

#else				/* }{ */

#define SDKL_API		extern

#endif				/* } */


/*
** More often than not the libs go together with the core.
*/
#define SDKLLIB_API	SDKL_API
#define SDKLMOD_API	SDKL_API


/*
@@ SDKLI_FUNC is a mark for all extern functions that are not to be
** exported to outside modules.
@@ SDKLI_DDEF and SDKLI_DDEC are marks for all extern (const) variables,
** none of which to be exported to outside modules (SDKLI_DDEF for
** definitions and SDKLI_DDEC for declarations).
** CHANGE them if you need to mark them in some special way. Elf/gcc
** (versions 3.2 and later) mark them as "hidden" to optimize access
** when SDKL is compiled as a shared library. Not all elf targets support
** this attribute. Unfortunately, gcc does not offer a way to check
** whether the target offers that support, and those without support
** give a warning about it. To avoid these warnings, change to the
** default definition.
*/
#if defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 302) && \
    defined(__ELF__)		/* { */
#define SDKLI_FUNC	__attribute__((visibility("internal"))) extern
#else				/* }{ */
#define SDKLI_FUNC	extern
#endif				/* } */

#define SDKLI_DDEC(dec)	SDKLI_FUNC dec
#define SDKLI_DDEF	/* empty */

/* }================================================================== */


/*
** {==================================================================
** Compatibility with previous versions
** ===================================================================
*/

/*
@@ SDKL_COMPAT_5_3 controls other macros for compatibility with SDKL 5.3.
** You can define it to get all options, or change specific options
** to fit your specific needs.
*/
#if defined(SDKL_COMPAT_5_3)	/* { */

/*
@@ SDKL_COMPAT_MATHLIB controls the presence of several deprecated
** functions in the mathematical library.
** (These functions were already officially removed in 5.3;
** nevertheless they are still available here.)
*/
#define SDKL_COMPAT_MATHLIB

/*
@@ SDKL_COMPAT_APIINTCASTS controls the presence of macros for
** manipulating other integer types (sdkl_pushunsigned, sdkl_tounsigned,
** sdklL_checkint, sdklL_checklong, etc.)
** (These macros were also officially removed in 5.3, but they are still
** available here.)
*/
#define SDKL_COMPAT_APIINTCASTS


/*
@@ SDKL_COMPAT_LT_LE controls the emulation of the '__le' metamethod
** using '__lt'.
*/
#define SDKL_COMPAT_LT_LE


/*
@@ The following macros supply trivial compatibility for some
** changes in the API. The macros themselves document how to
** change your code to avoid using them.
** (Once more, these macros were officially removed in 5.3, but they are
** still available here.)
*/
#define sdkl_strlen(L,i)		sdkl_rawlen(L, (i))

#define sdkl_objlen(L,i)		sdkl_rawlen(L, (i))

#define sdkl_equal(L,idx1,idx2)		sdkl_compare(L,(idx1),(idx2),SDKL_OPEQ)
#define sdkl_lessthan(L,idx1,idx2)	sdkl_compare(L,(idx1),(idx2),SDKL_OPLT)

#endif				/* } */

/* }================================================================== */



/*
** {==================================================================
** Configuration for Numbers (low-level part).
** Change these definitions if no predefined SDKL_FLOAT_* / SDKL_INT_*
** satisfy your needs.
** ===================================================================
*/

/*
@@ SDKLI_UACNUMBER is the result of a 'default argument promotion'
@@ over a floating number.
@@ l_floatatt(x) corrects float attribute 'x' to the proper float type
** by prefixing it with one of FLT/DBL/LDBL.
@@ SDKL_NUMBER_FRMLEN is the length modifier for writing floats.
@@ SDKL_NUMBER_FMT is the format for writing floats.
@@ sdkl_number2str converts a float to a string.
@@ l_mathop allows the addition of an 'l' or 'f' to all math operations.
@@ l_floor takes the floor of a float.
@@ sdkl_str2number converts a decimal numeral to a number.
*/


/* The following definitions are good for most cases here */

#define l_floor(x)		(l_mathop(floor)(x))

#define sdkl_number2str(s,sz,n)  \
	l_sprintf((s), sz, SDKL_NUMBER_FMT, (SDKLI_UACNUMBER)(n))

/*
@@ sdkl_numbertointeger converts a float number with an integral value
** to an integer, or returns 0 if float is not within the range of
** a sdkl_Integer.  (The range comparisons are tricky because of
** rounding. The tests here assume a two-complement representation,
** where MININTEGER always has an exact representation as a float;
** MAXINTEGER may not have one, and therefore its conversion to float
** may have an ill-defined value.)
*/
#define sdkl_numbertointeger(n,p) \
  ((n) >= (SDKL_NUMBER)(SDKL_MININTEGER) && \
   (n) < -(SDKL_NUMBER)(SDKL_MININTEGER) && \
      (*(p) = (SDKL_INTEGER)(n), 1))


/* now the variable definitions */

#if SDKL_FLOAT_TYPE == SDKL_FLOAT_FLOAT		/* { single float */

#define SDKL_NUMBER	float

#define l_floatatt(n)		(FLT_##n)

#define SDKLI_UACNUMBER	double

#define SDKL_NUMBER_FRMLEN	""
#define SDKL_NUMBER_FMT		"%.7g"

#define l_mathop(op)		op##f

#define sdkl_str2number(s,p)	strtof((s), (p))


#elif SDKL_FLOAT_TYPE == SDKL_FLOAT_LONGDOUBLE	/* }{ long double */

#define SDKL_NUMBER	long double

#define l_floatatt(n)		(LDBL_##n)

#define SDKLI_UACNUMBER	long double

#define SDKL_NUMBER_FRMLEN	"L"
#define SDKL_NUMBER_FMT		"%.19Lg"

#define l_mathop(op)		op##l

#define sdkl_str2number(s,p)	strtold((s), (p))

#elif SDKL_FLOAT_TYPE == SDKL_FLOAT_DOUBLE	/* }{ double */

#define SDKL_NUMBER	double

#define l_floatatt(n)		(DBL_##n)

#define SDKLI_UACNUMBER	double

#define SDKL_NUMBER_FRMLEN	""
#define SDKL_NUMBER_FMT		"%.14g"

#define l_mathop(op)		op

#define sdkl_str2number(s,p)	strtod((s), (p))

#else						/* }{ */

#error "numeric float type not defined"

#endif					/* } */



/*
@@ SDKL_UNSIGNED is the unsigned version of SDKL_INTEGER.
@@ SDKLI_UACINT is the result of a 'default argument promotion'
@@ over a SDKL_INTEGER.
@@ SDKL_INTEGER_FRMLEN is the length modifier for reading/writing integers.
@@ SDKL_INTEGER_FMT is the format for writing integers.
@@ SDKL_MAXINTEGER is the maximum value for a SDKL_INTEGER.
@@ SDKL_MININTEGER is the minimum value for a SDKL_INTEGER.
@@ SDKL_MAXUNSIGNED is the maximum value for a SDKL_UNSIGNED.
@@ SDKL_UNSIGNEDBITS is the number of bits in a SDKL_UNSIGNED.
@@ sdkl_integer2str converts an integer to a string.
*/


/* The following definitions are good for most cases here */

#define SDKL_INTEGER_FMT		"%" SDKL_INTEGER_FRMLEN "d"

#define SDKLI_UACINT		SDKL_INTEGER

#define sdkl_integer2str(s,sz,n)  \
	l_sprintf((s), sz, SDKL_INTEGER_FMT, (SDKLI_UACINT)(n))

/*
** use SDKLI_UACINT here to avoid problems with promotions (which
** can turn a comparison between unsigneds into a signed comparison)
*/
#define SDKL_UNSIGNED		unsigned SDKLI_UACINT


#define SDKL_UNSIGNEDBITS	(sizeof(SDKL_UNSIGNED) * CHAR_BIT)


/* now the variable definitions */

#if SDKL_INT_TYPE == SDKL_INT_INT		/* { int */

#define SDKL_INTEGER		int
#define SDKL_INTEGER_FRMLEN	""

#define SDKL_MAXINTEGER		INT_MAX
#define SDKL_MININTEGER		INT_MIN

#define SDKL_MAXUNSIGNED		UINT_MAX

#elif SDKL_INT_TYPE == SDKL_INT_LONG	/* }{ long */

#define SDKL_INTEGER		long
#define SDKL_INTEGER_FRMLEN	"l"

#define SDKL_MAXINTEGER		LONG_MAX
#define SDKL_MININTEGER		LONG_MIN

#define SDKL_MAXUNSIGNED		ULONG_MAX

#elif SDKL_INT_TYPE == SDKL_INT_LONGLONG	/* }{ long long */

/* use presence of macro LLONG_MAX as proxy for C99 compliance */
#if defined(LLONG_MAX)		/* { */
/* use ISO C99 stuff */

#define SDKL_INTEGER		long long
#define SDKL_INTEGER_FRMLEN	"ll"

#define SDKL_MAXINTEGER		LLONG_MAX
#define SDKL_MININTEGER		LLONG_MIN

#define SDKL_MAXUNSIGNED		ULLONG_MAX

#elif defined(SDKL_USE_WINDOWS) /* }{ */
/* in Windows, can use specific Windows types */

#define SDKL_INTEGER		__int64
#define SDKL_INTEGER_FRMLEN	"I64"

#define SDKL_MAXINTEGER		_I64_MAX
#define SDKL_MININTEGER		_I64_MIN

#define SDKL_MAXUNSIGNED		_UI64_MAX

#else				/* }{ */

#error "Compiler does not support 'long long'. Use option '-DSDKL_32BITS' \
  or '-DSDKL_C89_NUMBERS' (see file 'sdklconf.h' for details)"

#endif				/* } */

#else				/* }{ */

#error "numeric integer type not defined"

#endif				/* } */

/* }================================================================== */


/*
** {==================================================================
** Dependencies with C99 and other C details
** ===================================================================
*/

/*
@@ l_sprintf is equivalent to 'snprintf' or 'sprintf' in C89.
** (All uses in SDKL have only one format item.)
*/
#if !defined(SDKL_USE_C89)
#define l_sprintf(s,sz,f,i)	snprintf(s,sz,f,i)
#else
#define l_sprintf(s,sz,f,i)	((void)(sz), sprintf(s,f,i))
#endif


/*
@@ sdkl_strx2number converts a hexadecimal numeral to a number.
** In C99, 'strtod' does that conversion. Otherwise, you can
** leave 'sdkl_strx2number' undefined and SDKL will provide its own
** implementation.
*/
#if !defined(SDKL_USE_C89)
#define sdkl_strx2number(s,p)		sdkl_str2number(s,p)
#endif


/*
@@ sdkl_pointer2str converts a pointer to a readable string in a
** non-specified way.
*/
#define sdkl_pointer2str(buff,sz,p)	l_sprintf(buff,sz,"%p",p)


/*
@@ sdkl_number2strx converts a float to a hexadecimal numeral.
** In C99, 'sprintf' (with format specifiers '%a'/'%A') does that.
** Otherwise, you can leave 'sdkl_number2strx' undefined and SDKL will
** provide its own implementation.
*/
#if !defined(SDKL_USE_C89)
#define sdkl_number2strx(L,b,sz,f,n)  \
	((void)L, l_sprintf(b,sz,f,(SDKLI_UACNUMBER)(n)))
#endif


/*
** 'strtof' and 'opf' variants for math functions are not valid in
** C89. Otherwise, the macro 'HUGE_VALF' is a good proxy for testing the
** availability of these variants. ('math.h' is already included in
** all files that use these macros.)
*/
#if defined(SDKL_USE_C89) || (defined(HUGE_VAL) && !defined(HUGE_VALF))
#undef l_mathop  /* variants not available */
#undef sdkl_str2number
#define l_mathop(op)		(sdkl_Number)op  /* no variant */
#define sdkl_str2number(s,p)	((sdkl_Number)strtod((s), (p)))
#endif


/*
@@ SDKL_KCONTEXT is the type of the context ('ctx') for continuation
** functions.  It must be a numerical type; SDKL will use 'intptr_t' if
** available, otherwise it will use 'ptrdiff_t' (the nearest thing to
** 'intptr_t' in C89)
*/
#define SDKL_KCONTEXT	ptrdiff_t

#if !defined(SDKL_USE_C89) && defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 199901L
#include <stdint.h>
#if defined(INTPTR_MAX)  /* even in C99 this type is optional */
#undef SDKL_KCONTEXT
#define SDKL_KCONTEXT	intptr_t
#endif
#endif


/*
@@ sdkl_getlocaledecpoint gets the locale "radix character" (decimal point).
** Change that if you do not want to use C locales. (Code using this
** macro must include the header 'locale.h'.)
*/
#if !defined(sdkl_getlocaledecpoint)
#define sdkl_getlocaledecpoint()		(localeconv()->decimal_point[0])
#endif


/*
** macros to improve jump prediction, used mostly for error handling
** and debug facilities. (Some macros in the SDKL API use these macros.
** Define SDKL_NOBUILTIN if you do not want '__builtin_expect' in your
** code.)
*/
#if !defined(sdkli_likely)

#if defined(__GNUC__) && !defined(SDKL_NOBUILTIN)
#define sdkli_likely(x)		(__builtin_expect(((x) != 0), 1))
#define sdkli_unlikely(x)	(__builtin_expect(((x) != 0), 0))
#else
#define sdkli_likely(x)		(x)
#define sdkli_unlikely(x)	(x)
#endif

#endif


#if defined(SDKL_CORE) || defined(SDKL_LIB)
/* shorter names for SDKL's own use */
#define l_likely(x)	sdkli_likely(x)
#define l_unlikely(x)	sdkli_unlikely(x)
#endif



/* }================================================================== */


/*
** {==================================================================
** Language Variations
** =====================================================================
*/

/*
@@ SDKL_NOCVTN2S/SDKL_NOCVTS2N control how SDKL performs some
** coercions. Define SDKL_NOCVTN2S to turn off automatic coercion from
** numbers to strings. Define SDKL_NOCVTS2N to turn off automatic
** coercion from strings to numbers.
*/
/* #define SDKL_NOCVTN2S */
/* #define SDKL_NOCVTS2N */


/*
@@ SDKL_USE_APICHECK turns on several consistency checks on the C API.
** Define it as a help when debugging C code.
*/
#if defined(SDKL_USE_APICHECK)
#include <assert.h>
#define sdkli_apicheck(l,e)	assert(e)
#endif

/* }================================================================== */


/*
** {==================================================================
** Macros that affect the API and must be stable (that is, must be the
** same when you compile SDKL and when you compile code that links to
** SDKL).
** =====================================================================
*/

/*
@@ SDKLI_MAXSTACK limits the size of the SDKL stack.
** CHANGE it if you need a different limit. This limit is arbitrary;
** its only purpose is to stop SDKL from consuming unlimited stack
** space (and to reserve some numbers for pseudo-indices).
** (It must fit into max(size_t)/32.)
*/
#if SDKLI_IS32INT
#define SDKLI_MAXSTACK		1000000
#else
#define SDKLI_MAXSTACK		15000
#endif


/*
@@ SDKL_EXTRASPACE defines the size of a raw memory area associated with
** a SDKL state with very fast access.
** CHANGE it if you need a different size.
*/
#define SDKL_EXTRASPACE		(sizeof(void *))


/*
@@ SDKL_IDSIZE gives the maximum size for the description of the source
@@ of a function in debug information.
** CHANGE it if you want a different size.
*/
#define SDKL_IDSIZE	60


/*
@@ SDKLL_BUFFERSIZE is the buffer size used by the lauxlib buffer system.
*/
#define SDKLL_BUFFERSIZE   ((int)(16 * sizeof(void*) * sizeof(sdkl_Number)))


/*
@@ SDKLI_MAXALIGN defines fields that, when used in a union, ensure
** maximum alignment for the other items in that union.
*/
#define SDKLI_MAXALIGN  sdkl_Number n; double u; void *s; sdkl_Integer i; long l

/* }================================================================== */





/* =================================================================== */

/*
** Local configuration. You can use this space to add your redefinitions
** without modifying the main part of the file.
*/





#endif

