/*
 * Copyright (C) 2006-2008 Sebastien Vincent.
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
 * \file os.h
 * \brief Macro to know the operating system.
 *
 * UNIX      => Unix-like operating system.\n
 * LINUX     => Linux operating system.\n
 * BSD       => BSD operating system.\n
 * AIX       => AIX operating system.\n
 * SUNOS     => Sun operating system.\n
 * MACOSX    => Mac OS X operating system.\n
 * WINDOWS   => MS Windows operating system.\n
 * MSDOS     => MS DOS operating system.\n
 *
 * \author Sebastien Vincent
 * \todo Add more OS detection.
 */

#ifndef VSUTILS_OS_H
#define VSUTILS_OS_H

/* extract the "MACINTOSH" flag from compiler */
#if defined(__APPLE__)
/**
 * \def MACOSX
 * \brief MAC OS X operation system.
 */
#define MACOSX
#endif

/* extract the "SUNOS" flag from compiler */
#if defined(sun)
#define UNIX
/**
 * \def SUNOS
 * \brief SUN OS operating system.
 */
#define SUNOS
#endif

/* extract the "UNIX" flag from compiler */
#ifdef __linux__
/**
 * \def UNIX
 * \brief Unix operating system.
 */
#define UNIX
/**
 * \def LINUX
 * \brief Linux operating system.
 */
#define LINUX
#endif

/* extract the "BSD" flag from compiler */
#if defined(BSD) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#ifndef BSD
/**
 * \def BSD
 * \brief *BSD operating system.
 */
#define BSD
#endif
/**
 * \def UNIX
 * \brief Unix operating system.
 */
#define UNIX
#endif

/* extract the "AIX" flag from compiler */
#ifdef _AIX 
/**
 * \def AIX
 * \brief AIX operating system.
 */
#define AIX

/**
 * \def UNIX
 * \brief Unix operating system.
 */
#define UNIX
#endif

/* extract the "MSDOS" flag from the compiler */
#ifdef __MSDOS__
/**
 * \def MSDOS
 * \brief MSDOS operating system.
 */
#define MSDOS
#undef UNIX
#endif

/* extract the "WINDOWS" flag from the compiler. */
#if defined(_Windows) || defined(__WINDOWS__) || \
  defined(__WIN32__) || defined(WIN32) || \
defined(__WINNT__) || defined(__NT__) || \
defined(_WIN32) || defined(_WIN64)
/**
 * \def WINDOWS
 * \brief MS Windows operating system.
 */
#define WINDOWS
#undef UNIX
#undef MSDOS
#endif

/* remove the WINDOWS flag when using MACINTOSH */
#ifdef MACOSX
#undef WINDOWS
#endif

/* assume UNIX if not Windows, Macintosh or MSDOS */
#if !defined(WINDOWS) && !defined(MACINTOSH) && !defined(MSDOS)
/**
 * \def UNIX
 * \brief Unix operating system.
 */
#define UNIX
#endif

#endif /* VSUTILS_OS_H */

