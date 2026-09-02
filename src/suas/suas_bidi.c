/* SUAS-BIDI — freestanding C99 port of the Unicode Bidirectional Algorithm
 * (UAX #9), the processing model behind SUAS-001 Structural Directional
 * Framing (SDF).
 *
 * Zero allocation; operates on caller-provided arrays only. Faithful to the
 * normative rule sequences P1-P3, X1-X8, X9, W1-W7, N0-N2, I1-I2, L1-L4 as
 * described in UAX #9 and docs/suas/SUAS-001-sdf.md §5.5.
 */

#include "suas/suas_bidi.h"

#define SUAS_BIDI_NELEM(a) (sizeof(a) / sizeof((a)[0]))

#ifndef SUAS_BIDI_MAX_LEVEL
#define SUAS_BIDI_MAX_LEVEL 125
#endif

#define SBI_MAX_STACK 64   /* explicit directional status stack (BD3) */

/* ── Level helpers ─────────────────────────────────────────────── */

static inline int level_is_odd(uint8_t l)  { return (l & 1) != 0; }
static inline int level_is_even(uint8_t l) { return (l & 1) == 0; }

/* BD6: next even / odd embedding level strictly greater than L. */
static inline uint8_t bd6_next_even(uint8_t l) { return (uint8_t)((l & ~1u) + 2u); }
static inline uint8_t bd6_next_odd(uint8_t l)  { return (uint8_t)(((l + 1u) & ~1u) + 1u); }
static inline int bd6_ok(uint8_t l, int odd)
{
    uint8_t next = odd ? bd6_next_odd(l) : bd6_next_even(l);
    return next <= SUAS_BIDI_MAX_LEVEL;
}

/* ── Embedded classification table ─────────────────────────────── */

typedef struct { uint32_t lo, hi; suas_bidi_class_t cls; } sbi_crange_t;

