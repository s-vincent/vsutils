/*
 * Copyright (C) 2006-2013 Sebastien Vincent.
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
 * \file util_sys.c
 * \brief Some helper system functions.
 * \author Sebastien Vincent
 * \date 2006-2013
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <fcntl.h>

#include <sys/stat.h>

#if !defined(_WIN32) && !defined(_WIN64)
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <pwd.h>
#endif

#include "util_sys.h"

/**
 * \def SYS_UNKNOWN_ERROR
 * \brief Error string used when no other error string
 * are available.
 */
#define SYS_UNKNOWN_ERROR "Unknown error!"

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

int sys_microsleep(unsigned long usec)
{
  unsigned long sec = 0;
  struct timeval tv;

  sec = (unsigned long)usec / 1000000;
  usec = (unsigned long)usec % 1000000;

  tv.tv_sec = sec;
  tv.tv_usec = usec;

  select(0, NULL, NULL, NULL, &tv);

  return 0;
}

long sys_get_dtablesize(void)
{
#if !defined(_WIN32) && !defined(_WIN64)
  return sysconf(_SC_OPEN_MAX);
  /*
     struct rlimit limit;
     getrlimit(RLIMIT_NOFILE, &limit);
     return limit.rlim_cur;
   */
#else /* Windows */
#ifndef FD_SETSIZE
#define FD_SETSIZE 256
#endif
  return FD_SETSIZE;
#endif
}

int sys_is_big_endian(void)
{
  long one = 1;
  return !(*((char *)(&one)));
}

int sys_is_little_endian(void)
{
  long one = 1;
  return (*((char *)(&one)));
}

char* sys_get_error(int errnum, char* buf, size_t buflen)
{
  char* error = NULL;
#if _POSIX_C_SOURCE >= 200112L && !defined(_GNU_SOURCE)
  /* POSIX version */
  int ret = 0;
  ret = strerror_r(errnum, buf, buflen);
  if(ret == -1)
  {
    strncpy(buf, SYS_UNKNOWN_ERROR, buflen - 1);
    buf[buflen - 1] = 0x00;
  }
  error = buf;
#elif defined(_GNU_SOURCE)
  /* GNU libc */
  error = strerror_r(errnum, buf, buflen);
#else
  /* no strerror_r() function, assume that strerror is reentrant! */
  strncpy(buf, strerror(errnum), buflen);
  error = buf;
#endif
  return error;
}

int sys_daemon(const char* dir, mode_t mask, void (*cleanup)(void* arg),
    void* arg)
{
#if defined(_WIN32) || defined(_WIN64)
  errno = ENOSYS;
  return -1;
#else /* Unix */
  pid_t pid = -1;
  long max = 0;
  int fd = -1;

  pid = fork();

  if(pid > 0) /* father */
  {
    if(cleanup)
    {
      cleanup(arg);
    }
    _exit(EXIT_SUCCESS);
  }
  else if(pid == -1) /* error */
  {
    return -1;
  }

  /* child */
  errno = 0;

  if(setsid() == -1)
  {
    return -1;
  }

  max = sysconf(_SC_OPEN_MAX);
  for(long i = STDIN_FILENO + 1 ; i < max ; i++)
  {
    close(i);
  }

  /* change directory */
  if(!dir)
  {
    dir = "/";
  }

  if(chdir(dir) == -1)
  {
    return -1;
  }

  /* change mask */
  umask(mask);

  /* redirect stdin, stdout and stderr to /dev/null */
  /* open /dev/null */
  if((fd = open("/dev/null", O_RDWR, 0)) != -1)
  {
    /* redirect stdin, stdout and stderr to /dev/null */
    close(STDIN_FILENO);
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);

    if(fd > -1)
    {
      close(fd);
    }
  }

  return 0;
#endif
}

int sys_drop_privileges(uid_t uid_real, gid_t gid_real, uid_t uid_eff,
    gid_t gid_eff, const char* user_name)
{
#if defined(_WIN32) || defined(_WIN64)
  /* not implemented */
  errno = ENOSYS;
  return -1;
#else /* Unix */
  (void)gid_eff; /* not used for the moment */

  if(uid_real == 0 || uid_eff == 0)
  {
    /* program runs as root or sudoers */
    struct passwd user;
    struct passwd* tmpUser = &user;
    struct passwd* tmp = NULL;
    char buf[1024];

    if(!user_name)
    {
      if(uid_real == uid_eff)
      {
        /* runs as root and no user_name specified,
         * cannot drop privileges.
         */
        errno = EINVAL;
        return -1;
      }

#ifdef _POSIX_SAVED_IDS
      if(setegid(gid_real) != 0 || seteuid(uid_real) != 0)
      {
        return -1;
      }
#else /* i.e. for *BSD */
      if(setregid(-1, gid_real) != 0 || setreuid(-1, uid_real) != 0)
      {
        return -1;
      }
      return 0;
#endif
    }

    /* get user_name information (UID and GID) */
    if(getpwnam_r(user_name, tmpUser, buf, sizeof(buf), &tmp) == 0)
    {
      if(setegid(user.pw_gid) != 0 || seteuid(user.pw_uid) != 0)
      {
        return -1;
      }
      return 0;
    }
    else
    {
      /* user does not exist, cannot lost our privileges */
      errno = EINVAL;
      return -1;
    }
  }

  errno = EINVAL;

  /* cannot lost our privileges */
  return -1;
#endif
}

