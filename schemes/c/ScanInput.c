/*-------------------------------------------------------------------------
  Copyright (C) 2008, Charles Wang
  Author: Charles Wang <charlesw123456@gmail.com>
  License: LGPLv2 (see LICENSE-LGPL)
-------------------------------------------------------------------------*/
#include  <limits.h>
#include  "c/ScanInput.h"
#include  "c/IncPathList.h"

static CcsToken_t * CcsScanInput_NextToken(CcsScanInput_t * self);
static CcsBool_t
CcsScanInput_Init(CcsScanInput_t * self, void * scanner,
		  const CcsSI_Info_t * info, FILE * fp)
{
    self->next = NULL;
    self->scanner = scanner;
    self->info = info;
    self->fp = fp;
    if (!CcsBuffer(&self->buffer, fp)) goto errquit0;
    self->busyFirst = self->busyLast = NULL;
    self->readyFirst = self->readyLast = NULL;

    self->ch = 0; self->chBytes = 0;
    self->pos = 0; self->line = 1; self->col = 0;
    self->inComment = FALSE;
    self->numCommentEols = 0;
    self->chAfterComment = NoChr;
    self->chLastNonblank = NoChr;
    if (info->additionalInit && !info->additionalInit(self + 1, scanner))
	goto errquit1;
    return TRUE;
 errquit1:
    CcsBuffer_Destruct(&self->buffer);
 errquit0:
    return FALSE;
}

CcsScanInput_t *
CcsScanInput(void * scanner, const CcsSI_Info_t * info, FILE * fp)
{
    CcsScanInput_t * self;
    if (!(self = CcsMalloc(sizeof(CcsScanInput_t) + info->additionalSpace)))
	goto errquit0;
    self->fname = NULL;
    if (!CcsScanInput_Init(self, scanner, info, fp)) goto errquit1;
    return self;
 errquit1:
    CcsFree(self);
 errquit0:
    return NULL;
}

CcsScanInput_t *
CcsScanInput_ByName(void * scanner, const CcsSI_Info_t * info,
		    const CcsIncPathList_t * list, const char * includer,
		    const char * infn)
{
    FILE * fp;
    CcsScanInput_t * self;
    char infnpath[PATH_MAX];
    if (!(fp = CcsIncPathList_Open(list, infnpath, sizeof(infnpath),
				   includer, infn)))
	goto errquit0;
    if (!(self = CcsMalloc(sizeof(CcsScanInput_t) + info->additionalSpace +
			   strlen(infnpath) + 1)))
	goto errquit1;
    strcpy(self->fname = (char *)(self + 1) + info->additionalSpace, infnpath);
    if (!CcsScanInput_Init(self, scanner, info, fp)) goto errquit2;
    return self;
 errquit2:
    CcsFree(self);
 errquit1:
    fclose(fp);
 errquit0:
    return NULL;
}

static void
CcsScanInput_Destruct(CcsScanInput_t * self)
{
    CcsToken_t * cur, * next;

    CcsAssert(self->scanner == NULL);
    /* May be trigged by .atg semantic code. */
    CcsAssert(self->busyFirst == NULL && self->busyLast == NULL);
    if (self->info->additionalDestruct)
	self->info->additionalDestruct(self + 1);
    for (cur = self->readyFirst; cur; cur = next) {
	CcsAssert(cur->refcnt == 1);
	next = cur->next;
	CcsToken_Destruct(cur);
    }
    CcsBuffer_Destruct(&self->buffer);
    if (self->fname) fclose(self->fp);
    CcsFree(self);
}

