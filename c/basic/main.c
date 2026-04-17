/* https://github.com/jwillia3/BASIC */

#include <ctype.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYMSZ 16    /* SYMBOL SIZE */
#define PRGSZ 2048  /* PROGRAM SIZE */
#define STKSZ 256   /* STACK SIZE */
#define STRSZ 1024  /* STRING TABLE SIZE */
#define VARS 64     /* VARIABLE COUNT */
#define LOCS 8      /* LOCAL COUNT */
#define MAXLINES 50 /* MAX PROGRAM LINES */

typedef ptrdiff_t Val; /* SIGNED INT/POINTER */
typedef int (*Code)(); /* BYTE-CODE */

enum {
  NAME = 1,
  NUMBER,
  STRING,
  LP,
  RP,
  COMMA,
  ADD,
  SUBS,
  MUL,
  DIV,
  MOD,
  EQ,
  LT,
  GT,
  NE,
  LE,
  GE,
  AND,
  OR,
  PRINT,
  SUB,
  END,
  RETURN,
  LOCAL,
  WHILE,
  FOR,
  TO,
  IF,
  ELSE,
  THEN,
  DIM,
  UBOUND,
  BYE,
  BREAK,
  RESUME,
  REM
};
char *kwd[] = {"AND",    "OR",  "PRINT", "SUB",    "END",  "RETURN", "LOCAL",
               "WHILE",  "FOR", "TO",    "IF",     "ELSE", "THEN",   "DIM",
               "UBOUND", "BYE", "BREAK", "RESUME", "REM",  0};

/* Large arrays - NOT zeroed at startup (safe: always written before read) */
char lbuf[256] __attribute__((section(".noinit")));
char tokn[SYMSZ] __attribute__((section(".noinit")));
int (*prg[PRGSZ])() __attribute__((section(".noinit")));
int lmap[PRGSZ] __attribute__((section(".noinit")));
Val stk[STKSZ] __attribute__((section(".noinit")));
char name[VARS][SYMSZ] __attribute__((section(".noinit")));
int sub[VARS][LOCS + 2] __attribute__((section(".noinit")));
int cstk[STKSZ] __attribute__((section(".noinit")));
char stab[STRSZ] __attribute__((section(".noinit")));
struct {
  int linenum;
  char text[128];
} lines[MAXLINES] __attribute__((section(".noinit")));

/* Arrays that MUST be zero-initialized (BSS) */
Val value[VARS]; /* Variable values - read before write */
int mode[VARS];  /* Variable modes - checked before set */

/* Scalars - explicitly initialized */
char *lp;                   /* LEXER STATE */
int lnum, tok, tokv, ungot; /* LEXER STATE */
int (**pc)();               /* COMPILED PROGRAM */
int cpc = 0;                /* COMPILED PROGRAM - must be zero */
Val *sp;                    /* RUN-TIME STACK */
Val ret;                    /* FUNCTION RETURN VALUE */
int *csp;                   /* COMPILER STACK */
int nvar = 0, cursub = 0, temp = 0, compile = 0,
    ipc = 0;       /* COMPILER STATE - must be zero */
int (**opc)() = 0; /* COMPILER STATE */
char *stabp;       /* STRING TABLE */
jmp_buf trap;      /* TRAP ERRORS */
int nlines = 0;    /* Line count - must be zero */

#define A sp[1]                     /* LEFT OPERAND */
#define B sp[0]                     /* RIGHT OPERAND */
#define PCV ((Val) * pc++)          /* GET IMMEDIATE */
#define STEP return 1               /* CONTINUE RUNNING */
#define DRIVER while ((*pc++)())    /* RUN PROGRAM */
#define LOC(N) value[sub[v][N + 2]] /* SUBROUTINE LOCAL */

/* Forward declarations */
void err(char *msg);
int bad(char *msg);

