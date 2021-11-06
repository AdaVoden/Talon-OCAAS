#ifndef lint
#endif
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define yyclearin (yychar=(-1))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define YYPREFIX "yy"
#line 4 "gram.y"

#include "sa.h"

/* handy macros to compile various types at PC */
#define c_t(v,t)        do {*(t*)PC = (t)(v); PC += sizeof(t); } while (0)
#define c_o(o)          c_t(o,Opcode)   /* compile an opcode */
#define c_r(r)          c_t(r,VRef)     /* compile a variable reference */
#define c_c(c)          c_t(c,VType)    /* compile a constant value */
#define c_b(b)          c_t(b,BrType)   /* compile a branch offset */
#define c_p(p)          c_t(p,CPType)   /* compile a pointer */
#define c_B(B)          c_t(B,Byte)   	/* compile a Byte */

/* do/while/for all compile with same preamble code arrangement:
 *    br   A    jump over preamble
 *    br   B	"break" branch
 *    br   C    "continue" branch
 * A: 		<-- this addr is pushed, followed by PRFL
 *
 * The preamble allows compiling a "break" or "continue" to a known location
 * before knowing the size of the test and body of the loop. The reason for
 * the PRFL is to find this preamble among possible if/else statements within
 * the loop body.
 * 
 * Assuming pb is the stack address of the PRFL, ie, the preamble base:
 *    A is at pb[1]
 *    offset portion of B branch instruction is at pb[1]-2*BRISZ+OPSZ
 *    offset portion of C branch instruction is at pb[1]-BRISZ+OPSZ
 */

#define	PRFL	0x1d2c3b4a		/* flag to mark base of preamble */
#define	OPSZ	(sizeof(Opcode))	/* bytes in op code */
#define	BRSZ	(sizeof(BrType))	/* bytes in branch */
#define	BRP(a)	((BrType*)a)		/* addr as a BrType pointer */
#define	BRISZ	(OPSZ+BRSZ)		/* bytes in total branch instruction */
#define	BRKOP	((CPType)pb[1]-2*BRISZ)	/* addr of Break BR opcode */
#define	BRKBR	(BRKOP+OPSZ)		/* addr of Break BR branch */
#define	BRKBP	BRP(BRKBR)		/* " as a BrType pointer */
#define	CONOP	((CPType)pb[1]-BRISZ)	/* addr of Continue BR opcode */
#define	CONBR	(CONOP+OPSZ)		/* addr of Continue BR branch */
#define	CONBP	BRP(CONBR)		/* " as a BrType pointer */
#define	NEWOP	((CPType)pb[1])		/* addr of first instr */

/* compile the preamble code for a loop construct, as well as push info on
 * the stack to allow filling it in later as things are learned.
 * (we can use the same stack since this is compile time, not runtime)
 */
static void
pushPre(void)
{
	c_o (OC_BR);		/* compile branch around preamble */
	c_b (BRSZ+2*BRISZ);	/*   skip over us and two OC_BR instructions */
	c_o (OC_BR);		/* compile branch to "break" addr */
	c_b (0);		/*   just leave room for offset now */
	c_o (OC_BR);		/* compile branch to "continue" addr */
	c_b (0);		/*   just leave room for offset now */
	PUSH(PC);		/* push addr of first useful code of loop */
	PUSH(PRFL);		/* push preamble marker */
}

/* search back up stack to find closest-enclosing preamble.
 * return pointer to PRFL or NULL
 */
static VType *
findPre(void)
{
	VType *pb = SP;

	while (*pb != PRFL)
	    if (++pb == &STACK(NSTACK))
		return (NULL);

#ifdef TRPREAMBLE
	printf ("LOOP starts at %d\n", NEWOP);
	printf ("Preamble jumps %d\n", pb[1]-(BRSZ+OPSZ+BRSZ+OPSZ+BRSZ)));
	printf ("Break    jumps %d\n", BRKBR);
	printf ("Continue jumps %d\n", CONBR);
#endif /* TRPREAMBLE */

	return (pb);
}

/* pop the current preamble */
static void
popPre(void)
{
	POP();
	POP();
}

/* called to update pti.yyp.ntmpv with the largest stack location used in a
 * function so far during function definition. it ends up holding the number
 * of stack locations the function will need when it is invoked to hold the
 * largest local variable used by the function.
 * N.B. the VRef IDX for local vars is i+2, where i is used as $i. see var.c.
 */
static void
maxTmpv (VRef r)
{
	int class = (r >> VR_CLASSSHIFT) & VR_CLASSMASK;

	if (class == VC_FTMP) {
	    int n = ((r >> VR_IDXSHIFT) & VR_IDXMASK)-1;
	    if (n > pti.yyp.ntmpv)
		pti.yyp.ntmpv = n;
	}
}

/* compile a function call, called with nfa args.
 * return 0 if ok, else -1
 */