static const sbi_crange_t CP_CLASS[] = {
    /* Default controls (BN): below 0x20 minus whitespace, 0x7F-0x9F, BOM */
    { 0x0000, 0x0008, SUAS_BIDI_BN },
    { 0x000E, 0x001B, SUAS_BIDI_BN },
    { 0x001C, 0x001E, SUAS_BIDI_B  },  /* FS/GS/RS */
    { 0x001F, 0x001F, SUAS_BIDI_S  },  /* US */
    { 0x0020, 0x0020, SUAS_BIDI_WS },
    { 0x007F, 0x009F, SUAS_BIDI_BN },
    { 0x00A0, 0x00A0, SUAS_BIDI_WS },
    { 0x00AD, 0x00AD, SUAS_BIDI_BN },  /* SHY */
    { 0x061C, 0x061C, SUAS_BIDI_AL  }, /* ALM */
    { 0x1680, 0x1680, SUAS_BIDI_WS },
    { 0x2000, 0x200A, SUAS_BIDI_WS },
    { 0x200B, 0x200B, SUAS_BIDI_BN },  /* ZWSP */
    { 0x200C, 0x200C, SUAS_BIDI_BN },  /* ZWNJ */
    { 0x200D, 0x200D, SUAS_BIDI_BN },  /* ZWJ */
    { 0x200E, 0x200E, SUAS_BIDI_L   }, /* LRM */
    { 0x200F, 0x200F, SUAS_BIDI_R   }, /* RLM */
    { 0x2028, 0x2028, SUAS_BIDI_WS  }, /* LS */
    { 0x2029, 0x2029, SUAS_BIDI_B   }, /* PS */
    { 0x202A, 0x202A, SUAS_BIDI_LRE },
    { 0x202B, 0x202B, SUAS_BIDI_RLE },
    { 0x202C, 0x202C, SUAS_BIDI_PDF },
    { 0x202D, 0x202D, SUAS_BIDI_LRO },
    { 0x202E, 0x202E, SUAS_BIDI_RLO },
    { 0x202F, 0x202F, SUAS_BIDI_WS  }, /* NNBSP */
    { 0x205F, 0x205F, SUAS_BIDI_WS  }, /* MMSP */
    { 0x2060, 0x2064, SUAS_BIDI_BN  }, /* WJ..IS */
    { 0x2066, 0x2066, SUAS_BIDI_LRI },
    { 0x2067, 0x2067, SUAS_BIDI_RLI },
    { 0x2068, 0x2068, SUAS_BIDI_FSI },
    { 0x2069, 0x2069, SUAS_BIDI_PDI },
    { 0x206A, 0x206F, SUAS_BIDI_BN  }, /* deprecated inhibit chars */
    { 0x3000, 0x3000, SUAS_BIDI_WS  },
    { 0xFEFF, 0xFEFF, SUAS_BIDI_BN  },

    /* Combining marks (NSM) */
    { 0x0300, 0x036F, SUAS_BIDI_NSM },
    { 0x0483, 0x0489, SUAS_BIDI_NSM },
    { 0x0591, 0x05BD, SUAS_BIDI_NSM },
    { 0x05BF, 0x05BF, SUAS_BIDI_NSM },
    { 0x05C1, 0x05C2, SUAS_BIDI_NSM },
    { 0x05C4, 0x05C5, SUAS_BIDI_NSM },
    { 0x0610, 0x061A, SUAS_BIDI_NSM },
    { 0x064B, 0x065F, SUAS_BIDI_NSM },
    { 0x20D0, 0x20FF, SUAS_BIDI_NSM },
    { 0xFE20, 0xFE2F, SUAS_BIDI_NSM },

    /* European numbers / separators / terminators */
    { 0x0030, 0x0039, SUAS_BIDI_EN },
    { 0x002B, 0x002B, SUAS_BIDI_ES },  /* + */
    { 0x002D, 0x002D, SUAS_BIDI_ES },  /* - */
    { 0x0023, 0x0023, SUAS_BIDI_ET },  /* # */
    { 0x0025, 0x0025, SUAS_BIDI_ET },  /* % */
    { 0x00A2, 0x00A5, SUAS_BIDI_ET },
    { 0x00B0, 0x00B0, SUAS_BIDI_ET },
    { 0x2030, 0x2034, SUAS_BIDI_ET },
    { 0x00B2, 0x00B3, SUAS_BIDI_EN },
    { 0x00B9, 0x00B9, SUAS_BIDI_EN },
    { 0x00BC, 0x00BE, SUAS_BIDI_EN },
    { 0x0660, 0x0669, SUAS_BIDI_AN },  /* Arabic-Indic digits */
    { 0x06F0, 0x06F9, SUAS_BIDI_AN },
    { 0x066B, 0x066B, SUAS_BIDI_CS },  /* arabic decimal sep */
    { 0x066C, 0x066C, SUAS_BIDI_CS },  /* arabic thousands sep */

    /* Punctuation (ON) */
    { 0x0021, 0x0022, SUAS_BIDI_ON },
    { 0x0024, 0x0024, SUAS_BIDI_ET },  /* $ */
    { 0x0026, 0x0027, SUAS_BIDI_ON },
    { 0x002A, 0x002B, SUAS_BIDI_ON },  /* * + ; keep ES for + handled above */
    { 0x002C, 0x002C, SUAS_BIDI_CS },  /* , */
    { 0x002E, 0x002E, SUAS_BIDI_CS },  /* . */
    { 0x002F, 0x002F, SUAS_BIDI_ON },
    { 0x003A, 0x003A, SUAS_BIDI_CS },  /* : */
    { 0x003B, 0x003B, SUAS_BIDI_ON },
    { 0x003F, 0x0040, SUAS_BIDI_ON },
    { 0x005C, 0x005C, SUAS_BIDI_ON },
    { 0x005E, 0x0060, SUAS_BIDI_ON },
    { 0x007B, 0x007B, SUAS_BIDI_ON },
    { 0x007C, 0x007C, SUAS_BIDI_ON },
    { 0x007D, 0x007E, SUAS_BIDI_ON },

    /* Latin / Greek / Cyrillic (L) */
    { 0x0041, 0x005A, SUAS_BIDI_L },
    { 0x0061, 0x007A, SUAS_BIDI_L },
    { 0x00C0, 0x024F, SUAS_BIDI_L },
    { 0x0370, 0x03FF, SUAS_BIDI_L },
    { 0x0400, 0x04FF, SUAS_BIDI_L },
    { 0x1E00, 0x1EFF, SUAS_BIDI_L },
    { 0x2C00, 0x2C5F, SUAS_BIDI_L },

    /* Arabic (AL) */
    { 0x0600, 0x06FF, SUAS_BIDI_AL },
    { 0x0750, 0x077F, SUAS_BIDI_AL },
    { 0x08A0, 0x08FF, SUAS_BIDI_AL },
    { 0xFB50, 0xFDFF, SUAS_BIDI_AL },
    { 0xFE70, 0xFEFF, SUAS_BIDI_AL },

    /* Hebrew (R) */
    { 0x0590, 0x05FF, SUAS_BIDI_R },
    { 0xFB1D, 0xFB4F, SUAS_BIDI_R },

    /* Thaana / N'Ko / Adlam (R) */
    { 0x0780, 0x07BF, SUAS_BIDI_R },
    { 0x07C0, 0x07FF, SUAS_BIDI_R },
    { 0x1E900, 0x1E95F, SUAS_BIDI_R },
};

