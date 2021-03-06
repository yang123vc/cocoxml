/*---- license ----*/
/*-------------------------------------------------------------------------
  c-expr.atg -- atg for c expression input
  Copyright (C) 2008, Charles Wang <charlesw123456@gmail.com>
  Author: Charles Wang <charlesw123456@gmail.com>
  License: BSD (see LICENSE-BSD)
-------------------------------------------------------------------------*/
/*---- enable ----*/
#include  "Parser.h"
#include  "c/Token.h"
#if defined(CExprParser_USE_GetSS)
#include  "c/ScanInput.h"
#endif

/*---- cIncludes ----*/
/*---- enable ----*/

static void CExprParser_SynErr(CExprParser_t * self, int n);
#ifdef CExprParser_USE_StartOf
static const char * set[];
#endif /* CExprParser_USE_StartOf */

#if defined(CExprParser_USE_GetSS) || defined(CExprParser_USE_ExpectSS)
typedef CcsToken_t *
(* SubScanner_t)(CExprParser_t * self, const char * fname,
		 int pos, int line, int col);

static void
CExprParser_TokenIncRef(CExprParser_t * self, CcsToken_t * token)
{
    if (token->destructor) ++token->refcnt;
    else CExprScanner_TokenIncRef(&self->scanner, token);
}

static void
CExprParser_TokenDecRef(CExprParser_t * self, CcsToken_t * token)
{
    if (token->destructor) token->destructor(token);
    else CExprScanner_TokenDecRef(&self->scanner, token);
}
#else

#define CExprParser_TokenIncRef(self, token) \
    CExprScanner_TokenIncRef(&((self)->scanner), token)
#define CExprParser_TokenDecRef(self, token) \
    CExprScanner_TokenDecRef(&((self)->scanner), token)

#endif /* CExprParser_USE_GetSS || CExprParser_USE_ExpectSS */

static void
CExprParser_Get(CExprParser_t * self)
{
    if (self->t) CExprParser_TokenDecRef(self, self->t);
    self->t = self->la;
    for (;;) {
	self->la = CExprScanner_Scan(&self->scanner);
	if (self->la->kind <= self->maxT) { /*++self->errDist;*/ break; }
	/* May be implement pragmas here is wrong... But I still not found any
	 * needs to use pragmas, so just leave it along. */
	/*---- Pragmas ----*/
	/*---- enable ----*/
    }
}

#ifdef CExprParser_USE_StartOf
static CcsBool_t
CExprParser_StartOf(CExprParser_t * self, int s)
{
    return set[s][self->la->kind] == '*';
}
#endif

static void
CExprParser_Expect(CExprParser_t * self, int n)
{
    if (self->la->kind == n) CExprParser_Get(self);
    else CExprParser_SynErr(self, n);
}

#ifdef CExprParser_USE_GetSS
static void
CExprParser_GetSS(CExprParser_t * self, SubScanner_t subscanner)
{
    if (self->t) CExprParser_TokenDecRef(self, self->t);
    self->t = self->la;
    self->la = subscanner(self, self->scanner.cur->fname,
			  self->scanner.cur->pos,
			  self->scanner.cur->line,
			  self->scanner.cur->col);
}
#endif

#ifdef CExprParser_USE_ExpectSS
static void
CExprParser_ExpectSS(CExprParser_t * self, int n, SubScanner_t subscanner)
{
    if (self->la->kind == n) CExprParser_GetSS(self, subscanner);
    else CExprParser_SynErr(self, n);
}
#endif

#ifdef CExprParser_USE_ExpectWeak
static void
CExprParser_ExpectWeak(CExprParser_t * self, int n, int follow)
{
    if (self->la->kind == n) CExprParser_Get(self);
    else {
	CExprParser_SynErr(self, n);
	while (!CExprParser_StartOf(self, follow)) CExprParser_Get(self);
    }
}
#endif

