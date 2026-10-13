/* SUTS-002 — SuperUnicode Regular Expressions (SRE) — reference implementation.
 *
 * Freestanding C99, zero heap, zero dynamic state-table growth. 64-bit
 * ExtSUCS codepoint domain (Base SUCS is the 31-bit subset). Implements
 * docs/suts/SUTS-002-sre.md (UTS #18 analog):
 *
 *   §4  pattern compilation  — ASCII meta grammar -> bounded automaton
 *   §5  property classes     — SUCD-style lookup for Bridge, direct bitmask
 *                              testing for Native / ExtSUCS / SCP
 *   §6  matching             — single-pass linear simulation over a fixed
 *                              state subset (no backtracking, no heap)
 *   §7  assertions           — ^ $ \b \B and SDF scope anchors
 *
 * Patterns are compiled once into a caller-owned suts_sre_compiled_dfa_t;
 * matching then runs in O(N) time and O(1) extra space.
 */

#include "suts/suts_sre.h"

/* ============================================================================
 * Small helpers (no libc: freestanding)
 * ============================================================================ */
#define SRE_PEEK(p)  ((p)->pos < (p)->len ? (unsigned char)(p)->s[(p)->pos] : -1)
#define SRE_NEXT(p)  ((p)->pos < (p)->len ? (unsigned char)(p)->s[(p)->pos++] : -1)
#define SRE_PEEK2(p) ((p)->pos + 1 < (p)->len ? (unsigned char)(p)->s[(p)->pos + 1] : -1)

#define SRE_ACCEPT_BIT(d, s) ((((d)->accept_state_mask) >> (s)) & 1u)

typedef struct {
    const char*               s;
    size_t                    len;
    size_t                    pos;
    suts_sre_compiled_dfa_t*  d;
    int                       err;
} sre_p_t;

typedef struct { int start; int end; } sre_frag_t;

/* ============================================================================
 * Property classification (SUCD lookup for Bridge; bitmask for the rest)
 * ============================================================================ */
static int sre_in(sucs_ex_char_t cp, sucs_ex_char_t lo, sucs_ex_char_t hi)
{
    return cp >= lo && cp <= hi;
}

bool suts_sre_prop_match(suts_sre_prop_t prop, sucs_ex_char_t cp)
{
    switch (prop) {
    case SUTS_SRE_PROP_ANY:
        return true;

    case SUTS_SRE_PROP_ASCII:
        return cp <= 0x7FULL;

    case SUTS_SRE_PROP_LETTER:
        return sre_in(cp, 0x41, 0x5A) || sre_in(cp, 0x61, 0x7A) ||
               (sre_in(cp, 0xC0, 0xFF) && cp != 0xD7 && cp != 0xF7) ||
               sre_in(cp, 0x100, 0x24F) || sre_in(cp, 0x370, 0x3FF) ||
               sre_in(cp, 0x400, 0x52F) || sre_in(cp, 0x590, 0x5FF) ||
               sre_in(cp, 0x600, 0x6FF) || sre_in(cp, 0x750, 0x77F) ||
               sre_in(cp, 0x900, 0x97F) || sre_in(cp, 0x3040, 0x309F) ||
               sre_in(cp, 0x30A0, 0x30FF) || sre_in(cp, 0xAC00, 0xD7A3) ||
               sre_in(cp, 0x4E00, 0x9FFF) || sre_in(cp, 0x3400, 0x4DBF) ||
               sre_in(cp, 0xF900, 0xFAFF) || sre_in(cp, 0x1F00, 0x1FFF) ||
               sre_in(cp, 0x20000, 0x2FA1F);

    case SUTS_SRE_PROP_NUMBER:
        return sre_in(cp, 0x30, 0x39) || sre_in(cp, 0x660, 0x669) ||
               sre_in(cp, 0x6F0, 0x6F9) || sre_in(cp, 0x966, 0x96F) ||
               sre_in(cp, 0xFF10, 0xFF19);

    case SUTS_SRE_PROP_ARABIC:
        return sre_in(cp, 0x600, 0x6FF) || sre_in(cp, 0x750, 0x77F) ||
               sre_in(cp, 0x8A0, 0x8FF) || sre_in(cp, 0xFB50, 0xFDFF) ||
               sre_in(cp, 0xFE70, 0xFEFF);

    case SUTS_SRE_PROP_RNUR:
        return sre_in(cp, SRE_RNUR_MIN, SRE_RNUR_MAX);   /* RNUR (SUTR-8) */

    case SUTS_SRE_PROP_SCP:
        return cp >= SRE_ZONE_SCP_MIN && cp <= SRE_ZONE_SCP_MAX;

    case SUTS_SRE_PROP_BANCODE:
        return cp >= SRE_BANCODE_MIN && cp <= SRE_BANCODE_MAX;

    case SUTS_SRE_PROP_WHITESPACE:
        return sre_in(cp, 0x09, 0x0D) || cp == 0x20 || cp == 0x85 ||
               cp == 0xA0 || cp == 0x1680 || sre_in(cp, 0x2000, 0x200A) ||
               cp == 0x2028 || cp == 0x2029 || cp == 0x202F || cp == 0x205F ||
               cp == 0x3000;

    case SUTS_SRE_PROP_WORD:
        return suts_sre_prop_match(SUTS_SRE_PROP_LETTER, cp) ||
               suts_sre_prop_match(SUTS_SRE_PROP_NUMBER, cp) ||
               cp == 0x5FULL || cp == 0x200CULL || cp == 0x200DULL ||
               suts_sre_prop_match(SUTS_SRE_PROP_MARK, cp);

    case SUTS_SRE_PROP_UPPER:
        return sre_in(cp, 0x41, 0x5A) ||
               (sre_in(cp, 0xC0, 0xDE) && cp != 0xD7);

    case SUTS_SRE_PROP_LOWER:
        return sre_in(cp, 0x61, 0x7A) ||
               (sre_in(cp, 0xDF, 0xF6) && cp != 0xF7) ||
               sre_in(cp, 0xF8, 0xFF);

    case SUTS_SRE_PROP_MARK:
        return sre_in(cp, 0x300, 0x36F) || sre_in(cp, 0x483, 0x489) ||
               sre_in(cp, 0x1AB0, 0x1AFF) || sre_in(cp, 0x20D0, 0x20FF);

    case SUTS_SRE_PROP_NATIVE:
        return cp >= SRE_ZONE_NATIVE_MIN && cp <= SRE_ZONE_NATIVE_MAX;

    case SUTS_SRE_PROP_EXTSUCS:
        return cp > SRE_BASE_SUCS_MAX;

    case SUTS_SRE_PROP_GREEK:
        return sre_in(cp, 0x370, 0x3FF) || sre_in(cp, 0x1F00, 0x1FFF);

    case SUTS_SRE_PROP_CYRILLIC:
        return sre_in(cp, 0x400, 0x52F);

    case SUTS_SRE_PROP_HAN:
        return sre_in(cp, 0x4E00, 0x9FFF) || sre_in(cp, 0x3400, 0x4DBF) ||
               sre_in(cp, 0xF900, 0xFAFF) || sre_in(cp, 0x20000, 0x2FA1F);

    case SUTS_SRE_PROP_ASSIGNED:
        if (sre_in(cp, 0xD800, 0xDFFF)) return false;            /* surrogates */
        if (sre_in(cp, SRE_BASE_TRAP_MIN, 0x7FFFFFFFULL)) return false;
        return true;

    default:
        return false;
    }
}

/* ---------------------------------------------------------------------------
 * Case fold (simple, Latin-1 + ASCII). SRE_PROP_IGNORE_CASE.
 * ------------------------------------------------------------------------- */
static sucs_ex_char_t sre_fold(sucs_ex_char_t cp)
{
    if (sre_in(cp, 0x41, 0x5A)) return cp + 0x20;
    if (sre_in(cp, 0xC0, 0xDE) && cp != 0xD7) return cp + 0x20;
    return cp;
}

/* Inverse fold: lowercase -> uppercase, for case-insensitive range tests. */
static sucs_ex_char_t sre_unfold(sucs_ex_char_t cp)
{
    if (sre_in(cp, 0x61, 0x7A)) return cp - 0x20;
    if (sre_in(cp, 0xE0, 0xFE) && cp != 0xF7) return cp - 0x20;
    return cp;
}