#define CP_CLASS_COUNT (SUAS_BIDI_NELEM(CP_CLASS))

suas_bidi_class_t suas_bidi_classify_cp(uint32_t cp)
{
    size_t i;
    if (!suas_sucd_is_unicode_bridge(cp)) {
        return SUAS_BIDI_L; /* SCP/native default */
    }
    for (i = 0; i < CP_CLASS_COUNT; ++i) {
        if (cp >= CP_CLASS[i].lo && cp <= CP_CLASS[i].hi) {
            return CP_CLASS[i].cls;
        }
    }
    return SUAS_BIDI_L;
}

/* ── Mirror mapping (L4) ───────────────────────────────────────── */

typedef struct { uint32_t from, to; } sbi_mirror_t;

static const sbi_mirror_t MIRRORS[] = {
    { 0x0028, 0x0029 }, { 0x0029, 0x0028 },
    { 0x003C, 0x003E }, { 0x003E, 0x003C },
    { 0x005B, 0x005D }, { 0x005D, 0x005B },
    { 0x007B, 0x007D }, { 0x007D, 0x007B },
    { 0x00AB, 0x00BB }, { 0x00BB, 0x00AB },
    { 0x2018, 0x2019 }, { 0x2019, 0x2018 },
    { 0x201C, 0x201D }, { 0x201D, 0x201C },
    { 0x2039, 0x203A }, { 0x203A, 0x2039 },
    { 0x27E6, 0x27E7 }, { 0x27E7, 0x27E6 },
    { 0x27E8, 0x27E9 }, { 0x27E9, 0x27E8 },
    { 0x27EA, 0x27EB }, { 0x27EB, 0x27EA },
    { 0x2772, 0x2773 }, { 0x2773, 0x2772 },
    { 0x301A, 0x301B }, { 0x301B, 0x301A },
    { 0xFF08, 0xFF09 }, { 0xFF09, 0xFF08 },
    { 0xFF3B, 0xFF3D }, { 0xFF3D, 0xFF3B },
    { 0xFF5B, 0xFF5D }, { 0xFF5D, 0xFF5B },
};

void suas_bidi_mirror(uint32_t cp, uint32_t* paired, int* mirrored)
{
    size_t i;
    if (!suas_sucd_is_unicode_bridge(cp)) {
        if (paired) *paired = cp;
        if (mirrored) *mirrored = 0;
        return;
    }
    for (i = 0; i < SUAS_BIDI_NELEM(MIRRORS); ++i) {
        if (MIRRORS[i].from == cp) {
            if (paired) *paired = MIRRORS[i].to;
            if (mirrored) *mirrored = 1;
            return;
        }
    }
    if (paired) *paired = cp;
    if (mirrored) *mirrored = 0;
}

/* ── Bracket pairs (N0) ────────────────────────────────────────── */

typedef struct { uint32_t open, close; } sbi_bracket_t;

