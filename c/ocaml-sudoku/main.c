#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Memory configuration */
#define HEAP_SIZE_BYTES (2 * 1024)
#define STACK_SIZE_BYTES (32 * 1024)
#define TRAP_STACK_SIZE 64

typedef uintptr_t value;
typedef uintptr_t uintnat;
typedef intptr_t intnat;
typedef unsigned char uchar;

#define HEAP_SIZE (HEAP_SIZE_BYTES / sizeof(value))
#define STACK_SIZE (STACK_SIZE_BYTES / sizeof(value))

#define Is_int(v) (((v) & 1) != 0)
#define Is_block(v) (((v) & 1) == 0)

#define Val_int(x) (((value)(x) << 1) | 1)
#define Int_val(v) ((intnat)(v) >> 1)

#define Val_bool(x) ((x) ? Val_int(1) : Val_int(0))
#define Bool_val(v) (Int_val(v) != 0)

#define Val_unit Val_int(0)

#define Tag_closure 247
#define Tag_no_scan 251
#define Tag_string 252

/* Block header: tag (8 bits) | mark (1 bit) | size (remaining bits) */
#define Make_header(sz, tag) ((uintnat)(tag) | ((uintnat)(sz) << 9))
#define Header_tag(h) ((h) & 0xFF)
#define Header_size(h) ((h) >> 9)
#define Header_marked(h) (((h) >> 8) & 1)
#define Header_set_mark(h) ((h) | (1 << 8))
#define Header_clear_mark(h) ((h) & ~(uintnat)(1 << 8))

#define Field(v, i) (((value *)(v))[1 + (i)])
#define Header(v) (((value *)(v))[0])
#define Tag_val(v) Header_tag(Header(v))
#define Size_val(v) Header_size(Header(v))

/* Heap */
static value heap[HEAP_SIZE];
static value *hp = heap;

/* Stack */
static value stack[STACK_SIZE];
value *bp = stack;
value *sp = stack;

static void check_stack(intnat n) {
  if (sp + n > stack + STACK_SIZE) {
    printf("Stack overflow (%lu bytes)\n", (unsigned long)STACK_SIZE_BYTES);
    exit(1);
  }
}

void reserve_stack(intnat n) {
  check_stack(n);
  memset(sp, 1, n * sizeof(value)); /* 1 looks like int to GC */
  sp += n;
}

/* Exception handling */
typedef struct {
  jmp_buf buf;
  value *sp;
  value *bp;
} trap_frame;

trap_frame trap_stack[TRAP_STACK_SIZE];
trap_frame *trap_sp = trap_stack;
value exn_value = 0;

static void check_trap_stack(void) {
  if (trap_sp >= trap_stack + TRAP_STACK_SIZE) {
    printf("Trap stack overflow\n");
    exit(1);
  }
}

static void caml_raise(value exn) {
  if (trap_sp == trap_stack) {
    printf("Uncaught exception\n");
    exit(2);
  }
  trap_sp--;
  exn_value = exn;
  sp = trap_sp->sp;
  bp = trap_sp->bp;
  longjmp(trap_sp->buf, 1);
}

/* Closures - declared early for GC */
typedef struct {
  value (*fun)(value *);
  uintnat args_idx;
  uintnat total_args;
  value args[];
} closure_t;

#define Closure_data(v) ((closure_t *)&Field(v, 0))

static void mark(value v) {
  value *p;
  uintnat h, size, i;
  uchar tag;
  closure_t *c;
  if (Is_int(v))
    return;
  p = (value *)v;
  if (p < heap || p >= hp)
    return;
  h = *p;
  if (Header_marked(h))
    return;
  *p = Header_set_mark(h);
  size = Header_size(h);
  tag = Header_tag(h);
  if (tag == Tag_closure) {
    c = Closure_data(v);
    for (i = 0; i < c->args_idx; i++)
      mark(c->args[i]);
  } else if (tag < Tag_no_scan) {
    for (i = 0; i < size; i++)
      mark(Field(v, i));
  }
}

static value forward(value v) {
  value *p, *src, *dst;
  uintnat h, size;
  if (Is_int(v))
    return v;
  p = (value *)v;
  if (p < heap || p >= hp)
    return v;
  dst = heap;
  for (src = heap; src < p;) {
    h = *src;
    size = Header_size(h);
    if (Header_marked(h))
      dst += 1 + size;
    src += 1 + size;
  }
  return (value)dst;
}

