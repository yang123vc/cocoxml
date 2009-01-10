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
#include  <ctype.h>
#include  "Scanner.h"

static int Char2State(int chr);
static CcsBool_t CcsXmlScanner_Init(CcsXmlScanner_t * self);
static CcsToken_t * CcsXmlScanner_NextToken(CcsXmlScanner_t * self);
static void CcsXmlScanner_GetCh(CcsXmlScanner_t * self);

static const char * dummyval = "dummy";

CcsXmlScanner_t *
CcsXmlScanner(CcsXmlScanner_t * self, CcsErrorPool_t * errpool, FILE * fp)
{
    self->errpool = errpool;
    if (!(self->dummyToken = CcsToken(0, 0, 0, 0, dummyval, strlen(dummyval))))
	goto errquit0;
    if (!CcsBuffer(&self->buffer, fp)) goto errquit1;
    if (!CcsXmlScanner_Init(self)) goto errquit2;
    return self;
 errquit2:
    CcsBuffer_Destruct(&self->buffer);
 errquit1:
    CcsToken_Destruct(self->dummyToken);
 errquit0:
    return NULL;
}

CcsXmlScanner_t *
CcsXmlScanner_ByName(CcsXmlScanner_t * self, CcsErrorPool_t * errpool,
		  const char * fn)
{
    self->errpool = errpool;
    if (!(self->dummyToken = CcsToken(0, 0, 0, 0, dummyval, strlen(dummyval))))
	goto errquit0;
    if (!CcsBuffer_ByName(&self->buffer, fn)) goto errquit1;
    if (!CcsXmlScanner_Init(self)) goto errquit2;
    return self;
 errquit2:
    CcsBuffer_Destruct(&self->buffer);
 errquit1:
    CcsToken_Destruct(self->dummyToken);
 errquit0:
    return NULL;
}

static CcsBool_t
CcsXmlScanner_Init(CcsXmlScanner_t * self)
{
#ifdef CcsXmlScanner_INDENTATION
    self->lineStart = TRUE;
    if (!(self->indent = CcsMalloc(sizeof(int) * CcsXmlScanner_INDENT_START)))
	return FALSE;
    self->indentUsed = self->indent;
    self->indentLast = self->indent + CcsXmlScanner_INDENT_START;
    *self->indentUsed++ = 0;
#endif
    /*---- declarations ----*/
    self->eofSym = 0;
    self->maxT = 39;
    self->noSym = 39;
    /*---- enable ----*/

    self->busyTokenList = NULL;
    self->curToken = &self->busyTokenList;
    self->peekToken = &self->busyTokenList;

    self->ch = 0; self->chBytes = 0;
    self->pos = 0; self->line = 1; self->col = 0;
    self->oldEols = 0; self->oldEolsEOL = 0;
    CcsXmlScanner_GetCh(self);
    return TRUE;
}

void
CcsXmlScanner_Destruct(CcsXmlScanner_t * self)
{
    CcsToken_t * cur, * next;

#ifdef CcsXmlScanner_INDENTATION
    CcsFree(self->indent);
#endif
    for (cur = self->busyTokenList; cur; cur = next) {
	/* May be trigged by .atg semantic code. */
	CcsAssert(cur->refcnt == 1);
	next = cur->next;
	CcsToken_Destruct(cur);
    }
    /* May be trigged by .atg semantic code. */
    CcsAssert(self->dummyToken->refcnt == 1);
    CcsToken_Destruct(self->dummyToken);
    CcsBuffer_Destruct(&self->buffer);
}

CcsToken_t *
CcsXmlScanner_GetDummy(CcsXmlScanner_t * self)
{
    CcsXmlScanner_IncRef(self, self->dummyToken);
    return self->dummyToken;
}