/* ---------------------------------------------------------------------------
 * SUCD-lite line/word/direction helpers for assertions.
 * ------------------------------------------------------------------------- */
static int sre_is_linebreak(sucs_ex_char_t cp)
{
    return cp == 0x0A || cp == 0x0D || cp == 0x0B || cp == 0x0C ||
           cp == 0x85 || cp == 0x2028 || cp == 0x2029;
}

static int sre_is_word(sucs_ex_char_t cp)
{
    return suts_sre_prop_match(SUTS_SRE_PROP_WORD, cp);
}

static int sre_is_rtl(sucs_ex_char_t cp)
{
    return suts_sre_prop_match(SUTS_SRE_PROP_ARABIC, cp) ||
           sre_in(cp, 0x590, 0x5FF) || sre_in(cp, 0xFB1D, 0xFDFF);
}

static int sre_is_ltr(sucs_ex_char_t cp)
{
    return suts_sre_prop_match(SUTS_SRE_PROP_LETTER, cp) && !sre_is_rtl(cp);
}

/* ============================================================================
 * Predicate + fragment construction
 * ============================================================================ */
static suts_sre_pred_t sre_pred_zero(void)
{
    suts_sre_pred_t p;
    p.kind = SUTS_SRE_P_EPS; p.negate = 0; p.assertion = 0;
    p.class_index = 0; p.property_mask = SUTS_SRE_PROP_ANY;
    p.cp_lo = 0; p.cp_hi = 0; p.is_exact = false;
    return p;
}

static suts_sre_pred_t sre_pred_eps(void)              { return sre_pred_zero(); }
static suts_sre_pred_t sre_pred_any(void)
{
    suts_sre_pred_t p = sre_pred_zero(); p.kind = SUTS_SRE_P_ANY; return p;
}
static suts_sre_pred_t sre_pred_exact(sucs_ex_char_t cp)
{
    suts_sre_pred_t p = sre_pred_zero();
    p.kind = SUTS_SRE_P_EXACT; p.cp_lo = cp; p.cp_hi = cp; p.is_exact = true;
    return p;
}
static suts_sre_pred_t sre_pred_prop(suts_sre_prop_t prop, int negate)
{
    suts_sre_pred_t p = sre_pred_zero();
    p.kind = SUTS_SRE_P_PROP; p.property_mask = prop; p.negate = (uint8_t)(negate ? 1 : 0);
    return p;
}
static suts_sre_pred_t sre_pred_class(uint8_t idx)
{
    suts_sre_pred_t p = sre_pred_zero();
    p.kind = SUTS_SRE_P_CLASS; p.class_index = idx;
    return p;
}
static suts_sre_pred_t sre_pred_assert(uint8_t assertion)
{
    suts_sre_pred_t p = sre_pred_zero();
    p.kind = SUTS_SRE_P_ASSERT; p.assertion = assertion;
    return p;
}

static int sre_new_state(suts_sre_compiled_dfa_t* d, int* out)
{
    if (d->state_count >= SUTS_SRE_MAX_STATES) { *out = -1; return SUTS_SRE_ERR_STATE_OVERFLOW; }
    *out = (int)d->state_count++;
    return SUTS_SRE_OK;
}

static int sre_add_edge(suts_sre_compiled_dfa_t* d, int from, int to,
                        suts_sre_pred_t pred)
{
    suts_sre_transition_t* t;
    if (from < 0 || to < 0) return SUTS_SRE_ERR_STATE_OVERFLOW;
    if (d->transition_count >= SUTS_SRE_MAX_TRANSITIONS) return SUTS_SRE_ERR_EDGE_OVERFLOW;
    t = &d->transitions[d->transition_count++];
    t->from_state = (uint8_t)from;
    t->to_state = (uint8_t)to;
    t->kind = pred.kind;
    t->negate = pred.negate;
    t->property_mask = pred.property_mask;
    t->assertion = pred.assertion;
    t->class_index = pred.class_index;
    t->cp_lo = pred.cp_lo;
    t->cp_hi = pred.cp_hi;
    t->is_exact = pred.is_exact;
    return SUTS_SRE_OK;
}

static sre_frag_t sre_frag_err(void) { sre_frag_t f; f.start = 0; f.end = 0; return f; }

static sre_frag_t sre_frag_eps(sre_p_t* p)
{
    sre_frag_t f; int ns, ne;
    if (p->err) return sre_frag_err();
    if (sre_new_state(p->d, &ns) || sre_new_state(p->d, &ne)) { p->err = SUTS_SRE_ERR_STATE_OVERFLOW; return sre_frag_err(); }
    (void)sre_add_edge(p->d, ns, ne, sre_pred_eps());
    f.start = ns; f.end = ne; return f;
}

static sre_frag_t sre_frag_atom(sre_p_t* p, suts_sre_pred_t pred)
{
    sre_frag_t f; int ns, ne;
    if (p->err) return sre_frag_err();
    if (sre_new_state(p->d, &ns) || sre_new_state(p->d, &ne)) { p->err = SUTS_SRE_ERR_STATE_OVERFLOW; return sre_frag_err(); }
    if (sre_add_edge(p->d, ns, ne, pred)) { p->err = SUTS_SRE_ERR_EDGE_OVERFLOW; return sre_frag_err(); }
    f.start = ns; f.end = ne; return f;
}

static sre_frag_t sre_frag_concat(sre_p_t* p, sre_frag_t a, sre_frag_t b)
{
    sre_frag_t f;
    if (p->err) return a;
    if (sre_add_edge(p->d, a.end, b.start, sre_pred_eps())) { p->err = SUTS_SRE_ERR_EDGE_OVERFLOW; return a; }
    f.start = a.start; f.end = b.end; return f;
}

static sre_frag_t sre_frag_alt(sre_p_t* p, sre_frag_t a, sre_frag_t b)
{
    sre_frag_t f; int ns, ne;
    if (p->err) return a;
    if (sre_new_state(p->d, &ns) || sre_new_state(p->d, &ne)) { p->err = SUTS_SRE_ERR_STATE_OVERFLOW; return a; }
    if (sre_add_edge(p->d, ns, a.start, sre_pred_eps()) ||
        sre_add_edge(p->d, ns, b.start, sre_pred_eps()) ||
        sre_add_edge(p->d, a.end, ne, sre_pred_eps()) ||
        sre_add_edge(p->d, b.end, ne, sre_pred_eps())) { p->err = SUTS_SRE_ERR_EDGE_OVERFLOW; return a; }
    f.start = ns; f.end = ne; return f;
}

static sre_frag_t sre_frag_star(sre_p_t* p, sre_frag_t a)
{
    sre_frag_t f; int ns, ne;
    if (p->err) return a;
    if (sre_new_state(p->d, &ns) || sre_new_state(p->d, &ne)) { p->err = SUTS_SRE_ERR_STATE_OVERFLOW; return a; }
    if (sre_add_edge(p->d, ns, a.start, sre_pred_eps()) ||
        sre_add_edge(p->d, ns, ne, sre_pred_eps()) ||
        sre_add_edge(p->d, a.end, ns, sre_pred_eps())) { p->err = SUTS_SRE_ERR_EDGE_OVERFLOW; return a; }
    f.start = ns; f.end = ne; return f;
}

static sre_frag_t sre_frag_plus(sre_p_t* p, sre_frag_t a)
{
    sre_frag_t f; int ns, ne;
    if (p->err) return a;
    if (sre_new_state(p->d, &ns) || sre_new_state(p->d, &ne)) { p->err = SUTS_SRE_ERR_STATE_OVERFLOW; return a; }
    if (sre_add_edge(p->d, ns, a.start, sre_pred_eps()) ||
        sre_add_edge(p->d, a.end, ne, sre_pred_eps()) ||
        sre_add_edge(p->d, a.end, ns, sre_pred_eps())) { p->err = SUTS_SRE_ERR_EDGE_OVERFLOW; return a; }
    f.start = ns; f.end = ne; return f;
}