Val *bound(Val *mem, int n) {
  if (!mem)
    err("NOT AN ARRAY");
  if (n < 1 || n > *mem)
    err("BOUNDS");
  return mem + n;
}

int (*kwdhook)(char *kwd);        /* KEYWORD HOOK */
int (*funhook)(char *kwd, int n); /* FUNCTION CALL HOOK */

void initbasic(int comp) {
  pc = prg;
  sp = stk + STKSZ;
  csp = cstk + STKSZ;
  stabp = stab;
  compile = comp;
}
int bad(char *msg) {
  printf("ERROR %d: %s\n", lnum, msg);
  longjmp(trap, 1);
}
void err(char *msg) {
  int idx = pc - prg - 1;
  if (idx >= 0 && idx < PRGSZ)
    printf("ERROR %d: %s\n", lmap[idx], msg);
  else
    printf("ERROR: %s\n", msg);
  longjmp(trap, 2);
}
void emit(int opcode()) {
  if (cpc >= PRGSZ)
    bad("PROGRAM TOO LONG");
  lmap[cpc] = lnum;
  prg[cpc++] = opcode;
}
void inst(int opcode(), Val x) {
  emit(opcode);
  emit((Code)x);
}
int BYE_() { longjmp(trap, 4); }
int BREAK_() { longjmp(trap, 3); }
int RESUME_() {
  pc = opc ? opc : pc;
  opc = pc;
  cpc = ipc;
  STEP;
}
int NUMBER_() {
  if (sp <= stk)
    err("STACK OVERFLOW");
  *--sp = PCV;
  STEP;
}
int LOAD_() {
  if (sp <= stk)
    err("STACK OVERFLOW");
  *--sp = value[PCV];
  STEP;
}
int STORE_() {
  value[PCV] = *sp++;
  STEP;
}
int ECHO_() {
  printf("%d\n", (int)*sp++);
  STEP;
}
int PRINT_() {
  char *f;
  Val n = PCV, *ap = (sp += n) - 1;
  for (f = stab + *sp++; *f; f++)
    if (*f == '%')
      printf("%d", (int)*ap--);
    else if (*f == '$')
      printf("%s", (char *)*ap--);
    else
      putchar(*f);
  putchar('\n');
  STEP;
}
int ADD_() {
  A += B;
  sp++;
  STEP;
}
int SUBS_() {
  A -= B;
  sp++;
  STEP;
}
int MUL_() {
  A *= B;
  sp++;
  STEP;
}
int DIV_() {
  if (!B)
    sp += 2, err("DIVISION BY ZERO");
  A /= B;
  sp++;
  STEP;
}
int MOD_() {
  if (!B)
    sp += 2, err("MODULUS OF ZERO");
  A %= B;
  sp++;
  STEP;
}
int EQ_() {
  A = (A == B) ? -1 : 0;
  sp++;
  STEP;
}
int LT_() {
  A = (A < B) ? -1 : 0;
  sp++;
  STEP;
}
int GT_() {
  A = (A > B) ? -1 : 0;
  sp++;
  STEP;
}
int NE_() {
  A = (A != B) ? -1 : 0;
  sp++;
  STEP;
}
int LE_() {
  A = (A <= B) ? -1 : 0;
  sp++;
  STEP;
}
int GE_() {
  A = (A >= B) ? -1 : 0;
  sp++;
  STEP;
}
int AND_() {
  A &= B;
  sp++;
  STEP;
}
int OR_() {
  A |= B;
  sp++;
  STEP;
}
int JMP_() {
  pc = prg + (int)*pc;
  STEP;
}
int FALSE_() {
  if (*sp++)
    pc++;
  else
    pc = prg + (int)*pc;
  STEP;
}
int FOR_() {
  if (value[PCV] >= *sp)
    pc = prg + (int)*pc, sp++;
  else
    PCV;
  STEP;
}
int NEXT_() {
  value[PCV]++;
  STEP;
}
int CALL_() {
  Val v = PCV, n = sub[v][1], x, *ap = sp;
  while (n--) {
    x = LOC(n);
    LOC(n) = *ap;
    *ap++ = x;
  }
  for (n = sub[v][1]; n < sub[v][0]; n++)
    *--sp = LOC(n);
  *--sp = pc - prg;
  pc = prg + value[v];
  STEP;
}
int RETURN_() {
  int v = PCV, n = sub[v][0];
  pc = prg + *sp++;
  while (n--)
    LOC(n) = *sp++;
  STEP;
}
int SETRET_() {
  ret = *sp++;
  STEP;
}
int RV_() {
  *--sp = ret;
  STEP;
}
int DROP_() {
  sp += PCV;
  STEP;
}
int DIM_() {
  int v = PCV, n = *sp++;
  if (n < 1)
    err("BAD DIM SIZE");
  Val *mem = calloc(sizeof(Val), n + 1);
  if (!mem)
    err("OUT OF MEMORY");
  mem[0] = n;
  value[v] = (Val)mem;
  STEP;
}
int LOADI_() {
  Val x = *sp++;
  x = *bound((Val *)value[PCV], x);
  *--sp = x;
  STEP;
}
int STOREI_() {
  Val x = *sp++, i = *sp++;
  *bound((Val *)value[PCV], i) = x;
  STEP;
}
int UBOUND_() {
  Val *mem = (Val *)value[PCV];
  if (!mem)
    err("NOT AN ARRAY");
  if (sp <= stk)
    err("STACK OVERFLOW");
  *--sp = *mem;
  STEP;
}