CcsToken_t *
CcsXmlScanner_Scan(CcsXmlScanner_t * self)
{
    CcsToken_t * cur;
    if (*self->curToken == NULL) {
	*self->curToken = CcsXmlScanner_NextToken(self);
	if (self->curToken == &self->busyTokenList)
	    CcsBuffer_SetBusy(&self->buffer, self->busyTokenList->pos);
    }
    cur = *self->curToken;
    self->peekToken = self->curToken = &cur->next;
    ++cur->refcnt;
    return cur;
}

CcsToken_t *
CcsXmlScanner_Peek(CcsXmlScanner_t * self)
{
    CcsToken_t * cur;
    do {
	if (*self->peekToken == NULL) {
	    *self->peekToken = CcsXmlScanner_NextToken(self);
	    if (self->peekToken == &self->busyTokenList)
		CcsBuffer_SetBusy(&self->buffer, self->busyTokenList->pos);
	}
	cur = *self->peekToken;
	self->peekToken = &cur->next;
    } while (cur->kind > self->maxT); /* Skip pragmas */
    ++cur->refcnt;
    return cur;
}

void
CcsXmlScanner_ResetPeek(CcsXmlScanner_t * self)
{
    self->peekToken = self->curToken;
}

void
CcsXmlScanner_IncRef(CcsXmlScanner_t * self, CcsToken_t * token)
{
    ++token->refcnt;
}

void
CcsXmlScanner_DecRef(CcsXmlScanner_t * self, CcsToken_t * token)
{
    if (--token->refcnt > 1) return;
    CcsAssert(token->refcnt == 1);
    if (token != self->busyTokenList) return;
    /* Detach all tokens which is refered by self->busyTokenList only. */
    while (token && token->refcnt <= 1) {
	CcsAssert(token->refcnt == 1);
	/* Detach token. */
	if (self->curToken == &token->next)
	    self->curToken = &self->busyTokenList;
	if (self->peekToken == &token->next)
	    self->peekToken = &self->busyTokenList;
	self->busyTokenList = token->next;
	CcsToken_Destruct(token);
	token = self->busyTokenList;
    }
    /* Adjust CcsBuffer busy pointer */
    if (self->busyTokenList) {
	CcsAssert(self->busyTokenList->refcnt > 1);
	CcsBuffer_SetBusy(&self->buffer, self->busyTokenList->pos);
    } else {
	CcsBuffer_ClearBusy(&self->buffer);
    }
}

CcsPosition_t *
CcsXmlScanner_GetPosition(CcsXmlScanner_t * self, const CcsToken_t * begin,
		       const CcsToken_t * end)
{
    int len = end->pos - begin->pos;
    return CcsPosition(begin->pos, len, begin->col,
		       CcsBuffer_GetString(&self->buffer, begin->pos, len));
}

CcsPosition_t *
CcsXmlScanner_GetPositionBetween(CcsXmlScanner_t * self, const CcsToken_t * begin,
			      const CcsToken_t * end)
{
    int begpos = begin->pos + strlen(begin->val);
    int len = end->pos - begpos;
    const char * start = CcsBuffer_GetString(&self->buffer, begpos, len);
    const char * cur, * last = start + len;

    /* Skip the leading spaces. */
    for (cur = start; cur < last; ++cur)
	if (*cur != ' ' && *cur != '\t' && *cur != '\r' && *cur != '\n') break;
    return CcsPosition(begpos + (cur - start), last - cur, 0, cur);
}

/* All the following things are used by CcsXmlScanner_NextToken. */
typedef struct {
    int keyFrom;
    int keyTo;
    int val;
}  Char2State_t;