static void compact(void) {
  value *src, *dst;
  uintnat h, size, words, i;
  uchar tag;
  closure_t *c;

  /* Phase 1: Update roots */
  for (src = stack; src < sp; src++)
    *src = forward(*src);
  if (exn_value)
    exn_value = forward(exn_value);

  /* Phase 2: Update pointers in live objects */
  for (src = heap; src < hp;) {
    h = *src;
    size = Header_size(h);
    if (Header_marked(h)) {
      tag = Header_tag(h);
      if (tag == Tag_closure) {
        c = (closure_t *)&src[1];
        for (i = 0; i < c->args_idx; i++)
          c->args[i] = forward(c->args[i]);
      } else if (tag < Tag_no_scan) {
        for (i = 0; i < size; i++)
          src[1 + i] = forward(src[1 + i]);
      }
    }
    src += 1 + size;
  }

  /* Phase 3: Slide objects down */
  dst = heap;
  for (src = heap; src < hp;) {
    h = *src;
    size = Header_size(h);
    words = 1 + size;
    if (Header_marked(h)) {
      if (dst != src)
        memmove(dst, src, words * sizeof(value));
      *dst = Header_clear_mark(h);
      dst += words;
    }
    src += words;
  }
  hp = dst;
}

static void gc(void) {
  value *p;
  for (p = stack; p < sp; p++)
    mark(*p);
  if (exn_value)
    mark(exn_value);
  compact();
}

static value *alloc(uintnat size, uchar tag) {
  uintnat words = 1 + size;
  value *block;
  if (hp + words > heap + HEAP_SIZE) {
    gc();
    if (hp + words > heap + HEAP_SIZE) {
      printf("Out of heap memory (%lu bytes)\n",
             (unsigned long)HEAP_SIZE_BYTES);
      exit(1);
    }
  }
  block = hp;
  hp += words;
  *block = Make_header(size, tag);
  memset(block + 1, 1, size * sizeof(value)); /* 1 looks like int to GC */
  return block;
}

value caml_alloc(uchar tag, intnat size, ...) {
  va_list args;
  value *saved_sp = sp;
  value *block;
  intnat i;

  /* Push args to stack as GC roots */
  va_start(args, size);
  check_stack(size);
  for (i = 0; i < size; i++)
    *(sp++) = va_arg(args, value);
  va_end(args);

  block = alloc(size, tag);

  for (i = 0; i < size; i++)
    block[1 + i] = saved_sp[i];
  sp = saved_sp;

  return (value)block;
}

value caml_alloc_closure(value (*fun)(value *), uintnat num_args,
                         uintnat num_env) {
  uintnat data_size =
      (sizeof(closure_t) + (num_args + num_env) * sizeof(value) +
       sizeof(value) - 1) /
      sizeof(value);
  value *block;
  closure_t *c;
  block = alloc(data_size, Tag_closure);
  c = (closure_t *)&block[1];
  c->fun = fun;
  c->args_idx = 0;
  c->total_args = num_args + num_env;
  return (value)block;
}

void add_arg(value closure, value arg) {
  closure_t *c = Closure_data(closure);
  c->args[c->args_idx++] = arg;
}

static value caml_call_with_args(value closure, uintnat num_args,
                                 value *args_array) {
  closure_t *c = Closure_data(closure);
  closure_t *new_c;
  /* Cache before potential GC */
  value (*fun)(value *) = c->fun;
  uintnat closure_args_idx = c->args_idx;
  uintnat total_args = c->total_args;
  uintnat total_provided = closure_args_idx + num_args;
  uintnat i, excess;
  value *args_on_stack, *prev_bp;
  value result;

  check_stack(total_provided);

  args_on_stack = sp;
  for (i = 0; i < closure_args_idx; i++)
    *(sp++) = c->args[i];
  for (i = 0; i < num_args; i++)
    *(sp++) = args_array[i];

  if (total_provided >= total_args) {
    prev_bp = bp;
    bp = sp;
    result = fun(args_on_stack);
    sp = bp;
    bp = prev_bp;

    if (total_provided > total_args) {
      excess = total_provided - total_args;
      result = caml_call_with_args(result, excess, args_on_stack + total_args);
    }
  } else {
    result =
        caml_alloc_closure(fun, total_args - total_provided, total_provided);
    new_c = Closure_data(result);
    memcpy(new_c->args, args_on_stack, total_provided * sizeof(value));
    new_c->args_idx = total_provided;
  }

  sp = args_on_stack;
  return result;
}

