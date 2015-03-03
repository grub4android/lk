/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 *Copyright (c) 2009-2012, The Linux Foundation. All rights reserved.
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

#include <list.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <platform/qcom_ptable.h>

void qcom_ptable_init(struct qcom_ptable *ptable)
{
	ASSERT(ptable);

	memset(ptable, 0, sizeof(struct qcom_ptable));
}

const char* ptype[] = {"Apps", "Modem"};
const char* pperm[] = {"No", "Yes"};

void qcom_ptable_add(struct qcom_ptable *ptable, char *name, unsigned start,
		unsigned length, unsigned flags, char type, char perm)
{
	struct qcom_ptentry *ptn;

	ASSERT(ptable && ptable->count < MAX_QCOM_PTABLE_PARTS);

	ptn = &ptable->parts[ptable->count++];
	strlcpy(ptn->name, name, MAX_QCOM_PTENTRY_NAME);
	ptn->start = start;
	ptn->length = length;
	ptn->flags = flags;
	ptn->type = type;
	ptn->perm = perm;
}

void qcom_ptable_dump(struct qcom_ptable *ptable)
{
	struct qcom_ptentry *ptn;
	int i;

	for (i = 0; i < ptable->count; ++i) {
		ptn = &ptable->parts[i];
		dprintf(INFO, "ptn %d name='%s' start=%08x len=%08x "
			"flags=%08x type=%s Writable=%s\n", i, ptn->name, ptn->start, ptn->length,
			ptn->flags, ptype[(unsigned short) ptn->type], pperm[(unsigned short) ptn->perm]);
	}
}

struct qcom_ptentry *qcom_ptable_find(struct qcom_ptable *ptable, const char *name)
{
	struct qcom_ptentry *ptn;
	int i;

	for (i = 0; i < ptable->count; ++i) {
		ptn = &ptable->parts[i];
		if (!strcmp(ptn->name, name))
			return ptn;
	}

	return NULL;
}

struct qcom_ptentry *qcom_ptable_get(struct qcom_ptable *ptable, int n)
{
	if (n >= ptable->count)
		return NULL;
	return &ptable->parts[n];
}

int qcom_ptable_size(struct qcom_ptable *ptable)
{
    return ptable->count;
}

int qcom_ptable_get_index(struct qcom_ptable *ptable, const char *name)
{
	for(int i=0; i < ptable->count; i++) {
		if (!strcmp(ptable->parts[i].name, name))
			return i;
	}
	return -1;
}