static const Char2State_t c2sArr[] = {
    /*---- chars2states ----*/
    { EoF, EoF, -1 },
    { 34, 34, 10 },	/* '"' '"' */
    { 39, 39, 5 },	/* '\'' '\'' */
    { 40, 40, 26 },	/* '(' '(' */
    { 41, 41, 17 },	/* ')' ')' */
    { 46, 46, 24 },	/* '.' '.' */
    { 48, 57, 2 },	/* '0' '9' */
    { 60, 60, 25 },	/* '<' '<' */
    { 61, 61, 12 },	/* '=' '=' */
    { 62, 62, 13 },	/* '>' '>' */
    { 65, 90, 1 },	/* 'A' 'Z' */
    { 91, 91, 18 },	/* '[' '[' */
    { 93, 93, 19 },	/* ']' ']' */
    { 95, 95, 1 },	/* '_' '_' */
    { 97, 122, 1 },	/* 'a' 'z' */
    { 123, 123, 20 },	/* '{' '{' */
    { 124, 124, 16 },	/* '|' '|' */
    { 125, 125, 21 },	/* '}' '}' */
    /*---- enable ----*/
};
static const int c2sNum = sizeof(c2sArr) / sizeof(c2sArr[0]);

static int
c2sCmp(const void * key, const void * c2s)
{
    int keyval = *(const int *)key;
    const Char2State_t * ccc2s = (const Char2State_t *)c2s;
    if (keyval < ccc2s->keyFrom) return -1;
    if (keyval > ccc2s->keyTo) return 1;
    return 0;
}
static int
Char2State(int chr)
{
    Char2State_t * c2s;

    c2s = bsearch(&chr, c2sArr, c2sNum, sizeof(Char2State_t), c2sCmp);
    return c2s ? c2s->val : 0;
}

#ifdef CcsXmlScanner_KEYWORD_USED
typedef struct {
    const char * key;
    int val;
}  Identifier2KWKind_t;

static const Identifier2KWKind_t i2kArr[] = {
    /*---- identifiers2keywordkinds ----*/
    { "ANY", 34 },
    { "ATTRS", 20 },
    { "COMPILER", 6 },
    { "CONSTRUCTOR", 8 },
    { "DESTRUCTOR", 9 },
    { "END", 13 },
    { "IF", 36 },
    { "IGNORECASE", 17 },
    { "MEMBERS", 7 },
    { "NAMESPACE", 16 },
    { "OPTIONS", 18 },
    { "PROCESSING_INSTRUCTIONS", 21 },
    { "PRODUCTIONS", 10 },
    { "SCHEME", 14 },
    { "SECTION", 15 },
    { "SYNC", 35 },
    { "TAGS", 19 },
    { "WEAK", 27 },
    /*---- enable ----*/
};
static const int i2kNum = sizeof(i2kArr) / sizeof(i2kArr[0]);

static int
i2kCmp(const void * key, const void * i2k)
{
    return strcmp((const char *)key, ((const Identifier2KWKind_t *)i2k)->key);
}

static int
Identifier2KWKind(const char * key, size_t keylen, int defaultVal)
{
#ifndef CcsXmlScanner_CASE_SENSITIVE
    char * cur;
#endif
    char keystr[CcsXmlScanner_MAX_KEYWORD_LEN + 1];
    Identifier2KWKind_t * i2k;

    if (keylen > CcsXmlScanner_MAX_KEYWORD_LEN) return defaultVal;
    memcpy(keystr, key, keylen);
    keystr[keylen] = 0;
#ifndef CcsXmlScanner_CASE_SENSITIVE
    for (cur = keystr; *cur; ++cur) *cur = tolower(*cur);
#endif
    i2k = bsearch(keystr, i2kArr, i2kNum, sizeof(Identifier2KWKind_t), i2kCmp);
    return i2k ? i2k->val : defaultVal;
}

static int
CcsXmlScanner_GetKWKind(CcsXmlScanner_t * self, int start, int end, int defaultVal)
{
    return Identifier2KWKind(CcsBuffer_GetString(&self->buffer,
						 start, end - start),
			     end - start, defaultVal);
}
#endif /* CcsXmlScanner_KEYWORD_USED */