void
CcsScanInput_GetCh(CcsScanInput_t * self)
{
    if (self->inComment == FALSE) {
	if (self->numCommentEols > 0) {
	    self->ch = '\n';
	    --self->numCommentEols;
	    return;
	} else if (self->chAfterComment != NoChr) {
	    self->ch = self->chAfterComment;
	    self->chAfterComment = NoChr;
	    if (self->ch != ' ' && self->ch != '\t' &&
		self->ch != '\r' && self->ch != '\n')
		self->chLastNonblank = self->ch;
	    return;
	}
    }
    if (self->ch == '\n') {
	if (self->inComment) ++self->numCommentEols;
	++self->line; self->col = 0;
    } else if (self->ch == '\t') {
	self->col += 8 - self->col % 8;
    } else {
	/* FIX ME: May be the width of some specical character
	 * is NOT self->chBytes. */
	self->col += self->chBytes;
    }
    self->ch = CcsBuffer_Read(&self->buffer, &self->chBytes);
    self->pos = CcsBuffer_GetPos(&self->buffer);
    if (self->ch != ' ' && self->ch != '\t' &&
	self->ch != '\r' && self->ch != '\n')
	self->chLastNonblank = self->ch;
}

CcsToken_t *
CcsScanInput_NewToken(CcsScanInput_t * self, int kind)
{
    return CcsToken(self, kind, self->fname, self->pos,
		    self->line, self->col, NULL, 0);
}

void
CcsScanInput_Detach(CcsScanInput_t * self)
{
    self->scanner = NULL;
    if (self->busyFirst == NULL) CcsScanInput_Destruct(self);
}

static void
CcsScanInput_AdjustBusy(CcsScanInput_t * self)
{
    if (self->busyFirst) {
	CcsAssert(self->busyFirst->refcnt > 1);
	CcsBuffer_SetBusy(&self->buffer, self->busyFirst->pos);
    } else if (self->readyFirst) {
	CcsAssert(self->readyFirst->refcnt == 1);
	CcsBuffer_SetBusy(&self->buffer, self->readyFirst->pos);
    } else {
	CcsBuffer_ClearBusy(&self->buffer);
	if (!self->scanner) CcsScanInput_Destruct(self);
    }
}

CcsToken_t *
CcsScanInput_Scan(CcsScanInput_t * self)
{
    CcsToken_t * cur;
    /* Get a token from ready list or generate a new one. */
    if (self->readyFirst == NULL) {
	if (!(cur = CcsScanInput_NextToken(self))) return NULL;
	if (cur->next) {
	    self->readyFirst = self->readyPeek = self->readyLast = cur->next;
	    while (self->readyLast->next != NULL)
		self->readyLast = self->readyLast->next;
	}
    } else {
	cur = self->readyFirst;
	self->readyFirst = cur->next;
	if (self->readyPeek == cur) self->readyPeek = self->readyFirst;
	if (self->readyLast == cur) self->readyLast = NULL;
	cur->next = NULL;
    }
    /* Append to the busy list. */
    if (self->busyLast) {
	self->busyLast->next = cur;
	self->busyLast = cur;
    } else {
	self->busyFirst = self->busyLast = cur;
	CcsBuffer_SetBusy(&self->buffer, cur->pos);
    }
    /* Adjust refcnt */
    CcsAssert(cur->refcnt == 1);
    ++cur->refcnt; /* 2 */
    return cur;
}

void
CcsScanInput_WithDraw(CcsScanInput_t * self, CcsToken_t * token)
{
    CcsToken_t * prev, * cur;
    /* Check the validation of token, might be caused by .atg/.xatg. */
    CcsAssert(token == self->busyLast);
    CcsAssert(token->refcnt == 2);
    /* Detach the last token in busy list. */
    prev = NULL;
    for (cur = self->busyFirst; cur != token; cur = cur->next)
	prev = cur;
    self->busyLast = prev;
    if (self->busyLast == NULL) self->busyFirst = NULL;
    else self->busyLast->next = NULL;
    /* Attach to the beginning of readyList. */
    if (self->readyPeek == self->readyFirst) self->readyPeek = token;
    if (self->readyLast == NULL) self->readyLast = token;
    token->next = self->readyFirst;
    self->readyFirst = token;
    /* Adjust refcnt */
    --token->refcnt; /* 1 */
}

