/*-------------------------------------------------------------------------
  Copyright (C) 2008, Charles Wang
  Author: Charles Wang <charlesw123456@gmail.com>
  License: GPLv2 (see LICENSE-GPL)
-------------------------------------------------------------------------*/
#include  "lexical/CharSet.h"

CcRange_t *
new_Range(int from, int to)
{
    CcRange_t * self = CcMalloc(sizeof(CcRange_t));
    self->from = from; self->to = to; self->next = NULL;
    return self;
}
#define Del_Range(range)  CcFree(range)

CcCharSet_t *
CcCharSet(void)
{
    CcCharSet_t * self = CcMalloc(sizeof(CcCharSet_t));
    self->head = NULL;
    return self;
}

void
CcCharSet_Destruct(CcCharSet_t * self)
{
    CcCharSet_Clear(self);
    CcFree(self);
}

CcsBool_t
CcCharSet_Get(const CcCharSet_t * self, int i)
{
    CcRange_t * cur = self->head;
    for (cur = self->head; cur; cur = cur->next) {
	if (i < cur->from) return FALSE;
	else if (i <= cur->to) return TRUE;
    }
    return FALSE;
}

void
CcCharSet_Set(CcCharSet_t * self, int i)
{
    CcRange_t * cur, * prev = NULL, * tmp;

    for (cur = self->head; cur; cur = cur->next) {
	if (i < cur->from - 1) { /* New Range has to be setup. */
	    break;
	} else if (i == cur->from - 1) { /* Extend cur->from is ok. */
	    --cur->from;
	    return;
	} else if (i <= cur->to) { /* In cur already. */
	    return;
	} else if (i == cur->to + 1) { /* Extend cur->to is ok. */
	    if (cur->next != NULL && i == cur->next->from - 1) {
		/* Combine cur and cur->next. */
		tmp = cur->next;
		cur->to = cur->next->to;
		cur->next = cur->next->next;
		Del_Range(tmp);
	    } else {
		++cur->to;
	    }
	    return;
	}
	prev = cur;
    }
    tmp = new_Range(i, i);
    tmp->next = cur;
    if (prev == NULL) self->head = tmp;
    else prev->next = tmp;
}

CcCharSet_t *
CcCharSet_Clone(const CcCharSet_t * s)
{
    CcRange_t * prev, * curnew;
    const CcRange_t * cur1;
    CcCharSet_t * self = CcMalloc(sizeof(CcCharSet_t));
    self->head = NULL; prev = NULL;
    for (cur1 = s->head; cur1; cur1 = cur1->next) {
	curnew = new_Range(cur1->from, cur1->to);
	if (prev == NULL) self->head = curnew;
	else prev->next = curnew;
	prev = curnew;
    }
    return self;
}

CcsBool_t
CcCharSet_IsEmpty(const CcCharSet_t * self)
{
    return self->head == NULL;
}

CcsBool_t
CcCharSet_Equals(const CcCharSet_t * self, const CcCharSet_t * s)
{
    const CcRange_t * cur0, * cur1;
    cur0 = self->head; cur1 = s->head;
    while (cur0 && cur1) {
	if (cur0->from != cur1->from || cur0->to != cur1->to) return FALSE;
	cur0 = cur0->next; cur1 = cur1->next;
    }
    return cur0 == cur1;
}

int
CcCharSet_Elements(const CcCharSet_t * self)
{
    int cnt = 0;
    const CcRange_t * cur;
    for (cur = self->head; cur; cur = cur->next)
	cnt += cur->to - cur->from + 1;
    return cnt;
}

int
CcCharSet_NumRange(const CcCharSet_t * self)
{
    int cnt = 0;
    const CcRange_t * cur;
    for (cur = self->head; cur; cur = cur->next) ++cnt;
    return cnt;
}

int
CcCharSet_First(const CcCharSet_t * self)
{
    if (self->head) return self->head->from;
    return -1;
}

void
CcCharSet_Or(CcCharSet_t * self, const CcCharSet_t * s)
{
    CcRange_t * tmp, * cur0 = self->head, * prev = NULL;
    const CcRange_t * cur1 = s->head;

    while (cur0 && cur1) {
	if (cur0->from > cur1->to + 1) {
	    /* cur1 has to be inserted before cur0. */
	    tmp = new_Range(cur1->from, cur1->to);
	    if (prev == NULL) self->head = tmp;
	    else prev->next = tmp;
	    tmp->next = cur0;
	    prev = tmp; cur1 = cur1->next;
	} else if (cur0->to + 1 >= cur1->from) {
	    /* cur0, cur1 overlapped, expand cur0. */
	    if (cur0->from > cur1->from) cur0->from = cur1->from;
	    if (cur0->to < cur1->to) {
		cur0->to = cur1->to;
		/* Try to combine cur0->next. */
		while (cur0->next && cur0->next->from <= cur0->to + 1) {
		    tmp = cur0->next;
		    cur0->next = cur0->next->next;
		    if (cur0->to < tmp->to) cur0->to = tmp->to;
		    Del_Range(tmp);
		}
	    }
	    cur1 = cur1->next;
	} else { /* cur0->to + 1 < cur1->from */
	    prev = cur0; cur0 = cur0->next;
	}
    }
    while (cur1) { /* Add all of the remaining. */
	tmp = new_Range(cur1->from, cur1->to);
	if (prev == NULL) self->head = tmp;
	else prev->next = tmp;
	prev = tmp; cur1 = cur1->next;
    }
}

