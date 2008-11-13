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
#ifndef  COCO_LEXICAL_TRANSITION_H
#define  COCO_LEXICAL_TRANSITION_H

#ifndef  COCO_DEFS_H
#include  "Defs.h"
#endif

EXTC_BEGIN

typedef enum {
    trans_normal = 0,
    trans_context = 1
} CcTransitionCode_t;

struct CcTransition_s {
    CcArrayList_t * classes;
    CcTransitionCode_t code;
    CcsBool_t single;
    union {
	int chr; /* single is TRUE. */
	const CcCharSet_t * set; /* single is FALSE.
				    Point into CharClass in classes. */
    } u;
};

CcTransition_t *
CcTransition(CcTransition_t * self, int chr, CcTransitionCode_t code,
	     CcArrayList_t * classes);
CcTransition_t *
CcTransition_FromCharSet(CcTransition_t * self, const CcCharSet_t * s,
			 CcTransitionCode_t code, CcArrayList_t * classes);
CcTransition_t *
CcTransition_Clone(CcTransition_t * self, const CcTransition_t * t);

int CcTransition_Size(const CcTransition_t * self);

/* The returned CcCharSet_t must be destructed. */
CcCharSet_t * CcTransition_GetCharSet(const CcTransition_t * self);
void CcTransition_SetCharSet(CcTransition_t * self, const CcCharSet_t * trans);

void CcTransition_Destruct(CcTransition_t * self);

EXTC_END

#endif  /* COCO_LEXICAL_TRANSITION_H */