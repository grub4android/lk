/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2006,2007  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SETJMP_HEADER
#define SETJMP_HEADER	1

/* GCC version checking borrowed from glibc. */
#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#  define GNUC_PREREQ(maj,min) \
	((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
#  define GNUC_PREREQ(maj,min) 0
#endif

#if GNUC_PREREQ(4,0)
#define RETURNS_TWICE __attribute__ ((returns_twice))
#else
#define RETURNS_TWICE
#endif

/* This must define jmp_buf, and declare setjmp and
   longjmp.  */
#include <arch/setjmp.h>

#endif /* ! SETJMP_HEADER */