static void
CcsXmlScanner_GetCh(CcsXmlScanner_t * self)
{
    if (self->oldEols > 0) {
	self->ch = '\n'; --self->oldEols; self->oldEolsEOL = 1;
    } else {
	if (self->ch == '\n') {
	    if (self->oldEolsEOL) self->oldEolsEOL = 0;
	    else {
		++self->line; self->col = 0;
	    }
#ifdef CcsXmlScanner_INDENTATION
	    self->lineStart = TRUE;
#endif
	} else if (self->ch == '\t') {
	    self->col += 8 - self->col % 8;
	} else {
	    /* FIX ME: May be the width of some specical character
	     * is NOT self->chBytes. */
	    self->col += self->chBytes;
	}
	self->ch = CcsBuffer_Read(&self->buffer, &self->chBytes);
	self->pos = CcsBuffer_GetPos(&self->buffer);
    }
}

typedef struct {
    int ch, chBytes;
    int pos, line, col;
}  SLock_t;
static void
CcsXmlScanner_LockCh(CcsXmlScanner_t * self, SLock_t * slock)
{
    slock->ch = self->ch;
    slock->chBytes = self->chBytes;
    slock->pos = self->pos;
    slock->line = self->line;
    slock->col = self->col;
    CcsBuffer_Lock(&self->buffer);
}
static void
CcsXmlScanner_UnlockCh(CcsXmlScanner_t * self, SLock_t * slock)
{
    CcsBuffer_Unlock(&self->buffer);
}
static void
CcsXmlScanner_ResetCh(CcsXmlScanner_t * self, SLock_t * slock)
{
    self->ch = slock->ch;
    self->chBytes = slock->chBytes;
    self->pos = slock->pos;
    self->line = slock->line;
    CcsBuffer_LockReset(&self->buffer);
}

typedef struct {
    int start[2];
    int end[2];
    CcsBool_t nested;
}  CcsComment_t;

static const CcsComment_t comments[] = {
/*---- comments ----*/
    { { '/', '/' }, { '\n', 0 }, FALSE },
    { { '/', '*' }, { '*', '/' }, TRUE },
/*---- enable ----*/
};
static const CcsComment_t * commentsLast =
    comments + sizeof(comments) / sizeof(comments[0]);

static CcsBool_t
CcsXmlScanner_Comment(CcsXmlScanner_t * self, const CcsComment_t * c)
{
    SLock_t slock;
    int level = 1, line0 = self->line;

    if (c->start[1]) {
	CcsXmlScanner_LockCh(self, &slock); CcsXmlScanner_GetCh(self);
	if (self->ch != c->start[1]) {
	    CcsXmlScanner_ResetCh(self, &slock);
	    return FALSE;
	}
	CcsXmlScanner_UnlockCh(self, &slock);
    }
    CcsXmlScanner_GetCh(self);
    for (;;) {
	if (self->ch == c->end[0]) {
	    if (c->end[1] == 0) {
		if (--level == 0) break;
	    } else {
		CcsXmlScanner_LockCh(self, &slock); CcsXmlScanner_GetCh(self);
		if (self->ch == c->end[1]) {
		    CcsXmlScanner_UnlockCh(self, &slock);
		    if (--level == 0) break;
		} else {
		    CcsXmlScanner_ResetCh(self, &slock);
		}
	    }
	} else if (c->nested && self->ch == c->start[0]) {
	    if (c->start[1] == 0) {
		++level;
	    } else {
		CcsXmlScanner_LockCh(self, &slock); CcsXmlScanner_GetCh(self);
		if (self->ch == c->start[1]) {
		    CcsXmlScanner_UnlockCh(self, &slock);
		    ++level;
		} else {
		    CcsXmlScanner_ResetCh(self, &slock);
		}
	    }
	} else if (self->ch == EoF) {
	    return TRUE;
	}
	CcsXmlScanner_GetCh(self);
    }
    self->oldEols = self->line - line0;
    CcsXmlScanner_GetCh(self);
    return TRUE;
}