value caml_call(value closure, uintnat num_args, ...) {
  va_list args;
  uintnat i;
  value *args_on_stack = sp;
  value result;

  check_stack(num_args);
  va_start(args, num_args);
  for (i = 0; i < num_args; i++)
    *(sp++) = va_arg(args, value);
  va_end(args);

  result = caml_call_with_args(closure, num_args, args_on_stack);
  sp = args_on_stack;
  return result;
}

/* Strings */
#define String_length(v) Int_val(Field(v, 0))
#define String_data(v) ((char *)&Field(v, 1))

static value alloc_string(uintnat len) {
  uintnat data_words = (len + sizeof(value)) / sizeof(value);
  value *block = alloc(1 + data_words, Tag_string);
  block[1] = Val_int(len);
  return (value)block;
}

value caml_copy_string(const char *s) {
  uintnat len = strlen(s);
  value v = alloc_string(len);
  memcpy(String_data(v), s, len + 1);
  return v;
}

value caml_create_bytes(value len) {
  uintnat n = Int_val(len);
  value v = alloc_string(n);
  String_data(v)[n] = '\0';
  return v;
}

value caml_ml_string_length(value s) { return Field(s, 0); }
value caml_ml_bytes_length(value s) { return Field(s, 0); }

value caml_string_unsafe_get(value s, value i) {
  return Val_int((uchar)String_data(s)[Int_val(i)]);
}

value caml_bytes_unsafe_get(value s, value i) {
  return Val_int((uchar)String_data(s)[Int_val(i)]);
}

value caml_bytes_unsafe_set(value s, value i, value c) {
  String_data(s)[Int_val(i)] = Int_val(c);
  return Val_unit;
}

value caml_string_of_bytes(value s) { return s; }
value caml_bytes_of_string(value s) { return s; }

value caml_string_notequal(value s1, value s2) {
  uintnat len1 = String_length(s1);
  uintnat len2 = String_length(s2);
  if (len1 != len2)
    return Val_bool(1);
  return Val_bool(memcmp(String_data(s1), String_data(s2), len1) != 0);
}

value caml_string_concat(value s1, value s2) {
  uintnat len1 = String_length(s1);
  uintnat len2 = String_length(s2);
  value result = caml_create_bytes(Val_int(len1 + len2));
  memcpy(String_data(result), String_data(s1), len1);
  memcpy(String_data(result) + len1, String_data(s2), len2);
  return result;
}

value caml_blit_bytes(value src, value src_pos, value dst, value dst_pos,
                      value len) {
  memcpy(String_data(dst) + Int_val(dst_pos),
         String_data(src) + Int_val(src_pos), Int_val(len));
  return Val_unit;
}

/* I/O */
value caml_putc(value c) {
  putchar(Int_val(c));
  return Val_unit;
}

value caml_getc(value unit) {
  (void)unit;
  return Val_int(getchar());
}

/* Misc */
value caml_int_compare(value a, value b) {
  intnat x = Int_val(a), y = Int_val(b);
  return Val_int((x > y) - (x < y));
}

value caml_exit(value code) { exit(Int_val(code)); }
value caml_register_global(value a, value b, value c) {
  (void)a;
  (void)b;
  (void)c;
  return Val_unit;
}
value caml_ensure_stack_capacity(value n) {
  (void)n;
  return Val_unit;
}