CcsBool_t
CcsScanInput_Prepend(CcsScanInput_t * self, int kind,
		     const char * val, size_t vallen)
{
    CcsToken_t * cur;
    int pos, line, col;
    if (self->readyFirst) {
	pos = self->readyFirst->pos;
	line = self->readyFirst->loc.line;
	col = self->readyFirst->loc.col;
    } else {
	pos = self->pos;
	line = self->line;
	col = self->col;
    }
    if (!(cur = CcsToken(self, kind, self->fname,
			 pos, line, col, val, vallen)))
	return FALSE;
    cur->next = self->readyFirst;
    self->readyFirst = cur;
    if (self->readyPeek == NULL) self->readyPeek = cur;
    if (self->readyLast == NULL) self->readyLast = cur;
    return TRUE;
}

void
CcsScanInput_TokenIncRef(CcsScanInput_t * self, CcsToken_t * token)
{
    ++token->refcnt;
}

void
CcsScanInput_TokenDecRef(CcsScanInput_t * self, CcsToken_t * token)
{
    CcsToken_t * prev, * cur;
    if (--token->refcnt > 1) return;
    CcsAssert(token->refcnt == 1);
    prev = NULL;
    for (cur = self->busyFirst; cur != token; cur = cur->next) {
	/* token must in busy list, might be caused by .atg/.xatg. */
	CcsAssert(cur);
	prev = cur;
    }
    if (token != self->busyFirst) return;
    /* Clear the useless tokens in the head of busy list. */
    while (token && token->refcnt <= 1) {
	CcsAssert(token->refcnt == 1);
	self->busyFirst = token->next;
	if (self->busyFirst == NULL) self->busyLast = NULL;
	CcsToken_Destruct(token);
	token = self->busyFirst;
    }
    /* Adjust CcsBuffer busy pointer */
    CcsScanInput_AdjustBusy(self);
}

long
CcsScanInput_StringTo(CcsScanInput_t * self, size_t * len, const char * needle)
{
    return CcsBuffer_StringTo(&self->buffer, len, needle);
}
const char *
CcsScanInput_GetString(CcsScanInput_t * self, long start, size_t len)
{
    return CcsBuffer_GetString(&self->buffer, start, len);
}
void
CcsScanInput_Consume(CcsScanInput_t * self, long start, size_t len)
{
    const char * cur, * last;
    cur = CcsBuffer_GetString(&self->buffer, start, len);
    last = cur + len;
    while (cur < last) {
	if (*cur == '\t') {
	    self->col += 8 - self->col % 8;
	} else if (*cur == '\n') {
	    ++self->line; self->col = 0;
	} else {
	    ++self->col;
	}
	++cur;
    }
    CcsBuffer_Consume(&self->buffer, start, len);
    self->ch = CcsBuffer_Read(&self->buffer, &self->chBytes);
    self->pos = CcsBuffer_GetPos(&self->buffer);
    if (self->ch != ' ' && self->ch != '\t' &&
	self->ch != '\r' && self->ch != '\n')
	self->chLastNonblank = self->ch;
}

CcsPosition_t *
CcsScanInput_GetPosition(CcsScanInput_t * self, const CcsToken_t * begin,
			 const CcsToken_t * end)
{
    int len;
    CcsAssert(self == begin->input);
    CcsAssert(self == end->input);
    len = end->pos - begin->pos;
    return CcsPosition(begin->pos, len, begin->loc.col,
		       CcsBuffer_GetString(&self->buffer, begin->pos, len));
}

CcsPosition_t *
CcsScanInput_GetPositionBetween(CcsScanInput_t * self,
				const CcsToken_t * begin,
				const CcsToken_t * end)
{
    int begpos, len;
    CcsAssert(self == begin->input);
    CcsAssert(self == end->input);
    begpos = begin->pos + strlen(begin->val);
    len = end->pos - begpos;
    const char * start = CcsBuffer_GetString(&self->buffer, begpos, len);
    const char * cur, * last = start + len;

    /* Skip the leading spaces. */
    for (cur = start; cur < last; ++cur)
	if (*cur != ' ' && *cur != '\t' && *cur != '\r' && *cur != '\n') break;
    return CcsPosition(begpos + (cur - start), last - cur, 0, cur);
}