static int
compileCall(char *name, int nfa)
{
	UFSTblE *up = findUFunc (name);
	IFSTblE *ip;

	/* try user funcs first */
	if (up) {
	    if (up->nargs != nfa) {
		printf ("%s() expects %d arg%s, not %d\n", name, up->nargs,
					    up->nargs==1?"":"s", nfa);
		return (-1);
	    }
	    c_o(OC_PUSHC);
	    c_c(nfa);
	    c_o(OC_UCALL);
	    c_p(up->code);
	    c_B(up->ntmpv);
	    return (0);
	}

	/* then built-in funcs */
	ip = findIFunc (name);
	if (ip) {
	    if (nfa < (int)ip->minnargs || nfa > ip->maxnargs) {
		if (ip->minnargs == ip->maxnargs)
		    printf ("%s() expects %d arg%s, not %d\n", name,
			    ip->minnargs, ip->minnargs==1?"":"s", nfa);
		else
		    printf ("%s() expects at least %d args\n", name,
								ip->minnargs);
		return (-1);
	    }
	    c_o(OC_PUSHC);
	    c_c(nfa);
	    c_o(OC_ICALL);
	    c_p(ip->addr);

	    /* if printf, compile the format string in-line next */
	    if (!strcmp (name,"printf")) {
		int i, l;
		if (!pti.yyp.bigstring) {
		    printf ("%s() missing format\n", name);
		    return (-1);
		}
		l = strlen(pti.yyp.bigstring)+1; /* 0 too */
		for (i = 0; i < l; i++)
		    c_B(pti.yyp.bigstring[i]);
		free (pti.yyp.bigstring);
		pti.yyp.bigstring = NULL;
		pti.yyp.nbigstring = 0;
	    }
	    return (0);
	}

	/* neither */
	printf ("Undef func: %s\n", name);
	return (-1);
}

/* create a new user function.
 * the new code is already compiled in the per-task area, and nfpn and ntmpv
 *   are set.
 * return 0 if ok else -1
 */
static int
compileFunc(char *name)
{
	UFSTblE *fp = newUFunc();
	Byte *code;
	int ncode;
	int ret = 0;

	if (!fp) {
	    printf("No mem for new func\n");
	    ret = 1;
	    goto out;
	}

	c_o (OC_URETURN);
	ncode = PC - (CPType)pti.ucode;
	code = malloc (ncode);
	if (!code) {
	    printf("No mem for new code\n");
	    ret = 1;
	    goto out;
	}
	memcpy (code, pti.ucode, ncode);
	strncpy (fp->name, name, NNAME);
	fp->code = code;
	fp->size = ncode;
	fp->nargs = pti.yyp.nfpn;
	fp->ntmpv = pti.yyp.ntmpv;

    out:
	initPC();
	c_o (OC_HALT);

	return (0);
}