int find(char *var) {
  int i;
  for (i = 0; i < nvar && strcmp(var, name[i]); i++)
    ;
  if (i == nvar) {
    if (nvar >= VARS)
      bad("TOO MANY VARIABLES");
    strcpy(name[nvar++], var);
  }
  return i;
}

int read() { /* READ TOKEN */
  char *p, *d, **k, *pun = "(),+-*/%=<>", *dub = "<><==>";
  if (ungot)
    return ungot = 0, tok; /* UNGOT PREVIOUS */
  while (isspace(*lp))
    lp++; /* SKIP SPACE */
  if (!*lp || *lp == '#')
    return tok = 0; /* END OF LINE */
  if (isdigit(*lp)) /* NUMBER */
    return tokv = strtol(lp, &lp, 0), tok = NUMBER;
  if ((p = strchr(pun, *lp)) && lp++) { /* PUNCTUATION */
    for (d = dub; *d && strncmp(d, lp - 1, 2); d += 2)
      ;
    if (!*d)
      return tok = (p - pun) + LP;
    return lp++, tok = (d - dub) / 2 + NE;
  } else if (isalpha(*lp)) { /* IDENTIFIER */
    for (p = tokn; isalnum(*lp);)
      if (p < tokn + SYMSZ - 1)
        *p++ = toupper(*lp++);
      else
        lp++;
    for (*p = 0, k = kwd; *k && strcmp(tokn, *k); k++)
      ;
    if (*k) {
      int token = (k - kwd) + AND;
      if (token == REM)
        return tok = 0; /* REM is a comment, treat as end of line */
      return tok = token;
    }
    return tokv = find(tokn), tok = NAME;
  } else if (*lp == '"' && lp++) { /* STRING */
    for (p = stabp; *lp && *lp != '"';) {
      if (stabp >= stab + STRSZ - 1)
        bad("STRING TOO LONG");
      *stabp++ = *lp++;
    }
    *stabp++ = 0;
    if (*lp != '"')
      bad("UNTERMINATED STRING");
    lp++;
    return tokv = p - stab, tok = STRING;
  } else
    return bad("BAD TOKEN");
}

