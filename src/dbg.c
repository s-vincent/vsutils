/*
 * Copyright (C) 2006-2016 Sebastien Vincent.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * \file dbg.c
 * \brief Some routines to print debug message.
 * \author Sebastien Vincent
 * \date 2006-2016
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include <stdint.h>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include <time.h>

#include "dbg.h"

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

void dbg_fprint(const char* file, const char* func, int line, FILE* out,
    const char* format, ...)
{
  va_list args;

#ifdef _MSC_VER
  SYSTEMTIME tlt;
  GetLocalTime (&tlt);
  fprintf(out, "%02d:%02d:%02d.%03u|[%s/%s:%d] ", tlt.wHour, tlt.wMinute,
      tlt.wSecond, tlt.wMilliseconds, file, func, line);
#else
  struct timeval lt;
  struct tm* tlt = NULL;
  struct tm tmp;

  gettimeofday(&lt, NULL);
  tlt = localtime_r((time_t*)&lt.tv_sec, &tmp);
  fprintf(out, "%02d:%02d:%02d.%06u [%s/%s:%d] ", tlt->tm_hour, tlt->tm_min,
      tlt->tm_sec, (uint32_t)lt.tv_usec, file, func, line);
#endif

  va_start(args, format);
  vfprintf(out, format, args);
  va_end(args);
}

void dbg_fprint_hexa(const char* file, const char* func, int line, FILE* out,
    const char* buf, size_t len, const char* format, ...)
{
  size_t i = 0;
  va_list args;

#ifdef _MSC_VER
  SYSTEMTIME tlt;
  GetLocalTime (&tlt);
  fprintf(out, "%02d:%02d:%02d.%03u [%s/%s:%d] ", tlt.wHour, tlt.wMinute,
      tlt.wSecond, tlt.wMilliseconds, file, func, line);
#else
  struct timeval lt;
  struct tm* tlt = NULL;
  struct tm tmp;

  gettimeofday(&lt, NULL);
  tlt = localtime_r((time_t*)&lt.tv_sec, &tmp);
  fprintf(out, "%02d:%02d:%02d.%06u [%s/%s:%d] ", tlt->tm_hour, tlt->tm_min,
      tlt->tm_sec, (uint32_t)lt.tv_usec, file, func, line);
#endif

  va_start(args, format);
  vfprintf(out, format, args);
  va_end(args);

  fprintf(out, " ");
  for(i = 0 ; i < len ; i++)
  {
    fprintf(out, "%02x ", (unsigned int)buf[i]);
  }
  fprintf(out, "\n");
}

#ifdef __cplusplus
}
#endif