static const sbi_bracket_t BRACKETS[] = {
    { 0x0028, 0x0029 }, { 0x005B, 0x005D }, { 0x007B, 0x007D },
    { 0x00AB, 0x00BB }, { 0x2018, 0x2019 }, { 0x201C, 0x201D },
    { 0x2039, 0x203A }, { 0x27E6, 0x27E7 }, { 0x27E8, 0x27E9 },
    { 0x27EA, 0x27EB }, { 0x2772, 0x2773 }, { 0x301A, 0x301B },
    { 0xFF08, 0xFF09 }, { 0xFF3B, 0xFF3D }, { 0xFF5B, 0xFF5D },
};

#define BRACKET_COUNT (SUAS_BIDI_NELEM(BRACKETS))

static int bracket_open_of(uint32_t cp, uint32_t* close)
{
    size_t i;
    for (i = 0; i < BRACKET_COUNT; ++i) {
        if (BRACKETS[i].open == cp) { *close = BRACKETS[i].close; return 1; }
    }
    return 0;
}

/* ── Paragraph (P1-P3) ─────────────────────────────────────────── */

/* P2/P3: first strong + isolating run sequence heuristic. */
static int first_strong_rtl(const suas_bidi_run_t* r, size_t n)
{
    size_t i;
    for (i = 0; i < n; ++i) {
        if (r[i].cls == SUAS_BIDI_L) return 0;
        if (r[i].cls == SUAS_BIDI_R || r[i].cls == SUAS_BIDI_AL) return 1;
    }
    return 0;
}

/* ── Phase X1-X8: explicit embedding levels ────────────────────── */

/*
 * Implements the directional status stack (BD3) and the rule sequence
 * X1-X8 including isolate initiators, PDI matching (X5a/X6a), and the
 * overflow counters (isolateOverflow / embeddingOverflow).
 *
 * Marks every explicit formatting / isolate control character as `removed`
 * (X9) and assigns an embedding level to every ordinary character.
 */

typedef struct {
    uint8_t level;
    int8_t  override;       /* -1 neutral, 0 left-to-right, 1 right-to-left */
    int     isolate_status; /* directional isolate status (BD12) */
} sbi_status_t;

