/*
 * Copyright (c) 2008, Google Inc.
 * All rights reserved.
 *
 * Copyright (c) 2009-2012, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the 
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#ifndef __LIB_PTABLE_MSM_H
#define __LIB_PTABLE_MSM_H

/* flash partitions are defined in terms of blocks
 * (flash erase units)
 */
#define MAX_PTENTRY_MSM_NAME	16
#define MAX_PTABLE_MSM_PARTS	32

#define TYPE_MODEM_PARTITION	1
#define TYPE_APPS_PARTITION	0
#define PERM_NON_WRITEABLE	0
#define PERM_WRITEABLE		1
struct ptentry_msm
{
	char name[MAX_PTENTRY_MSM_NAME];
	unsigned start;
	unsigned length;
	unsigned flags;
	char type;
	char perm;
};

struct ptable_msm
{
	struct ptentry_msm parts[MAX_PTABLE_MSM_PARTS];
	int count;
};

/* tools to populate and query the partition table */
void ptable_msm_init(struct ptable_msm *ptable);
void ptable_msm_add(struct ptable_msm *ptable, char *name, unsigned start,
		unsigned length, unsigned flags, char type, char perm);
struct ptentry_msm *ptable_msm_find(struct ptable_msm *ptable, const char *name);
struct ptentry_msm *ptable_msm_get(struct ptable_msm *ptable, int n);
int ptable_msm_get_index(struct ptable_msm *ptable, const char *name);
int ptable_msm_size(struct ptable_msm *ptable);
void ptable_msm_dump(struct ptable_msm *ptable);

#endif /* __LIB_PTABLE_MSM_H */
