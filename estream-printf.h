/* estream-printf.h - Versatile C-99 compliant printf formatting.
 * Copyright (C) 2007 g10 Code GmbH
 *
 * This file is part of Libestream.
 *
 * Libestream is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * Libestream is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Libestream; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ESTREAM_PRINTF_H
#define ESTREAM_PRINTF_H

#include <stdarg.h>
#include <stdio.h>

/* To use this file with libraries the following macro is useful:

     #define _ESTREAM_EXT_SYM_PREFIX _foo_
   
       This prefixes all external symbols with "_foo_".

   For the implementation of the code (estream-printf.c) the following
   macros may be used to tune the implementation for certain systems:

     #define _ESTREAM_PRINTF_MALLOC foo_malloc
     #define _ESTREAM_PRINTF_FREE   foo_free

       Make estream_asprintf and estream_vasprintf use foo_malloc and
       foo_free instead of the standard malloc and free functions to
       allocate the memory returned to the caller.

     #define  _ESTREAM_PRINTF_EXTRA_INCLUDE "foo.h"

       This includes the file "foo.h" which may provide prototypes for
       the custom memory allocation functions.
 */


# define _ESTREAM_GCC_A_PRINTF( f, a )  __attribute__ ((format (printf,f,a)))

#ifdef __cplusplus
extern "C"
{
#endif

typedef int (*estream_printf_out_t)
     (void *outfncarg,  const char *buf, size_t buflen);

int estream_format (estream_printf_out_t outfnc, void *outfncarg,
                    const char *format, va_list vaargs) 
     _ESTREAM_GCC_A_PRINTF(3,0);
int estream_printf (const char *format, ...) 
     _ESTREAM_GCC_A_PRINTF(1,2);
int estream_fprintf (FILE *fp, const char *format, ... )
     _ESTREAM_GCC_A_PRINTF(2,3);
int estream_vfprintf (FILE *fp, const char *format, va_list arg_ptr)
     _ESTREAM_GCC_A_PRINTF(2,0);
int estream_snprintf (char *buf, size_t bufsize, const char *format, ...)
     _ESTREAM_GCC_A_PRINTF(3,4);
int estream_vsnprintf (char *buf,size_t bufsize, 
                       const char *format, va_list arg_ptr) 
     _ESTREAM_GCC_A_PRINTF(3,0);
int estream_asprintf (char **bufp, const char *format, ...)
     _ESTREAM_GCC_A_PRINTF(2,3);
int estream_vasprintf (char **bufp, const char *format, va_list arg_ptr)
     _ESTREAM_GCC_A_PRINTF(2,0);

#ifdef __cplusplus
}
#endif
#endif /*ESTREAM_PRINTF_H*/