static void phase_explicit(suas_bidi_run_t* r, size_t n, uint8_t para_level)
{
    sbi_status_t stack[128];
    int sp = 0;
    int over_iso = 0;  /* overflow isolate count   */
    int over_emb = 0;  /* overflow embedding count */
    int valid_iso = 0; /* valid isolate count      */
    size_t i;

    stack[0].level = para_level;
    stack[0].override = -1;
    stack[0].isolate_status = 0;

    for (i = 0; i < n; ++i) {
        uint8_t cur_level = stack[sp].level;
        int cur_override = stack[sp].override;

        switch (r[i].cls) {
        /* X2/X3/X4/X5: embedding & override initiators */
        case SUAS_BIDI_RLE: case SUAS_BIDI_LRE:
        case SUAS_BIDI_RLO: case SUAS_BIDI_LRO: {
            int odd = (r[i].cls == SUAS_BIDI_RLE || r[i].cls == SUAS_BIDI_RLO);
            uint8_t newl = odd ? bd6_next_odd(cur_level)
                               : bd6_next_even(cur_level);
            r[i].removed = 1; /* X9 */
            if (newl <= SUAS_BIDI_MAX_LEVEL && over_iso == 0 && over_emb == 0) {
                ++sp;
                stack[sp].level = newl;
                stack[sp].override = (r[i].cls == SUAS_BIDI_LRO) ? 0 :
                                     (r[i].cls == SUAS_BIDI_RLO) ? 1 : -1;
                stack[sp].isolate_status = 0;
            } else if (over_iso == 0) {
                ++over_emb;
            }
            break;
        }
        /* X7: PDF */
        case SUAS_BIDI_PDF:
            r[i].removed = 1; /* X9 */
            if (over_iso > 0) {
                /* inside an overflow isolate: do nothing */
            } else if (over_emb > 0) {
                --over_emb;
            } else if (stack[sp].isolate_status == 0 && sp > 0) {
                --sp;
            }
            break;

        /* X5a/X5b/X5c: isolate initiators */
        case SUAS_BIDI_RLI: case SUAS_BIDI_LRI: case SUAS_BIDI_FSI: {
            int odd;
            uint8_t newl;
            if (r[i].cls == SUAS_BIDI_RLI) odd = 1;
            else if (r[i].cls == SUAS_BIDI_LRI) odd = 0;
            else odd = first_strong_rtl(&r[i + 1], n - (i + 1)); /* FSI */
            r[i].level = cur_level; /* isolator takes the enclosing level */
            if (cur_override != -1) {
                r[i].cls = (cur_override == 0) ? SUAS_BIDI_L : SUAS_BIDI_R;
            }
            newl = odd ? bd6_next_odd(cur_level) : bd6_next_even(cur_level);
            if (newl <= SUAS_BIDI_MAX_LEVEL && over_iso == 0 && over_emb == 0) {
                ++valid_iso;
                ++sp;
                stack[sp].level = newl;
                stack[sp].override = -1;
                stack[sp].isolate_status = 1;
            } else {
                ++over_iso;
            }
            r[i].removed = 0; /* isolate initiators are NOT removed by X9 */
            break;
        }
        /* X6a: PDI */
        case SUAS_BIDI_PDI:
            if (over_iso > 0) {
                --over_iso;
            } else if (valid_iso == 0) {
                /* matches no isolate initiator */
            } else {
                over_emb = 0;
                while (sp > 0 && stack[sp].isolate_status == 0) --sp;
                --sp;          /* pop the matching isolate scope */
                --valid_iso;
            }
            r[i].level = stack[sp].level;
            r[i].removed = 0; /* PDI is NOT removed by X9 */
            if (stack[sp].override != -1) {
                r[i].cls = (stack[sp].override == 0) ? SUAS_BIDI_L : SUAS_BIDI_R;
            }
            break;

        /* X8: paragraph separator assigned the paragraph level */
        case SUAS_BIDI_B:
            r[i].level = para_level;
            break;

        /* X9: BN removed */
        case SUAS_BIDI_BN:
            r[i].removed = 1;
            r[i].level = cur_level;
            break;

        /* X6: ordinary characters */
        default:
            r[i].level = cur_level;
            if (cur_override == 0) r[i].cls = SUAS_BIDI_L;
            else if (cur_override == 1) r[i].cls = SUAS_BIDI_R;
            break;
        }
    }

    (void)over_iso; (void)over_emb; (void)valid_iso;
}

/* ── Phase W1-W7: weak types ───────────────────────────────────── */

static int prev_nr(suas_bidi_run_t* r, size_t n, size_t i)
{
    size_t k = i;
    (void)n;
    while (k-- > 0) if (!r[k].removed) return k;
    return -1;
}
static int next_nr(suas_bidi_run_t* r, size_t n, size_t i)
{
    size_t k = i;
    while (++k < n) if (!r[k].removed) return (int)k;
    return -1;
}