#ifdef CcsXmlScanner_INDENTATION
static CcsToken_t *
CcsXmlScanner_IndentGenerator(CcsXmlScanner_t * self)
{
    int newLen; int * newIndent, * curIndent;
    CcsToken_t * head, * cur;

    if (!self->lineStart) return NULL;
    CcsAssert(self->indent < self->indentUsed);
    /* Skip blank lines. */
    if (self->ch == '\r' || self->ch == '\n') return NULL;
    /* Dump all required IndentOut when EoF encountered. */
    if (self->ch == EoF) {
	head = NULL;
	while (self->indent < self->indentUsed - 1) {
	    cur = CcsToken(CcsXmlScanner_INDENT_OUT, self->pos,
			   self->col, self->line, NULL, 0);
	    cur->next = head; head = cur;
	    --self->indentUsed;
	}
	return head;
    }
    self->lineStart = FALSE;
    if (self->col > self->indentUsed[-1]) {
	if (self->indentUsed == self->indentLast) {
	    newLen = (self->indentLast - self->indent) + CcsXmlScanner_INDENT_START;
	    newIndent = CcsRealloc(self->indent, sizeof(int) * newLen);
	    if (!newIndent) return NULL;
	    self->indentUsed = newIndent + (self->indentUsed - self->indent);
	    self->indentLast = newIndent + newLen;
	    self->indent = newIndent;
	}
	CcsAssert(self->indentUsed < self->indentLast);
	*self->indentUsed++ = self->col;
	return CcsToken(CcsXmlScanner_INDENT_IN, self->pos,
			self->col, self->line, NULL, 0);
    }
    for (curIndent = self->indentUsed - 1; self->col < *curIndent; --curIndent);
    if (self->col > *curIndent)
	return CcsToken(CcsXmlScanner_INDENT_ERR, self->pos,
			self->col, self->line, NULL, 0);
    head = NULL;
    while (curIndent < self->indentUsed - 1) {
	cur = CcsToken(CcsXmlScanner_INDENT_OUT, self->pos,
		       self->col, self->line, NULL, 0);
	cur->next = head; head = cur;
	--self->indentUsed;
    }
    return head;
}
#endif