#ifdef CExprParser_USE_WeakSeparator
static CcsBool_t
CExprParser_WeakSeparator(CExprParser_t * self, int n, int syFol, int repFol)
{
    if (self->la->kind == n) { CExprParser_Get(self); return TRUE; }
    else if (CExprParser_StartOf(self, repFol)) { return FALSE; }
    CExprParser_SynErr(self, n);
    while (!(CExprParser_StartOf(self, syFol) ||
	     CExprParser_StartOf(self, repFol) ||
	     CExprParser_StartOf(self, 0)))
	CExprParser_Get(self);
    return CExprParser_StartOf(self, syFol);
}
#endif /* CExprParser_USE_WeakSeparator */

/*---- ProductionsHeader ----*/
static void CExprParser_CExpr(CExprParser_t * self);
static void CExprParser_expr1(CExprParser_t * self, int * value);
static void CExprParser_expr2(CExprParser_t * self, int * value);
static void CExprParser_expr3(CExprParser_t * self, int * value);
static void CExprParser_expr4(CExprParser_t * self, int * value);
static void CExprParser_expr5(CExprParser_t * self, int * value);
static void CExprParser_expr6(CExprParser_t * self, int * value);
static void CExprParser_expr7(CExprParser_t * self, int * value);
static void CExprParser_expr8(CExprParser_t * self, int * value);
static void CExprParser_expr9(CExprParser_t * self, int * value);
static void CExprParser_expr10(CExprParser_t * self, int * value);
static void CExprParser_expr11(CExprParser_t * self, int * value);
static void CExprParser_expr12(CExprParser_t * self, int * value);
/*---- enable ----*/

void
CExprParser_Parse(CExprParser_t * self)
{
    self->t = NULL;
    self->la = CExprScanner_GetDummy(&self->scanner);
    CExprParser_Get(self);
    /*---- ParseRoot ----*/
    CExprParser_CExpr(self);
    /*---- enable ----*/
    CExprParser_Expect(self, 0);
}

void
CExprParser_SemErr(CExprParser_t * self, const CcsToken_t * token,
		 const char * format, ...)
{
    va_list ap;
    va_start(ap, format);
    CcsErrorPool_VError(&self->errpool, &token->loc, format, ap);
    va_end(ap);
}

void
CExprParser_SemErrT(CExprParser_t * self, const char * format, ...)
{
    va_list ap;
    va_start(ap, format);
    CcsErrorPool_VError(&self->errpool, &self->t->loc, format, ap);
    va_end(ap);
}

static CcsBool_t
CExprParser_Init(CExprParser_t * self)
{
    self->t = self->la = NULL;
    /*---- constructor ----*/
    self->maxT = 24;
    self->value = 0;
    /*---- enable ----*/
    return TRUE;
}

CExprParser_t *
CExprParser(CExprParser_t * self, FILE  * infp, FILE * errfp)
{
    if (!CcsErrorPool(&self->errpool, errfp)) goto errquit0;
    if (!CExprScanner(&self->scanner, &self->errpool, infp)) goto errquit1;
    if (!CExprParser_Init(self)) goto errquit2;
    return self;
 errquit2:
    CExprScanner_Destruct(&self->scanner);
 errquit1:
    CcsErrorPool_Destruct(&self->errpool);
 errquit0:
    return NULL;
}

CExprParser_t *
CExprParser_ByName(CExprParser_t * self, const char * infn, FILE * errfp)
{
    if (!CcsErrorPool(&self->errpool, errfp)) goto errquit0;
    if (!CExprScanner_ByName(&self->scanner, &self->errpool, infn))
	goto errquit1;
    if (!CExprParser_Init(self)) goto errquit2;
    return self;
 errquit2:
    CExprScanner_Destruct(&self->scanner);
 errquit1:
    CcsErrorPool_Destruct(&self->errpool);
 errquit0:
    return NULL;
}

void
CExprParser_Destruct(CExprParser_t * self)
{
    /*---- destructor ----*/
    /*---- enable ----*/
    if (self->la) CExprParser_TokenDecRef(self, self->la);
    if (self->t) CExprParser_TokenDecRef(self, self->t);
    CExprScanner_Destruct(&self->scanner);
    CcsErrorPool_Destruct(&self->errpool);
}

