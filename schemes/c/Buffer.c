/*-------------------------------------------------------------------------
  Copyright (C) 2008, Charles Wang
  Author: Charles Wang <charlesw123456@gmail.com>
  License: LGPLv2 (see LICENSE-LGPL)
-------------------------------------------------------------------------*/
#include  "Buffer.h"

/* CcsBuffer_t private members. */
/*#define BUFSTEP  8*/
#define BUFSTEP 4096
/* Return 0 for success, -1 for error. */
static int CcsBuffer_Load(CcsBuffer_t * self);

/* Return 0 for success, EoF for end of file, ErrorChr for error. */
static int CcsBuffer_LoadMore(CcsBuffer_t * self);

/* Return 0 for sucess, -1 for error.
 * The returned character is saved in *value */
static int CcsBuffer_ReadByte(CcsBuffer_t * self, int * value);

CcsBuffer_t *
CcsBuffer(CcsBuffer_t * self, FILE * fp)
{
    self->fp = fp;
    self->start = 0;
    if (!(self->buf = CcsMalloc(BUFSTEP))) goto errquit0;
    self->busyFirst = self->lockCur = NULL;
    self->cur = NULL;
    self->next = self->loaded = self->buf;
    self->last = self->buf + BUFSTEP;
    if (CcsBuffer_Load(self) < 0) goto errquit1;
    return self;
 errquit1:
    CcsFree(self->buf);
 errquit0:
    return NULL;
}

void
CcsBuffer_Destruct(CcsBuffer_t * self)
{
    CcsFree(self->buf);
}

long
CcsBuffer_GetPos(CcsBuffer_t * self)
{
    return self->cur ? self->start + (self->cur - self->buf) : 0L;
}

int
CcsBuffer_Read(CcsBuffer_t * self, int * retBytes)
{
    int ch, c1, c2, c3, c4;
    /* self->start might be changed in CcsBuffer_ReadByte */
    long next = self->start + (self->next - self->buf);

    self->cur = self->next;

    if (CcsBuffer_ReadByte(self, &ch) < 0) goto quit;

    if (ch < 128) goto quit;

    if ((ch & 0xC0) != 0xC0) /* Inside UTF-8 character! */
	return ErrorChr;
    if ((ch & 0xF0) == 0xF0) {
	/* 1110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
	c1 = ch & 0x07;
	if (CcsBuffer_ReadByte(self, &ch) < 0) goto quit;
	c2 = ch & 0x3F;
	if (CcsBuffer_ReadByte(self, &ch) < 0) goto quit;
	c3 = ch & 0x3F;
	if (CcsBuffer_ReadByte(self, &ch) < 0) goto quit;
	c4 = ch & 0x3F;
	ch = (((((c1 << 6) | c2) << 6) | c3) << 6) | c4;
    } else if ((ch & 0xE0) == 0xE0) {
	/* 1110xxxx 10xxxxxx 10xxxxxx */
	c1 = ch & 0x0F;
	if (CcsBuffer_ReadByte(self, &ch) < 0) goto quit;
	c2 = ch & 0x3F;
	if (CcsBuffer_ReadByte(self, &ch) < 0) goto quit;
	c3 = ch & 0x3F;
	ch = (((c1 << 6) | c2) << 6) | c3;
    } else {
	/* (ch & 0xC0) == 0xC0 */
	/* 110xxxxx 10xxxxxx */
	c1 = ch & 0x1F;
	if (CcsBuffer_ReadByte(self, &ch) < 0) goto quit;
	c2 = ch & 0x3F;
	ch = (c1 << 6) | c2;
    }
 quit:
    *retBytes = self->start + (self->next - self->buf) - next;
    return ch;
}