#line 224 "gram.tab.c"
#define ASSIGN 257
#define ADDOP 258
#define SUBOP 259
#define MULTOP 260
#define DIVOP 261
#define MODOP 262
#define ANDOP 263
#define XOROP 264
#define OROP 265
#define LSHOP 266
#define RSHOP 267
#define CHIF 268
#define CHELSE 269
#define LAND 270
#define LOR 271
#define AND 272
#define XOR 273
#define OR 274
#define EQ 275
#define NE 276
#define LT 277
#define LE 278
#define GT 279
#define GE 280
#define LSHIFT 281
#define RSHIFT 282
#define ADD 283
#define SUB 284
#define MULT 285
#define DIV 286
#define MOD 287
#define NOT 288
#define COMP 289
#define INC 290
#define DEC 291
#define UPLUS 292
#define UMINUS 293
#define NUMBER 294
#define NAME 295
#define STRING 296
#define SEMI 297
#define LPAREN 298
#define RPAREN 299
#define BLKSRT 300
#define BLKEND 301
#define IF 302
#define ELSE 303
#define DO 304
#define WHILE 305
#define FOR 306
#define BREAK 307
#define CONT 308
#define RETURN 309
#define DEFINE 310
#define UNDEF 311
#define COMMA 312
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    0,    0,    1,    6,    8,    2,    5,    5,    9,
    9,   10,    3,    4,    4,    4,    4,    4,    4,    4,
    4,    4,    4,    4,    4,   11,   33,   33,    7,    7,
   35,   29,   34,   34,   30,   31,   36,   32,   13,   14,
   16,   17,   18,   19,   21,   23,   24,   25,   26,   28,
   27,   12,   20,   20,   22,   22,   15,   15,   15,   15,
   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,
   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,
   15,   15,   15,   15,   15,   15,   15,   15,   15,   15,
   15,   15,   15,   15,   15,   15,   15,   15,   37,   38,
   39,   42,   40,   41,   41,   43,   43,   44,   44,
};
short yylen[] = {                                         2,
    1,    1,    1,    1,    0,    0,    8,    0,    1,    1,
    3,    1,    3,    2,    9,    8,   11,    7,   10,    1,
    1,    1,    1,    1,    1,    1,    1,    2,    2,    3,
    0,    4,    0,    1,    2,    2,    0,    4,    1,    0,
    0,    0,    0,    0,    1,    1,    0,    0,    0,    0,
    0,    1,    0,    1,    0,    1,    1,    1,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    2,
    2,    2,    2,    2,    2,    2,    2,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    7,    3,    1,    1,    1,
    1,    0,    5,    0,    1,    1,    3,    1,    1,
};
short yydefred[] = {                                      0,
    0,    0,    0,    0,    0,    0,    0,   99,    0,   26,
    0,    0,    0,   39,   42,    0,    0,    0,    0,    0,
    0,    0,    1,    2,    3,    4,   20,   25,    0,    0,
    0,   21,   22,   23,   24,   57,   58,    0,   98,    0,
   76,   77,   70,   71,  101,   72,   74,  102,    0,   29,
   27,    0,    0,    0,    0,   35,   36,    0,   31,    0,
    0,   14,    0,   49,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   73,   75,    0,    0,   97,   30,   28,
    0,    0,   54,    0,    0,    5,   13,   40,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   78,   79,   80,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   38,  109,
    0,    0,    0,  106,   49,    0,   45,    0,   32,    0,
    0,    0,  103,    0,    0,   43,    0,    0,   12,    0,
    0,   10,    0,   50,  107,    0,    0,   46,    0,    6,
    0,    0,    0,   50,   18,   44,   47,    0,   11,   41,
    0,    0,   16,    0,    7,    0,   51,    0,   15,   19,
   48,   17,
};
short yydgoto[] = {                                      22,
   23,   24,   25,   26,  160,  150,   27,  178,  161,  162,
   28,   29,   30,  151,   31,  186,   54,  167,  183,  104,
  148,  158,  169,  184,  192,  109,  175,  173,   32,   33,
   34,   35,   52,   59,  105,   96,   36,   37,   38,   39,
  142,   97,  143,  144,
};
short yysindex[] = {                                   -246,
 -259, -259, -259, -259, -259, -283, -283,    0, -272,    0,
 -259, -160, -270,    0,    0, -265, -281, -281, -259, -254,
 -249,    0,    0,    0,    0,    0,    0,    0, -281,   29,
  595,    0,    0,    0,    0,    0,    0,  452,    0,  595,
    0,    0,    0,    0,    0,    0,    0,    0,  453,    0,
    0,  -34, -259, -251, -259,    0,    0,  595,    0, -248,
 -281,    0, -252,    0, -259, -259, -259, -259, -259, -259,
 -259, -259, -259, -259, -259, -259, -259, -259, -259, -259,
 -259, -259, -259, -259, -259, -259, -259, -259, -259, -259,
 -259, -259, -259,    0,    0, -281, -176,    0,    0,    0,
  483, -259,    0, -281, -281,    0,    0,    0, -259,  611,
  611,  624,  624,  624,  -42,  -42,  -98,  -98,  -98,  -98,
 -126, -126, -277, -277,    0,    0,    0,  595,  595,  595,
  595,  595,  595,  595,  595,  595,  595,  595,    0,    0,
  595, -242, -216,    0,    0,  513,    0, -259,    0, -197,
 -198,  575,    0, -176,   29,    0,  595, -281,    0, -196,
 -210,    0, -259,    0,    0, -186,   29,    0, -259,    0,
 -197,  543, -259,    0,    0,    0,    0, -195,    0,    0,
  595,   29,    0, -193,    0, -281,    0,   29,    0,    0,
    0,    0,
};
short yyrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,    0,    0,  -65,    0,
    0,    0,    0,    0,    0,    0,    0,    0, -172,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
 -282,    0,    0,    0,    0,    0,    0,    0,    0, -171,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0, -170,    0,    0, -165,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0, -178,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  371,
  378,  314,  346,  360,  104,  149,  222,  267,  281,  326,
  161,  206,   71,  116,    0,    0,    0, -133, -130,  -18,
   59,  157,  158,  338,  355,  367,  387,  392,    0,    0,
 -285,    0, -166,    0,    0,    0,    0, -146,    0, -147,
    0,    0,    0,    0,    0,    0, -144,    0,    0,    0,
 -145,    0,    0,    0,    0,    1,    0,    0, -137,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  396,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,
};
short yygindex[] = {                                      0,
    0,    0,    0,  -12,    0,    0,  -10,    0,    0,    3,
    5,  -53,    0,    0,    2,    0,    0,    0,    0,    4,
    0,    0,    0,    0,    0,   26,  -15,    6,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   13,    0,
    0,    0,    0,   23,
};
#define YYTABLESIZE 911
short yytable[] = {                                      51,
   51,  103,   40,   41,   42,   43,   44,   80,   81,   82,
    1,   45,   49,  108,   52,   10,   52,   63,   46,   47,
   58,   56,   57,    2,    3,   48,  108,   53,    4,    5,
    6,    7,   55,   62,    8,    9,    2,    3,   11,  100,
   60,    4,    5,    6,    7,   61,  102,    8,    9,  106,
   10,   11,  108,   12,  101,   13,  153,   14,   15,   16,
   17,   18,   19,   20,   21,  107,  110,  111,  112,  113,
  114,  115,  116,  117,  118,  119,  120,  121,  122,  123,
  124,  125,  126,  127,  128,  129,  130,  131,  132,  133,
  134,  135,  136,  137,  138,  154,    1,  159,  141,  163,
  139,  171,  170,  146,   12,  188,    2,    3,  147,  149,
  152,    4,    5,    6,    7,  103,  174,    8,    9,  140,
  104,   11,    2,    3,   33,   37,   53,    4,    5,    6,
    7,   34,  105,    8,    9,   59,   10,   11,   60,   12,
   50,   13,  166,   14,   15,   16,   17,   18,   19,  157,
   55,    8,   56,    9,  176,  141,   78,   79,   80,   81,
   82,   53,  168,   59,  172,   59,   60,  185,   60,  187,
  155,  190,  177,  179,  181,  191,  165,    0,   59,  182,
    0,   60,   76,   77,   78,   79,   80,   81,   82,    0,
  189,  101,  101,  101,  101,  101,  101,  101,  101,  101,
  101,  101,  100,  100,  100,  100,  100,  100,  100,  100,
  100,  100,  100,  100,  100,  100,  100,  100,  100,  100,
  100,  100,    1,    0,  101,  101,    0,    0,    0,    0,
    0,  100,    0,  100,   72,   73,   74,   75,   76,   77,
   78,   79,   80,   81,   82,    0,  100,    0,    2,    3,
   61,    0,    0,    4,    5,    6,    7,   51,    0,    8,
    9,    0,   10,   11,    0,   12,   99,   13,    0,   14,
   15,   16,   17,   18,   19,    0,    0,    0,   61,    0,
   61,    0,    0,   51,   51,    1,    0,    0,   51,   51,
   51,   51,    0,   61,   51,   51,    0,   51,   51,    0,
   51,   51,   51,    0,   51,   51,   51,   51,   51,   51,
    0,    2,    3,    0,    0,    0,    4,    5,    6,    7,
    0,    0,    8,    9,    0,   10,   11,   62,   12,    0,
   13,    0,   14,   15,   16,   17,   18,   19,   81,   81,
   81,   81,   81,   81,   81,   81,   81,   81,   81,   81,
   81,   81,   81,   81,   81,   62,    0,   62,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   81,    0,   81,
   62,   89,   89,   89,   89,   89,   89,   89,   89,   89,
    0,    0,   81,   82,   82,   82,   82,   82,   82,   82,
   82,   82,   82,   82,   82,   82,   82,   82,   82,   82,
   89,    0,   89,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   82,    0,   82,   89,   90,   90,   90,   90,
   90,   90,   90,   90,   90,   63,   64,   82,   83,   83,
   83,   83,   83,   83,   83,   83,   83,   83,   83,   83,
   83,   83,   83,    0,    0,   90,    0,   90,    0,    0,
    0,    0,    0,   63,   64,   63,   64,   83,    0,   83,
   90,    0,    0,    0,    0,    0,    0,    0,   63,   64,
    0,    0,   83,   84,   84,   84,   84,   84,   84,   84,
   84,   84,   84,   84,   84,   84,   84,   84,    0,   85,
   85,   85,   85,   85,   85,   85,   85,   85,   85,   85,
   85,   85,   84,    0,   84,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   84,   85,    0,
   85,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   85,   86,   86,   86,   86,   86,   86,
   86,   86,   86,   86,   86,   86,   86,    0,   87,   87,
   87,   87,   87,   87,   87,   87,   87,   87,   87,   87,
   87,    0,    0,   86,    0,   86,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   87,   86,   87,
    0,   91,   91,   91,   91,   91,   91,   91,    0,    0,
    0,    0,   87,   88,   88,   88,   88,   88,   88,   88,
   88,   88,   88,   88,   88,   88,   67,    0,    0,    0,
   91,    0,   91,   93,   93,   93,   93,   93,   93,   93,
    0,    0,   88,   66,   88,   91,    0,   92,   92,   92,
   92,   92,   92,   92,   67,   65,   67,   88,   94,   94,
   94,   94,   93,    0,   93,   95,   95,   95,   95,   67,
    0,   66,    0,   66,    0,   68,   92,   93,   92,    0,
   69,    0,    0,   65,   96,   65,   66,   94,    0,   94,
    0,   92,    0,    0,   95,    0,   95,    0,   65,    0,
    0,    0,   94,   68,    0,   68,    0,    0,   69,   95,
   69,    0,   96,    0,   96,    0,    0,    0,   68,    0,
    0,    0,    0,   69,    0,    0,    0,   96,   83,   84,
   85,   86,   87,   88,   89,   90,   91,   92,   93,    0,
   64,    0,   65,   66,   67,   68,   69,   70,   71,   72,
   73,   74,   75,   76,   77,   78,   79,   80,   81,   82,
    0,   94,   95,    0,    0,    0,    0,    0,    0,    0,
   64,   98,   65,   66,   67,   68,   69,   70,   71,   72,
   73,   74,   75,   76,   77,   78,   79,   80,   81,   82,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   64,  145,   65,   66,   67,   68,   69,   70,   71,   72,
   73,   74,   75,   76,   77,   78,   79,   80,   81,   82,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   64,  156,   65,   66,   67,   68,   69,   70,   71,   72,
   73,   74,   75,   76,   77,   78,   79,   80,   81,   82,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  180,   64,  164,   65,   66,   67,   68,   69,   70,
   71,   72,   73,   74,   75,   76,   77,   78,   79,   80,
   81,   82,   64,    0,   65,   66,   67,   68,   69,   70,
   71,   72,   73,   74,   75,   76,   77,   78,   79,   80,
   81,   82,   67,   68,   69,   70,   71,   72,   73,   74,
   75,   76,   77,   78,   79,   80,   81,   82,   70,   71,
   72,   73,   74,   75,   76,   77,   78,   79,   80,   81,
   82,
};
short yycheck[] = {                                      12,
    0,   55,    1,    2,    3,    4,    5,  285,  286,  287,
  257,  295,   11,  299,  297,  297,  299,   30,    6,    7,
   19,   17,   18,  283,  284,  298,  312,  298,  288,  289,
  290,  291,  298,   29,  294,  295,  283,  284,  298,   52,
  295,  288,  289,  290,  291,  295,  298,  294,  295,  298,
  297,  298,  305,  300,   53,  302,  299,  304,  305,  306,
  307,  308,  309,  310,  311,   61,   65,   66,   67,   68,
   69,   70,   71,   72,   73,   74,   75,   76,   77,   78,
   79,   80,   81,   82,   83,   84,   85,   86,   87,   88,
   89,   90,   91,   92,   93,  312,  257,  295,   97,  298,
   96,  312,  299,  102,  300,  299,  283,  284,  104,  105,
  109,  288,  289,  290,  291,  169,  303,  294,  295,  296,
  299,  298,  283,  284,  297,  297,  297,  288,  289,  290,
  291,  297,  299,  294,  295,  269,  297,  298,  269,  300,
  301,  302,  155,  304,  305,  306,  307,  308,  309,  148,
  297,  299,  297,  299,  167,  154,  283,  284,  285,  286,
  287,  299,  158,  297,  163,  299,  297,  178,  299,  182,
  145,  187,  169,  171,  173,  188,  154,   -1,  312,  174,
   -1,  312,  281,  282,  283,  284,  285,  286,  287,   -1,
  186,  257,  258,  259,  260,  261,  262,  263,  264,  265,
  266,  267,  268,  269,  270,  271,  272,  273,  274,  275,
  276,  277,  278,  279,  280,  281,  282,  283,  284,  285,
  286,  287,  257,   -1,  290,  291,   -1,   -1,   -1,   -1,
   -1,  297,   -1,  299,  277,  278,  279,  280,  281,  282,
  283,  284,  285,  286,  287,   -1,  312,   -1,  283,  284,
  269,   -1,   -1,  288,  289,  290,  291,  257,   -1,  294,
  295,   -1,  297,  298,   -1,  300,  301,  302,   -1,  304,
  305,  306,  307,  308,  309,   -1,   -1,   -1,  297,   -1,
  299,   -1,   -1,  283,  284,  257,   -1,   -1,  288,  289,
  290,  291,   -1,  312,  294,  295,   -1,  297,  298,   -1,
  300,  301,  302,   -1,  304,  305,  306,  307,  308,  309,
   -1,  283,  284,   -1,   -1,   -1,  288,  289,  290,  291,
   -1,   -1,  294,  295,   -1,  297,  298,  269,  300,   -1,
  302,   -1,  304,  305,  306,  307,  308,  309,  268,  269,
  270,  271,  272,  273,  274,  275,  276,  277,  278,  279,
  280,  281,  282,  283,  284,  297,   -1,  299,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  297,   -1,  299,
  312,  268,  269,  270,  271,  272,  273,  274,  275,  276,
   -1,   -1,  312,  268,  269,  270,  271,  272,  273,  274,
  275,  276,  277,  278,  279,  280,  281,  282,  283,  284,
  297,   -1,  299,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  297,   -1,  299,  312,  268,  269,  270,  271,
  272,  273,  274,  275,  276,  269,  269,  312,  268,  269,
  270,  271,  272,  273,  274,  275,  276,  277,  278,  279,
  280,  281,  282,   -1,   -1,  297,   -1,  299,   -1,   -1,
   -1,   -1,   -1,  297,  297,  299,  299,  297,   -1,  299,
  312,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  312,  312,
   -1,   -1,  312,  268,  269,  270,  271,  272,  273,  274,
  275,  276,  277,  278,  279,  280,  281,  282,   -1,  268,
  269,  270,  271,  272,  273,  274,  275,  276,  277,  278,
  279,  280,  297,   -1,  299,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  312,  297,   -1,
  299,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  312,  268,  269,  270,  271,  272,  273,
  274,  275,  276,  277,  278,  279,  280,   -1,  268,  269,
  270,  271,  272,  273,  274,  275,  276,  277,  278,  279,
  280,   -1,   -1,  297,   -1,  299,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  297,  312,  299,
   -1,  268,  269,  270,  271,  272,  273,  274,   -1,   -1,
   -1,   -1,  312,  268,  269,  270,  271,  272,  273,  274,
  275,  276,  277,  278,  279,  280,  269,   -1,   -1,   -1,
  297,   -1,  299,  268,  269,  270,  271,  272,  273,  274,
   -1,   -1,  297,  269,  299,  312,   -1,  268,  269,  270,
  271,  272,  273,  274,  297,  269,  299,  312,  268,  269,
  270,  271,  297,   -1,  299,  268,  269,  270,  271,  312,
   -1,  297,   -1,  299,   -1,  269,  297,  312,  299,   -1,
  269,   -1,   -1,  297,  269,  299,  312,  297,   -1,  299,
   -1,  312,   -1,   -1,  297,   -1,  299,   -1,  312,   -1,
   -1,   -1,  312,  297,   -1,  299,   -1,   -1,  297,  312,
  299,   -1,  297,   -1,  299,   -1,   -1,   -1,  312,   -1,
   -1,   -1,   -1,  312,   -1,   -1,   -1,  312,  257,  258,
  259,  260,  261,  262,  263,  264,  265,  266,  267,   -1,
  268,   -1,  270,  271,  272,  273,  274,  275,  276,  277,
  278,  279,  280,  281,  282,  283,  284,  285,  286,  287,
   -1,  290,  291,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  268,  299,  270,  271,  272,  273,  274,  275,  276,  277,
  278,  279,  280,  281,  282,  283,  284,  285,  286,  287,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  268,  299,  270,  271,  272,  273,  274,  275,  276,  277,
  278,  279,  280,  281,  282,  283,  284,  285,  286,  287,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  268,  299,  270,  271,  272,  273,  274,  275,  276,  277,
  278,  279,  280,  281,  282,  283,  284,  285,  286,  287,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  299,  268,  269,  270,  271,  272,  273,  274,  275,
  276,  277,  278,  279,  280,  281,  282,  283,  284,  285,
  286,  287,  268,   -1,  270,  271,  272,  273,  274,  275,
  276,  277,  278,  279,  280,  281,  282,  283,  284,  285,
  286,  287,  272,  273,  274,  275,  276,  277,  278,  279,
  280,  281,  282,  283,  284,  285,  286,  287,  275,  276,
  277,  278,  279,  280,  281,  282,  283,  284,  285,  286,
  287,
};
#define YYFINAL 22
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 312
#if YYDEBUG
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"ASSIGN","ADDOP","SUBOP","MULTOP",
"DIVOP","MODOP","ANDOP","XOROP","OROP","LSHOP","RSHOP","CHIF","CHELSE","LAND",
"LOR","AND","XOR","OR","EQ","NE","LT","LE","GT","GE","LSHIFT","RSHIFT","ADD",
"SUB","MULT","DIV","MOD","NOT","COMP","INC","DEC","UPLUS","UMINUS","NUMBER",
"NAME","STRING","SEMI","LPAREN","RPAREN","BLKSRT","BLKEND","IF","ELSE","DO",
"WHILE","FOR","BREAK","CONT","RETURN","DEFINE","UNDEF","COMMA",
};
char *yyrule[] = {
"$accept : Input",
"Input : ImmediateStatement",
"Input : FunctionDefinition",
"Input : FunctionUndefine",
"ImmediateStatement : Statement",
"$$1 :",
"$$2 :",
"FunctionDefinition : DEFINE NAME LPAREN $$1 Params RPAREN $$2 CompoundStatement",
"Params :",
"Params : ParamList",
"ParamList : Param",
"ParamList : ParamList COMMA Param",
"Param : NAME",
"FunctionUndefine : UNDEF NAME EOS",
"Statement : ExpP EOS",
"Statement : DoStart Statement WHILE DoTest LPAREN Exp RPAREN DWEnd EOS",
"Statement : WHILE WUStart LPAREN Exp RPAREN WhileTest Statement WUEnd",
"Statement : FOR LPAREN ExpPOp For1 ExpOp For2 ExpPOp For3 RPAREN Statement ForEnd",
"Statement : IF LPAREN Exp RPAREN IfTest Statement IfEnd",
"Statement : IF LPAREN Exp RPAREN IfTest Statement ELSE IfElse Statement IfEnd",
"Statement : CompoundStatement",
"Statement : Return",
"Statement : Break",
"Statement : Continue",
"Statement : SimplePrint",
"Statement : EOS",
"EOS : SEMI",
"StatementList : Statement",
"StatementList : StatementList Statement",
"CompoundStatement : BLKSRT BLKEND",
"CompoundStatement : BLKSRT StatementList BLKEND",
"$$3 :",
"Return : RETURN ReturnVal $$3 EOS",
"ReturnVal :",
"ReturnVal : Exp",
"Break : BREAK EOS",
"Continue : CONT EOS",
"$$4 :",
"SimplePrint : ASSIGN Exp $$4 EOS",
"DoStart : DO",
"DoTest :",
"DWEnd :",
"WUStart :",
"WhileTest :",
"WUEnd :",
"For1 : EOS",
"For2 : EOS",
"For3 :",
"ForEnd :",
"IfTest :",
"IfElse :",
"IfEnd :",
"ExpP : Exp",
"ExpPOp :",
"ExpPOp : ExpP",
"ExpOp :",
"ExpOp : Exp",
"Exp : Const",
"Exp : Var",
"Exp : Ref ASSIGN Exp",
"Exp : Ref ADDOP Exp",
"Exp : Ref SUBOP Exp",
"Exp : Ref MULTOP Exp",
"Exp : Ref DIVOP Exp",
"Exp : Ref MODOP Exp",
"Exp : Ref OROP Exp",
"Exp : Ref XOROP Exp",
"Exp : Ref ANDOP Exp",
"Exp : Ref LSHOP Exp",
"Exp : Ref RSHOP Exp",
"Exp : NOT Exp",
"Exp : COMP Exp",
"Exp : INC Ref",
"Exp : Ref INC",
"Exp : DEC Ref",
"Exp : Ref DEC",
"Exp : ADD Exp",
"Exp : SUB Exp",
"Exp : Exp MULT Exp",
"Exp : Exp DIV Exp",
"Exp : Exp MOD Exp",
"Exp : Exp ADD Exp",
"Exp : Exp SUB Exp",
"Exp : Exp LSHIFT Exp",
"Exp : Exp RSHIFT Exp",
"Exp : Exp LT Exp",
"Exp : Exp LE Exp",
"Exp : Exp GT Exp",
"Exp : Exp GE Exp",
"Exp : Exp EQ Exp",
"Exp : Exp NE Exp",
"Exp : Exp AND Exp",
"Exp : Exp OR Exp",
"Exp : Exp XOR Exp",
"Exp : Exp LAND Exp",
"Exp : Exp LOR Exp",
"Exp : Exp CHIF IfTest Exp CHELSE IfElse Exp",
"Exp : LPAREN Exp RPAREN",
"Exp : FunctionCall",
"Const : NUMBER",
"Var : NAME",
"Ref : NAME",
"$$5 :",
"FunctionCall : NAME LPAREN $$5 Args RPAREN",
"Args :",
"Args : ArgList",
"ArgList : Arg",
"ArgList : ArgList COMMA Arg",
"Arg : Exp",
"Arg : STRING",
};
#endif
#define yynerrs         pti.yyp.yynerrs
#define yyerrflag       pti.yyp.yyerrflag
#define yychar          pti.yyp.yychar
#define yyssp           pti.yyp.yyssp
#define yyvsp           pti.yyp.yyvsp
#define yyval           pti.yyp.yyval
#define yylval          pti.yyp.yylval
#define yyss            pti.yyp.yyss
#define yyvs            pti.yyp.yyvs
#define maxnyyvs	pti.yyp.maxnyyvs
void maxnyys(void) { int n = yyvsp - yyvs; if (n > maxnyyvs) maxnyyvs = n; }
#define yystacksize YYSTACKSIZE
#line 612 "gram.y"

