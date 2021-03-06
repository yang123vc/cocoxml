/*-------------------------------------------------------------------------
  Copyright (C) 2008, Charles Wang
  Author: Charles Wang <charlesw123456@gmail.com>
  License: LGPLv2 (see LICENSE-LGPL)
-------------------------------------------------------------------------*/
using System;
using System.Diagnostics;
using System.IO;

public class CcsBuffer_t {
    public const int EoF = -1;
    public const int ErrorChr = -2;
    public const int BUFSTEP = 4096;
    Stream stream;
    bool eof;
    long start;
    byte[] buf;
    int busyFirst;
    int lockCur;
    int lockNext;
    int cur;
    int next;
    int loaded;
    int last;

    public CcsBuffer_t(Stream s)
    {
	stream = s;
	eof = false;
	start = 0;
	buf = new byte[BUFSTEP];
	busyFirst = lockCur = -1;
	cur = -1;
	next = loaded = 0;
	last = BUFSTEP;
	Load();
    }

    ~CcsBuffer_t()
    {
	stream.Close();
    }

    public long GetPos()
    {
	return cur >= 0 ? start + cur : 0L;
    }

    public int Read(out int retBytes)
    {
	int ch, c1, c2, c3, c4;
	/* start might be changed in ReadByte */
	long localnext = start + next;

	cur = next;

	ch = ReadByte();

	if (ch < 128) goto quit;

	if ((ch & 0xC0) != 0xC0) { /* Inside UTF-8 character! */
	    ch = ErrorChr;
	    goto quit;
	}
	if ((ch & 0xF0) == 0xF0) {
	    /* 1110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
	    c1 = ch & 0x07;
	    ch = ReadByte();
	    c2 = ch & 0x3F;
	    ch = ReadByte();
	    c3 = ch & 0x3F;
	    ch = ReadByte();
	    c4 = ch & 0x3F;
	    ch = (((((c1 << 6) | c2) << 6) | c3) << 6) | c4;
	} else if ((ch & 0xE0) == 0xE0) {
	    /* 1110xxxx 10xxxxxx 10xxxxxx */
	    c1 = ch & 0x0F;
	    ch = ReadByte();
	    c2 = ch & 0x3F;
	    ch = ReadByte();
	    c3 = ch & 0x3F;
	    ch = (((c1 << 6) | c2) << 6) | c3;
	} else {
	    /* (ch & 0xC0) == 0xC0 */
	    /* 110xxxxx 10xxxxxx */
	    c1 = ch & 0x1F;
	    ch = ReadByte();
	    c2 = ch & 0x3F;
	    ch = (c1 << 6) | c2;
	}
	quit:
	retBytes = (int)((start + next) - localnext);
	return ch;
    }

    public string GetString(long start, int size)
    {
	char[] charbuf = null;
	if (size == 0) return null;
	Debug.Assert(start >= this.start && start < this.start + cur);
	Debug.Assert(start + size <= loaded);
	charbuf = new char[size];
	int bufstart = (int)(start - this.start);
	for (int idx = 0; idx < size; ++idx) charbuf[idx] = (char)buf[bufstart + idx];
	return new String(charbuf);
    }

    public void SetBusy(long startBusy)
    {
	Debug.Assert(startBusy >= start);
	busyFirst = (int)(startBusy - start);
	Debug.Assert(busyFirst <= cur);
    }

    public void ClearBusy()
    {
	busyFirst = -1;
    }

    public void Lock()
    {
	Debug.Assert(lockCur >= 0);
	lockCur = cur;
	lockNext = next;
    }

    public void LockReset()
    {
	Debug.Assert(lockCur >= 0);
	cur = lockCur;
	next = lockNext;
	lockCur = -1;
    }

    public void Unlock()
    {
	Debug.Assert(lockCur >= 0);
	lockCur = -1;
    }

    private void Load()
    {
	int rc = stream.Read(buf, loaded, last - loaded);
	eof = (rc == 0);
	loaded += rc;
    }

    private int ReadByte()
    {
	int delta; int keptFirst;
	while (next >= loaded) {
	    /* Calculate keptFirst */
	    keptFirst = cur;
	    if (busyFirst >= 0 && busyFirst < keptFirst)
		keptFirst = busyFirst;
	    if (lockCur >= 0 && lockCur < keptFirst)
		keptFirst = lockCur;
	    if (keptFirst > 0) { /* Remove the unprotected data. */
		delta = keptFirst;
		Array.Copy(buf, keptFirst, buf, 0, loaded - keptFirst);
		start += delta;
		if (busyFirst >= 0) busyFirst -= delta;
		if (lockCur >= 0) {
		    lockCur -= delta; lockNext -= delta;
		}
		cur -= delta;
		next -= delta;
		loaded -= delta;
	    }
	    if (eof) return EoF;
	    /* Try to extend the storage space */
	    while (loaded >= last) {
		last += BUFSTEP;
		Array.Resize(ref buf, last);
	    }
	    Load();
	}
	return buf[next++];
    }
};