int want(int type) { return !(ungot = read() != type); }
void need(int type) {
  if (!want(type))
    bad("SYNTAX ERROR");
}
#define LIST(BODY)                                                             \
  if (!want(0))                                                                \
    do {                                                                       \
      BODY;                                                                    \
  } while (want(COMMA))

void base();
void expr();

void base() { /* BASIC EXPRESSION */
  int neg = want(SUBS) ? (inst(NUMBER_, 0), 1) : 0;
  if (want(NUMBER))
    inst(NUMBER_, tokv);
  else if (want(STRING))
    inst(NUMBER_, (Val)(stab + tokv));
  else if (want(NAME)) {
    int var = tokv;
    if (want(LP))
      if (mode[var] == 1) /* DIM */
        expr(), need(RP), inst(LOADI_, var);
      else {
        int n = 0;
        LIST(if (tok == RP) break; expr(); n++);
        need(RP);
        if (!funhook || !funhook(name[var], n)) {
          if (mode[var] != 2 || n != sub[var][1])
            bad("BAD SUB/ARG COUNT");
          inst(CALL_, var);
          emit(RV_);
        }
      }
    else
      inst(LOAD_, var);
  } else if (want(LP))
    expr(), need(RP);
  else if (want(UBOUND))
    need(LP), need(NAME), need(RP), inst(UBOUND_, tokv);
  else
    bad("BAD EXPRESSION");
  if (neg)
    emit(SUBS_); /* NEGATE */
}

int (*bin[])() = {ADD_, SUBS_, MUL_, DIV_, MOD_, EQ_, LT_,
                  GT_,  NE_,   LE_,  GE_,  AND_, OR_};
#define BIN(NAME, LO, HI, ELEM)                                                \
  void NAME() {                                                                \
    int (*o)();                                                                \
    ELEM();                                                                    \
    while (want(0), LO <= tok && tok <= HI)                                    \
      o = bin[tok - ADD], read(), ELEM(), emit(o);                             \
  }
BIN(factor, MUL, MOD, base)
BIN(addition, ADD, SUBS, factor)
BIN(relation, EQ, GE, addition)
BIN(expr, AND, OR, relation)

void cpush(int v) {
  if (csp <= cstk)
    bad("NESTING TOO DEEP");
  *--csp = v;
}