static void
yyerror (char s[])
{
	if (!(pti.flags & (TF_INTERRUPT|TF_WEDIE)))
	    printf ("%s\n", s);
}

/* For RCS Only -- Do Not Edit
 * @(#) $RCSfile: gram.tab.c,v $ $Date: 2003/04/15 20:46:49 $ $Revision: 1.1.1.1 $ $Name:  $
 */
#line 747 "gram.tab.c"
#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
yyparse(void)
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register char *yys;
    extern char *getenv();

    if (yys = getenv("YYDEBUG"))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if (yyn = yydefred[yystate]) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yyss + yystacksize - 1)
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
maxnyys();
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#ifdef lint
    goto yynewerror;
#endif
yynewerror:
    yyerror("syntax error");
#ifdef lint
    goto yyerrlab;
#endif
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yyss + yystacksize - 1)
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
maxnyys();
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 1:
#line 255 "gram.y"
{ return (0); }
break;
case 2:
#line 256 "gram.y"
{ return (0); }
break;
case 3:
#line 257 "gram.y"
{ return (0); }
break;
case 4:
#line 261 "gram.y"
{ c_o(OC_HALT); }
break;
case 5:
#line 265 "gram.y"
{ freeUFunc (yyvsp[-1].s);
					  pti.yyp.fpn = (NameSpc*) calloc
						    (NFUNP, sizeof(NameSpc));
					  pti.yyp.nfpn = 0;
					}
