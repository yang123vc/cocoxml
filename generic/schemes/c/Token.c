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
#include  "Token.h"

CcsToken_t *
CcsToken(int kind, int pos, int col, int line, const char * val, size_t vallen)
{
    CcsToken_t * self;
    if (!(self = CcsMalloc(sizeof(CcsToken_t) + vallen + 1))) return NULL;
    self->refcnt = 0;
    self->next = NULL;
    self->kind = kind;
    self->pos = pos;
    self->col = col;
    self->line = line;
    self->val = (char *)(self + 1);
    if (vallen > 0) memcpy(self->val, val, vallen);
    self->val[vallen] = 0;
    return self;
}

void
CcsToken_Destruct(CcsToken_t * self)
{
    CcsFree(self);
}