void stmt() { /* STATEMENT */
  int n, var;
  switch (read()) {
  case PRINT:
    need(STRING), inst(NUMBER_, tokv);
    n = 0;
    if (want(COMMA))
      LIST(expr(); n++);
    inst(PRINT_, n);
    break;
  case SUB: /* CSTK: {SUB,INDEX,JMP} */
    if (!compile)
      bad("SUB MUST BE COMPILED");
    compile++;                                 /* MUST BALANCE WITH END */
    need(NAME), mode[cursub = var = tokv] = 2; /* SUB NAME */
    n = 0;
    LIST(need(NAME); sub[var][n++ + 2] = tokv); /* PARAMS */
    cpush(cpc + 1), inst(JMP_, 0);              /* JUMP OVER CODE */
    sub[var][0] = sub[var][1] = n;              /* LOCAL=PARAM COUNT */
    value[var] = cpc;                           /* ADDRESS */
    cpush(var), cpush(SUB);                     /* FOR "END" CLAUSE */
    break;
  case LOCAL:
    LIST(need(NAME); sub[cursub][sub[cursub][0]++ + 2] = tokv;);
    break;
  case RETURN:
    if (temp)
      inst(DROP_, temp);
    if (!want(0))
      expr(), emit(SETRET_);
    inst(RETURN_, cursub);
    break;
  case WHILE:  /* CSTK: {WHILE,TEST-FALSE,TOP} */
    compile++; /* BODY IS COMPILED */
    cpush(cpc), expr();
    cpush(cpc + 1), cpush(WHILE), inst(FALSE_, 0);
    break;
  case FOR:    /* CSTK: {FOR,TEST-FALSE,I,TOP}; STK:{HI} */
    compile++; /* BODY IS COMPILED */
    need(NAME), var = tokv, temp++;
    need(EQ), expr(), inst(STORE_, var);
    need(TO), expr();
    cpush(cpc), inst(FOR_, var), emit(0);
    cpush(var), cpush(cpc - 1), cpush(FOR);
    break;
  case IF: /* CSTK: {IF,N,ENDS...,TEST-FALSE} */
    expr(), inst(FALSE_, 0), cpush(cpc - 1);
    if (want(THEN)) {
      stmt();
      prg[*csp++] = (Code)cpc;
    } else
      compile++, cpush(0), cpush(IF);
    break;
  case ELSE:
    if (csp >= cstk + STKSZ || csp[0] != IF)
      bad("ELSE WITHOUT IF");
    n = csp[1] + 1;
    inst(JMP_, 0);                             /* JUMP OVER "ELSE" */
    cpush(IF), csp[1] = n, csp[2] = cpc - 1;  /* ADD A FIXUP */
    prg[csp[2 + n]] = (Code)cpc;               /* PATCH "ELSE" */
    csp[2 + n] = !want(IF) ? 0 :               /* "ELSE IF" */
                     (expr(), inst(FALSE_, 0), cpc - 1);
    break;
  case END:
    if (csp >= cstk + STKSZ)
      bad("UNMATCHED END");
    need(*csp++), compile--; /* MATCH BLOCK */
    if (csp[-1] == SUB) {
      inst(RETURN_, *csp++);
      prg[*csp++] = (Code)cpc; /* PATCH JUMP */
    } else if (csp[-1] == WHILE) {
      prg[*csp++] = (Code)(cpc + 2); /* PATCH TEST */
      inst(JMP_, *csp++);            /* LOOP TO TEST */
    } else if (csp[-1] == FOR) {
      prg[*csp++] = (Code)(cpc + 4); /* PATCH TEST */
      inst(NEXT_, *csp++);           /* INCREMENT */
      inst(JMP_, *csp++);            /* LOOP TO TEST */
      temp--;                        /* ONE LESS TEMP */
    } else if (csp[-1] == IF) {
      for (n = *csp++; n--;) /* PATCH BLOCK ENDS */
        prg[*csp++] = (Code)cpc;
      if ((n = *csp++))
        prg[n] = (Code)cpc; /* PATCH "ELSE" */
    }
    break;
  case NAME:
    var = tokv;
    if (want(EQ))
      expr(), inst(STORE_, var);
    else if (want(LP))
      expr(), need(RP), need(EQ), expr(), inst(STOREI_, var);
    else if (!kwdhook || !kwdhook(tokn)) {
      int n = 0;
      LIST(expr(); n++);
      if (!funhook || !funhook(name[var], n)) {
        if (mode[var] != 2 || n != sub[var][1])
          bad("BAD SUB/ARG COUNT");
        inst(CALL_, var);
      }
    }
    break;
  case DIM:
    need(NAME), mode[var = tokv] = 1; /* SET VAR MODE TO DIM */
    need(LP), expr(), need(RP), inst(DIM_, var);
    break;
  case RESUME:
    if (!want(0))
      expr();
    emit(RESUME_);
    break;
  case BREAK:
    emit(BREAK_);
    break;
  case BYE:
    emit(BYE_);
    break;
  case GT:
    expr();
    emit(ECHO_);
    break;
  default:
    if (tok)
      bad("BAD STATEMENT");
  }
  if (!want(0))
    bad("TOKENS AFTER STATEMENT");
}

/* REPL Commands */
void cmd_list() {
  for (int i = 0; i < nlines; i++) {
    printf("%d %s\n", lines[i].linenum, lines[i].text);
  }
}

void cmd_new() {
  nlines = 0;
  nvar = 0;
  cpc = 0;
  ipc = 0;
  compile = 0;
  stabp = stab;
  memset(value, 0, sizeof(value));
  memset(mode, 0, sizeof(mode));
  printf("READY\n");
}