static sre_frag_t sre_frag_opt(sre_p_t* p, sre_frag_t a)
{
    sre_frag_t f; int ns, ne;
    if (p->err) return a;
    if (sre_new_state(p->d, &ns) || sre_new_state(p->d, &ne)) { p->err = SUTS_SRE_ERR_STATE_OVERFLOW; return a; }
    if (sre_add_edge(p->d, ns, a.start, sre_pred_eps()) ||
        sre_add_edge(p->d, ns, ne, sre_pred_eps()) ||
        sre_add_edge(p->d, a.end, ne, sre_pred_eps())) { p->err = SUTS_SRE_ERR_EDGE_OVERFLOW; return a; }
    f.start = ns; f.end = ne; return f;
}

/* ============================================================================
 * Property-name lookup (loose matching, UAX44-LM style)
 * ============================================================================ */
static int sre_streq(const char* a, size_t n, const char* b)
{
    size_t i = 0;
    while (i < n && b[i] && a[i] == b[i]) i++;
    return (i == n && b[i] == 0);
}

static int sre_prop_of_name(const char* buf, size_t n, suts_sre_prop_t* out)
{
    if (sre_streq(buf, n, "any"))        { *out = SUTS_SRE_PROP_ANY;        return 1; }
    if (sre_streq(buf, n, "ascii"))      { *out = SUTS_SRE_PROP_ASCII;      return 1; }
    if (sre_streq(buf, n, "assigned"))   { *out = SUTS_SRE_PROP_ASSIGNED;   return 1; }
    if (sre_streq(buf, n, "lu") || sre_streq(buf, n, "upper") ||
        sre_streq(buf, n, "uppercase"))  { *out = SUTS_SRE_PROP_UPPER;      return 1; }
    if (sre_streq(buf, n, "ll") || sre_streq(buf, n, "lower") ||
        sre_streq(buf, n, "lowercase"))  { *out = SUTS_SRE_PROP_LOWER;      return 1; }
    if (sre_streq(buf, n, "l") || sre_streq(buf, n, "letter") ||
        sre_streq(buf, n, "alphabetic") || sre_streq(buf, n, "alpha") ||
        sre_streq(buf, n, "lo") || sre_streq(buf, n, "lm") ||
        sre_streq(buf, n, "lt"))         { *out = SUTS_SRE_PROP_LETTER;     return 1; }
    if (sre_streq(buf, n, "n") || sre_streq(buf, n, "number") ||
        sre_streq(buf, n, "nd") || sre_streq(buf, n, "digit") ||
        sre_streq(buf, n, "decimalnumber") || sre_streq(buf, n, "decimal") ||
        sre_streq(buf, n, "numeric"))    { *out = SUTS_SRE_PROP_NUMBER;     return 1; }
    if (sre_streq(buf, n, "arabic") || sre_streq(buf, n, "arab")) { *out = SUTS_SRE_PROP_ARABIC; return 1; }
    if (sre_streq(buf, n, "rnur"))       { *out = SUTS_SRE_PROP_RNUR;       return 1; }
    if (sre_streq(buf, n, "scp"))        { *out = SUTS_SRE_PROP_SCP;        return 1; }
    if (sre_streq(buf, n, "bancode") || sre_streq(buf, n, "ban")) { *out = SUTS_SRE_PROP_BANCODE; return 1; }
    if (sre_streq(buf, n, "whitespace") || sre_streq(buf, n, "space") || sre_streq(buf, n, "ws")) { *out = SUTS_SRE_PROP_WHITESPACE; return 1; }
    if (sre_streq(buf, n, "word") || sre_streq(buf, n, "w")) { *out = SUTS_SRE_PROP_WORD; return 1; }
    if (sre_streq(buf, n, "mark") || sre_streq(buf, n, "m") ||
        sre_streq(buf, n, "mn") || sre_streq(buf, n, "mc") ||
        sre_streq(buf, n, "me"))         { *out = SUTS_SRE_PROP_MARK;       return 1; }
    if (sre_streq(buf, n, "native"))     { *out = SUTS_SRE_PROP_NATIVE;     return 1; }
    if (sre_streq(buf, n, "extsucs") || sre_streq(buf, n, "plugin")) { *out = SUTS_SRE_PROP_EXTSUCS; return 1; }
    if (sre_streq(buf, n, "greek") || sre_streq(buf, n, "grek")) { *out = SUTS_SRE_PROP_GREEK; return 1; }
    if (sre_streq(buf, n, "cyrillic") || sre_streq(buf, n, "cyrl")) { *out = SUTS_SRE_PROP_CYRILLIC; return 1; }
    if (sre_streq(buf, n, "han") || sre_streq(buf, n, "hani")) { *out = SUTS_SRE_PROP_HAN; return 1; }
    return 0;
}

/* Normalize a property spec ("Letter", "sc=Greek", "General_Category=Lu")
 * into the loose form and look it up. Returns 1 on success. */
static int sre_lookup_prop(const char* name, size_t n, suts_sre_prop_t* out)
{
    char buf[48];
    size_t i, w = 0, eq = 0;
    for (i = 0; i < n; i++) {
        int c = (unsigned char)name[i];
        if (c == '=') { eq = 1; w = 0; continue; }
        if (c == ' ' || c == '_' || c == '-' || c == '\t') continue;
        if (c >= 'A' && c <= 'Z') c += 0x20;
        if (w < sizeof(buf) - 1) buf[w++] = (char)c;
    }
    buf[w] = 0;
    (void)eq;
    if (sre_prop_of_name(buf, w, out)) return 1;
    if (w > 2 && buf[0] == 'i' && buf[1] == 's')
        if (sre_prop_of_name(buf + 2, w - 2, out)) return 1;
    return 0;
}

/* ============================================================================
 * Escapes, classes, and the recursive-descent parser
 * ============================================================================ */