break;
case 6:
#line 270 "gram.y"
{ initPC(); pti.yyp.ntmpv=0; }
break;
case 7:
#line 271 "gram.y"
{ if (compileFunc (yyvsp[-6].s) < 0)
	  				    return (1);
					}
break;
case 12:
#line 287 "gram.y"
{ if (pti.yyp.nfpn == NFUNP) {
					    printf ("Max %d params\n", NFUNP);
					    return (1);
					  } else if (yyvsp[0].s[0] == '$'
							&& !isdigit(yyvsp[0].s[1]))
					    strncpy(pti.yyp.fpn[pti.yyp.nfpn++],
					    			yyvsp[0].s, NNAME);
					  else {
					    printf ("Illeg Param: %s\n", yyvsp[0].s);
					    return (1);
					  }
					}
break;
case 13:
#line 302 "gram.y"
{ c_o(OC_HALT); /* just mark empty */
					  if (freeUFunc (yyvsp[-1].s) < 0) {
					    printf ("No func: %s\n", yyvsp[-1].s);
					    return (1);
					  }
					}
break;
case 31:
#line 340 "gram.y"
{ c_o (OC_URETURN); }
break;
case 35:
#line 349 "gram.y"
{ VType *pb = findPre();
					  if (!pb) {
					    printf ("`break' but no loop\n");
					    return (1);
					  }
					  c_o (OC_BR);
					  c_b (BRKOP - PC);
					}