static intnat fresh_oo_id = 0;
value caml_fresh_oo_id(value unit) {
  (void)unit;
  return Val_int(fresh_oo_id++);
}
value c464(value *env);
value c3488(value *env);
value c3191(value *env);
value c312(value *env);
value c3156(value *env);
value c0(value *env);
value s_64835407;
value s_632290335;
value s_176434974;
value s_294329037;
value s_252069304;
value s_834731544;
value s_671588152;
value s_186459799;
value s_154408390;
value s_208627236;
value s_656651812;
value s_756918818;
value s_114338481;
value s_979516401;
value s_417075888;
value s_0;
value c464(value *env) {
  reserve_stack(4);
  bp[2] = env[0];
  bp[3] = env[1];

b464:;

  goto b3600;
b3600:;

  goto b475;
b475:;
  bp[1] = Val_int(Int_val(bp[2]) - Int_val(bp[3]));

  goto b3599;
b3599:;
  bp[0] = Val_bool(Val_int(0L) == bp[1]);
  return bp[0];
}

value c3488(value *env) {
  reserve_stack(5);
  bp[1] = env[0];
  bp[2] = env[1];
  bp[3] = env[2];
  bp[4] = env[3];

b3488:;
  bp[0] = caml_bytes_unsafe_set(bp[2], bp[3], bp[4]);
  return bp[1];
}

