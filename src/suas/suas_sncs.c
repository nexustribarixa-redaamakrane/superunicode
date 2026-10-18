/* SUAS-008 — System Named Character Sequences (SNCS)
 *
 * Reference implementation of docs/suas/SUAS-008-sncs.md.
 *
 * Freestanding C99, zero allocation. A static table of curated named
 * sequences is indexed once into a fixed-size open-addressing array, so that
 * a name resolves to its packed multi-codepoint token in O(1). No heap, no
 * dynamic table growth, no string parsing.
 *
 * The curated table is a subset of the Unicode named sequence registry for the
 * Bridge, plus a representative set of RNUR neography compound sequences for
 * the Native plane. Extending the table is a bridge-data update (a minor
 * revision) and never changes the API.
 */

#include "suas/suas_sncs.h"

/* ── Small string helpers (no <string.h>: freestanding subset only) ─ */

static int sncs_streq(const char* a, const char* b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

static int sncs_is_alnum(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9');
}

/* FNV-1a 32-bit over the (upper-case) name bytes. */
static uint32_t sncs_hash(const char* s)
{
    uint32_t h = 2166136261u;
    while (*s) {
        h ^= (uint8_t)*s++;
        h *= 16777619u;
    }
    return h;
}

/* ── The curated static registry (§3, §6.1) ───────────────────────── */

#define SNCS_CP(...) { __VA_ARGS__ }

static const suas_named_sequence_t sncs_table[] = {
    /* ── Bridge sequences: Unicode named character sequences (§3.1) ── */
    { .name = "LATIN SMALL LETTER A WITH CIRCUMFLEX AND ACUTE",
      .codepoints = SNCS_CP(0x0061ULL, 0x0302ULL, 0x0301ULL), .length = 3u },
    { .name = "LATIN SMALL LETTER E WITH CIRCUMFLEX AND ACUTE",
      .codepoints = SNCS_CP(0x0065ULL, 0x0302ULL, 0x0301ULL), .length = 3u },
    { .name = "LATIN SMALL LETTER I WITH MACRON AND GRAVE",
      .codepoints = SNCS_CP(0x012BULL, 0x0300ULL), .length = 2u },
    { .name = "LATIN CAPITAL LETTER X WITH RING ABOVE",
      .codepoints = SNCS_CP(0x0058ULL, 0x030AULL), .length = 2u },
    { .name = "MODIFIER LETTER EXTRA-HIGH EXTRA-LOW CONTOUR TONE BAR",
      .codepoints = SNCS_CP(0x02E5ULL, 0x02E9ULL), .length = 2u },
    { .name = "KATAKANA LETTER AINU P",
      .codepoints = SNCS_CP(0x31F7ULL, 0x309AULL), .length = 2u },
    { .name = "KHMER VOWEL SIGN OM",
      .codepoints = SNCS_CP(0x17BBULL, 0x17C6ULL), .length = 2u },
    { .name = "KHMER VOWEL SIGN AAM",
      .codepoints = SNCS_CP(0x17B6ULL, 0x17C6ULL), .length = 2u },
    { .name = "KHMER CONSONANT SIGN COENG KA",
      .codepoints = SNCS_CP(0x17D2ULL, 0x1780ULL), .length = 2u },
    { .name = "DEVANAGARI LETTER KA WITH NUKTA",
      .codepoints = SNCS_CP(0x0915ULL, 0x093CULL), .length = 2u },
    { .name = "BENGALI LETTER KA WITH NUKTA",
      .codepoints = SNCS_CP(0x0995ULL, 0x09BCULL), .length = 2u },
    { .name = "TAMIL CONSONANT K",
      .codepoints = SNCS_CP(0x0B95ULL, 0x0BCDULL), .length = 2u },
    { .name = "EMOJI FLAG JAPAN",
      .codepoints = SNCS_CP(0x1F1EFULL, 0x1F1F5ULL), .length = 2u },
    { .name = "EMOJI FLAG ENGLAND",
      .codepoints = SNCS_CP(0x1F3F4ULL, 0xE0067ULL, 0xE0062ULL, 0xE0065ULL,
                            0xE006EULL, 0xE0067ULL, 0xE007FULL), .length = 7u },
    { .name = "EMOJI FAMILY MAN WOMAN GIRL",
      .codepoints = SNCS_CP(0x1F468ULL, 0x200DULL, 0x1F469ULL, 0x200DULL,
                            0x1F467ULL), .length = 5u },

    /* ── Native & neography sequences: RNUR registry (§3.2) ────────── */
    { .name = "RNUR LIGATURE KA DHE",
      .codepoints = SNCS_CP(0x160000ULL, 0x160001ULL), .length = 2u },
    { .name = "RNUR LIGATURE KA DHI",
      .codepoints = SNCS_CP(0x160000ULL, 0x160002ULL), .length = 2u },
    { .name = "RNUR COMPOUND SIGN OM",
      .codepoints = SNCS_CP(0x160010ULL, 0x160011ULL, 0x160012ULL), .length = 3u },
    { .name = "RNUR DIPHTHONG AE",
      .codepoints = SNCS_CP(0x160020ULL, 0x160021ULL), .length = 2u },
    { .name = "RNUR TRIGRAPH KSH",
      .codepoints = SNCS_CP(0x160030ULL, 0x160031ULL, 0x160032ULL), .length = 3u },
    { .name = "RNUR VOWEL SIGN AA",
      .codepoints = SNCS_CP(0x160040ULL, 0x160045ULL), .length = 2u },
    { .name = "RNUR LOGOGRAPH DHARMA",
      .codepoints = SNCS_CP(0x160100ULL, 0x160101ULL), .length = 2u },
    { .name = "RNUR SIGN COENG KA",
      .codepoints = SNCS_CP(0x160200ULL, 0x160201ULL), .length = 2u },
};

#define SNCS_TABLE_COUNT ((uint32_t)(sizeof(sncs_table) / sizeof(sncs_table[0])))

/* ── The static lookup index (§6.2) ───────────────────────────────── */

typedef struct {
    uint8_t used; /* slot occupied                        */
    uint8_t seq;  /* index into sncs_table (capacity < 256) */
} sncs_slot_t;

static sncs_slot_t sncs_index[SUAS_SNCS_INDEX_SIZE];
static bool        sncs_ready = false;

static void sncs_build(void)
{
    uint32_t i;

    for (i = 0; i < SUAS_SNCS_INDEX_SIZE; i++) {
        sncs_index[i].used = 0u;
        sncs_index[i].seq = 0u;
    }
    for (i = 0; i < SNCS_TABLE_COUNT; i++) {
        uint32_t h = sncs_hash(sncs_table[i].name) &
                     (SUAS_SNCS_INDEX_SIZE - 1u);
        while (sncs_index[h].used)
            h = (h + 1u) & (SUAS_SNCS_INDEX_SIZE - 1u);
        sncs_index[h].used = 1u;
        sncs_index[h].seq = (uint8_t)i;
    }
    sncs_ready = true;
}

/* ── Setup and names ──────────────────────────────────────────────── */

const char* suas_sncs_version_string(void)
{
    return "1.0.0";
}

const char* suas_sncs_registry_name(suas_sncs_registry_t registry)
{
    return registry == SUAS_SNCS_REGISTRY_NATIVE ? "Native" : "Bridge";
}

void suas_sncs_init_registry(void)
{
    sncs_build();
}

/* ── Resolution (§6.2) — O(1) ─────────────────────────────────────── */

bool suas_sncs_resolve_sequence(const char* name,
                                suas_named_sequence_t* out_seq)
{
    uint32_t h;
    uint32_t start;

    if (!name || !out_seq)
        return false;
    if (!sncs_ready)
        sncs_build(); /* lazy build, so resolve is safe without setup */

    h = sncs_hash(name) & (SUAS_SNCS_INDEX_SIZE - 1u);
    start = h;
    for (;;) {
        const suas_named_sequence_t* cand;
        if (!sncs_index[h].used)
            return false;
        cand = &sncs_table[sncs_index[h].seq];
        if (sncs_streq(cand->name, name)) {
            *out_seq = *cand; /* fixed-size struct copy: no allocation */
            return true;
        }
        h = (h + 1u) & (SUAS_SNCS_INDEX_SIZE - 1u);
        if (h == start)
            return false;
    }
}

/* ── Enumeration and classification (§3, §6) ──────────────────────── */

uint32_t suas_sncs_registry_count(void)
{
    return SNCS_TABLE_COUNT;
}

bool suas_sncs_sequence_at(uint32_t index, suas_named_sequence_t* out_seq)
{
    if (!out_seq || index >= SNCS_TABLE_COUNT)
        return false;
    *out_seq = sncs_table[index];
    return true;
}

suas_sncs_registry_t suas_sncs_registry_of(const suas_named_sequence_t* seq)
{
    uint32_t k;

    if (!seq)
        return SUAS_SNCS_REGISTRY_BRIDGE;
    for (k = 0; k < seq->length; k++) {
        if (suas_sncs_is_native_codepoint(seq->codepoints[k]))
            return SUAS_SNCS_REGISTRY_NATIVE;
    }
    return SUAS_SNCS_REGISTRY_BRIDGE;
}

/* ── Zone predicates ──────────────────────────────────────────────── */

bool suas_sncs_is_bridge_codepoint(sucs_ex_char_t cp)
{
    return cp <= SUAS_SNCS_ZONE_UNICODE_MAX;
}

bool suas_sncs_is_native_codepoint(sucs_ex_char_t cp)
{
    if (cp >= SUAS_SNCS_ZONE_NATIVE_MIN && cp <= SUAS_SNCS_ZONE_NATIVE_LAST)
        return true;
    return cp > SUAS_SNCS_ZONE_SENTINEL; /* extSUCS plugin plane */
}

/* ── Name well-formedness (§4) ────────────────────────────────────── */

bool suas_sncs_is_valid_name(const char* name)
{
    size_t i = 0;
    int at_word_start = 1;

    if (!name || !name[0])
        return false;
    for (; name[i]; i++) {
        char c = name[i];
        if (c == ' ') {
            at_word_start = 1;
            continue;
        }
        if (c == '-') {
            /* R1 admits HYPHEN-MINUS; a leading hyphen does not begin a
             * word, so it does not clear the word-start state. */
            continue;
        }
        if (c >= 'A' && c <= 'Z') {
            at_word_start = 0;
            continue;
        }
        if (c >= '0' && c <= '9') {
            if (at_word_start) /* a digit may not begin a word */
                return false;
            at_word_start = 0;
            continue;
        }
        return false; /* anything else is not part of an SNCS name */
    }
    return i < SUAS_SNCS_MAX_NAME_LEN;
}

static int sncs_is_ignored_word(const char* w)
{
    return sncs_streq(w, "LETTER") || sncs_streq(w, "CHARACTER") ||
           sncs_streq(w, "DIGIT");
}

/* Reduce a name to its R3 canonical form: drop SPACE and medial
 * HYPHEN-MINUS, and drop the words "LETTER", "CHARACTER", and "DIGIT". */
static size_t sncs_canon(const char* name, char* out, size_t cap)
{
    size_t i = 0;
    size_t o = 0;

    while (name[i]) {
        char   word[SUAS_SNCS_MAX_NAME_LEN];
        size_t w = 0;
        size_t j;

        while (name[i] && name[i] != ' ') {
            if (w + 1u < sizeof(word))
                word[w++] = name[i];
            i++;
        }
        word[w] = '\0';
        while (name[i] == ' ')
            i++;

        if (w == 0u || sncs_is_ignored_word(word))
            continue;

        for (j = 0; j < w; j++) {
            char c = word[j];
            if (c == '-') {
                int prev = (j > 0u) && sncs_is_alnum(word[j - 1u]);
                int next = (j + 1u < w) && sncs_is_alnum(word[j + 1u]);
                if (prev && next)
                    continue; /* medial hyphen: ignored for uniqueness */
            }
            if (o + 1u < cap)
                out[o++] = c;
        }
    }
    out[o] = '\0';
    return o;
}

bool suas_sncs_names_equal(const char* a, const char* b)
{
    char ca[SUAS_SNCS_MAX_NAME_LEN * 2u];
    char cb[SUAS_SNCS_MAX_NAME_LEN * 2u];

    if (!a || !b)
        return false;
    sncs_canon(a, ca, sizeof(ca));
    sncs_canon(b, cb, sizeof(cb));
    return sncs_streq(ca, cb);
}

/* ── Sequence well-formedness and comparison ──────────────────────── */

bool suas_sncs_is_well_formed(const suas_named_sequence_t* seq)
{
    uint32_t k;

    if (!seq || !suas_sncs_is_valid_name(seq->name))
        return false;
    if (seq->length < 2u || seq->length > SUAS_SNCS_MAX_SEQUENCE_LEN)
        return false;
    for (k = 0; k < seq->length; k++) {
        sucs_ex_char_t cp = seq->codepoints[k];
        if (cp >= SUAS_SNCS_ZONE_SCP_MIN && cp <= SUAS_SNCS_ZONE_SCP_MAX)
            return false; /* SCP markers never appear inside a sequence */
        if (cp >= SUAS_SNCS_ZONE_TRAP_MIN && cp <= SUAS_SNCS_ZONE_SENTINEL)
            return false; /* trap slots and the Sentinel */
    }
    return true;
}

bool suas_sncs_sequence_equals(const suas_named_sequence_t* a,
                               const suas_named_sequence_t* b)
{
    uint32_t k;

    if (!a || !b || a->length != b->length)
        return false;
    for (k = 0; k < a->length; k++) {
        if (a->codepoints[k] != b->codepoints[k])
            return false;
    }
    return true;
}
