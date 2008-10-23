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
#include  <stdlib.h>
#include  <string.h>
#include  "BitArray.h"

#define NB2SZ(nb)   (((nb) + 7) >> 3)
static int bitmask[] = { 0xFF, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F };

BitArray_t *
BitArray(BitArray_t * self, int numbits)
{
    self->numbits = numbits;
    if (numbits) {
	if (!(self->data = malloc(NB2SZ(numbits)))) return NULL;
	bzero(self->data, NB2SZ(numbits));
    } else {
	self->data = NULL;
    }
    return self;
}

BitArray_t *
BitArray_Clone(BitArray_t * self, const BitArray_t * value)
{
    if (value->data) {
	if (!(self->data = malloc(NB2SZ(value->numbits)))) return NULL;
	memcpy(self->data, value->data, NB2SZ(value->numbits));
    } else {
	self->data = NULL;
    }
    self->numbits = value->numbits;
    return self;
}

void
BitArray_Destruct(BitArray_t * self)
{
    if (self->data) {
	free(self->data);
	self->data = NULL;
    }
    self->numbits = 0;
}

int
BitArray_getCount(const BitArray_t * self)
{
    return self->numbits;
}

int
BitArray_Elements(const BitArray_t * self)
{
    int bit;
    int elements = 0;
    int bits = self->numbits;
    const unsigned char * cur = self->data;
    while (bits >= 8) {
	for (bit = 0; bit < 8; ++bit)
	    if ((*cur & (1 << bit))) ++elements;
	bits = bits - 8;
	++cur;
    }
    for (bit = 0; bit < bits; ++bit)
	if ((*cur & (1 << bit))) ++elements;
    return elements;
}

int
BitArray_Get(const BitArray_t * self, int index)
{
    if (index < 0 || index >= self->numbits) return -1;
    return self->data[index >> 3] & (1 << (index & 0x07));
}

int
BitArray_Set(BitArray_t * self, int index, gboolean value)
{
    /*
    if (NB2SZ(index) > NB2SZ(self->numbits)) {
	if (!(newdata = realloc(self->data, NB2SZ(index)))) return -1;
	bzero(newdata + NB2SZ(self->numbits),
	      NB2SZ(index) - NB2SZ(self->numbits));
    }
    if (index > self->numbits) self->numbits = index;
    */
    if (value) self->data[index >> 3] |= 1 << (index & 0x07);
    else self->data[index >> 3] &= ~(1 << (index & 0x07));
    return 0;
}

void
BitArray_SetAll(BitArray_t * self, gboolean value)
{
    if (self->data) {
	if (value) memset(self->data, 1, NB2SZ(self->numbits));
	else bzero(self->data, NB2SZ(self->numbits));
    }
}

int
BitArray_Equal(const BitArray_t * self1, const BitArray_t * self2)
{
    int boffset, bmask;
    if (self1->numbits != self2->numbits) return 0;
    if (!self1->data && !self2->data) return 1;
    if (!self1->data || !self2->data) return 0;
    if (self1->numbits > 8 &&
	memcmp(self1->data, self2->data, NB2SZ(self1->numbits) - 1))
	return 0;
    boffset = NB2SZ(self1->numbits) - 1;
    bmask = bitmask[self1->numbits & 0x07];
    if ((self1->data[boffset] & bmask) != (self2->data[boffset] & bmask))
	return 0;
    return 1;
}

void
BitArray_Not(BitArray_t * self)
{
    unsigned char * cur;
    for (cur = self->data; cur - self->data < NB2SZ(self->numbits); ++cur)
	*cur ^= 0xFF;
}

int
BitArray_And(BitArray_t * self, const BitArray_t * value)
{
    unsigned char * cur0, * cur1;
    if (self->numbits > value->numbits) return -1;
    for (cur0 = self->data, cur1 = value->data;
	 cur0 - self->data < NB2SZ(self->numbits); ++cur0, ++cur1)
	*cur0 &= *cur1;
    return 0;
}

int
BitArray_Or(BitArray_t * self, const BitArray_t * value)
{
    unsigned char * cur0, * cur1;
    if (self->numbits > value->numbits) return -1;
    for (cur0 = self->data, cur1 = value->data;
	 cur0 - self->data < NB2SZ(self->numbits); ++cur0, ++cur1)
	*cur0 |= *cur1;
    return 0;
}

int
BitArray_Xor(BitArray_t * self, const BitArray_t * value)
{
    unsigned char * cur0, * cur1;
    if (self->numbits > value->numbits) return -1;
    for (cur0 = self->data, cur1 = value->data;
	 cur0 - self->data < NB2SZ(self->numbits); ++cur0, ++cur1)
	*cur0 ^= *cur1;
    return 0;
}

gboolean
BitArray_Intersect(const BitArray_t * self1, const BitArray_t * self2)
{
    /* assert(self1->numbits == self2->numbits2); */
    int idx, numbytes = NB2SZ(self1->numbits);
    if (numbytes == 0) return 0;
    for (idx = 0; idx < numbytes - 1; ++idx)
	if ((self1->data[idx] & self2->data[idx])) return 1;
    if ((self1->data[numbytes - 1] & self2->data[numbytes - 1] &
	 bitmask[self1->numbits & 0x07]))
	return 1;
    return 0;
}

void
BitArray_Subtract(BitArray_t * self, const BitArray_t * b)
{
    /* assert(self->numbits == b->numbits); */
    int idx;
    for (idx = 0; idx < NB2SZ(self->numbits); ++idx)
	self->data[idx] &= ~ b->data[idx];
}

void
BitArray_Dump(const BitArray_t * self, DumpBuffer_t * buf)
{
    int idx, numbits = BitArray_getCount(self);
    for (idx = 0; idx < numbits; ++idx)
	DumpBuffer_Print(buf, "%c", BitArray_Get(self, idx) ? "1" : ".");
}