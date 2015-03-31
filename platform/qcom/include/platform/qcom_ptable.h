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


#ifndef __QCOM_PTABLE_H
#define __QCOM_PTABLE_H

/* flash partitions are defined in terms of blocks
 * (flash erase units)
 */
#define MAX_QCOM_PTENTRY_NAME	16
#define MAX_QCOM_PTABLE_PARTS	32

#define TYPE_MODEM_PARTITION	1
#define TYPE_APPS_PARTITION	0
#define PERM_NON_WRITEABLE	0
#define PERM_WRITEABLE		1
struct qcom_ptentry
{
	char name[MAX_QCOM_PTENTRY_NAME];
	unsigned start;
	unsigned length;
	unsigned flags;
	char type;
	char perm;
};

struct qcom_ptable
{
	struct qcom_ptentry parts[MAX_QCOM_PTABLE_PARTS];
	int count;
};

/* tools to populate and query the partition table */
void qcom_ptable_init(struct qcom_ptable *ptable);
void qcom_ptable_add(struct qcom_ptable *ptable, char *name, unsigned start,
		unsigned length, unsigned flags, char type, char perm);
struct qcom_ptentry *qcom_ptable_find(struct qcom_ptable *ptable, const char *name);
struct qcom_ptentry *qcom_ptable_get(struct qcom_ptable *ptable, int n);
int qcom_ptable_get_index(struct qcom_ptable *ptable, const char *name);
int qcom_ptable_size(struct qcom_ptable *ptable);
void qcom_ptable_dump(struct qcom_ptable *ptable);

#endif /* __QCOM_PTABLE_H */
