/* estream.h - Extended stream I/O Library
 * Copyright (C) 2004, 2005, 2006, 2007, 2010 g10 Code GmbH
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
 *
 * ALTERNATIVELY, Libestream may be distributed under the terms of the
 * following license, in which case the provisions of this license are
 * required INSTEAD OF the GNU General Public License. If you wish to
 * allow use of your version of this file only under the terms of the
 * GNU General Public License, and not to allow others to use your
 * version of this file under the terms of the following license,
 * indicate your decision by deleting this paragraph and the license
 * below.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, and the entire permission notice in its entirety,
 *    including the disclaimer of warranties.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ESTREAM_H
#define ESTREAM_H

#ifdef __sun__
#define off_t	off64_t
#endif
#include <stdio.h>
#include <stdarg.h>

#define ES__RESTRICT __restrict__
#define _ESTREAM_GCC_A_PRINTF( f, a )  __attribute__ ((format (printf,f,a)))

/* Forward declaration for the (opaque) internal type.  */
struct estream_internal;

/* The definition of this struct is entirely private.  You must not
   use it for anything.  It is only here so some functions can be
   implemented as macros.  */
struct es__stream
{
  /* The layout of this struct must never change.  It may be grown,
     but only if all functions which access the new members are
     versioned.  */

  /* A pointer to the stream buffer.  */
  unsigned char *buffer;

  /* The size of the buffer in bytes.  */
  size_t buffer_size;

  /* The length of the usable data in the buffer, only valid when in
     read mode (see flags).  */
  size_t data_len;

  /* The current position of the offset pointer, valid in read and
     write mode.  */
  size_t data_offset;

  size_t data_flushed;
  unsigned char *unread_buffer;
  size_t unread_buffer_size;

  /* The number of unread bytes.  */
  size_t unread_data_len;

  /* Various flags.  */
  struct {
    unsigned int writing: 1;
    unsigned int reserved: 7;
  } flags;

  /* A pointer to our internal data for this stream.  */
  struct estream_internal *intern;
};

typedef ssize_t (*es_cookie_read_function_t)(void *cookie, void *buffer, size_t size);
typedef ssize_t (*es_cookie_write_function_t)(void *cookie, const void *buffer, size_t size);
typedef int     (*es_cookie_seek_function_t)(void *cookie, off_t *offset, int whence);
typedef int     (*es_cookie_close_function_t)(void *cookie);
typedef struct es_cookie_io_functions_t
{
  es_cookie_read_function_t  func_read;
  es_cookie_write_function_t func_write;
  es_cookie_seek_function_t  func_seek;
  es_cookie_close_function_t func_close;
} es_cookie_io_functions_t;

/* The opaque type for an estream.  */
typedef struct es__stream *estream_t;

int es_init (void);

estream_t es_fopencookie (void *ES__RESTRICT cookie,
			  const char *ES__RESTRICT mode,
			  es_cookie_io_functions_t functions);
estream_t es_fopen (const char *ES__RESTRICT path,
		    const char *ES__RESTRICT mode);
estream_t _es_get_std_stream (int fd);

#define es_stdin  _es_get_std_stream (0)
#define es_stdout _es_get_std_stream (1)
#define es_stderr _es_get_std_stream (2)

int es_fclose (estream_t stream);
int es_fseek  (estream_t stream, long int offset, int whence);
int es_fseeko (estream_t stream, off_t offset, int whence);
int es_fileno (estream_t stream);
int es_ferror (estream_t stream);
int es_fflush (estream_t stream);
void es_clearerr (estream_t stream);
void es_clearerr_unlocked (estream_t stream);
ssize_t es_getline (char *ES__RESTRICT *ES__RESTRICT lineptr,
		    size_t *ES__RESTRICT n,
		    estream_t stream);
int es_setvbuf (estream_t ES__RESTRICT stream,
		char *ES__RESTRICT buf, int mode, size_t size);
int es_read      (estream_t ES__RESTRICT stream,
	          void *ES__RESTRICT buffer, size_t bytes_to_read,
	          size_t *ES__RESTRICT bytes_read);
int es_write     (estream_t ES__RESTRICT stream,
	          const void *ES__RESTRICT buffer, size_t bytes_to_write,
	          size_t *ES__RESTRICT bytes_written);
size_t es_fread  (void *ES__RESTRICT ptr, size_t size, size_t nitems,
		  estream_t ES__RESTRICT stream);
size_t es_fwrite (const void *ES__RESTRICT ptr, size_t size, size_t memb,
		  estream_t ES__RESTRICT stream);
void es_free (void *a);

int es_fprintf (estream_t ES__RESTRICT stream,
		const char *ES__RESTRICT format, ...)
     _ESTREAM_GCC_A_PRINTF(2,3);
int es_fprintf_unlocked (estream_t ES__RESTRICT stream,
                         const char *ES__RESTRICT format, ...)
     _ESTREAM_GCC_A_PRINTF(2,3);

int es_vfprintf (estream_t ES__RESTRICT stream,
		 const char *ES__RESTRICT format, va_list ap)
     _ESTREAM_GCC_A_PRINTF(2,0);
int es_vfprintf_unlocked (estream_t ES__RESTRICT stream,
                          const char *ES__RESTRICT format, va_list ap)
     _ESTREAM_GCC_A_PRINTF(2,0);
#endif /*ESTREAM_H*/