value c3191(value *env) {
  reserve_stack(60);
  bp[54] = env[0];
  bp[55] = env[1];
  bp[56] = env[2];
  bp[57] = env[3];
  bp[58] = env[4];
  bp[59] = env[5];

  value t59_0 = bp[59];
  bp[30] = t59_0;

b3191:;
  bp[12] = Val_bool(Val_int(81L) == bp[30]);
  if (Bool_val(bp[12])) {
    goto b3195;
  } else {
    goto b3200;
  }
b3195:;

  goto b3570;
b3570:;

  goto b3328;
b3328:;
  bp[10] = caml_putc(Val_int(10L));
  bp[24] = Val_int(0L);

  value t9_0 = bp[24];
  bp[13] = t9_0;

  goto b3340;
b3340:;

  goto b3569;
b3569:;

  goto b3567;
b3567:;
  bp[52] = caml_bytes_unsafe_get(bp[58], bp[13]);

  goto b3568;
b3568:;
  bp[53] = caml_putc(bp[52]);
  bp[26] = Val_int(Int_val(bp[13]) + Int_val(Val_int(1L)));
  bp[34] = Val_int(Int_val(bp[26]) % Int_val(Val_int(9L)));
  bp[14] = Val_bool(Val_int(0L) == bp[34]);
  if (Bool_val(bp[14])) {
    goto b3357;
  } else {
    goto b3361;
  }
b3357:;
  bp[46] = caml_putc(Val_int(10L));

  goto b3361;
b3361:;
  bp[27] = Val_int(Int_val(bp[13]) + Int_val(Val_int(1L)));
  bp[48] = Val_bool(Val_int(80L) != bp[13]);
  if (Bool_val(bp[48])) {
    value t16_0 = bp[27];
    bp[13] = t16_0;

    goto b3340;
  } else {
    goto b3371;
  }

b3371:;
  bp[2] = caml_exit(Val_int(0L));
  return bp[54];

b3200:;
  bp[29] = Val_int(48L);

  goto b3574;
b3574:;

  goto b3497;
b3497:;
  bp[18] = caml_bytes_unsafe_get(bp[58], bp[30]);

  goto b3573;
b3573:;
  bp[36] = caml_call(bp[55], 2, bp[18], bp[29]);
  if (Bool_val(bp[36])) {
    goto b3219;
  } else {
    goto b3212;
  }
b3219:;
  bp[8] = Val_int(1L);

  value t23_0 = bp[8];
  bp[4] = t23_0;

  goto b3227;
b3227:;
  bp[6] = Val_int(Int_val(Val_int(48L)) + Int_val(bp[4]));
  bp[31] = caml_call(bp[56], 3, bp[58], bp[30], bp[6]);

  goto b3572;
b3572:;

  goto b3469;
b3469:;

  goto b3566;
b3566:;

  goto b3564;
b3564:;
  bp[1] = caml_bytes_unsafe_get(bp[58], bp[30]);

  goto b3565;
b3565:;
  bp[35] = Val_int(0L);

  goto b3563;
b3563:;
  value t30_0 = bp[35];
  bp[9] = t30_0;

  goto b3379;
b3379:;
  bp[47] = Val_bool(Int_val(bp[9]) < Int_val(Val_int(81L)));
  if (Bool_val(bp[47])) {
    goto b3385;
  } else {
    value t32_0 = bp[47];
    bp[3] = t32_0;

    goto b3464;
  }
b3385:;
  bp[42] = Val_int(Int_val(bp[9]) / Int_val(Val_int(9L)));
  bp[7] = Val_int(Int_val(bp[30]) / Int_val(Val_int(9L)));
  bp[32] = Val_bool(bp[7] == bp[42]);
  if (Bool_val(bp[32])) {
    value t33_0 = bp[32];
    bp[22] = t33_0;

    goto b3435;
  } else {
    goto b3397;
  }
b3435:;
  bp[49] = Val_bool(bp[30] != bp[9]);
  if (Bool_val(bp[49])) {
    goto b3441;
  } else {
    value t36_0 = bp[49];
    bp[41] = t36_0;

    goto b3454;
  }
b3441:;
  if (Bool_val(bp[22])) {
    goto b3444;
  } else {
    value t38_0 = bp[22];
    bp[41] = t38_0;

    goto b3454;
  }
b3444:;

  goto b3562;
b3562:;

  goto b3560;
b3560:;
  bp[43] = caml_bytes_unsafe_get(bp[58], bp[9]);

  goto b3561;
b3561:;
  bp[40] = caml_call(bp[55], 2, bp[1], bp[43]);
  value t42_0 = bp[40];
  bp[41] = t42_0;

  goto b3454;
b3454:;
  if (Bool_val(bp[41])) {
    goto b3462;
  } else {
    goto b3456;
  }
b3462:;
  value t45_0 = bp[41];
  bp[3] = t45_0;

  goto b3464;
b3464:;

  goto b3571;
b3571:;
  bp[50] = Val_bool(!Bool_val(bp[3]));
  if (Bool_val(bp[50])) {
    goto b3244;
  } else {
    goto b3250;
  }
b3244:;
  bp[23] = Val_int(Int_val(bp[30]) + Int_val(Val_int(1L)));
  bp[45] = caml_call(bp[57], 2, bp[58], bp[23]);

  goto b3250;
b3250:;
  bp[51] = Val_int(Int_val(bp[4]) + Int_val(Val_int(1L)));
  bp[19] = Val_bool(Val_int(9L) != bp[4]);
  if (Bool_val(bp[19])) {
    value t50_0 = bp[51];
    bp[4] = t50_0;

    goto b3227;
  } else {
    goto b3260;
  }

b3260:;
  bp[5] = Val_int(48L);

  bp[44] = caml_call(bp[56], 3, bp[58], bp[30], bp[5]);
  return bp[44];

b3456:;
  bp[39] = Val_int(Int_val(bp[9]) + Int_val(Val_int(1L)));
  value t52_0 = bp[39];
  bp[9] = t52_0;

  goto b3379;

b3397:;
  bp[25] = Val_int(Int_val(bp[9]) % Int_val(Val_int(9L)));
  bp[15] = Val_int(Int_val(bp[30]) % Int_val(Val_int(9L)));
  bp[11] = Val_bool(bp[15] == bp[25]);
  if (Bool_val(bp[11])) {
    value t53_0 = bp[11];
    bp[22] = t53_0;

    goto b3435;
  } else {
    goto b3409;
  }

b3409:;
  bp[28] = Val_int(Int_val(bp[9]) / Int_val(Val_int(27L)));
  bp[33] = Val_int(Int_val(bp[30]) / Int_val(Val_int(27L)));
  bp[0] = Val_bool(bp[33] == bp[28]);
  if (Bool_val(bp[0])) {
    goto b3421;
  } else {
    value t56_0 = bp[0];
    bp[22] = t56_0;

    goto b3435;
  }
b3421:;
  bp[17] = Val_int(Int_val(bp[9]) % Int_val(Val_int(9L)));
  bp[20] = Val_int(Int_val(bp[17]) / Int_val(Val_int(3L)));
  bp[21] = Val_int(Int_val(bp[30]) % Int_val(Val_int(9L)));
  bp[37] = Val_int(Int_val(bp[21]) / Int_val(Val_int(3L)));
  bp[38] = Val_bool(bp[37] == bp[20]);
  value t57_0 = bp[38];
  bp[22] = t57_0;

  goto b3435;

b3212:;
  bp[16] = Val_int(Int_val(bp[30]) + Int_val(Val_int(1L)));
  value t58_0 = bp[16];
  bp[30] = t58_0;

  goto b3191;
}

