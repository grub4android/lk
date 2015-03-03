/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <asm.h>
#include <arch/asm.h>
#include <arch/arm/cores.h>
#include <kernel/vm.h>
#include <platform/iomap.h>

// ALIGN(x,ps) (((x) + (ps)-1) & (~((ps)-1)))
FUNCTION(arm_qcom_align)
	add r0, r1
	sub r0, #1

	sub r1, #1
	mvn r1, r1
	and r0, r1

	bx lr

FUNCTION(arm_qcom_get_phys_offset)
	/* calculate our physical to virtual offset */
	mov		r0, pc
	ldr		r1, =.Laddr1
.Laddr1:
	sub		r0, r1

	bx lr

FUNCTION(arm_qcom_mmu_table_size)
	push	{ lr }

	// get phys table addr
	bl arm_qcom_get_phys_offset
	ldr r1, =mmu_initial_mappings
	add r1, r0
	// return value
	mov r0, #0

0:
	ldr r2, [r1, #8] // size
	cmp r2, #0

	// return r0
	popeq { lr }
	bxeq lr
	
	// next
	add r1, #20
	add r0, #1
	b 0b

FUNCTION(arm_qcom_mmu_table_add)
	push	{ r5-r12, lr }

	push	{ r0-r4 }
	// get table size
	bl arm_qcom_mmu_table_size
	mov r5, r0
	// get phys table addr
	bl arm_qcom_get_phys_offset
	ldr r6, =mmu_initial_mappings
	add r6, r0
	pop { r0-r4}

	// skip to next free table offset
	mov r7, #20
	mul r7, r5
	add r6, r7

	// store entry
	str r0, [ r6, #0  ]
	str r1, [ r6, #4  ]
	str r2, [ r6, #8  ]
	str r3, [ r6, #12 ]
	str r4, [ r6, #16 ]

	// add null entry
	add r6, #20
	mov r0, #0
	str r0, [ r6, #0  ]
	str r0, [ r6, #4  ]
	str r0, [ r6, #8  ]
	str r0, [ r6, #12 ]
	str r0, [ r6, #16 ]

	pop { r5-r12, lr }
	bx lr

FUNCTION(platform_mmu_callback)
	push	{ r1-r12, lr }

#ifdef QCOM_MMU_IDENTITY_MAP
	// full identity map
	mov r0, #0
	mov r1, #0
	mov r2, #4294967295
	ldr r3, =MMU_INITIAL_MAPPING_FLAG_DEVICE
	ldr r4, =arm_qcom_mmu_name_identity
	bl arm_qcom_mmu_table_add
#endif

	// SMEM
	bl arm_qcom_mmu_table_add_smem

	// LK memory
	ldr r0, =MEMBASE
	ldr r1, =KERNEL_BASE
	ldr r2, =MEMSIZE
	ldr r3, =MMU_INITIAL_MAPPING_FLAG_DEVICE
	ldr r4, =arm_qcom_mmu_name_memory
	bl arm_qcom_mmu_table_add

	pop		{ r1-r12, lr }
	bx		lr


FUNCTION(arm_smem_ptable_load_v0)
	push	{ r4-r12, lr }

	// r12 = ptable
	mov r12, r0

	ldr r11, =0x9DA5E0A8 // MAGIC1
	ldr r10, [r12]
	cmp r10, r11
	bne smem_v0_exit_err

	ldr r11, =0xAF9EC4E2 // MAGIC2
	ldr r10, [r12, #4]
	cmp r10, r11
	bne smem_v0_exit_err

	ldr r11, [r12, #8] // version
	cmp r11, #0
	bne smem_v0_exit_err

	// r10 = len
	ldr r10, [r12, #16]
	// r9 = loop_count
	mov r9, #0

// START OF LOOP
	// r7 = parts
	mov r7, r12
	add r7, #20
0:
	// r6 = part[i]
	mov r6, r7
	mov r5, #56 // sizeof(struct smem_ram_ptn)
	mul r5, r9
	add r6, r5

	// check type
	ldr r5, [r6, #36]
	cmp r5, #1 // SYS_MEMORY
	bne smem_v0_loop_skip

	// check category
	ldr r5, [r6, #28]
	cmp r5, #0xE // SDRAM
	ldr r4, =arm_qcom_mmu_name_memory // name
	beq smem_v0_loop_add

	// check category
	ldr r5, [r6, #28]
	cmp r5, #5 // IMEM
	mov r4, #0 // name = NULL
	bne smem_v0_loop_skip

	// add entry
smem_v0_loop_add:
	// size
	ldr r0, [r6, #20]
	mov r1, #0x100000 // 1MB
	bl arm_qcom_align
	mov r2, r0

	ldr r0, [r6, #16] // addr->phys
	mov r1, r0 // virt
	mov r3, #MMU_INITIAL_MAPPING_FLAG_DEVICE // flags
	bl arm_qcom_mmu_table_add

smem_v0_loop_skip:
	// continue
	add r9, #1
	cmp r9, r10
	bne 0b
// END OF LOOP

	b smem_v0_exit

smem_v0_exit_err:
	mov r0, #-1	

smem_v0_exit:
	pop		{ r4-r12, lr }
	bx		lr

FUNCTION(arm_qcom_mmu_table_add_smem)
	push	{ r4-r12, lr }

	// r12 = smem_addr
	ldr r12, =MSM_SHARED_BASE
	// r11 = smem->alloc_info
	mov r11, r12
	add r11, #0xd0
	// r11 = &alloc_info[type]
	ldr r0, =402 // SMEM_USABLE_RAM_PARTITION_TABLE
	mov r1, #16 // sizeof(struct smem_alloc_info)
	mul r0, r1
	add r11, r0

	// ainfo->allocated==0
	ldr r0, [r11]
	cmp r0, #0
	beq smem_exit_err

	// r10 = ainfo->offset
	ldr r10, [r11, #4]
	// r9 = ainfo->size
	ldr r9, [r11, #8]
	// r8 = ainfo->base_ext
	ldr r8, [r11, #12]

	// smem + ainfo->offset + offset;
	mov r7, r12 // smem
	add r7, r10 // ainfo->offset
	add r7, #8 // SMEM_RAM_PTABLE_VERSION_OFFSET

	// r0 = ptable_version
	ldr r6, [r7]

	// r5 = ptable_addr
	mov r5, r12 // smem_addr
	add r5, r10 // offset

	// V0
	mov r0, r5
	bl arm_smem_ptable_load_v0

	b smem_exit

smem_exit_err:
	mov r0, #-1

smem_exit:
	pop		{ r4-r12, lr }
	bx		lr

.data
DATA(arm_qcom_mmu_name_memory)
	.ascii "memory\0"
DATA(arm_qcom_mmu_name_identity)
	.ascii "identity\0"

.align 2
DATA(mmu_initial_mappings)
	.rept 10*20
	.word 0
	.endr