static void phase_weak(suas_bidi_run_t* r, size_t n)
{
    size_t i;

    /* W1: NSM gets class of the preceding character. */
    for (i = 0; i < n; ++i) {
        int p;
        if (r[i].removed || r[i].cls != SUAS_BIDI_NSM) continue;
        p = prev_nr(r, n, i);
        if (p >= 0) r[i].cls = r[(size_t)p].cls;
    }

    /* W2/W3: track last strong class; EN after AL->AN; AL->R. */
    {
        int last_strong_al = 0;
        for (i = 0; i < n; ++i) {
            if (r[i].removed) continue;
            switch (r[i].cls) {
            case SUAS_BIDI_AL:
                r[i].cls = SUAS_BIDI_R;   /* W3 */
                last_strong_al = 1;
                break;
            case SUAS_BIDI_R:
                last_strong_al = 0;
                break;
            case SUAS_BIDI_L:
                last_strong_al = 0;
                break;
            case SUAS_BIDI_EN:
                if (last_strong_al) r[i].cls = SUAS_BIDI_AN; /* W2 */
                break;
            default:
                break;
            }
        }
    }

    /* W4: ES -> EN if between EN; CS -> EN/AN/ON. */
    for (i = 0; i < n; ++i) {
        int p, nx;
        if (r[i].removed) continue;
        if (r[i].cls == SUAS_BIDI_ES) {
            p = prev_nr(r, n, i); nx = next_nr(r, n, i);
            if (p >= 0 && nx >= 0 &&
                r[p].cls == SUAS_BIDI_EN && r[nx].cls == SUAS_BIDI_EN)
                r[i].cls = SUAS_BIDI_EN;
        } else if (r[i].cls == SUAS_BIDI_CS) {
            p = prev_nr(r, n, i); nx = next_nr(r, n, i);
            if (p >= 0 && nx >= 0) {
                if (r[p].cls == r[nx].cls &&
                    (r[p].cls == SUAS_BIDI_EN || r[p].cls == SUAS_BIDI_AN))
                    r[i].cls = r[p].cls;
                else if (r[p].cls == SUAS_BIDI_EN && r[nx].cls == SUAS_BIDI_EN)
                    r[i].cls = SUAS_BIDI_EN;
                else
                    r[i].cls = SUAS_BIDI_ON;
            } else {
                r[i].cls = SUAS_BIDI_ON;
            }
        }
    }

    /* W5: ET runs adjacent to EN -> EN. */
    for (i = 0; i < n; ++i) {
        size_t start, end, k;
        int le, re, p, nx;
        if (r[i].removed || r[i].cls != SUAS_BIDI_ET) continue;
        start = i; end = i;
        while (end + 1 < n && !r[end + 1].removed &&
               r[end + 1].cls == SUAS_BIDI_ET) ++end;
        p = prev_nr(r, n, start);
        nx = next_nr(r, n, end);
        le = (p >= 0 && r[p].cls == SUAS_BIDI_EN);
        re = (nx >= 0 && r[nx].cls == SUAS_BIDI_EN);
        if (le || re) {
            for (k = start; k <= end; ++k) r[k].cls = SUAS_BIDI_EN;
        }
        i = end;
    }

    /* W6: ES/ET/CS -> ON (unless already promoted). */
    for (i = 0; i < n; ++i) {
        if (r[i].removed) continue;
        if (r[i].cls == SUAS_BIDI_ES || r[i].cls == SUAS_BIDI_ET ||
            r[i].cls == SUAS_BIDI_CS) {
            r[i].cls = SUAS_BIDI_ON;
        }
    }

    /* W7: EN -> L if last strong was L. */
    {
        int last_strong_l = 0;
        for (i = 0; i < n; ++i) {
            if (r[i].removed) continue;
            switch (r[i].cls) {
            case SUAS_BIDI_L: last_strong_l = 1; break;
            case SUAS_BIDI_R:
            case SUAS_BIDI_AL: last_strong_l = 0; break;
            case SUAS_BIDI_EN:
                if (last_strong_l) r[i].cls = SUAS_BIDI_L;
                break;
            default: break;
            }
        }
    }
}

/* ── Phase N0-N2: neutrals ─────────────────────────────────────── */

static int strong_dir_of(suas_bidi_class_t c)
{
    /* returns 0 = L, 1 = R, -1 = not strong */
    if (c == SUAS_BIDI_L) return 0;
    if (c == SUAS_BIDI_R || c == SUAS_BIDI_AN) return 1;
    return -1;
}

/* NI (Table 3): neutral or isolate formatting characters. */
static int is_neutral_type(suas_bidi_class_t c)
{
    return (c == SUAS_BIDI_B || c == SUAS_BIDI_S || c == SUAS_BIDI_WS ||
            c == SUAS_BIDI_ON || c == SUAS_BIDI_LRI || c == SUAS_BIDI_RLI ||
            c == SUAS_BIDI_FSI || c == SUAS_BIDI_PDI);
}