value c312(value *env) {
  reserve_stack(4);
  bp[3] = env[0];

b312:;
  bp[0] = caml_ml_bytes_length(bp[3]);
  bp[2] = caml_create_bytes(bp[0]);
  bp[1] = caml_blit_bytes(bp[3], Val_int(0L), bp[2], Val_int(0L), bp[0]);
  return bp[2];
}

value c3156(value *env) {
  reserve_stack(12);
  bp[10] = env[0];
  bp[11] = env[1];

b3156:;

  goto b3583;
b3583:;

  goto b2970;
b2970:;
  bp[5] = caml_ml_string_length(bp[11]);
  bp[7] = Val_int(0L);

  bp[8] = Val_int(Int_val(bp[5]) + Int_val(Val_int(-1L)));
  bp[4] = Val_bool(Int_val(bp[8]) < Int_val(Val_int(0L)));
  if (Bool_val(bp[4])) {
    goto b2998;
  } else {
    value t64_0 = bp[7];
    bp[2] = t64_0;

    goto b2982;
  }
b2998:;

  goto b3582;
b3582:;
  bp[0] = caml_putc(Val_int(10L));
  return bp[10];

b2982:;
  bp[1] = caml_string_unsafe_get(bp[11], bp[2]);

  goto b3606;
b3606:;

  goto b3151;
b3151:;
  bp[3] = caml_putc(bp[1]);

  goto b3605;
b3605:;
  bp[6] = Val_int(Int_val(bp[2]) + Int_val(Val_int(1L)));
  bp[9] = Val_bool(bp[8] != bp[2]);
  if (Bool_val(bp[9])) {
    value t69_0 = bp[6];
    bp[2] = t69_0;

    goto b2982;
  } else {
    goto b2998;
  }
}

