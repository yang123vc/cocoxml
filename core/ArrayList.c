/*-------------------------------------------------------------------------
  Copyright (C) 2008, Charles Wang
  Author: Charles Wang <charlesw123456@gmail.com>
  License: GPLv2 (see LICENSE-GPL)
-------------------------------------------------------------------------*/
#include  "ArrayList.h"
#include  "Object.h"

CcArrayList_t *
CcArrayList(CcArrayList_t * self)
{
    self->Count = 0;
    self->Capacity = 16;
    self->Objects = CcMalloc(sizeof(void *) * self->Capacity);
    return self;
}

void
CcArrayList_Destruct(CcArrayList_t * self)
{
    CcArrayList_Clear(self);
    if (self->Objects)  CcFree(self->Objects);
}

CcObject_t *
CcArrayList_New(CcArrayList_t * self, CcObject_t * object)
{
    int newCapacity;

    if (self->Count >= self->Capacity) {
	newCapacity = self->Capacity + self->Capacity;
	self->Objects = CcRealloc(self->Objects,
				  sizeof(CcObject_t *) * newCapacity);
	self->Capacity = newCapacity;
    }
    object->index = self->Count;
    self->Objects[self->Count++] = object;
    return object;
}

CcObject_t *
CcArrayList_Get(CcArrayList_t * self, int index)
{
    return (index >= 0 && index < self->Count) ? self->Objects[index] : NULL;
}

const CcObject_t *
CcArrayList_GetC(const CcArrayList_t * self, int index)
{
    return (index >= 0 && index < self->Count) ? self->Objects[index] : NULL;
}

void
CcArrayList_Clear(CcArrayList_t * self)
{
    int idx;
    for (idx = 0; idx < self->Count; ++idx)
	CcObject_VDestruct(self->Objects[idx]);
    self->Count = 0;
}

CcObject_t *
CcArrayList_First(CcArrayList_t * self, CcArrayListIter_t * iter)
{
    iter->index = 0;
    return CcArrayList_Next(self, iter);
}

CcObject_t *
CcArrayList_Next(CcArrayList_t * self, CcArrayListIter_t * iter)
{
    return iter->index < self->Count ? self->Objects[iter->index++] : NULL;
}

const CcObject_t *
CcArrayList_FirstC(const CcArrayList_t * self, CcArrayListIter_t * iter)
{
    iter->index = 0;
    return CcArrayList_NextC(self, iter);
}

const CcObject_t *
CcArrayList_NextC(const CcArrayList_t * self, CcArrayListIter_t * iter)
{
    return iter->index < self->Count ? self->Objects[iter->index++] : NULL;
}

CcArrayListIter_t *
CcArrayListIter_Copy(CcArrayListIter_t * self, const CcArrayListIter_t * orig)
{
    self->index = orig->index;
    return self;
}

void
CcArrayList_Filter(CcArrayList_t * self, CcArrayList_FilterFunc_t func,
		   void * data)
{
    int curidx, newidx;
    CcObject_t * object;

    newidx = 0;
    for (curidx = 0; curidx < self->Count; ++curidx) {
	if ((object = func(self->Objects[curidx], curidx, newidx, data))) {
	    object->index = newidx;
	    self->Objects[newidx++] = object;
	}
    }
    self->Count = newidx;
}