void cmd_run() {
  /* Reset all compiler state for clean recompile */
  cpc = 0;
  nvar = 0;
  cursub = 0;
  temp = 0;
  opc = 0;
  stabp = stab;
  memset(value, 0, sizeof(value));
  memset(mode, 0, sizeof(mode));
  initbasic(1);

  int code = setjmp(trap);
  if (code == 0) {
    /* Compile all stored lines */
    for (int i = 0; i < nlines; i++) {
      lnum = lines[i].linenum;
      lp = lines[i].text;
      ungot = 0;
      stmt();
    }

    /* Check all blocks closed */
    if (compile != 1)
      bad("UNCLOSED BLOCK");

    /* Run the compiled program */
    ipc = cpc;
    compile = 0;
    emit(BYE_);
    pc = prg;
    sp = stk + STKSZ;
    DRIVER;
  }
  if (code == 1) {
    /* compile error - already printed */
  } else if (code == 2) {
    /* runtime error - already printed */
  } else if (code == 3) {
    printf("BREAK\n");
  }
}

void store_line(int linenum, char *text) {
  /* Remove line if text is empty */
  if (!*text || *text == '\n') {
    int found = -1;
    for (int i = 0; i < nlines; i++) {
      if (lines[i].linenum == linenum) {
        found = i;
        break;
      }
    }
    if (found >= 0) {
      for (int i = found; i < nlines - 1; i++) {
        lines[i] = lines[i + 1];
      }
      nlines--;
    }
    return;
  }

  /* Find insertion point */
  int i;
  for (i = 0; i < nlines && lines[i].linenum < linenum; i++)
    ;

  /* Replace existing line */
  if (i < nlines && lines[i].linenum == linenum) {
    strncpy(lines[i].text, text, sizeof(lines[i].text) - 1);
    lines[i].text[sizeof(lines[i].text) - 1] = 0;
    return;
  }

  /* Insert new line */
  if (nlines < MAXLINES) {
    for (int j = nlines; j > i; j--) {
      lines[j] = lines[j - 1];
    }
    lines[i].linenum = linenum;
    strncpy(lines[i].text, text, sizeof(lines[i].text) - 1);
    lines[i].text[sizeof(lines[i].text) - 1] = 0;
    nlines++;
  } else {
    printf("OUT OF MEMORY\n");
  }
}

int cmdmatch(char *input, char *cmd) {
  while (*cmd)
    if (toupper(*input++) != *cmd++)
      return 0;
  return !isalnum(*input);
}

int main() {
  printf("BASIC INTERPRETER\n");
  printf("COMMANDS: LIST, NEW, RUN, BYE\n\n");

  initbasic(0);

  for (;;) {
    printf("> ");
    fflush(stdout);

    if (!fgets(lbuf, sizeof(lbuf), stdin))
      break;

    lp = lbuf;

    /* Trim newline */
    char *nl = strchr(lbuf, '\n');
    if (nl)
      *nl = 0;

    /* Skip empty lines */
    while (isspace(*lp))
      lp++;
    if (!*lp)
      continue;

    /* Check for commands (case-insensitive, word-boundary) */
    if (cmdmatch(lp, "LIST")) {
      cmd_list();
      continue;
    }
    if (cmdmatch(lp, "NEW")) {
      cmd_new();
      continue;
    }
    if (cmdmatch(lp, "RUN")) {
      cmd_run();
      continue;
    }
    if (cmdmatch(lp, "BYE")) {
      break;
    }

    /* Lines must start with a number */
    if (isdigit(*lp)) {
      int linenum = strtol(lp, &lp, 10);
      while (isspace(*lp))
        lp++;
      store_line(linenum, lp);
    } else {
      printf("LINE NUMBER REQUIRED\n");
    }
  }

  printf("BYE\n");
  return 0;
}
