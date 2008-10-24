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
#ifndef  COCO_BITARRAY_H
#define  COCO_BITARRAY_H

#ifndef  COCO_DEFS_H
#include  "Defs.h"
#endif

EXTC_BEGIN

struct BitArray_s {
    int numbits;
    unsigned char * data;
};

BitArray_t * BitArray(BitArray_t * self, int numbits);
BitArray_t * BitArray_Clone(BitArray_t * self, const BitArray_t * value);
void BitArray_Destruct(BitArray_t * self);

/* Return -1 for error. */
int BitArray_getCount(const BitArray_t * self);
int BitArray_Elements(const BitArray_t * self);
Bool_t BitArray_Get(const BitArray_t * self, int index);
void BitArray_Set(BitArray_t * self, int index, Bool_t value);
void BitArray_SetAll(BitArray_t * self, Bool_t value);
Bool_t BitArray_Equal(const BitArray_t * self1, const BitArray_t * self2);
void BitArray_Not(BitArray_t * self);
int BitArray_And(BitArray_t * self, const BitArray_t * value);
int BitArray_Or(BitArray_t * self, const BitArray_t * value);
int BitArray_Xor(BitArray_t * self, const BitArray_t * value);
Bool_t BitArray_Intersect(const BitArray_t * self1, const BitArray_t * self2);
void BitArray_Subtract(BitArray_t * self, const BitArray_t * b);

void BitArray_Dump(const BitArray_t * self, DumpBuffer_t * buf);

EXTC_END

#endif  /* COCO_BITARRAY_H */
