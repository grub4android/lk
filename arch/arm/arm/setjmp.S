/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2013  Free Software Foundation, Inc.
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

#include <asm.h>

/*
 * int setjmp (jmp_buf env)
 */
FUNCTION(setjmp)
	mov	r12, sp
	stm	r0, { r4-r12, lr }
	mov	r0, #0
	bx	lr

/*
 * int longjmp (jmp_buf env, int val)
 */
FUNCTION(longjmp)
	ldm	r0, { r4-r12, lr }
	mov	sp, r12
	movs	r0, r1
	it	eq
	moveq	r0, #1
	bx	lr
