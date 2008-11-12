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
#ifndef  COCO_SYNTAX_NODES_H
#define  COCO_SYNTAX_NODES_H

#ifndef  COCO_EBNF_H
#include  "EBNF.h"
#endif

#ifndef  COCO_BITARRAY_H
#include  "BitArray.h"
#endif

typedef struct {
    CcNode_t base;
    CcSymbol_t * sym;
    CcsPosition_t * pos;
} CcNodeT_t;
extern const CcObjectType_t * node_t;

typedef struct {
    CcNode_t base;
} CcNodePR_t;
extern const CcObjectType_t * node_pr;

typedef struct {
    CcNode_t base;
    CcSymbol_t * sym;
    CcsPosition_t * pos;
} CcNodeNT_t;
extern const CcObjectType_t * node_nt;

typedef struct {
    CcNode_t base;
    CcSymbol_t * sym;
    CcsPosition_t * pos;
} CcNodeWT_t;
extern const CcObjectType_t * node_wt;

typedef struct {
    CcNode_t base;
    CcBitArray_t * set;
    CcBitArray_t setSpace;
} CcNodeANY_t;
extern const CcObjectType_t * node_any;

typedef struct {
    CcNode_t base;
    CcBitArray_t * set;
    CcBitArray_t setSpace;
} CcNodeSYNC_t;
extern const CcObjectType_t * node_sync;

typedef struct {
    CcNode_t base;
    CcsPosition_t * pos;
} CcNodeSEM_t;
extern const CcObjectType_t * node_sem;

typedef struct {
    CcNode_t base;
    CcsPosition_t * pos;
} CcNodeRSLV_t;
extern const CcObjectType_t * node_rslv;

#endif  /* COCO_SYNTAX_NODES_H */
