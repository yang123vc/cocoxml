/*-------------------------------------------------------------------------
  Author (C) 2008, Charles Wang <charlesw123456@gmail.com>

  This program is free software; you can redistribute it and/or modify it 
  under the terms of the GNU General Public License as published by the 
  Free Software Foundation; either version 2, or (at your option) any 
  later version.

  This program is distributed in the hope that it will be useful, but 
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
  for more details.

  You should have received a copy of the GNU General Public License along 
  with this program; if not, write to the Free Software Foundation, Inc., 
  59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

  As an exception, it is allowed to write an extension of Coco/R that is
  used as a plugin in non-free software.

  If not otherwise stated, any source code generated by Coco/R (other than 
  Coco/R itself) does not fall under the GNU General Public License.
-------------------------------------------------------------------------*/
#ifndef  COCO_OBJECT_H
#define  COCO_OBJECT_H

#ifndef  COCO_DEFS_H
#include "Defs.h"
#endif

EXTC_BEGIN

struct CcObjectType_s {
    size_t size;
    const char * name;
    void (* destruct)(CcObject_t * self);
};

struct CcObject_s {
    const CcObjectType_t * type;
    int index;
};

CcObject_t * CcObject(const CcObjectType_t * type);
void CcObject_Destruct(CcObject_t * self);

void CcObject_VDestruct(CcObject_t * self);

EXTC_END

#endif  /* COCO_OBJECT_H */