/*---- SubScanners ----*/
/*---- enable ----*/

/*---- ProductionsBody ----*/
static void
CExprParser_CExpr(CExprParser_t * self)
{
    int value; 
    CExprParser_expr1(self, &value);
    self->value = value; 
}

static void
CExprParser_expr1(CExprParser_t * self, int * value)
{
    int subvalue1, subvalue2; 
    CExprParser_expr2(self, value);
    while (self->la->kind == 2) {
	CExprParser_Get(self);
	CExprParser_expr2(self, &subvalue1);
	CExprParser_Expect(self, 3);
	CExprParser_expr2(self, &subvalue2);
	*value = *value ? subvalue1 : subvalue2; 
    }
}

static void
CExprParser_expr2(CExprParser_t * self, int * value)
{
    int subvalue; 
    CExprParser_expr3(self, value);
    while (self->la->kind == 4) {
	CExprParser_Get(self);
	CExprParser_expr3(self, &subvalue);
	*value = *value || subvalue; 
    }
}

static void
CExprParser_expr3(CExprParser_t * self, int * value)
{
    int subvalue; 
    CExprParser_expr4(self, value);
    while (self->la->kind == 5) {
	CExprParser_Get(self);
	CExprParser_expr4(self, &subvalue);
	*value = *value && subvalue; 
    }
}

static void
CExprParser_expr4(CExprParser_t * self, int * value)
{
    int subvalue; 
    CExprParser_expr5(self, value);
    while (self->la->kind == 6) {
	CExprParser_Get(self);
	CExprParser_expr5(self, &subvalue);
	*value |= subvalue; 
    }
}

static void
CExprParser_expr5(CExprParser_t * self, int * value)
{
    int subvalue; 
    CExprParser_expr6(self, value);
    while (self->la->kind == 7) {
	CExprParser_Get(self);
	CExprParser_expr6(self, &subvalue);
	*value ^= subvalue; 
    }
}

static void
CExprParser_expr6(CExprParser_t * self, int * value)
{
    int subvalue; 
    CExprParser_expr7(self, value);
    while (self->la->kind == 8) {
	CExprParser_Get(self);
	CExprParser_expr7(self, &subvalue);
	*value &= subvalue; 
    }
}

static void
CExprParser_expr7(CExprParser_t * self, int * value)
{
    int op, subvalue; 
    CExprParser_expr8(self, value);
    while (self->la->kind == 9 || self->la->kind == 10) {
	if (self->la->kind == 9) {
	    CExprParser_Get(self);
	    op = 0; 
	} else {
	    CExprParser_Get(self);
	    op = 1; 
	}
	CExprParser_expr8(self, &subvalue);
	if (op == 0) *value = *value == subvalue;
	else if (op == 1) *value = *value != subvalue; 
    }
}

static void
CExprParser_expr8(CExprParser_t * self, int * value)
{
    int op, subvalue; 
    CExprParser_expr9(self, value);
    while (CExprParser_StartOf(self, 1)) {
	if (self->la->kind == 11) {
	    CExprParser_Get(self);
	    op = 0; 
	} else if (self->la->kind == 12) {
	    CExprParser_Get(self);
	    op = 1; 
	} else if (self->la->kind == 13) {
	    CExprParser_Get(self);
	    op = 2; 
	} else {
	    CExprParser_Get(self);
	    op = 3; 
	}
	CExprParser_expr9(self, &subvalue);
	if (op == 0) *value = *value > subvalue;
	else if (op == 1) *value = *value >= subvalue;
	else if (op == 2) *value = *value < subvalue;
	else if (op == 3) *value = *value <= subvalue; 
    }
}

static void
CExprParser_expr9(CExprParser_t * self, int * value)
{
    int op, subvalue; 
    CExprParser_expr10(self, value);
    while (self->la->kind == 15 || self->la->kind == 16) {
	if (self->la->kind == 15) {
	    CExprParser_Get(self);
	    op = 0; 
	} else {
	    CExprParser_Get(self);
	    op = 1; 
	}
	CExprParser_expr10(self, &subvalue);
	if (op == 0) *value <<= subvalue;
	else if (op == 1) *value >>= subvalue; 
    }
}