break;
case 36:
#line 360 "gram.y"
{ VType *pb = findPre();
					  if (!pb) {
					    printf ("`continue' but no loop\n");
					    return (1);
					  }
					  c_o (OC_BR);
					  c_b (CONOP - PC);
					}
break;
case 37:
#line 371 "gram.y"
{ IFSTblE *ip = findIFunc("printf");
					  c_o(OC_PUSHC);
					  c_c(1);
					  c_o(OC_ICALL);
					  c_p(ip->addr);
					  c_B('%'); c_B('d'); c_B('\n'); c_B(0);
					  c_o(OC_POPS);
					}
break;
case 39:
#line 383 "gram.y"
{ pushPre(); }
break;
case 40:
#line 387 "gram.y"
{ VType *pb = findPre();
	  				  *CONBP = PC-CONBR;
					}
break;
case 41:
#line 393 "gram.y"
{ VType *pb = findPre();
					  c_o (OC_BRT);
					  c_b (NEWOP - PC);
					  *BRKBP = PC-BRKBR;
					  popPre();
					}
break;
case 42:
#line 403 "gram.y"
{ VType *pb;
	  				  pushPre();
					  pb = findPre();
	  				  *CONBP = PC-CONBR;
					}
break;
case 43:
#line 411 "gram.y"
{ VType *pb = findPre();
					  c_o (OC_BRF);
					  c_b (BRKOP - PC);
					}