value c0(value *env) {
  reserve_stack(79);

b0:;
  bp[34] = Val_unit;
  bp[5] = caml_alloc(248, 2, s_834731544, Val_int(-1L));

  bp[69] = caml_alloc(248, 2, s_114338481, Val_int(-2L));

  bp[54] = caml_alloc(248, 2, s_176434974, Val_int(-3L));

  bp[66] = caml_alloc(248, 2, s_656651812, Val_int(-4L));

  bp[68] = caml_alloc(248, 2, s_756918818, Val_int(-5L));

  bp[49] = caml_alloc(248, 2, s_154408390, Val_int(-6L));

  bp[24] = caml_alloc(248, 2, s_294329037, Val_int(-7L));

  bp[45] = caml_alloc(248, 2, s_979516401, Val_int(-8L));

  bp[4] = caml_alloc(248, 2, s_208627236, Val_int(-9L));

  bp[59] = caml_alloc(248, 2, s_186459799, Val_int(-10L));

  bp[29] = caml_alloc(248, 2, s_671588152, Val_int(-11L));

  bp[56] = caml_alloc(248, 2, s_252069304, Val_int(-12L));

  bp[46] = s_64835407;

  bp[30] = s_632290335;

  bp[2] = s_0;

  bp[76] = s_417075888;

  bp[9] = caml_register_global(Val_int(11L), bp[56], s_252069304);
  bp[19] = caml_register_global(Val_int(10L), bp[29], s_671588152);
  bp[75] = caml_register_global(Val_int(9L), bp[59], s_186459799);
  bp[40] = caml_register_global(Val_int(8L), bp[4], s_208627236);
  bp[71] = caml_register_global(Val_int(7L), bp[45], s_979516401);
  bp[14] = caml_register_global(Val_int(6L), bp[24], s_294329037);
  bp[48] = caml_register_global(Val_int(5L), bp[49], s_154408390);
  bp[73] = caml_register_global(Val_int(4L), bp[68], s_756918818);
  bp[18] = caml_register_global(Val_int(3L), bp[66], s_656651812);
  bp[70] = caml_register_global(Val_int(2L), bp[54], s_176434974);
  bp[0] = caml_register_global(Val_int(1L), bp[69], s_114338481);
  bp[72] = caml_register_global(Val_int(0L), bp[5], s_834731544);

  goto b234;
b234:;

  goto b328;
b328:;
  bp[64] = caml_alloc_closure(c312, 1, 0);
  ;

  goto b640;
b640:;
  bp[13] = caml_alloc_closure(c464, 2, 0);
  ;

  goto b2284;
b2284:;
  bp[28] = caml_ensure_stack_capacity(Val_int(75L));

  goto b3027;
b3027:;

  goto b3170;
b3170:;
  bp[1] = caml_alloc_closure(c3156, 1, 1);
  ;
  add_arg(bp[1], bp[34]);

  goto b3502;
b3502:;
  bp[77] = caml_alloc_closure(c3488, 3, 1);
  ;
  bp[16] = caml_alloc_closure(c3191, 2, 4);
  ;
  add_arg(bp[77], bp[34]);

  add_arg(bp[16], bp[34]);
  add_arg(bp[16], bp[13]);
  add_arg(bp[16], bp[77]);
  add_arg(bp[16], bp[16]);

  goto b3604;
b3604:;

  goto b3270;
b3270:;
  bp[36] = caml_call(bp[1], 1, bp[46]);
  bp[17] = caml_call(bp[1], 1, bp[30]);

  goto b3603;
b3603:;

  goto b3602;
b3602:;

  goto b3313;
b3313:;
  bp[31] = Val_int(9L);

  goto b3581;
b3581:;
  value t85_0 = bp[2];
  value t85_1 = bp[31];
  bp[44] = t85_0;
  bp[11] = t85_1;

  goto b3286;
b3286:;
  bp[74] = Val_bool(Val_int(0L) == bp[11]);
  if (Bool_val(bp[74])) {
    goto b3290;
  } else {
    goto b3293;
  }
b3290:;

  goto b3580;
b3580:;

  goto b3579;
b3579:;

  goto b306;
b306:;
  bp[53] = caml_bytes_of_string(bp[44]);
  bp[6] = caml_call(bp[64], 1, bp[53]);

  goto b3601;
b3601:;
  bp[65] = caml_call(bp[1], 1, bp[76]);
  bp[25] = Val_int(0L);

  bp[23] = caml_call(bp[16], 2, bp[6], bp[25]);
  return Val_unit;

b3293:;
  bp[60] = caml_putc(Val_int(62L));

  goto b3578;
b3578:;

  goto b3121;
b3121:;
  bp[55] = Val_int(0L);

  goto b3592;
b3592:;
  value t95_0 = bp[55];
  bp[20] = t95_0;

  goto b3092;
b3092:;
  bp[33] = caml_getc(Val_int(0L));
  bp[32] = Val_bool(Val_int(10L) == bp[33]);
  if (Bool_val(bp[32])) {
    goto b3106;
  } else {
    goto b3099;
  }
b3106:;

  goto b3584;
b3584:;

  goto b2165;
b2165:;
  bp[63] = Val_int(0L);

  goto b3594;
b3594:;
  value t101_0 = bp[63];
  value t101_1 = bp[20];
  bp[57] = t101_0;
  bp[62] = t101_1;

  goto b727;
b727:;
  if (Bool_val(bp[62])) {
    goto b730;
  } else {
    goto b742;
  }
b730:;
  bp[22] = Field(bp[62], 1);
  bp[61] = Field(bp[62], 0);
  bp[15] = caml_alloc(0, 2, bp[61], bp[57]);
  value t104_0 = bp[15];
  value t104_1 = bp[22];
  bp[57] = t104_0;
  bp[62] = t104_1;

  goto b727;

b742:;

  goto b3593;
b3593:;

  goto b3591;
b3591:;

  goto b3590;
b3590:;

  goto b2279;
b2279:;
  bp[8] = Val_int(0L);

  goto b3598;
b3598:;
  value t110_0 = bp[57];
  value t110_1 = bp[8];
  bp[52] = t110_0;
  bp[10] = t110_1;

  goto b709;
b709:;
  if (Bool_val(bp[52])) {
    goto b712;
  } else {
    goto b721;
  }
b712:;
  bp[38] = Field(bp[52], 1);
  bp[41] = Val_int(Int_val(bp[10]) + Int_val(Val_int(1L)));
  value t113_0 = bp[38];
  value t113_1 = bp[41];
  bp[52] = t113_0;
  bp[10] = t113_1;

  goto b709;

b721:;

  goto b3597;
b3597:;

  goto b3589;
b3589:;
  bp[3] = caml_create_bytes(bp[10]);

  goto b3588;
b3588:;

  goto b2083;
b2083:;
  bp[37] = Val_int(0L);

  goto b3596;
b3596:;
  value t119_0 = bp[57];
  value t119_1 = bp[37];
  bp[47] = t119_0;
  bp[43] = t119_1;

  goto b1186;
b1186:;
  if (Bool_val(bp[47])) {
    goto b1189;
  } else {
    goto b1205;
  }
b1189:;
  bp[26] = Field(bp[47], 1);
  bp[51] = Field(bp[47], 0);

  goto b3608;
b3608:;

  goto b3115;
b3115:;
  bp[42] = caml_bytes_unsafe_set(bp[3], bp[43], bp[51]);

  goto b3607;
b3607:;
  bp[7] = Val_int(Int_val(bp[43]) + Int_val(Val_int(1L)));
  value t125_0 = bp[26];
  value t125_1 = bp[7];
  bp[47] = t125_0;
  bp[43] = t125_1;

  goto b1186;

b1205:;

  goto b3595;
b3595:;

  goto b3587;
b3587:;

  goto b3586;
b3586:;

  goto b299;
b299:;
  bp[21] = caml_call(bp[64], 1, bp[3]);
  bp[39] = caml_string_of_bytes(bp[21]);

  goto b3585;
b3585:;

  goto b3577;
b3577:;

  goto b3576;
b3576:;

  goto b174;
b174:;
  bp[78] = caml_ml_string_length(bp[44]);
  bp[27] = caml_ml_string_length(bp[39]);
  bp[35] = Val_int(Int_val(bp[78]) + Int_val(bp[27]));
  bp[67] = caml_string_concat(bp[44], bp[39]);
  bp[50] = caml_bytes_of_string(bp[67]);

  goto b3575;
b3575:;
  bp[12] = Val_int(Int_val(bp[11]) + Int_val(Val_int(-1L)));
  value t135_0 = bp[67];
  value t135_1 = bp[12];
  bp[44] = t135_0;
  bp[11] = t135_1;

  goto b3286;

b3099:;
  bp[58] = caml_alloc(0, 2, bp[33], bp[20]);
  value t136_0 = bp[58];
  bp[20] = t136_0;

  goto b3092;
}