void
CcCharSet_And(CcCharSet_t * self, const CcCharSet_t * s)
{
    CcRange_t * tmp, * cur0 = self->head, * prev = NULL;
    const CcRange_t * cur1 = s->head;

    while (cur0 && cur1) {
	if (cur0->from > cur1->to) {
	    cur1 = cur1->next;
	} else if (cur0->to < cur1->from) {
	    tmp = cur0; cur0 = cur0->next;
	    if (prev == NULL) self->head = cur0;
	    else prev->next = cur0;
	    Del_Range(tmp);
	} else if (cur0->from < cur1->from && cur0->to <= cur1->to) {
	    cur0->from = cur1->from;
	    prev = cur0; cur0 = cur0->next;
	} else if (cur0->from < cur1->from && cur0->to > cur1->to) {
	    tmp = new_Range(cur1->to + 2, cur0->to);
	    tmp->next = cur0->next; cur0->next = tmp;
	    cur0->from = cur1->from; cur0->to = cur1->to;
	    prev = cur0; cur0 = cur0->next;
	    cur1 = cur1->next;
	} else if (cur0->from >= cur1->from && cur0->to <= cur1->to) {
	    prev = cur0; cur0 = cur0->next;
	} else if (cur0->from >= cur1->from && cur0->to > cur1->to) {
	    tmp = new_Range(cur1->to + 2, cur0->to);
	    tmp->next = cur0->next; cur0->next = tmp;
	    cur0->to = cur1->to;
	    prev = cur0; cur0 = cur0->next;
	    cur1 = cur1->next;
	}
    }
    if (cur0) {
	if (prev == NULL) self->head = NULL;
	else prev->next = NULL;
	while (cur0) {
	    tmp = cur0; cur0 = cur0->next;
	    Del_Range(tmp);
	}
    }
}

void
CcCharSet_Subtract(CcCharSet_t * self, const CcCharSet_t * s)
{
    CcRange_t * tmp, * cur0 = self->head, * prev = NULL;
    const CcRange_t * cur1 = s->head;

    while (cur0 && cur1) {
	if (cur0->from > cur1->to) {
	    cur1 = cur1->next;
	} else if (cur0->to < cur1->from) {
	    prev = cur0; cur0 = cur0->next;
	} else if (cur0->from < cur1->from && cur0->to <= cur1->to) {
	    cur0->to = cur1->from - 1;
	    prev = cur0; cur0 = cur0->next;
	} else if (cur0->from < cur1->from && cur0->to > cur1->to) {
	    tmp = new_Range(cur1->to + 1, cur0->to);
	    cur0->to = cur1->from - 1;
	    tmp->next = cur0->next; cur0->next = tmp;
	    prev = cur0; cur0 = cur0->next;
	    cur1 = cur1->next;
	} else if (cur0->from >= cur1->from && cur0->to <= cur1->to) {
	    tmp = cur0; cur0 = cur0->next;
	    if (prev == NULL) self->head = cur0;
	    else prev->next = cur0;
	    Del_Range(tmp);
	} else if (cur0->from >= cur1->from && cur0->to > cur1->to) {
	    cur0->from = cur1->to + 1;
	    cur1 = cur1->next;
	}
    }
}

CcsBool_t
CcCharSet_Includes(const CcCharSet_t * self, const CcCharSet_t * s)
{
    const CcRange_t * cur0 = self->head, * cur1 = s->head;

    while (cur0 && cur1) {
	if (cur0->to < cur1->from)  cur0 = cur0->next;
	else if (cur0->from <= cur1->from && cur0->to >= cur1->to)
	    cur1 = cur1->next;
	else  return FALSE;
    }
    return cur1 == NULL;
}

CcsBool_t
CcCharSet_Intersects(const CcCharSet_t * self, const CcCharSet_t * s)
{
    const CcRange_t * cur0 = self->head, * cur1 = s->head;

    while (cur0 && cur1) {
	if (cur0->from > cur1->to) cur1 = cur1->next;
	else if (cur0->to < cur1->from) cur0 = cur0->next;
	else return TRUE;
    }
    return FALSE;
}

void
CcCharSet_Clear(CcCharSet_t * self)
{
    CcRange_t * tmp;
    while (self->head != NULL) {
	tmp = self->head;
	self->head = self->head->next;
	Del_Range(tmp);
    }
}

void
CcCharSet_Fill(CcCharSet_t * self, int maxchar)
{
    CcCharSet_Clear(self);
    self->head = new_Range(0, maxchar);
}