break;
case 44:
#line 417 "gram.y"
{ VType *pb = findPre();
					  c_o (OC_BR);
                                          c_b (NEWOP - PC);
					  *BRKBP = PC-BRKBR;
					  popPre();
					}
break;
case 45:
#line 426 "gram.y"
{ pushPre(); }
break;
case 46:
#line 433 "gram.y"
{ VType *pb = findPre();
	  				  CPType t;
					  c_o (OC_BRF);
					  c_b (BRKOP - PC);
	  				  c_o (OC_BR);
					  t = PC;
	  				  c_b (0);
	  				  *CONBP = PC-CONBR;
					  PUSH(t);
					}
break;
case 47:
#line 448 "gram.y"
{ VType *pb = findPre();
					  CPType t = (CPType)POP();
					  c_o (OC_BR);
					  c_b (NEWOP - PC);
					  *BRP(t) = (BrType)(PC-t);
					}
break;
case 48:
#line 457 "gram.y"
{ VType *pb = findPre();
					  c_o (OC_BR);
					  c_b (CONOP - PC);
					  *BRKBP = PC-BRKBR;
					  popPre();
					}
break;
case 49:
#line 466 "gram.y"
{ c_o (OC_BRF);
					  PUSH(PC);
					  c_b (0);
					}
break;
case 50:
#line 475 "gram.y"
{ CPType t = (CPType)POP();
					  c_o (OC_BR);
					  PUSH(PC);
					  c_b (0);
					  *BRP(t) = (BrType)(PC-t);
					}
