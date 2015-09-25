/* This is used to customize Lens to a particular system or machine.  You
   should try to localize any machine-specific customizations to sysext.h. */

#ifndef SYSTEM_H
#define SYSTEM_H

#include <math.h>


/*************************** BASIC TYPE DEFINITIONS **************************/
/* Boolean values */
/* Don't change these, I use the fact that FALSE is 0 and TRUE is non-zero */
#define FALSE 0
#define TRUE  1

/* Types */
typedef int mask; /* a type bit field or a mode specifier */
typedef int flag; /* a boolean.  This is an int because I use
			    Tcl_LinkVar and you can't link to a char */
// #undef DOUBLE_REAL
#ifdef DOUBLE_REAL
  typedef double real;
#else
  typedef float real;
# ifndef FLOAT_REAL
#   define FLOAT_REAL
# endif /* FLOAT_REAL */
#endif /* DOUBLE_REAL */

/*
  NaNf   is a 4-byte nan
  NaNd   is a 8-byte nan
  NaN    is a real nan, whichever that is
  isNaNf tests a 4-byte nan
  isNaNd tests a 8-byte nan
  isNaN  tests a real nan, whichever that is
*/

/************************* MACHINE-SPECIFIC SETTINGS *************************/
#ifdef MACHINE_INTEL
#  ifndef LITTLE_END
#    define LITTLE_END
#  endif /* LITTLE_END */
#endif /* MACHINE_INTEL */

#ifdef MACHINE_LINUX
#  include <bits/nan.h>
#  ifndef LITTLE_END
#    define LITTLE_END
#  endif /* LITTLE_END */
#endif /* MACHINE_LINUX */


#ifdef MACHINE_WINDOWS
#  include <sys/cygwin.h>
#  define NULL_FILE "NUL:"
#  ifndef LITTLE_END
#    define LITTLE_END
#  endif /* LITTLE_END */
#endif /* MACHINE_WINDOWS */


#ifdef MACHINE_MACINTOSH
#  include <sys/types.h>
#  include <machine/types.h>
#  define NO_DRAND48
#  ifdef LITTLE_END
#    undef LITTLE_END
#  endif /* LITTLE_END */
#endif /* MACHINE_MACINTOSH */


#ifdef MACHINE_ALPHA
#  include <float.h>
#  include <stdarg.h>
#  define _VARARGS_H_ /* prevents this from overriding va_start */
#  ifndef LITTLE_END
#    define LITTLE_END
#  endif /* LITTLE_END */
#  define isNaNf(x)  isnanf((float) x)

#  ifdef FLOAT_REAL
#    define REAL_MATH
#    define LOG(x)   ((real) logf((float) (x)))
#    define EXP(x)   ((real) expf((float) (x)))
#    define SQRT(x)  ((real) sqrtf((float) (x)))
#    define POW(x,y) ((real) powf((float) (x), (float) (y)))
#    define ABS(x)   ((real) fabsf((float) (x)))
#    define CEIL(x)  ((int)  ceilf((float) (x)))
#    define FLOOR(x) ((int)  floorf((float) (x)))
#  endif /* FLOAT_REAL */
#endif /* MACHINE_ALPHA */


#ifdef MACHINE_EAGLE
#endif /* MACHINE_EAGLE */


#ifdef MACHINE_CONDOR
#  ifndef LITTLE_END
#    define LITTLE_END
#  endif /* LITTLE_END */
#endif /* MACHINE_CONDOR */


#ifdef MACHINE_SP
#  ifdef DOUBLE_REAL
Using doubles may cause problems writing binary files on the sp
because there is a bug in the xlc optimizer.
#  endif
#  include <float.h>
#  undef NTOHL
#  undef HTONL
#  define HAVE_LIMITS_H
#  define HAVE_UNISTD_H
#endif /* MACHINE_SP */


#ifdef MACHINE_HYDRA
#  include <ieeefp.h>
#  include "stdarg.h" /* It has a screwed up version installed */
#  include <nan.h>
#  define isNaNf(x) isnanf((float) x)
#  define isNaNd(x) innand((double) x)
#endif /* MACHINE_HYDRA */


#ifdef MACHINE_SGI
#undef NTOHL
#undef HTONL
#define __LIMITS_H__
#endif /* MACHINE_SGI */


#include "sysext.h"


/***************************** DEFAULT SETTINGS ******************************/
#ifndef _H_STDARG
#  include <stdarg.h> /* needed for va_start */
#endif

#ifndef REAL_MATH
#  define LOG(x)   ((real) log((double) (x)))
#  define EXP(x)   ((real) exp((double) (x)))
#  define SQRT(x)  ((real) sqrt((double) (x)))
#  define POW(x,y) ((real) pow((double) (x), (double) (y)))
#  define ABS(x)   ((real) fabs((double) (x)))
#  define CEIL(x)  ((int)  ceil((double) (x)))
#  define FLOOR(x) ((int)  floor((double) (x)))
#endif /* REAL_MATH */

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  define NaNfbytes {{ 0, 0, 0xc0, 0x7f }}
#  define NaNdbytes {{ 0, 0, 0, 0, 0, 0, 0xf8, 0xff }}
#  ifndef NTOHL
#    define NTOHL(x) ntohl(x)
#  endif
#  ifndef HTONL
#    define HTONL(x) htonl(x)
#  endif
#else
#  ifndef NTOHL
#    define NTOHL(x) (x)
#  endif
#  ifndef HTONL
#    define HTONL(x) (x)
#  endif
#  define NaNfbytes {{ 0x7f, 0xc0, 0, 0 }}
#  define NaNdbytes {{ 0xff, 0xf8, 0, 0, 0, 0, 0, 0 }}
#endif /* LITTLE_END */

union nanfunion {unsigned char __c[4]; float __r;};
extern union nanfunion NaNfunion;
union nandunion {unsigned char __c[8]; double __r;};
extern union nandunion NaNdunion;

#define NaNf NaNfunion.__r
#define NaNd NaNdunion.__r
#ifndef isNaNf
#  define isNaNf(x) isnan((float) x)
#endif /* isNaNf */
#ifndef isNaNd
#  define isNaNd(x) isnan((double) x)
#endif /* isNaNd */

#define IS_NEGATIVE(x) ((((int *) &x)[0] & (1 << 31)) != 0)

#ifdef FLOAT_REAL
#  define NaN   NaNf
#  define isNaN isNaNf
#else
#  define NaN   NaNd
#  define isNaN isNaNd
#  if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#    undef  IS_NEGATIVE
#    define IS_NEGATIVE(x) ((((int *) &x)[1] & (1 << 31)) != 0)
#  endif /* LITTLE_END */
#endif /* FLOAT_REAL */

/************************** LIMITS AND OTHER STUFF ***************************/

/* System Commands */
#ifndef NULL_FILE
#  define NULL_FILE "/dev/null"
#endif /* NULL_FILE */
#define BUNZIP2  "bzip2 -d"
#define BZIP2    "bzip2"
#define UNZIP    "gzip -d"
#define ZIP      "gzip"
#define COMPRESS "compress"

/* Miscellaneous */
#define NO_VALUE "-"

#endif /* SYSTEM_H */