static CcsToken_t *
CcsXmlScanner_NextToken(CcsXmlScanner_t * self)
{
    int pos, line, col, state, kind; CcsToken_t * t;
    const CcsComment_t * curComment;
    for (;;) {
	while (self->ch == ' '
	       /*---- scan1 ----*/
	       || (self->ch >= '\t' && self->ch <= '\n')
	       || self->ch == '\r'
	       /*---- enable ----*/
	       ) CcsXmlScanner_GetCh(self);
#ifdef CcsXmlScanner_INDENTATION
	if ((t = CcsXmlScanner_IndentGenerator(self))) return t;
#endif
	for (curComment = comments; curComment < commentsLast; ++curComment)
	    if (self->ch == curComment->start[0] &&
		CcsXmlScanner_Comment(self, curComment)) break;
	if (curComment >= commentsLast) break;
    }
    pos = self->pos; line = self->line; col = self->col;
    CcsBuffer_Lock(&self->buffer);
    state = Char2State(self->ch);
    CcsXmlScanner_GetCh(self);
    switch (state) {
    case -1: kind = self->eofSym; break;
    case 0: kind = self->noSym; break;
    /*---- scan3 ----*/
    case 1: case_1:
	if ((self->ch >= '0' && self->ch <= '9') ||
	    (self->ch >= 'A' && self->ch <= 'Z') ||
	    self->ch == '_' ||
	    (self->ch >= 'a' && self->ch <= 'z')) {
	    CcsXmlScanner_GetCh(self); goto case_1;
	} else { kind = CcsXmlScanner_GetKWKind(self, pos, self->pos, 1); break; }
    case 2: case_2:
	if ((self->ch >= '0' && self->ch <= '9')) {
	    CcsXmlScanner_GetCh(self); goto case_2;
	} else { kind = 2; break; }
    case 3: case_3:
	{ kind = 3; break; }
    case 4: case_4:
	{ kind = 4; break; }
    case 5:
	if ((self->ch >= 0 && self->ch <= '\t') ||
	    (self->ch >= '\v' && self->ch <= '\f') ||
	    (self->ch >= 14 && self->ch <= '&') ||
	    (self->ch >= '(' && self->ch <= '[') ||
	    (self->ch >= ']' && self->ch <= 65535)) {
	    CcsXmlScanner_GetCh(self); goto case_6;
	} else if (self->ch == '\\') {
	    CcsXmlScanner_GetCh(self); goto case_7;
	} else { kind = self->noSym; break; }
    case 6: case_6:
	if (self->ch == '\'') {
	    CcsXmlScanner_GetCh(self); goto case_9;
	} else { kind = self->noSym; break; }
    case 7: case_7:
	if ((self->ch >= ' ' && self->ch <= '~')) {
	    CcsXmlScanner_GetCh(self); goto case_8;
	} else { kind = self->noSym; break; }
    case 8: case_8:
	if ((self->ch >= '0' && self->ch <= '9') ||
	    (self->ch >= 'a' && self->ch <= 'f')) {
	    CcsXmlScanner_GetCh(self); goto case_8;
	} else if (self->ch == '\'') {
	    CcsXmlScanner_GetCh(self); goto case_9;
	} else { kind = self->noSym; break; }
    case 9: case_9:
	{ kind = 5; break; }
    case 10: case_10:
	if ((self->ch >= 0 && self->ch <= '\t') ||
	    (self->ch >= '\v' && self->ch <= '\f') ||
	    (self->ch >= 14 && self->ch <= '!') ||
	    (self->ch >= '#' && self->ch <= '[') ||
	    (self->ch >= ']' && self->ch <= 65535)) {
	    CcsXmlScanner_GetCh(self); goto case_10;
	} else if (self->ch == '"') {
	    CcsXmlScanner_GetCh(self); goto case_3;
	} else if (self->ch == '\\') {
	    CcsXmlScanner_GetCh(self); goto case_11;
	} else if (self->ch == '\n' ||
	    self->ch == '\r') {
	    CcsXmlScanner_GetCh(self); goto case_4;
	} else { kind = self->noSym; break; }
    case 11: case_11:
	if ((self->ch >= ' ' && self->ch <= '~')) {
	    CcsXmlScanner_GetCh(self); goto case_10;
	} else { kind = self->noSym; break; }
    case 12:
	{ kind = 11; break; }
    case 13:
	{ kind = 23; break; }
    case 14: case_14:
	{ kind = 24; break; }
    case 15: case_15:
	{ kind = 25; break; }
    case 16:
	{ kind = 26; break; }
    case 17:
	{ kind = 29; break; }
    case 18:
	{ kind = 30; break; }
    case 19:
	{ kind = 31; break; }
    case 20:
	{ kind = 32; break; }
    case 21:
	{ kind = 33; break; }
    case 22: case_22:
	{ kind = 37; break; }
    case 23: case_23:
	{ kind = 38; break; }
    case 24:
	if (self->ch == '>') {
	    CcsXmlScanner_GetCh(self); goto case_15;
	} else if (self->ch == ')') {
	    CcsXmlScanner_GetCh(self); goto case_23;
	} else { kind = 12; break; }
    case 25:
	if (self->ch == '.') {
	    CcsXmlScanner_GetCh(self); goto case_14;
	} else { kind = 22; break; }
    case 26:
	if (self->ch == '.') {
	    CcsXmlScanner_GetCh(self); goto case_22;
	} else { kind = 28; break; }
    /*---- enable ----*/
    }
    t = CcsToken(kind, pos, col, line,
		 CcsBuffer_GetString(&self->buffer, pos, self->pos - pos),
		 self->pos - pos);
    CcsBuffer_Unlock(&self->buffer);
    return t;
}