static void
CExprParser_expr10(CExprParser_t * self, int * value)
{
    int op, subvalue; 
    CExprParser_expr11(self, value);
    while (self->la->kind == 17 || self->la->kind == 18) {
	if (self->la->kind == 17) {
	    CExprParser_Get(self);
	    op = 0; 
	} else {
	    CExprParser_Get(self);
	    op = 1; 
	}
	CExprParser_expr11(self, &subvalue);
	if (op == 0) *value += subvalue;
	else if (op == 1) *value -= subvalue; 
    }
}

static void
CExprParser_expr11(CExprParser_t * self, int * value)
{
    int op, subvalue; 
    CExprParser_expr12(self, value);
    while (self->la->kind == 19 || self->la->kind == 20 || self->la->kind == 21) {
	if (self->la->kind == 19) {
	    CExprParser_Get(self);
	    op = 0; 
	} else if (self->la->kind == 20) {
	    CExprParser_Get(self);
	    op = 1; 
	} else {
	    CExprParser_Get(self);
	    op = 2; 
	}
	CExprParser_expr12(self, &subvalue);
	if (op == 0) *value *= subvalue;
	else if (op == 1) *value /= subvalue;
	else if (op == 2) *value %= subvalue; 
    }
}

static void
CExprParser_expr12(CExprParser_t * self, int * value)
{
    if (self->la->kind == 1) {
	CExprParser_Get(self);
	*value = atoi(self->t->val); 
    } else if (self->la->kind == 22) {
	CExprParser_Get(self);
	CExprParser_expr1(self, value);
	CExprParser_Expect(self, 23);
    } else CExprParser_SynErr(self, 25);
}

/*---- enable ----*/

static void
CExprParser_SynErr(CExprParser_t * self, int n)
{
    const char * s; char format[20];
    switch (n) {
    /*---- SynErrors ----*/
    case 0: s = "\"" "EOF" "\" expected"; break;
    case 1: s = "\"" "number" "\" expected"; break;
    case 2: s = "\"" "?" "\" expected"; break;
    case 3: s = "\"" ":" "\" expected"; break;
    case 4: s = "\"" "||" "\" expected"; break;
    case 5: s = "\"" "&&" "\" expected"; break;
    case 6: s = "\"" "|" "\" expected"; break;
    case 7: s = "\"" "^" "\" expected"; break;
    case 8: s = "\"" "&" "\" expected"; break;
    case 9: s = "\"" "==" "\" expected"; break;
    case 10: s = "\"" "!=" "\" expected"; break;
    case 11: s = "\"" ">" "\" expected"; break;
    case 12: s = "\"" ">=" "\" expected"; break;
    case 13: s = "\"" "<" "\" expected"; break;
    case 14: s = "\"" "<=" "\" expected"; break;
    case 15: s = "\"" "<<" "\" expected"; break;
    case 16: s = "\"" ">>" "\" expected"; break;
    case 17: s = "\"" "+" "\" expected"; break;
    case 18: s = "\"" "-" "\" expected"; break;
    case 19: s = "\"" "*" "\" expected"; break;
    case 20: s = "\"" "/" "\" expected"; break;
    case 21: s = "\"" "%" "\" expected"; break;
    case 22: s = "\"" "(" "\" expected"; break;
    case 23: s = "\"" ")" "\" expected"; break;
    case 24: s = "\"" "???" "\" expected"; break;
    case 25: s = "this symbol not expected in \"" "expr12" "\""; break;
    /*---- enable ----*/
    default:
	snprintf(format, sizeof(format), "error %d", n);
	s = format;
	break;
    }
    CExprParser_SemErr(self, self->la, "%s", s);
}

#ifdef CExprParser_USE_StartOf
static const char * set[] = {
    /*---- InitSet ----*/
    /*    5    0    5    0     */
    "*.........................", /* 0 */
    "...........****..........."  /* 1 */
    /*---- enable ----*/
};
#endif /* CExprParser_USE_StartOf */