long
CcsBuffer_StringTo(CcsBuffer_t * self, size_t * len, const char * needle)
{
    int loadret;
    char * cur; long curpos;
    size_t nlen = strlen(needle);

    for (cur = self->cur; (cur + nlen) - self->cur <= *len; ++cur) {
	while ((cur + nlen) > self->loaded) {
	    /* There is not enough data in the buffer. */
	    curpos = self->start + (cur - self->buf);
	    loadret = CcsBuffer_LoadMore(self);
	    cur = self->buf + (curpos - self->start);
	    if (loadret == ErrorChr) return -1;
	    if (loadret == EoF) {
		cur = self->loaded; goto done;
	    }
	}
	if (*cur == *needle && !memcmp(cur + 1, needle + 1, nlen - 1)) {
	    /* needle found. */
	    cur += nlen; break;
	}
    }
 done:
    CcsAssert(cur - self->cur <= *len);
    *len = cur - self->cur;
    return self->start + (self->cur - self->buf);
}
const char *
CcsBuffer_GetString(CcsBuffer_t * self, long start, size_t size)
{
    if (size == 0) return NULL;
    if (start < self->start) goto errquit0;
    if (start < self->start + (self->cur - self->buf)) {
	if (start + size > self->start + (self->cur - self->buf))
	    goto errquit1;
    } else if (start + size > self->start + (self->loaded - self->buf)) {
	goto errquit1;
    }
    return self->buf + (start - self->start);
 errquit0:
    fprintf(stderr, "start is out of range.\n");
    exit(-1);
 errquit1:
    fprintf(stderr, "start + size is out of range.\n");
    exit(-1);
}
void
CcsBuffer_Consume(CcsBuffer_t * self, long start, size_t size)
{
    if (start != self->start + (self->cur - self->buf)) {
	fprintf(stderr, "Consume must started from the current characters.\n");
	exit(-1);
    }
    if (self->cur + size > self->loaded) {
	fprintf(stderr, "Consume too many characters.\n");
	exit(-1);
    }
    self->cur += size;
    self->next = self->cur;
}

void
CcsBuffer_SetBusy(CcsBuffer_t * self, long startBusy)
{
    CcsAssert(startBusy >= self->start);
    self->busyFirst = self->buf + (startBusy - self->start);
    CcsAssert(self->busyFirst <= self->cur);
}

void
CcsBuffer_ClearBusy(CcsBuffer_t * self)
{
    self->busyFirst = NULL;
}

void
CcsBuffer_Lock(CcsBuffer_t * self)
{
    CcsAssert(self->lockCur == NULL);
    self->lockCur = self->cur;
    self->lockNext = self->next;
}

void
CcsBuffer_LockReset(CcsBuffer_t * self)
{
    CcsAssert(self->lockCur != NULL);
    self->cur = self->lockCur;
    self->next = self->lockNext;
    self->lockCur = NULL;
}

void
CcsBuffer_Unlock(CcsBuffer_t * self)
{
    CcsAssert(self->lockCur != NULL);
    self->lockCur = NULL;
}

static int
CcsBuffer_Load(CcsBuffer_t * self)
{
    size_t rc = fread(self->loaded, 1, self->last - self->loaded, self->fp);
    if (rc > 0) self->loaded += rc;
    else if (ferror(self->fp)) return -1;
    return 0;
}

static int
CcsBuffer_LoadMore(CcsBuffer_t * self)
{
    int delta; char * keptFirst, * newbuf;
    /* Calculate keptFirst */
    keptFirst = self->cur;
    if (self->busyFirst && self->busyFirst < keptFirst)
	keptFirst = self->busyFirst;
    if (self->lockCur && self->lockCur < keptFirst)
	keptFirst = self->lockCur;
    if (self->buf < keptFirst) { /* Remove the unprotected data. */
	delta = keptFirst - self->buf;
	memmove(self->buf, keptFirst, self->loaded - keptFirst);
	self->start += delta;
	if (self->busyFirst) self->busyFirst -= delta;
	if (self->lockCur) {
	    self->lockCur -= delta; self->lockNext -= delta;
	}
	self->cur -= delta;
	self->next -= delta;
	self->loaded -= delta;
    }
    if (feof(self->fp)) return EoF;
    /* Try to extend the storage space */
    while (self->loaded >= self->last) {
	if (!(newbuf =
	      CcsRealloc(self->buf, self->last - self->buf + BUFSTEP)))
	    return ErrorChr;
	if (self->busyFirst)
	    self->busyFirst = newbuf + (self->busyFirst - self->buf);
	if (self->lockCur) {
	    self->lockCur = newbuf + (self->lockCur - self->buf);
	    self->lockNext = newbuf + (self->lockNext - self->buf);
	}
	self->cur = newbuf + (self->cur - self->buf);
	self->next = newbuf + (self->next - self->buf);
	self->loaded = newbuf + (self->loaded - self->buf);
	self->last = newbuf + (self->last - self->buf + BUFSTEP);
	self->buf = newbuf;
    }
    if (CcsBuffer_Load(self) < 0) return ErrorChr;
    return 0;
}

static int
CcsBuffer_ReadByte(CcsBuffer_t * self, int * value)
{
    while (self->next >= self->loaded)
	if ((*value = CcsBuffer_LoadMore(self))) return -1;
    *value = *self->next++;
    return 0;
}