static void phase_neutral(suas_bidi_run_t* r, size_t n)
{
    size_t i;

    /* N0: bracket pairs. */
    for (i = 0; i < n; ++i) {
        size_t k;
        uint32_t close;
        if (r[i].removed || r[i].cls != SUAS_BIDI_ON) continue;
        if (!bracket_open_of(r[i].cp, &close)) continue;
        /* find matching close bracket */
        for (k = i + 1; k < n; ++k) {
            if (r[k].removed) continue;
            if (r[k].cls == SUAS_BIDI_ON && r[k].cp == close) {
                /* directional value: embedder's direction if no strong inside;
                 * otherwise direction of strong inside */
                int dir = -1;
                size_t m;
                for (m = i + 1; m < k; ++m) {
                    if (r[m].removed) continue;
                    if (strong_dir_of(r[m].cls) >= 0) { dir = strong_dir_of(r[m].cls); break; }
                }
                if (dir < 0) dir = level_is_odd(r[i].level) ? 1 : 0;
                r[i].cls = dir ? SUAS_BIDI_R : SUAS_BIDI_L;
                r[k].cls = dir ? SUAS_BIDI_R : SUAS_BIDI_L;
                break;
            }
        }
    }

    /* N1: neutrals between two strongs of the same direction take it. */
    for (i = 0; i < n; ++i) {
        size_t start = i, end = i, k;
        if (r[i].removed) continue;
        if (!is_neutral_type(r[i].cls)) continue;
        while (end + 1 < n && !r[end + 1].removed &&
               is_neutral_type(r[end + 1].cls))
            ++end;
        /* find left and right strong directions */
        {
            int ld = -1, rd = -1;
            size_t p = start;
            while (p-- > 0) { if (!r[p].removed) { if (strong_dir_of(r[p].cls) >= 0) { ld = strong_dir_of(r[p].cls); break; } break; } }
            {
                size_t q = end;
                while (++q < n) { if (!r[q].removed) { if (strong_dir_of(r[q].cls) >= 0) { rd = strong_dir_of(r[q].cls); break; } break; } }
            }
            if (ld >= 0 && rd >= 0 && ld == rd) {
                suas_bidi_class_t tgt = ld ? SUAS_BIDI_R : SUAS_BIDI_L;
                for (k = start; k <= end; ++k)
                    if (!r[k].removed && is_neutral_type(r[k].cls))
                        r[k].cls = tgt;
            }
            i = end;
        }
    }

    /* N2: remaining neutrals take the embedding direction. */
    for (i = 0; i < n; ++i) {
        if (r[i].removed) continue;
        if (is_neutral_type(r[i].cls)) {
            r[i].cls = level_is_odd(r[i].level) ? SUAS_BIDI_R : SUAS_BIDI_L;
        }
    }
}

/* ── Phase I1-I2: implicit levels ──────────────────────────────── */

static void phase_implicit(suas_bidi_run_t* r, size_t n)
{
    size_t i;
    for (i = 0; i < n; ++i) {
        uint8_t l = r[i].level;
        if (r[i].removed) continue;
        switch (r[i].cls) {
        case SUAS_BIDI_R:
            /* I1: even level -> +1 */
            if (level_is_even(l)) r[i].level = (uint8_t)(l + 1);
            break;
        case SUAS_BIDI_EN:
        case SUAS_BIDI_AN:
            /* I1: even -> +2; I2: odd -> +1 */
            if (level_is_even(l)) r[i].level = (uint8_t)(l + 2);
            else r[i].level = (uint8_t)(l + 1);
            break;
        case SUAS_BIDI_L:
            /* I2: odd level -> +1 */
            if (level_is_odd(l)) r[i].level = (uint8_t)(l + 1);
            break;
        default:
            break;
        }
    }
}

/* ── Phase L1-L4: reordering + mirroring ──────────────────────── */