int main() {
  check_stack(16);
  s_64835407 = caml_copy_string("SUDOKU SOLVER");
  *(sp++) = s_64835407;
  s_632290335 =
      caml_copy_string("ENTER 9 LINES OF 9 DIGITS (USE 0 FOR EMPTY CELLS):");
  *(sp++) = s_632290335;
  s_176434974 = caml_copy_string("Failure");
  *(sp++) = s_176434974;
  s_294329037 = caml_copy_string("Not_found");
  *(sp++) = s_294329037;
  s_252069304 = caml_copy_string("Undefined_recursive_module");
  *(sp++) = s_252069304;
  s_834731544 = caml_copy_string("Out_of_memory");
  *(sp++) = s_834731544;
  s_671588152 = caml_copy_string("Assert_failure");
  *(sp++) = s_671588152;
  s_186459799 = caml_copy_string("Sys_blocked_io");
  *(sp++) = s_186459799;
  s_154408390 = caml_copy_string("Division_by_zero");
  *(sp++) = s_154408390;
  s_208627236 = caml_copy_string("Stack_overflow");
  *(sp++) = s_208627236;
  s_656651812 = caml_copy_string("Invalid_argument");
  *(sp++) = s_656651812;
  s_756918818 = caml_copy_string("End_of_file");
  *(sp++) = s_756918818;
  s_114338481 = caml_copy_string("Sys_error");
  *(sp++) = s_114338481;
  s_979516401 = caml_copy_string("Match_failure");
  *(sp++) = s_979516401;
  s_417075888 = caml_copy_string("SOLVING...");
  *(sp++) = s_417075888;
  s_0 = caml_copy_string("");
  *(sp++) = s_0;
  bp = sp;
  c0(NULL);
  return 0;
}