int sys_gain_privileges(uid_t uid_eff, gid_t gid_eff)
{
#if defined(_WIN32) || defined(_WIN64)
  errno = ENOSYS;
  return -1;
#else /* Unix */
#ifdef _POSIX_SAVED_IDS
  if(setegid(gid_eff) != 0 || seteuid(uid_eff) != 0)
  {
    return -1;
  }
  return 0;
#else /* i.e for *BSD */
  if(setregid(-1, gid_eff) != 0 || setreuid(-1, uid_eff) != 0)
  {
    return -1;
  }
  return 0;
#endif
#endif
}

void sys_convert_to_hex(const unsigned char* bin, size_t bin_len,
    unsigned char* hex, size_t hex_len)
{
  size_t i = 0;

  for(i = 0 ; i < bin_len && (i * 2) < hex_len ; i++)
  {
    unsigned char j = (bin[i] >> 4) & 0x0f;

    if(j <= 9)
    {
      hex[i * 2] = (j + '0');
    }
    else
    {
      hex[i * 2] = (j + 'a' - 10);
    }

    j = bin[i] & 0x0f;

    if(j <= 9)
    {
      hex[i * 2 + 1] = (j + '0');
    }
    else
    {
      hex[i * 2 + 1] = (j + 'a' - 10);
    }
  }
}

void sys_convert_to_uint32(const unsigned char* data, size_t data_len,
    uint32_t* t)
{
  unsigned int i = 0;
  *t = 0;

  for(i = 0 ; i < data_len ; i++)
  {
    *t = (*t) * 16;

    if(data[i] >= '0' && data[i] <= '9')
    {
      *t += data[i] - '0';
    }
    else if(data[i] >= 'a' && data[i] <='f')
    {
      *t += data[i] - 'a' + 10;
    }
  }
}

void sys_convert_to_uint64(const unsigned char* data, size_t data_len,
    uint64_t* t)
{
  unsigned int i = 0;
  *t = 0;

  for(i = 0 ; i < data_len ; i++)
  {
    *t = (*t) * 16;

    if(data[i] >= '0' && data[i] <= '9')
    {
      *t += data[i] - '0';
    }
    else if(data[i] >= 'a' && data[i] <='f')
    {
      *t += data[i] - 'a' + 10;
    }
  }
}

char* sys_s_strncpy(char* dest, const char* src, size_t n)
{
  char* ret = NULL;

  ret = strncpy(dest, src, n - 1);
  dest[n - 1] = 0x00; /* add the final NULL character */

  return ret;
}

int sys_s_snprintf(char* str, size_t size, const char* format, ...)
{
  va_list args;
  int ret = 0;

  va_start(args, format);
  ret = snprintf(str, size - 1, format,  args);
  str[size - 1] = 0x00; /* add the final NULL character */
  va_end(args);
  return ret;
}

void* sys_s_memset(void* src, int c, size_t len)
{
  volatile unsigned char* ptr = src;
  size_t tmp = len;

  while(tmp--)
  {
    *ptr++ = c;
  }
  return src;
}

size_t sys_get_cores(void)
{
  long nb = 0;

#ifdef _SC_NPROCESSORS_ONLN
  /*
   * XXX find a POSIX portable way to determine number of core
   * _SC_NPROCESSORS_ONLN is not standard but works on GNU/Linux,
   * MacOS X >= 10.4 , Solaris, AIX, FreeBSD >= 5.0 and some recent
   * versions of OpenBSD and NetBSD
   */
  nb = sysconf(_SC_NPROCESSORS_ONLN);
  if(nb == -1)
  {
    nb = 1;
  }
#elif defined(_WIN32) || defined(_WIN64)
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);

  nb = sysinfo.dwNumberOfProcessors;
#else
  /* not supported platform, considered 1 core */
  nb = 1;
#endif

  return nb;
}

#ifdef __cplusplus
}
#endif