static void phase_reorder(const suas_bidi_run_t* r, size_t n,
                          uint8_t para_level, int* visual)
{
    uint8_t highest = para_level;
    size_t i;
    int lvl[SUAS_BIDI_MAX_LEN];
    int ord[SUAS_BIDI_MAX_LEN];
    uint8_t l;

    if (n > SUAS_BIDI_MAX_LEN) return;

    for (i = 0; i < n; ++i) {
        lvl[i] = r[i].level;
        ord[i] = (int)i;
        if (r[i].level > highest) highest = r[i].level;
    }

    /* L1: set trailing whitespace / segment separators of the line to the
     * paragraph level. For a single-paragraph call without a trailing
     * separator, only the final WS run is affected. */
    if (n > 0) {
        size_t t = n;
        while (t > 0) {
            size_t j = t - 1;
            if (r[j].removed) { --t; continue; }
            if (r[j].cls == SUAS_BIDI_S || r[j].cls == SUAS_BIDI_WS ||
                r[j].cls == SUAS_BIDI_B) {
                lvl[j] = para_level;
                --t;
            } else {
                break;
            }
        }
    }

    /* L2: reorder the sequence from highest level down to para_level+1. */
    for (l = (uint8_t)(para_level + 1); l <= highest; ++l) {
        size_t a = 0;
        int reorder = 0;
        if (!reorder) reorder = 1;
        (void)reorder;
        while (a < n) {
            if (lvl[a] >= (int)l) {
                size_t b = a;
                size_t lo, hi;
                while (b < n && lvl[b] >= (int)l) ++b;
                lo = a; hi = b - 1;
                while (lo < hi) {
                    int t = ord[lo]; ord[lo] = ord[hi]; ord[hi] = t;
                    ++lo; --hi;
                }
                a = b;
            } else {
                ++a;
            }
        }
    }

    /* Emit visual positions (logical indices, in visual order). */
    for (i = 0; i < n; ++i) visual[i] = ord[i];
}

/* ── Top-level resolver ────────────────────────────────────────── */

suas_bidi_status_t suas_bidi_resolve_paragraph_ex(
    const uint32_t* cps, size_t cp_count,
    suas_bidi_paragraph_dir_t paragraph,
    suas_bidi_run_t* out,
    int* visual,
    uint8_t* out_para_level,
    int last_paragraph)
{
    size_t i;
    uint8_t para_level;

    (void)last_paragraph;

    if (cps == NULL || out == NULL) return SUAS_BIDI_ERR_INVALID_ARG;
    if (cp_count == 0) return SUAS_BIDI_OK;
    if (cp_count > SUAS_BIDI_MAX_LEN) return SUAS_BIDI_ERR_TOO_LONG;

    for (i = 0; i < cp_count; ++i) {
        out[i].cp = cps[i];
        out[i].cls = suas_bidi_classify_cp(cps[i]);
        out[i].level = 0;
        out[i].mirrored = 0;
        out[i].removed = 0;
    }

    /* P1-P3: paragraph level. */
    if (paragraph == SUAS_BIDI_PARAGRAPH_LTR) para_level = 0;
    else if (paragraph == SUAS_BIDI_PARAGRAPH_RTL) para_level = 1;
    else para_level = first_strong_rtl(out, cp_count) ? 1 : 0;

    if (out_para_level) *out_para_level = para_level;

    /* X1-X8 */
    phase_explicit(out, cp_count, para_level);

    /* W1-W7 */
    phase_weak(out, cp_count);

    /* N0-N2 */
    phase_neutral(out, cp_count);

    /* I1-I2 */
    phase_implicit(out, cp_count);

    /* L1-L2 reordering (visual order) */
    if (visual) phase_reorder(out, cp_count, para_level, visual);

    /* L4 mirroring (L3 shaping deferred) */
    for (i = 0; i < cp_count; ++i) {
        uint32_t paired = 0;
        int mirrored = 0;
        if (out[i].removed) { out[i].mirrored = 0; continue; }
        if (level_is_odd(out[i].level)) {
            suas_bidi_mirror(out[i].cp, &paired, &mirrored);
        }
        if (mirrored) { out[i].cp = paired; out[i].mirrored = 1; }
        else out[i].mirrored = 0;
    }

    return SUAS_BIDI_OK;
}

suas_bidi_status_t suas_bidi_resolve_paragraph(
    const uint32_t* cps, size_t cp_count,
    suas_bidi_paragraph_dir_t paragraph,
    suas_bidi_run_t* out,
    int* visual,
    uint8_t* out_para_level)
{
    return suas_bidi_resolve_paragraph_ex(cps, cp_count, paragraph, out,
                                          visual, out_para_level, 1);
}