CcsToken_t *
CcsScanInput_Peek(CcsScanInput_t * self)
{
    CcsToken_t * cur;
    if (self->readyPeek) {
	cur = self->readyPeek; self->readyPeek = cur->next;
	return cur;
    }
    if (!(cur = CcsScanInput_NextToken(self))) return NULL;
    if (self->readyLast) {
	self->readyLast->next = cur;
	self->readyLast = cur;
    } else {
	self->readyFirst = self->readyLast = cur;
	if (self->busyFirst == NULL)
	    CcsBuffer_SetBusy(&self->buffer, self->readyFirst->pos);
    }
    return cur;
}

void
CcsScanInput_ResetPeek(CcsScanInput_t * self)
{
    self->readyPeek = self->readyFirst;
}

static CcsToken_t *
CcsScanInput_NextToken(CcsScanInput_t * self)
{
    int pos, line, col, kind; CcsToken_t * t;

    if ((t = self->info->skip(self->scanner, self))) return t;
    pos = self->pos; line = self->line; col = self->col;
    CcsBuffer_Lock(&self->buffer);
    kind = self->info->kind(self->scanner, self);
    t = CcsToken(self, kind, self->fname, pos, line, col,
		 CcsBuffer_GetString(&self->buffer, pos, self->pos - pos),
		 self->pos - pos);
    CcsBuffer_Unlock(&self->buffer);
    return t;
}

typedef struct {
    int ch, chBytes;
    int pos, line, col;
}  SLock_t;
static void
CcsScanInput_LockCh(CcsScanInput_t * self, SLock_t * slock)
{
    slock->ch = self->ch;
    slock->chBytes = self->chBytes;
    slock->pos = self->pos;
    slock->line = self->line;
    slock->col = self->col;
    CcsBuffer_Lock(&self->buffer);
}
static void
CcsScanInput_UnlockCh(CcsScanInput_t * self, SLock_t * slock)
{
    CcsBuffer_Unlock(&self->buffer);
}
static void
CcsScanInput_ResetCh(CcsScanInput_t * self, SLock_t * slock)
{
    self->ch = slock->ch;
    self->chBytes = slock->chBytes;
    self->pos = slock->pos;
    self->line = slock->line;
    CcsBuffer_LockReset(&self->buffer);
}

CcsBool_t
CcsScanInput_Comment(CcsScanInput_t * self, const CcsComment_t * c)
{
    SLock_t slock;
    int level = 1;

    CcsAssert(self->inComment == FALSE &&
	      self->numCommentEols == 0 &&
	      self->chAfterComment == NoChr);
    self->inComment = TRUE;
    if (c->start[1]) {
	CcsScanInput_LockCh(self, &slock); CcsScanInput_GetCh(self);
	if (self->ch != c->start[1]) {
	    CcsScanInput_ResetCh(self, &slock);
	    self->inComment = FALSE;
	    self->numCommentEols = 0;
	    self->chAfterComment = NoChr;
	    return FALSE;
	}
	CcsScanInput_UnlockCh(self, &slock);
    }
    CcsScanInput_GetCh(self);
    for (;;) {
	if (self->ch == c->end[0]) {
	    if (c->end[1] == 0) {
		if (--level == 0) {
		    CcsScanInput_GetCh(self);
		    break;
		}
	    } else {
		CcsScanInput_LockCh(self, &slock); CcsScanInput_GetCh(self);
		if (self->ch == c->end[1]) {
		    CcsScanInput_UnlockCh(self, &slock);
		    if (--level == 0) {
			CcsScanInput_GetCh(self);
			break;
		    }
		} else {
		    CcsScanInput_ResetCh(self, &slock);
		}
	    }
	} else if (c->nested && self->ch == c->start[0]) {
	    if (c->start[1] == 0) {
		++level;
	    } else {
		CcsScanInput_LockCh(self, &slock); CcsScanInput_GetCh(self);
		if (self->ch == c->start[1]) {
		    CcsScanInput_UnlockCh(self, &slock);
		    ++level;
		} else {
		    CcsScanInput_ResetCh(self, &slock);
		}
	    }
	} else if (self->ch == EoF) {
	    break;
	}
	CcsScanInput_GetCh(self);
    }
    CcsAssert(self->chAfterComment == NoChr);
    self->chAfterComment = self->ch;
    self->inComment = FALSE;
    CcsScanInput_GetCh(self);
    return TRUE;
}