break;
case 51:
#line 484 "gram.y"
{ CPType t = (CPType)PEEK(0);
					  *BRP(t) = (BrType)(PC-t);
					  POP();
					}
break;
case 52:
#line 491 "gram.y"
{ c_o (OC_POPS); }
break;
case 55:
#line 500 "gram.y"
{ c_o(OC_PUSHC); /* `True' default */
					  c_c(1);
					}
break;
case 59:
#line 509 "gram.y"
{ c_o(OC_ASSIGN); }
break;
case 60:
#line 510 "gram.y"
{ c_o(OC_ADDOP); }
break;
case 61:
#line 511 "gram.y"
{ c_o(OC_SUBOP); }
break;
case 62:
#line 512 "gram.y"
{ c_o(OC_MULTOP); }
break;
case 63:
#line 513 "gram.y"
{ c_o(OC_DIVOP); }
break;
case 64:
#line 514 "gram.y"
{ c_o(OC_MODOP); }
break;
case 65:
#line 515 "gram.y"
{ c_o(OC_OROP); }
break;
case 66:
#line 516 "gram.y"
{ c_o(OC_XOROP); }
break;
case 67:
#line 517 "gram.y"
{ c_o(OC_ANDOP); }
break;
case 68:
#line 518 "gram.y"
{ c_o(OC_LSHOP); }
break;
case 69:
#line 519 "gram.y"
{ c_o(OC_RSHOP); }
break;
case 70:
#line 520 "gram.y"
{ c_o(OC_NOT); }
break;
case 71:
#line 521 "gram.y"
{ c_o(OC_COMP); }
break;
case 72:
#line 522 "gram.y"
{ c_o(OC_PREINC); }
break;
case 73:
#line 523 "gram.y"
{ c_o(OC_POSTINC); }
break;
case 74:
#line 524 "gram.y"
{ c_o(OC_PREDEC); }
break;
case 75:
#line 525 "gram.y"
{ c_o(OC_POSTDEC); }
break;
case 77:
#line 527 "gram.y"
{ c_o(OC_UMINUS); }
break;
case 78:
#line 528 "gram.y"
{ c_o(OC_MULT); }
break;
case 79:
#line 529 "gram.y"
{ c_o(OC_DIV); }
break;
case 80:
#line 530 "gram.y"
{ c_o(OC_MOD); }
break;
case 81:
#line 531 "gram.y"
{ c_o(OC_ADD); }
break;
case 82:
#line 532 "gram.y"
{ c_o(OC_SUB); }
break;
case 83:
#line 533 "gram.y"
{ c_o(OC_LSHIFT); }
break;
case 84:
#line 534 "gram.y"
{ c_o(OC_RSHIFT); }
break;
case 85:
#line 535 "gram.y"
{ c_o(OC_LT); }
break;
case 86:
#line 536 "gram.y"
{ c_o(OC_LE); }
break;
case 87:
#line 537 "gram.y"
{ c_o(OC_GT); }
break;
case 88:
#line 538 "gram.y"
{ c_o(OC_GE); }
break;
case 89:
#line 539 "gram.y"
{ c_o(OC_EQ); }
break;
case 90:
#line 540 "gram.y"
{ c_o(OC_NE); }
break;
case 91:
#line 541 "gram.y"
{ c_o(OC_AND); }
break;
case 92:
#line 542 "gram.y"
{ c_o(OC_OR); }
break;
case 93:
#line 543 "gram.y"
{ c_o(OC_XOR); }
break;
case 94:
#line 544 "gram.y"
{ c_o(OC_LAND); }
break;
case 95:
#line 545 "gram.y"
{ c_o(OC_LOR); }
break;
case 96:
#line 547 "gram.y"
{ CPType t = (CPType)PEEK(0);
					  *BRP(t) = (BrType)(PC-t);
					  POP();
					}
break;
case 99:
#line 556 "gram.y"
{ c_o(OC_PUSHC);
	                                  c_c(yyvsp[0].v);
					}
break;
case 100:
#line 562 "gram.y"
{ VRef r = vgr(yyvsp[0].s, pti.yyp.fpn,
	  							pti.yyp.nfpn);
	  				  if (r != BADVREF) {
					    c_o(OC_PUSHV);
					    c_r(r);
					    maxTmpv (r);
					  } else {
					    printf ("Bad var name: %s\n", yyvsp[0].s);
					    return (1);
					  }
					}
break;
case 101:
#line 576 "gram.y"
{ VRef r = vgr(yyvsp[0].s, pti.yyp.fpn,
	  							pti.yyp.nfpn);
	  				  if (r != BADVREF) {
					    c_o(OC_PUSHR);
					    c_r(r);
					    maxTmpv (r);
					  } else {
					    printf ("Bad var name: %s\n", yyvsp[0].s);
					    return (1);
					  }
					}
break;
case 102:
#line 591 "gram.y"
{ PUSH(0); /* init arg count */ }
break;
case 103:
#line 592 "gram.y"
{ if (compileCall (yyvsp[-4].s, POP()) < 0)
	  				    return (1);
					}
break;
case 108:
#line 608 "gram.y"
{ PEEK(0)++; /* another arg */ }
break;
#line 1293 "gram.tab.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
maxnyys();
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yyss + yystacksize - 1)
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
maxnyys();
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
