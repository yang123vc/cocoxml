/*---- license ----*/
/*-------------------------------------------------------------------------
 Coco.ATG -- Attributed Grammar
 Compiler Generator Coco/R,
 Copyright (c) 1990, 2004 Hanspeter Moessenboeck, University of Linz
 extended by M. Loeberbauer & A. Woess, Univ. of Linz
 with improvements by Pat Terry, Rhodes University.
 ported to C by Charles Wang <charlesw123456@gmail.com>

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
/*---- enable ----*/
#include  "Parser.h"
#include  "c/Token.h"

/*---- cIncludes ----*/
/*---- enable ----*/

static void PgnParser_SynErr(PgnParser_t * self, int n);
static const char * set[];

static void
PgnParser_Get(PgnParser_t * self)
{
    self->t = self->la;
    for (;;) {
	self->la = PgnScanner_Scan(&self->scanner);
	if (self->la->kind <= self->maxT) { /*++self->errDist;*/ break; }
	/*---- Pragmas ----*/
	/*---- enable ----*/
    }
}

static CcsBool_t
PgnParser_StartOf(PgnParser_t * self, int s)
{
    return set[s][self->la->kind] == '*';
}

static void
PgnParser_Expect(PgnParser_t * self, int n)
{
    if (self->la->kind == n) PgnParser_Get(self);
    else PgnParser_SynErr(self, n);
}

static void
PgnParser_ExpectWeak(PgnParser_t * self, int n, int follow)
{
    if (self->la->kind == n) PgnParser_Get(self);
    else {
	PgnParser_SynErr(self, n);
	while (!PgnParser_StartOf(self, follow)) PgnParser_Get(self);
    }
}

static CcsBool_t
PgnParser_WeakSeparator(PgnParser_t * self, int n, int syFol, int repFol)
{
    if (self->la->kind == n) { PgnParser_Get(self); return TRUE; }
    else if (PgnParser_StartOf(self, repFol)) { return FALSE; }
    PgnParser_SynErr(self, n);
    while (!(PgnParser_StartOf(self, syFol) ||
	     PgnParser_StartOf(self, repFol) ||
	     PgnParser_StartOf(self, 0)))
	PgnParser_Get(self);
    return PgnParser_StartOf(self, syFol);
}

/*---- ProductionsHeader ----*/
/*---- enable ----*/

void
PgnParser_Parse(PgnParser_t * self)
{
    self->t = NULL;
    self->la = PgnScanner_GetDummy(&self->scanner);
    PgnParser_Get(self);
    /*---- ParseRoot ----*/
    /*---- enable ----*/
    PgnParser_Expect(self, 0);
}

void
PgnParser_SemErr(PgnParser_t * self, const CcsToken_t * token,
		 const char * format, ...)
{
    va_list ap;
    va_start(ap, format);
    CcsErrorPool_VError(&self->errpool, token->line, token->col,
			format, ap);
    va_end(ap);
}

void
PgnParser_SemErrT(PgnParser_t * self, const char * format, ...)
{
    va_list ap;
    va_start(ap, format);
    CcsErrorPool_VError(&self->errpool, self->t->line, self->t->col,
			format, ap);
    va_end(ap);
}

PgnParser_t *
PgnParser(PgnParser_t * self, const char * fname, FILE * errfp)
{
    if (!CcsErrorPool(&self->errpool, errfp)) goto errquit0;
    if (!PgnScanner(&self->scanner, &self->errpool, fname)) goto errquit1;
    self->t = self->la = NULL;
    /*---- constructor ----*/
    /*---- enable ----*/
    return self;
 ERRQUIT:
 errquit1:
    CcsErrorPool_Destruct(&self->errpool);
 errquit0:
    return NULL;
}

void
PgnParser_Destruct(PgnParser_t * self)
{
    /*---- destructor ----*/
    /*---- enable ----*/
    PgnScanner_Destruct(&self->scanner);
    CcsErrorPool_Destruct(&self->errpool);
}

/*---- ProductionsBody ----*/
/*---- enable ----*/

static void
PgnParser_SynErr(PgnParser_t * self, int n)
{
    const char * s; char format[20];
    switch (n) {
    /*---- SynErrors ----*/
    /*---- enable ----*/
    default:
	snprintf(format, sizeof(format), "error %d", n);
	s = format;
	break;
    }
    PgnParser_SemErr(self, self->la, "%s", s);
}

static const char * set[] = {
    /*---- InitSet ----*/
    /*---- enable ----*/
};