static int sre_hexval(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static suts_sre_prop_t sre_shorthand_prop(int c, int* negate)
{
    *negate = 0;
    switch (c) {
    case 'd': return SUTS_SRE_PROP_NUMBER;
    case 'D': *negate = 1; return SUTS_SRE_PROP_NUMBER;
    case 'w': return SUTS_SRE_PROP_WORD;
    case 'W': *negate = 1; return SUTS_SRE_PROP_WORD;
    case 's': return SUTS_SRE_PROP_WHITESPACE;
    case 'S': *negate = 1; return SUTS_SRE_PROP_WHITESPACE;
    default:  return SUTS_SRE_PROP_ANY;
    }
}

static sucs_ex_char_t sre_simple_escape(int c)
{
    switch (c) {
    case 'n': return 0x0A;
    case 'r': return 0x0D;
    case 't': return 0x09;
    case 'f': return 0x0C;
    case 'v': return 0x0B;
    case 'a': return 0x07;
    case 'e': return 0x1B;
    default:  return (sucs_ex_char_t)(unsigned char)c;
    }
}

static void sre_note_cp(sre_p_t* p, sucs_ex_char_t cp)
{
    if (cp >= SRE_ZONE_SCP_MIN && cp <= SRE_ZONE_SCP_MAX)
        p->d->flags |= SUTS_SRE_FLAG_MATCH_SCP;
}

static sre_frag_t sre_frag_from_cps(sre_p_t* p, const sucs_ex_char_t* cps, size_t n)
{
    sre_frag_t f;
    size_t i;
    if (n == 0) return sre_frag_eps(p);
    sre_note_cp(p, cps[0]);
    f = sre_frag_atom(p, sre_pred_exact(cps[0]));
    for (i = 1; i < n; i++) {
        sre_note_cp(p, cps[i]);
        f = sre_frag_concat(p, f, sre_frag_atom(p, sre_pred_exact(cps[i])));
    }
    return f;
}

static sre_frag_t sre_parse_alt(sre_p_t* p);

/* \u{...} — one or more space-separated codepoints. */
static sre_frag_t sre_parse_hexbrace(sre_p_t* p)
{
    sucs_ex_char_t cps[16];
    size_t n = 0;
    int cur = 0, have = 0, c;
    if (SRE_PEEK(p) != '{') { p->err = SUTS_SRE_ERR_BAD_PATTERN; return sre_frag_err(); }
    (void)SRE_NEXT(p);
    for (;;) {
        c = SRE_PEEK(p);
        if (c == -1) { p->err = SUTS_SRE_ERR_BAD_PATTERN; return sre_frag_err(); }
        if (c == '}') { (void)SRE_NEXT(p); break; }
        if (c == ' ' || c == '\t') {
            if (have) { if (n < 16) cps[n++] = (sucs_ex_char_t)cur; cur = 0; have = 0; }
            (void)SRE_NEXT(p);
            continue;
        }
        { int hv = sre_hexval(c);
          if (hv < 0) { p->err = SUTS_SRE_ERR_BAD_PATTERN; return sre_frag_err(); }
          cur = (cur << 4) | hv; have = 1;
        }
        (void)SRE_NEXT(p);
    }
    if (have) { if (n < 16) cps[n++] = (sucs_ex_char_t)cur; }
    if (n == 0) { p->err = SUTS_SRE_ERR_BAD_PATTERN; return sre_frag_err(); }
    return sre_frag_from_cps(p, cps, n);
}

static sre_frag_t sre_parse_hexfixed(sre_p_t* p, int digits)
{
    int i;
    sucs_ex_char_t cp = 0;
    for (i = 0; i < digits; i++) {
        int c = SRE_PEEK(p), hv = sre_hexval(c);
        if (hv < 0) { p->err = SUTS_SRE_ERR_BAD_PATTERN; return sre_frag_err(); }
        cp = (cp << 4) | (sucs_ex_char_t)hv;
        (void)SRE_NEXT(p);
    }
    sre_note_cp(p, cp);
    return sre_frag_atom(p, sre_pred_exact(cp));
}

static sre_frag_t sre_parse_prop_braced(sre_p_t* p, int negate)
{
    char name[48];
    size_t nl = 0;
    suts_sre_prop_t prop;
    if (SRE_PEEK(p) != '{') { p->err = SUTS_SRE_ERR_BAD_PATTERN; return sre_frag_err(); }
    (void)SRE_NEXT(p);
    for (;;) {
        int c = SRE_PEEK(p);
        if (c == -1) { p->err = SUTS_SRE_ERR_BAD_PATTERN; return sre_frag_err(); }
        if (c == '}') { (void)SRE_NEXT(p); break; }
        if (nl < sizeof(name) - 1) name[nl++] = (char)c;
        (void)SRE_NEXT(p);
    }
    if (!sre_lookup_prop(name, nl, &prop)) { p->err = SUTS_SRE_ERR_BAD_PATTERN; return sre_frag_err(); }
    if (prop == SUTS_SRE_PROP_SCP || prop == SUTS_SRE_PROP_BANCODE)
        p->d->flags |= SUTS_SRE_FLAG_MATCH_SCP;
    return sre_frag_atom(p, sre_pred_prop(prop, negate));
}

/* [:name:] / [:^name:]  — '[' already consumed. */
static sre_frag_t sre_parse_posix(sre_p_t* p)
{
    int negate = 0;
    suts_sre_prop_t prop;
    if (SRE_PEEK(p) == '^') { negate = 1; (void)SRE_NEXT(p); }
    {
        char name[48];
        size_t nl = 0;
        for (;;) {
            int c = SRE_PEEK(p);
            if (c == -1) { p->err = SUTS_SRE_ERR_BAD_PATTERN; return sre_frag_err(); }
            if (c == ':') { (void)SRE_NEXT(p); break; }
            if (nl < sizeof(name) - 1) name[nl++] = (char)c;
            (void)SRE_NEXT(p);
        }
        if (SRE_PEEK(p) != ']') { p->err = SUTS_SRE_ERR_BAD_PATTERN; return sre_frag_err(); }
        (void)SRE_NEXT(p);
        if (!sre_lookup_prop(name, nl, &prop)) { p->err = SUTS_SRE_ERR_BAD_PATTERN; return sre_frag_err(); }
    }
    if (prop == SUTS_SRE_PROP_SCP || prop == SUTS_SRE_PROP_BANCODE)
        p->d->flags |= SUTS_SRE_FLAG_MATCH_SCP;
    return sre_frag_atom(p, sre_pred_prop(prop, negate));
}

/* Forward decl for nested character classes. */
static int sre_parse_class(sre_p_t* p, uint8_t* out_index);

static int sre_class_item_escape(sre_p_t* p, suts_sre_class_item_t* it)
{
    int e = SRE_NEXT(p);
    it->kind = SUTS_SRE_ITEM_EXACT; it->negate = 0; it->class_index = 0;
    it->property = SUTS_SRE_PROP_ANY;
    switch (e) {
    case 'p': case 'P':
        if (SRE_PEEK(p) == '{') {
            char name[48]; size_t nl = 0; suts_sre_prop_t prop;
            (void)SRE_NEXT(p);
            for (;;) {
                int c = SRE_PEEK(p);
                if (c == -1) { p->err = SUTS_SRE_ERR_BAD_PATTERN; return -1; }
                if (c == '}') { (void)SRE_NEXT(p); break; }
                if (nl < sizeof(name) - 1) name[nl++] = (char)c;
                (void)SRE_NEXT(p);
            }
            if (!sre_lookup_prop(name, nl, &prop)) { p->err = SUTS_SRE_ERR_BAD_PATTERN; return -1; }
            if (prop == SUTS_SRE_PROP_SCP || prop == SUTS_SRE_PROP_BANCODE)
                p->d->flags |= SUTS_SRE_FLAG_MATCH_SCP;
            it->kind = SUTS_SRE_ITEM_PROP; it->property = prop; it->negate = (uint8_t)(e == 'P');
        } else { p->err = SUTS_SRE_ERR_BAD_PATTERN; return -1; }
        return 0;
    case 'd': case 'D': case 'w': case 'W': case 's': case 'S':
        { int neg = 0; it->kind = SUTS_SRE_ITEM_PROP;
          it->property = sre_shorthand_prop(e, &neg); it->negate = (uint8_t)neg; }
        return 0;
    case 'u':
        if (SRE_PEEK(p) == '{') {
            sre_frag_t f = sre_parse_hexbrace(p);   /* reuse for single cp check */
            (void)f;
            /* NB: a multi-cp \u{...} inside a class is not representable. */
            p->err = SUTS_SRE_ERR_UNSUPPORTED; return -1;
        } else {
            int i; sucs_ex_char_t cp = 0;
            for (i = 0; i < 4; i++) { int c = SRE_PEEK(p), hv = sre_hexval(c);
                if (hv < 0) { p->err = SUTS_SRE_ERR_BAD_PATTERN; return -1; }
                cp = (cp << 4) | (sucs_ex_char_t)hv; (void)SRE_NEXT(p); }
            it->lo = it->hi = cp; return 0;
        }
    case 'U': {
        int i; sucs_ex_char_t cp = 0;
        for (i = 0; i < 8; i++) { int c = SRE_PEEK(p), hv = sre_hexval(c);
            if (hv < 0) { p->err = SUTS_SRE_ERR_BAD_PATTERN; return -1; }
            cp = (cp << 4) | (sucs_ex_char_t)hv; (void)SRE_NEXT(p); }
        it->lo = it->hi = cp; return 0;
    }
    default:
        it->lo = it->hi = sre_simple_escape(e);
        return 0;
    }
}

static int sre_parse_item(sre_p_t* p, suts_sre_class_item_t* it)
{
    int c;
    it->kind = SUTS_SRE_ITEM_EXACT; it->op = SUTS_SRE_OP_UNION; it->negate = 0;
    it->class_index = 0; it->property = SUTS_SRE_PROP_ANY; it->lo = 0; it->hi = 0;

    while (SRE_PEEK(p) == ' ') (void)SRE_NEXT(p);
    c = SRE_PEEK(p);

    if (c == '\\') {
        (void)SRE_NEXT(p);
        if (sre_class_item_escape(p, it)) return -1;
    } else if (c == '[') {
        (void)SRE_NEXT(p);
        if (sre_parse_class(p, &it->class_index)) return -1;
        it->kind = SUTS_SRE_ITEM_CLASS;
        return 0;
    } else if (c == -1) {
        p->err = SUTS_SRE_ERR_BAD_PATTERN; return -1;
    } else {
        (void)SRE_NEXT(p);
        if (c >= 0x80) { p->err = SUTS_SRE_ERR_BAD_PATTERN; return -1; }
        it->lo = it->hi = (sucs_ex_char_t)(unsigned char)c;
    }

    /* optional range: EXACT '-' EXACT, but '-' not starting "--" or ending "]". */
    if (it->kind == SUTS_SRE_ITEM_EXACT && SRE_PEEK(p) == '-') {
        int n1 = SRE_PEEK2(p);
        if (n1 != '-' && n1 != ']' && n1 != -1) {
            sucs_ex_char_t hi;
            (void)SRE_NEXT(p); /* consume '-' */
            if (SRE_PEEK(p) == '\\') {
                suts_sre_class_item_t end;
                (void)SRE_NEXT(p);
                if (sre_class_item_escape(p, &end)) return -1;
                hi = end.lo;
            } else {
                int e2 = SRE_NEXT(p);
                if (e2 < 0 || e2 >= 0x80) { p->err = SUTS_SRE_ERR_BAD_PATTERN; return -1; }
                hi = (sucs_ex_char_t)(unsigned char)e2;
            }
            if (it->lo > hi) { p->err = SUTS_SRE_ERR_BAD_PATTERN; return -1; }
            it->kind = SUTS_SRE_ITEM_RANGE; it->hi = hi;
        }
    }
    return 0;
}

static int sre_parse_class(sre_p_t* p, uint8_t* out_index)
{
    uint8_t idx;
    int first = 1;
    uint8_t op = SUTS_SRE_OP_UNION;
    suts_sre_class_t* cl;

    if (p->d->class_count >= SUTS_SRE_MAX_CLASSES) { p->err = SUTS_SRE_ERR_CLASS_OVERFLOW; return -1; }
    idx = p->d->class_count++;
    cl = &p->d->classes[idx];
    cl->negate = 0; cl->count = 0;

    if (SRE_PEEK(p) == '^') { cl->negate = 1; (void)SRE_NEXT(p); }

    for (;;) {
        while (SRE_PEEK(p) == ' ') (void)SRE_NEXT(p);
        if (SRE_PEEK(p) == -1) { p->err = SUTS_SRE_ERR_BAD_PATTERN; return -1; }
        if (SRE_PEEK(p) == ']') { (void)SRE_NEXT(p); break; }

        if (!first) {
            if (SRE_PEEK(p) == '&' && SRE_PEEK2(p) == '&') { (void)SRE_NEXT(p); (void)SRE_NEXT(p); op = SUTS_SRE_OP_INTERSECT; }
            else if (SRE_PEEK(p) == '-' && SRE_PEEK2(p) == '-') { (void)SRE_NEXT(p); (void)SRE_NEXT(p); op = SUTS_SRE_OP_DIFF; }
            else if (SRE_PEEK(p) == '|' && SRE_PEEK2(p) == '|') { (void)SRE_NEXT(p); (void)SRE_NEXT(p); op = SUTS_SRE_OP_UNION; }
            else if (SRE_PEEK(p) == '~' && SRE_PEEK2(p) == '~') { (void)SRE_NEXT(p); (void)SRE_NEXT(p); op = SUTS_SRE_OP_SYMDIFF; }
            else op = SUTS_SRE_OP_UNION;
            while (SRE_PEEK(p) == ' ') (void)SRE_NEXT(p);
        }

        if (cl->count >= SUTS_SRE_MAX_CLASS_ITEMS) { p->err = SUTS_SRE_ERR_CLASS_OVERFLOW; return -1; }
        if (sre_parse_item(p, &cl->items[cl->count])) return -1;
        cl->items[cl->count].op = op;
        cl->count++;
        first = 0;
    }
    *out_index = idx;
    return 0;
}

static sre_frag_t sre_parse_escape(sre_p_t* p)
{
    int e = SRE_NEXT(p);
    switch (e) {
    case -1:
        p->err = SUTS_SRE_ERR_BAD_PATTERN; return sre_frag_err();
    case 'u':
        return (SRE_PEEK(p) == '{') ? sre_parse_hexbrace(p) : sre_parse_hexfixed(p, 4);
    case 'U':
        return sre_parse_hexfixed(p, 8);
    case 'p':
        return sre_parse_prop_braced(p, 0);
    case 'P':
        return sre_parse_prop_braced(p, 1);
    case 'd': case 'D': case 'w': case 'W': case 's': case 'S':
        { int neg = 0; suts_sre_prop_t prop = sre_shorthand_prop(e, &neg);
          if (prop == SUTS_SRE_PROP_ANY) { p->err = SUTS_SRE_ERR_BAD_PATTERN; return sre_frag_err(); }
          return sre_frag_atom(p, sre_pred_prop(prop, neg)); }
    case 'b': {
        int assertion = SUTS_SRE_ASSERT_WORD;
        if (SRE_PEEK(p) == '{') {
            char name[16]; size_t nl = 0;
            (void)SRE_NEXT(p);
            for (;;) {
                int c = SRE_PEEK(p);
                if (c == -1) { p->err = SUTS_SRE_ERR_BAD_PATTERN; return sre_frag_err(); }
                if (c == '}') { (void)SRE_NEXT(p); break; }
                if (nl < sizeof(name) - 1) name[nl++] = (char)c;
                (void)SRE_NEXT(p);
            }
            if (sre_streq(name, nl, "w") || sre_streq(name, nl, "g") || sre_streq(name, nl, "word")) assertion = SUTS_SRE_ASSERT_WORD;
            else if (sre_streq(name, nl, "l") || sre_streq(name, nl, "line")) assertion = SUTS_SRE_ASSERT_BOL;
            else if (sre_streq(name, nl, "s") || sre_streq(name, nl, "sentence")) assertion = SUTS_SRE_ASSERT_EOL;
            else if (sre_streq(name, nl, "ltr")) assertion = SUTS_SRE_ASSERT_LTR;
            else if (sre_streq(name, nl, "rtl")) assertion = SUTS_SRE_ASSERT_RTL;
            else { p->err = SUTS_SRE_ERR_BAD_PATTERN; return sre_frag_err(); }
        }
        return sre_frag_atom(p, sre_pred_assert((uint8_t)assertion));
    }
    case 'B':
        return sre_frag_atom(p, sre_pred_assert(SUTS_SRE_ASSERT_NONWORD));
    case 'A':
        return sre_frag_atom(p, sre_pred_assert(SUTS_SRE_ASSERT_BOL));
    case 'Z': case 'z':
        return sre_frag_atom(p, sre_pred_assert(SUTS_SRE_ASSERT_EOL));
    default:
        { sucs_ex_char_t cp = sre_simple_escape(e);
          sre_note_cp(p, cp);
          return sre_frag_atom(p, sre_pred_exact(cp)); }
    }
}

static sre_frag_t sre_parse_atom(sre_p_t* p)
{
    int c = SRE_PEEK(p);
    if (c == -1 || c == '|' || c == ')') return sre_frag_eps(p);

    if (c == '(') {
        sre_frag_t f;
        (void)SRE_NEXT(p);
        if (SRE_PEEK(p) == '?' && SRE_PEEK2(p) == ':') { (void)SRE_NEXT(p); (void)SRE_NEXT(p); }
        f = sre_parse_alt(p);
        if (SRE_PEEK(p) != ')') { p->err = SUTS_SRE_ERR_BAD_PATTERN; return sre_frag_err(); }
        (void)SRE_NEXT(p);
        return f;
    }
    if (c == '.') {
        (void)SRE_NEXT(p);
        return sre_frag_atom(p, sre_pred_any());
    }
    if (c == '^') {
        (void)SRE_NEXT(p);
        return sre_frag_atom(p, sre_pred_assert(SUTS_SRE_ASSERT_BOL));
    }
    if (c == '$') {
        (void)SRE_NEXT(p);
        return sre_frag_atom(p, sre_pred_assert(SUTS_SRE_ASSERT_EOL));
    }
    if (c == '\\') {
        (void)SRE_NEXT(p);
        return sre_parse_escape(p);
    }
    if (c == '[') {
        (void)SRE_NEXT(p);
        if (SRE_PEEK(p) == ':') { (void)SRE_NEXT(p); return sre_parse_posix(p); }
        {
            uint8_t idx;
            if (sre_parse_class(p, &idx)) return sre_frag_err();
            return sre_frag_atom(p, sre_pred_class(idx));
        }
    }
    if (c >= 0x80) { p->err = SUTS_SRE_ERR_BAD_PATTERN; return sre_frag_err(); }
    (void)SRE_NEXT(p);
    sre_note_cp(p, (sucs_ex_char_t)(unsigned char)c);
    return sre_frag_atom(p, sre_pred_exact((sucs_ex_char_t)(unsigned char)c));
}

static sre_frag_t sre_parse_repeat(sre_p_t* p)
{
    sre_frag_t f = sre_parse_atom(p);
    for (;;) {
        int c = SRE_PEEK(p);
        if (p->err) return f;
        if (c == '*') { (void)SRE_NEXT(p); f = sre_frag_star(p, f); }
        else if (c == '+') { (void)SRE_NEXT(p); f = sre_frag_plus(p, f); }
        else if (c == '?') { (void)SRE_NEXT(p); f = sre_frag_opt(p, f); }
        else break;
    }
    return f;
}

static sre_frag_t sre_parse_concat(sre_p_t* p)
{
    sre_frag_t f = sre_parse_repeat(p);
    for (;;) {
        int c = SRE_PEEK(p);
        if (p->err) return f;
        if (c == -1 || c == '|' || c == ')') break;
        { sre_frag_t g = sre_parse_repeat(p); f = sre_frag_concat(p, f, g); }
    }
    return f;
}

static sre_frag_t sre_parse_alt(sre_p_t* p)
{
    sre_frag_t f = sre_parse_concat(p);
    while (!p->err && SRE_PEEK(p) == '|') {
        sre_frag_t g;
        (void)SRE_NEXT(p);
        g = sre_parse_concat(p);
        f = sre_frag_alt(p, f, g);
    }
    return f;
}

/* ============================================================================
 * Public: compile
 * ============================================================================ */
const char* suts_sre_version_string(void)
{
    return "SRE 0.1.0 (SUTS-002)";
}

suts_sre_status_t suts_sre_compile(const char* pattern, size_t len,
                                   suts_sre_flags_t flags,
                                   suts_sre_compiled_dfa_t* out)
{
    sre_p_t p;
    sre_frag_t f;
    size_t i;

    if (!out) return SUTS_SRE_ERR_INVALID_ARG;
    if (!pattern && len != 0) return SUTS_SRE_ERR_INVALID_ARG;

    out->transition_count = 0;
    out->class_count = 0;
    out->state_count = 0;
    out->start_state = 0;
    out->accept_state_mask = 0;
    out->flags = flags;
    for (i = 0; i < SUTS_SRE_MAX_STATES; i++) { out->transitions[i].from_state = 0; }

    p.s = pattern; p.len = len; p.pos = 0; p.d = out; p.err = 0;
    f = sre_parse_alt(&p);
    if (p.err) return (suts_sre_status_t)p.err;
    if (p.pos != len) return SUTS_SRE_ERR_BAD_PATTERN;
    if (f.end < 0 || f.end >= (int)SUTS_SRE_MAX_STATES) return SUTS_SRE_ERR_BAD_PATTERN;
    out->accept_state_mask = (uint64_t)1 << f.end;
    out->start_state = (uint8_t)f.start;
    return SUTS_SRE_OK;
}

/* ============================================================================
 * Matching: predicate evaluation, closure, run
 * ============================================================================ */
typedef struct {
    int             prev_valid;
    sucs_ex_char_t  prev;
    int             next_valid;
    sucs_ex_char_t  next;
    size_t          pos;
    size_t          n;
    suts_sre_flags_t flags;
} sre_ctx_t;

static int sre_class_eval(const suts_sre_compiled_dfa_t* d, uint8_t class_index,
                          sucs_ex_char_t cp, suts_sre_flags_t flags);

static int sre_item_eval(const suts_sre_compiled_dfa_t* d,
                         const suts_sre_class_item_t* it,
                         sucs_ex_char_t cp, suts_sre_flags_t flags)
{
    int r = 0;
    switch (it->kind) {
    case SUTS_SRE_ITEM_EXACT:
        if (flags & SUTS_SRE_FLAG_IGNORE_CASE) r = (sre_fold(cp) == sre_fold(it->lo));
        else r = (cp == it->lo);
        break;
    case SUTS_SRE_ITEM_RANGE:
        r = (cp >= it->lo && cp <= it->hi);
        if (!r && (flags & SUTS_SRE_FLAG_IGNORE_CASE)) {
            sucs_ex_char_t f = sre_fold(cp), u = sre_unfold(cp);
            r = (f >= it->lo && f <= it->hi) || (u >= it->lo && u <= it->hi);
        }
        break;
    case SUTS_SRE_ITEM_PROP:
        r = suts_sre_prop_match(it->property, cp);
        break;
    case SUTS_SRE_ITEM_CLASS:
        r = sre_class_eval(d, it->class_index, cp, flags);
        break;
    default:
        r = 0;
    }
    return it->negate ? !r : r;
}

static int sre_class_eval(const suts_sre_compiled_dfa_t* d, uint8_t class_index,
                          sucs_ex_char_t cp, suts_sre_flags_t flags)
{
    const suts_sre_class_t* cl;
    int acc = 0, first = 1;
    int i;
    if (class_index >= d->class_count) return 0;
    cl = &d->classes[class_index];
    for (i = 0; i < (int)cl->count; i++) {
        int m = sre_item_eval(d, &cl->items[i], cp, flags);
        if (first) { acc = m; first = 0; continue; }
        switch (cl->items[i].op) {
        case SUTS_SRE_OP_UNION:     acc = acc | m; break;
        case SUTS_SRE_OP_INTERSECT: acc = acc & m; break;
        case SUTS_SRE_OP_DIFF:      acc = acc & !m; break;
        case SUTS_SRE_OP_SYMDIFF:   acc = acc ^ m; break;
        default:                    acc = acc | m; break;
        }
    }
    return cl->negate ? !acc : acc;
}

static int sre_pred_cp(const suts_sre_compiled_dfa_t* d,
                       const suts_sre_transition_t* e,
                       sucs_ex_char_t cp, suts_sre_flags_t flags)
{
    int r;
    switch (e->kind) {
    case SUTS_SRE_P_ANY:
        return 1;
    case SUTS_SRE_P_EXACT:
        if (flags & SUTS_SRE_FLAG_IGNORE_CASE) return sre_fold(cp) == sre_fold(e->cp_lo);
        return cp == e->cp_lo;
    case SUTS_SRE_P_RANGE:
        return cp >= e->cp_lo && cp <= e->cp_hi;
    case SUTS_SRE_P_PROP:
        r = suts_sre_prop_match(e->property_mask, cp);
        return e->negate ? !r : r;
    case SUTS_SRE_P_CLASS:
        r = sre_class_eval(d, e->class_index, cp, flags);
        return e->negate ? !r : r;
    default:
        return 0;
    }
}

static int sre_assert_eval(uint8_t assertion, const sre_ctx_t* c)
{
    switch (assertion) {
    case SUTS_SRE_ASSERT_BOL:
        if (c->pos == 0) return 1;
        return (c->flags & SUTS_SRE_FLAG_MULTILINE) && c->prev_valid && sre_is_linebreak(c->prev);
    case SUTS_SRE_ASSERT_EOL:
        if (c->pos == c->n) return 1;
        return (c->flags & SUTS_SRE_FLAG_MULTILINE) && c->next_valid && sre_is_linebreak(c->next);
    case SUTS_SRE_ASSERT_WORD: {
        int a = c->prev_valid && sre_is_word(c->prev);
        int b = c->next_valid && sre_is_word(c->next);
        return a != b;
    }
    case SUTS_SRE_ASSERT_NONWORD: {
        int a = c->prev_valid && sre_is_word(c->prev);
        int b = c->next_valid && sre_is_word(c->next);
        return a == b;
    }
    case SUTS_SRE_ASSERT_LTR: {
        int a = c->prev_valid && sre_is_ltr(c->prev);
        int b = c->next_valid && sre_is_ltr(c->next);
        return !a && b;
    }
    case SUTS_SRE_ASSERT_RTL: {
        int a = c->prev_valid && sre_is_rtl(c->prev);
        int b = c->next_valid && sre_is_rtl(c->next);
        return !a && b;
    }
    default:
        return 1;
    }
}

static void sre_closure(const suts_sre_compiled_dfa_t* d,
                        uint8_t* active, uint32_t* startof,
                        int* stack, int sp,
                        const sre_ctx_t* ctx, uint32_t best_start, int prune)
{
    while (sp > 0) {
        int s = stack[--sp];
        uint16_t i;
        for (i = 0; i < d->transition_count; i++) {
            const suts_sre_transition_t* e = &d->transitions[i];
            int t;
            uint32_t ns;
            if (e->from_state != (uint8_t)s) continue;
            if (e->kind != SUTS_SRE_P_EPS && e->kind != SUTS_SRE_P_ASSERT) continue;
            if (e->kind == SUTS_SRE_P_ASSERT && !sre_assert_eval(e->assertion, ctx)) continue;
            t = e->to_state;
            ns = startof[s];
            if (prune && ns > best_start) continue;
            if (!active[t] || ns < startof[t]) {
                active[t] = 1; startof[t] = ns;
                if (sp < (int)SUTS_SRE_MAX_STATES) stack[sp++] = t;
            }
        }
    }
}

static suts_sre_status_t sre_run(const suts_sre_compiled_dfa_t* d,
                                 const sucs_ex_char_t* eff, size_t m,
                                 size_t first, int anchored,
                                 size_t* os, size_t* oe)
{
    uint8_t  active[SUTS_SRE_MAX_STATES];
    uint8_t  next_active[SUTS_SRE_MAX_STATES];
    uint32_t startof[SUTS_SRE_MAX_STATES];
    uint32_t nstart[SUTS_SRE_MAX_STATES];
    int      stack[SUTS_SRE_MAX_STATES];
    size_t   states = d->state_count, i, pos;
    uint32_t best_start = 0xFFFFFFFFu, best_end = 0;
    int      found = 0;

    for (i = 0; i < states; i++) { active[i] = 0; startof[i] = 0; }

    for (pos = first; pos <= m; pos++) {
        sre_ctx_t ctx;
        int prune = found && !anchored;
        int sp = 0;

        ctx.prev_valid = (pos > 0);
        ctx.prev = (pos > 0) ? eff[pos - 1] : 0;
        ctx.next_valid = (pos < m);
        ctx.next = (pos < m) ? eff[pos] : 0;
        ctx.pos = pos;
        ctx.n = m;
        ctx.flags = d->flags;

        if (!anchored || pos == first) {
            if (!(prune && pos > best_start)) {
                int s0 = d->start_state;
                if (!active[s0] || (uint32_t)pos < startof[s0]) {
                    active[s0] = 1; startof[s0] = (uint32_t)pos;
                }
            }
        }

        for (i = 0; i < states; i++) if (active[i]) stack[sp++] = (int)i;
        sre_closure(d, active, startof, stack, sp, &ctx, best_start, prune);

        for (i = 0; i < states; i++) {
            if (active[i] && SRE_ACCEPT_BIT(d, i)) {
                uint32_t ss = startof[i];
                if (!found || ss < best_start || (ss == best_start && (uint32_t)pos > best_end)) {
                    found = 1; best_start = ss; best_end = (uint32_t)pos;
                }
            }
        }

        if (found && !anchored) {
            for (i = 0; i < states; i++)
                if (active[i] && startof[i] > best_start) active[i] = 0;
        }

        if (pos == m) break;

        for (i = 0; i < states; i++) { next_active[i] = 0; nstart[i] = 0; }
        sp = 0;
        for (i = 0; i < states; i++) {
            uint32_t ns;
            uint16_t k;
            if (!active[i]) continue;
            ns = startof[i];
            if (found && !anchored && ns > best_start) continue;
            for (k = 0; k < d->transition_count; k++) {
                const suts_sre_transition_t* e = &d->transitions[k];
                int t;
                if (e->from_state != (uint8_t)i) continue;
                if (e->kind == SUTS_SRE_P_EPS || e->kind == SUTS_SRE_P_ASSERT) continue;
                if (!sre_pred_cp(d, e, eff[pos], d->flags)) continue;
                t = e->to_state;
                if (!next_active[t] || ns < nstart[t]) {
                    next_active[t] = 1; nstart[t] = ns;
                    if (sp < (int)SUTS_SRE_MAX_STATES) stack[sp++] = t;
                }
            }
        }
        for (i = 0; i < states; i++) { active[i] = next_active[i]; startof[i] = nstart[i]; }
    }

    if (found) { *os = (size_t)best_start; *oe = (size_t)best_end; return SUTS_SRE_MATCH; }
    return SUTS_SRE_NOMATCH;
}

/* ============================================================================
 * Effect preprocessing: SCP transparency + optional SUCF-C composition
 * ============================================================================ */
typedef struct { sucs_ex_char_t base; sucs_ex_char_t mark; sucs_ex_char_t pre; } sre_comp_t;

static const sre_comp_t SRE_COMP[] = {
    {0x61,0x300,0xE0},{0x61,0x301,0xE1},{0x61,0x302,0xE2},{0x61,0x303,0xE3},
    {0x61,0x308,0xE4},{0x61,0x30A,0xE5},{0x63,0x327,0xE7},{0x65,0x300,0xE8},
    {0x65,0x301,0xE9},{0x65,0x302,0xEA},{0x65,0x308,0xEB},{0x69,0x300,0xEC},
    {0x69,0x301,0xED},{0x69,0x302,0xEE},{0x69,0x308,0xEF},{0x6E,0x303,0xF1},
    {0x6F,0x300,0xF2},{0x6F,0x301,0xF3},{0x6F,0x302,0xF4},{0x6F,0x303,0xF5},
    {0x6F,0x308,0xF6},{0x75,0x300,0xF9},{0x75,0x301,0xFA},{0x75,0x302,0xFB},
    {0x75,0x308,0xFC},{0x79,0x301,0xFD},{0x79,0x308,0xFF},
    {0x41,0x300,0xC0},{0x41,0x301,0xC1},{0x41,0x302,0xC2},{0x41,0x303,0xC3},
    {0x41,0x308,0xC4},{0x41,0x30A,0xC5},{0x43,0x327,0xC7},{0x45,0x300,0xC8},
    {0x45,0x301,0xC9},{0x45,0x302,0xCA},{0x45,0x308,0xCB},{0x49,0x300,0xCC},
    {0x49,0x301,0xCD},{0x49,0x302,0xCE},{0x49,0x308,0xCF},{0x4E,0x303,0xD1},
    {0x4F,0x300,0xD2},{0x4F,0x301,0xD3},{0x4F,0x302,0xD4},{0x4F,0x303,0xD5},
    {0x4F,0x308,0xD6},{0x55,0x300,0xD9},{0x55,0x301,0xDA},{0x55,0x302,0xDB},
    {0x55,0x308,0xDC},{0x59,0x301,0xDD}
};
#define SRE_COMP_COUNT (sizeof(SRE_COMP) / sizeof(SRE_COMP[0]))

static int sre_scf_compose(sucs_ex_char_t a, sucs_ex_char_t b, sucs_ex_char_t* out)
{
    size_t i;
    for (i = 0; i < SRE_COMP_COUNT; i++) {
        if (SRE_COMP[i].base == a && SRE_COMP[i].mark == b) { *out = SRE_COMP[i].pre; return 1; }
    }
    return 0;
}

static suts_sre_status_t sre_build_effects(const suts_sre_compiled_dfa_t* d,
                                           const sucs_ex_char_t* cps, size_t n,
                                           sucs_ex_char_t* eff, size_t* map,
                                           size_t cap, size_t* out_m)
{
    size_t w = 0, i = 0;
    int scp_transparent = !(d->flags & SUTS_SRE_FLAG_MATCH_SCP);
    int scf = (d->flags & SUTS_SRE_FLAG_SCF_NORMALIZED) ? 1 : 0;

    while (i < n) {
        sucs_ex_char_t cp = cps[i];
        if (scp_transparent && cp >= SRE_ZONE_SCP_MIN && cp <= SRE_ZONE_SCP_MAX) { i++; continue; }
        if (scf && i + 1 < n) {
            sucs_ex_char_t comp;
            sucs_ex_char_t nxt = cps[i + 1];
            if (scp_transparent && nxt >= SRE_ZONE_SCP_MIN && nxt <= SRE_ZONE_SCP_MAX) nxt = 0;
            if (sre_scf_compose(cp, nxt, &comp)) {
                if (w >= cap) return SUTS_SRE_ERR_BUFFER_TOO_SMALL;
                eff[w] = comp; map[w] = i; w++; i += 2; continue;
            }
        }
        if (w >= cap) return SUTS_SRE_ERR_BUFFER_TOO_SMALL;
        eff[w] = cp; map[w] = i; w++; i++;
    }
    *out_m = w;
    return SUTS_SRE_OK;
}

static size_t sre_eff_start(const size_t* map, size_t m, size_t raw_start)
{
    size_t i;
    for (i = 0; i < m; i++) if (map[i] >= raw_start) return i;
    return m;
}

/* ============================================================================
 * Public: match_at / search
 * ============================================================================ */
suts_sre_status_t suts_sre_match_at(const suts_sre_compiled_dfa_t* dfa,
                                    const sucs_ex_char_t* cps, size_t n,
                                    size_t start,
                                    size_t* out_start, size_t* out_end)
{
    sucs_ex_char_t eff[SUTS_SRE_MAX_INPUT];
    size_t         map[SUTS_SRE_MAX_INPUT];
    size_t m = 0, s = 0, e = 0, first;
    suts_sre_status_t st;

    if (!dfa || !cps || !out_start || !out_end) return SUTS_SRE_ERR_INVALID_ARG;
    if (start > n) return SUTS_SRE_ERR_INVALID_ARG;

    st = sre_build_effects(dfa, cps, n, eff, map, SUTS_SRE_MAX_INPUT, &m);
    if (st != SUTS_SRE_OK) return st;

    first = sre_eff_start(map, m, start);
    st = sre_run(dfa, eff, m, first, 1, &s, &e);
    if (st == SUTS_SRE_MATCH) {
        *out_start = (s < m) ? map[s] : n;
        if (e < m) *out_end = map[e];
        else if (m > 0) *out_end = map[m - 1] + 1;
        else *out_end = n;
        if (*out_end < *out_start) *out_end = *out_start;
    }
    return st;
}

suts_sre_status_t suts_sre_search(const suts_sre_compiled_dfa_t* dfa,
                                  const sucs_ex_char_t* cps, size_t n,
                                  size_t start,
                                  size_t* out_start, size_t* out_end)
{
    sucs_ex_char_t eff[SUTS_SRE_MAX_INPUT];
    size_t         map[SUTS_SRE_MAX_INPUT];
    size_t m = 0, s = 0, e = 0, first;
    suts_sre_status_t st;

    if (!dfa || !cps || !out_start || !out_end) return SUTS_SRE_ERR_INVALID_ARG;
    if (start > n) return SUTS_SRE_ERR_INVALID_ARG;

    st = sre_build_effects(dfa, cps, n, eff, map, SUTS_SRE_MAX_INPUT, &m);
    if (st != SUTS_SRE_OK) return st;

    first = sre_eff_start(map, m, start);
    st = sre_run(dfa, eff, m, first, 0, &s, &e);
    if (st == SUTS_SRE_MATCH) {
        *out_start = (s < m) ? map[s] : n;
        if (e < m) *out_end = map[e];
        else if (m > 0) *out_end = map[m - 1] + 1;
        else *out_end = n;
        if (*out_end < *out_start) *out_end = *out_start;
    }
    return st;
}

/* ============================================================================
 * Public: streaming step engine
 * ============================================================================ */
static void sre_step_closure(const suts_sre_compiled_dfa_t* d,
                             suts_sre_match_state_t* match, const sre_ctx_t* ctx)
{
    uint8_t active[SUTS_SRE_MAX_STATES];
    int     stack[SUTS_SRE_MAX_STATES];
    int     sp = 0;
    size_t  i;
    for (i = 0; i < d->state_count; i++) {
        active[i] = (uint8_t)((match->current_state >> i) & 1u);
        if (active[i]) stack[sp++] = (int)i;
    }
    sre_closure(d, active, match->start_of, stack, sp, ctx, 0xFFFFFFFFu, 0);
    match->current_state = 0;
    for (i = 0; i < d->state_count; i++)
        if (active[i]) match->current_state |= (uint64_t)1 << i;
}

void suts_sre_init_match(suts_sre_match_state_t* match,
                         const suts_sre_compiled_dfa_t* dfa)
{
    sre_ctx_t c;
    size_t i;
    if (!match || !dfa) return;
    match->current_state = 0;
    match->match_start_offset = 0;
    match->match_end_offset = 0;
    match->is_accepting = false;
    match->best_start = 0xFFFFFFFFu;
    match->best_end = 0;
    match->have_match = false;
    match->seeded = false;
    match->prev_cp = 0;
    for (i = 0; i < SUTS_SRE_MAX_STATES; i++) match->start_of[i] = 0xFFFFFFFFu;
    if (dfa->state_count == 0) return;
    match->start_of[dfa->start_state] = 0;
    match->current_state = (uint64_t)1 << dfa->start_state;
    c.prev_valid = 0; c.prev = 0; c.next_valid = 0; c.next = 0;
    c.pos = 0; c.n = (size_t)-1; c.flags = dfa->flags;
    sre_step_closure(dfa, match, &c);
    {
        uint64_t a = match->current_state & dfa->accept_state_mask;
        if (a) { match->is_accepting = true; match->have_match = true; match->best_start = 0; match->best_end = 0; }
    }
    match->seeded = true;
}

bool suts_sre_step(const suts_sre_compiled_dfa_t* dfa,
                   suts_sre_match_state_t* match,
                   sucs_ex_char_t input_cp,
                   uint32_t current_offset)
{
    uint8_t  next_active[SUTS_SRE_MAX_STATES];
    uint32_t nstart[SUTS_SRE_MAX_STATES];
    size_t   i;
    sre_ctx_t c;

    if (!dfa || !match) return false;

    for (i = 0; i < dfa->state_count; i++) { next_active[i] = 0; nstart[i] = 0; }
    for (i = 0; i < dfa->state_count; i++) {
        uint16_t k;
        uint32_t ns;
        if (!((match->current_state >> i) & 1u)) continue;
        ns = match->start_of[i];
        for (k = 0; k < dfa->transition_count; k++) {
            const suts_sre_transition_t* e = &dfa->transitions[k];
            int t;
            if (e->from_state != (uint8_t)i) continue;
            if (e->kind == SUTS_SRE_P_EPS || e->kind == SUTS_SRE_P_ASSERT) continue;
            if (!sre_pred_cp(dfa, e, input_cp, dfa->flags)) continue;
            t = e->to_state;
            if (!next_active[t] || ns < nstart[t]) {
                next_active[t] = 1; nstart[t] = ns;
            }
        }
    }
    match->current_state = 0;
    for (i = 0; i < dfa->state_count; i++) {
        match->start_of[i] = nstart[i];
        if (next_active[i]) match->current_state |= (uint64_t)1 << i;
    }

    c.prev_valid = 1; c.prev = input_cp; c.next_valid = 0; c.next = 0;
    c.pos = (size_t)current_offset + 1; c.n = (size_t)-1; c.flags = dfa->flags;
    sre_step_closure(dfa, match, &c);

    match->is_accepting = false;
    for (i = 0; i < dfa->state_count; i++) {
        if (((match->current_state >> i) & 1u) && SRE_ACCEPT_BIT(dfa, i)) {
            match->is_accepting = true;
            if (!match->have_match) {
                match->have_match = true;
                match->best_start = match->start_of[i];
                match->best_end = (uint32_t)current_offset + 1;
                match->match_start_offset = match->best_start;
                match->match_end_offset = match->best_end;
            } else if (match->start_of[i] == match->best_start) {
                match->best_end = (uint32_t)current_offset + 1;
                match->match_end_offset = match->best_end;
            }
        }
    }
    match->prev_cp = (uint32_t)input_cp;
    return match->is_accepting;
}
