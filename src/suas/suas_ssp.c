/* SUAS-005 — System Script Property (SSP)
 *
 * Reference implementation of docs/suas/SUAS-005-ssp.md.
 *
 * Freestanding C99, zero allocation. A range-dispatched engine resolves the
 * 16-bit script id for any SUCS / ExtSUCS codepoint in O(log R) time from
 * immutable curated tables:
 *
 *   Unicode Bridge (0x000000-0x10FFFF) : explicit + Common range tables,
 *                                        with a curated Script_Extensions set;
 *   System Control Plane (0x110000-..) : script-context directives decode to
 *                                        their selected script, O(1);
 *   RNUR carve-out (0x160000-0x16FFFF) : registry carve-out, no script mounted
 *                                        yet;
 *   Native plane / ExtSUCS             : unknown unless registered.
 *
 * The tables are a curated subset of sucd/Scripts.txt + ScriptExtensions.txt;
 * SUCD is the source of truth. See the header for the full contract.
 */

#include "suas/suas_ssp.h"

#define SSP_NELEM(a) (sizeof((a)) / sizeof((a)[0]))

/* ── Explicit Bridge script ranges (sorted ascending by lo) ──────────
 * Non-overlapping. A codepoint not in this table (nor in the Common table)
 * is Unknown. This is a representative subset of sucd/Scripts.txt covering
 * the scripts exercised by the other SUAS/SUTS faces. */
typedef struct {
    uint32_t      lo;
    uint32_t      hi;
    suas_script_t script;
} ssp_range_t;

static const ssp_range_t SSP_SCRIPT_TABLE[] = {
    { 0x0041, 0x005A, SUAS_SCRIPT_LATIN },
    { 0x0061, 0x007A, SUAS_SCRIPT_LATIN },
    { 0x00AA, 0x00AA, SUAS_SCRIPT_LATIN },
    { 0x00BA, 0x00BA, SUAS_SCRIPT_LATIN },
    { 0x00C0, 0x00D6, SUAS_SCRIPT_LATIN },
    { 0x00D8, 0x00F6, SUAS_SCRIPT_LATIN },
    { 0x00F8, 0x02B8, SUAS_SCRIPT_LATIN },
    { 0x0370, 0x0373, SUAS_SCRIPT_GREEK },
    { 0x0376, 0x0377, SUAS_SCRIPT_GREEK },
    { 0x037A, 0x037D, SUAS_SCRIPT_GREEK },
    { 0x037F, 0x037F, SUAS_SCRIPT_GREEK },
    { 0x0386, 0x0386, SUAS_SCRIPT_GREEK },
    { 0x0388, 0x038A, SUAS_SCRIPT_GREEK },
    { 0x038C, 0x038C, SUAS_SCRIPT_GREEK },
    { 0x038E, 0x03A1, SUAS_SCRIPT_GREEK },
    { 0x03A3, 0x03E1, SUAS_SCRIPT_GREEK },
    { 0x03E2, 0x03EF, SUAS_SCRIPT_COPTIC },
    { 0x03F0, 0x03FF, SUAS_SCRIPT_GREEK },
    { 0x0400, 0x052F, SUAS_SCRIPT_CYRILLIC },
    { 0x0531, 0x0556, SUAS_SCRIPT_ARMENIAN },
    { 0x0560, 0x0588, SUAS_SCRIPT_ARMENIAN },
    { 0x0591, 0x05C7, SUAS_SCRIPT_HEBREW },
    { 0x05D0, 0x05EA, SUAS_SCRIPT_HEBREW },
    { 0x05EF, 0x05F4, SUAS_SCRIPT_HEBREW },
    { 0x0600, 0x0604, SUAS_SCRIPT_ARABIC },
    { 0x0606, 0x060B, SUAS_SCRIPT_ARABIC },
    { 0x060D, 0x061A, SUAS_SCRIPT_ARABIC },
    { 0x061C, 0x061E, SUAS_SCRIPT_ARABIC },
    { 0x0620, 0x063F, SUAS_SCRIPT_ARABIC },
    { 0x0641, 0x064A, SUAS_SCRIPT_ARABIC },
    { 0x0656, 0x066F, SUAS_SCRIPT_ARABIC },
    { 0x0671, 0x06DC, SUAS_SCRIPT_ARABIC },
    { 0x06DE, 0x06FF, SUAS_SCRIPT_ARABIC },
    { 0x0700, 0x070D, SUAS_SCRIPT_SYRIAC },
    { 0x0710, 0x074A, SUAS_SCRIPT_SYRIAC },
    { 0x074D, 0x074F, SUAS_SCRIPT_SYRIAC },
    { 0x0750, 0x077F, SUAS_SCRIPT_ARABIC },
    { 0x0860, 0x086A, SUAS_SCRIPT_SYRIAC },
    { 0x08A0, 0x08FF, SUAS_SCRIPT_ARABIC },
    { 0x0900, 0x0950, SUAS_SCRIPT_DEVANAGARI },
    { 0x0953, 0x0963, SUAS_SCRIPT_DEVANAGARI },
    { 0x0966, 0x097F, SUAS_SCRIPT_DEVANAGARI },
    { 0x0E01, 0x0E3A, SUAS_SCRIPT_THAI },
    { 0x0E40, 0x0E5B, SUAS_SCRIPT_THAI },
    { 0x0E81, 0x0E82, SUAS_SCRIPT_LAO },
    { 0x0E84, 0x0E84, SUAS_SCRIPT_LAO },
    { 0x0E86, 0x0E8A, SUAS_SCRIPT_LAO },
    { 0x0E8C, 0x0EA3, SUAS_SCRIPT_LAO },
    { 0x0EA5, 0x0EA5, SUAS_SCRIPT_LAO },
    { 0x0EA7, 0x0EBD, SUAS_SCRIPT_LAO },
    { 0x0EC0, 0x0EC4, SUAS_SCRIPT_LAO },
    { 0x0EC6, 0x0EC6, SUAS_SCRIPT_LAO },
    { 0x0EC8, 0x0ECD, SUAS_SCRIPT_LAO },
    { 0x0ED0, 0x0ED9, SUAS_SCRIPT_LAO },
    { 0x0EDC, 0x0EDF, SUAS_SCRIPT_LAO },
    { 0x0F00, 0x0F47, SUAS_SCRIPT_TIBETAN },
    { 0x0F49, 0x0F6C, SUAS_SCRIPT_TIBETAN },
    { 0x0F71, 0x0F97, SUAS_SCRIPT_TIBETAN },
    { 0x0F99, 0x0FBC, SUAS_SCRIPT_TIBETAN },
    { 0x0FBE, 0x0FCC, SUAS_SCRIPT_TIBETAN },
    { 0x0FCE, 0x0FD4, SUAS_SCRIPT_TIBETAN },
    { 0x0FD9, 0x0FDA, SUAS_SCRIPT_TIBETAN },
    { 0x10A0, 0x10C5, SUAS_SCRIPT_GEORGIAN },
    { 0x10C7, 0x10C7, SUAS_SCRIPT_GEORGIAN },
    { 0x10CD, 0x10CD, SUAS_SCRIPT_GEORGIAN },
    { 0x10D0, 0x10FA, SUAS_SCRIPT_GEORGIAN },
    { 0x10FC, 0x10FF, SUAS_SCRIPT_GEORGIAN },
    { 0x1100, 0x11FF, SUAS_SCRIPT_HANGUL },
    { 0x1C80, 0x1C88, SUAS_SCRIPT_CYRILLIC },
    { 0x1C90, 0x1CBF, SUAS_SCRIPT_GEORGIAN },
    { 0x1E00, 0x1EFF, SUAS_SCRIPT_LATIN },
    { 0x1F00, 0x1FFF, SUAS_SCRIPT_GREEK },
    { 0x2800, 0x28FF, SUAS_SCRIPT_BRAILLE },
    { 0x2C60, 0x2C7F, SUAS_SCRIPT_LATIN },
    { 0x2C80, 0x2CFF, SUAS_SCRIPT_COPTIC },
    { 0x2D00, 0x2D2D, SUAS_SCRIPT_GEORGIAN },
    { 0x2DE0, 0x2DFF, SUAS_SCRIPT_CYRILLIC },
    { 0x2E80, 0x2E99, SUAS_SCRIPT_HAN },
    { 0x2E9B, 0x2EF3, SUAS_SCRIPT_HAN },
    { 0x2F00, 0x2FD5, SUAS_SCRIPT_HAN },
    { 0x3005, 0x3005, SUAS_SCRIPT_HAN },
    { 0x3007, 0x3007, SUAS_SCRIPT_HAN },
    { 0x3021, 0x3029, SUAS_SCRIPT_HAN },
    { 0x302E, 0x302F, SUAS_SCRIPT_HANGUL },
    { 0x3038, 0x303B, SUAS_SCRIPT_HAN },
    { 0x3041, 0x3096, SUAS_SCRIPT_HIRAGANA },
    { 0x309D, 0x309F, SUAS_SCRIPT_HIRAGANA },
    { 0x30A1, 0x30FA, SUAS_SCRIPT_KATAKANA },
    { 0x30FD, 0x30FF, SUAS_SCRIPT_KATAKANA },
    { 0x3131, 0x318E, SUAS_SCRIPT_HANGUL },
    { 0x31F0, 0x31FF, SUAS_SCRIPT_KATAKANA },
    { 0x3200, 0x321E, SUAS_SCRIPT_HANGUL },
    { 0x3260, 0x327E, SUAS_SCRIPT_HANGUL },
    { 0x32D0, 0x32FE, SUAS_SCRIPT_KATAKANA },
    { 0x3300, 0x3357, SUAS_SCRIPT_KATAKANA },
    { 0x3400, 0x4DBF, SUAS_SCRIPT_HAN },
    { 0x4E00, 0x9FFF, SUAS_SCRIPT_HAN },
    { 0xA640, 0xA69F, SUAS_SCRIPT_CYRILLIC },
    { 0xA720, 0xA7FF, SUAS_SCRIPT_LATIN },
    { 0xA8E0, 0xA8FF, SUAS_SCRIPT_DEVANAGARI },
    { 0xA960, 0xA97C, SUAS_SCRIPT_HANGUL },
    { 0xAC00, 0xD7A3, SUAS_SCRIPT_HANGUL },
    { 0xF900, 0xFA6D, SUAS_SCRIPT_HAN },
    { 0xFA70, 0xFAD9, SUAS_SCRIPT_HAN },
    { 0xFB00, 0xFB06, SUAS_SCRIPT_LATIN },
    { 0xFB13, 0xFB17, SUAS_SCRIPT_ARMENIAN },
    { 0xFB1D, 0xFB4F, SUAS_SCRIPT_HEBREW },
    { 0xFB50, 0xFDCF, SUAS_SCRIPT_ARABIC },
    { 0xFDF0, 0xFDFF, SUAS_SCRIPT_ARABIC },
    { 0xFE70, 0xFEFF, SUAS_SCRIPT_ARABIC },
    { 0xFF21, 0xFF3A, SUAS_SCRIPT_LATIN },
    { 0xFF41, 0xFF5A, SUAS_SCRIPT_LATIN },
    { 0xFF66, 0xFF9D, SUAS_SCRIPT_KATAKANA },
    { 0xFFA0, 0xFFDC, SUAS_SCRIPT_HANGUL },
    { 0x20000, 0x2A6DF, SUAS_SCRIPT_HAN },
    { 0x2A700, 0x2B739, SUAS_SCRIPT_HAN },
    { 0x2B740, 0x2B81D, SUAS_SCRIPT_HAN },
    { 0x2B820, 0x2CEA1, SUAS_SCRIPT_HAN },
    { 0x2CEB0, 0x2EBE0, SUAS_SCRIPT_HAN },
    { 0x2F800, 0x2FA1D, SUAS_SCRIPT_HAN },
    { 0x30000, 0x3134A, SUAS_SCRIPT_HAN },
};

/* ── Common ranges (sorted ascending by lo) ─────────────────────────
 * Shared punctuation, symbols, digits, and the merged Inherited marks.
 * Overlaps with the explicit table are resolved in favor of the explicit
 * table (checked first). Curated subset of sucd/Scripts.txt. */
static const ssp_range_t SSP_COMMON_TABLE[] = {
    { 0x0000, 0x0040, SUAS_SCRIPT_COMMON },
    { 0x005B, 0x0060, SUAS_SCRIPT_COMMON },
    { 0x007B, 0x00A9, SUAS_SCRIPT_COMMON },
    { 0x00AB, 0x00B9, SUAS_SCRIPT_COMMON },
    { 0x00BB, 0x00BF, SUAS_SCRIPT_COMMON },
    { 0x00D7, 0x00D7, SUAS_SCRIPT_COMMON },
    { 0x00F7, 0x00F7, SUAS_SCRIPT_COMMON },
    { 0x02B9, 0x02FF, SUAS_SCRIPT_COMMON },
    { 0x0300, 0x036F, SUAS_SCRIPT_COMMON }, /* combining diacritical marks */
    { 0x060C, 0x060C, SUAS_SCRIPT_COMMON }, /* ARABIC COMMA        */
    { 0x0640, 0x0640, SUAS_SCRIPT_COMMON }, /* ARABIC TATWEEL      */
    { 0x064B, 0x0655, SUAS_SCRIPT_COMMON }, /* Arabic combining    */
    { 0x2000, 0x2BFF, SUAS_SCRIPT_COMMON },
    { 0x2E00, 0x2E7F, SUAS_SCRIPT_COMMON },
    { 0x3000, 0x3004, SUAS_SCRIPT_COMMON },
    { 0x3006, 0x3006, SUAS_SCRIPT_COMMON },
    { 0x3008, 0x3020, SUAS_SCRIPT_COMMON },
    { 0x3030, 0x3037, SUAS_SCRIPT_COMMON },
    { 0x303C, 0x303F, SUAS_SCRIPT_COMMON },
    { 0x3099, 0x309C, SUAS_SCRIPT_COMMON }, /* Kana voiced marks + dots */
    { 0x30FB, 0x30FC, SUAS_SCRIPT_COMMON }, /* Kana middle dot + choon  */
    { 0xFE00, 0xFE0F, SUAS_SCRIPT_COMMON }, /* variation selectors      */
    { 0xFE10, 0xFE19, SUAS_SCRIPT_COMMON },
    { 0xFE30, 0xFE6F, SUAS_SCRIPT_COMMON },
    { 0xFF00, 0xFF0F, SUAS_SCRIPT_COMMON },
    { 0xFF1A, 0xFF20, SUAS_SCRIPT_COMMON },
    { 0xFF3B, 0xFF40, SUAS_SCRIPT_COMMON },
    { 0xFF5B, 0xFF65, SUAS_SCRIPT_COMMON },
    { 0xFFE0, 0xFFE6, SUAS_SCRIPT_COMMON },
    { 0x1F000, 0x1FAFF, SUAS_SCRIPT_COMMON }, /* emoji / pictographs    */
};

/* ── Script_Extensions (scx) special cases ──────────────────────────
 * Sorted ascending by cp; non-overlapping. Each entry is a multi-script set.
 * Codepoints absent here default to the singleton { Script(cp) }. Curated
 * subset of sucd/ScriptExtensions.txt. */
typedef struct {
    uint32_t      cp;
    suas_script_t scripts[SUAS_SSP_SCX_MAX];
    uint8_t       n;
} ssp_scx_entry_t;

static const ssp_scx_entry_t SSP_SCX_TABLE[] = {
    { 0x060C, { SUAS_SCRIPT_ARABIC, SUAS_SCRIPT_SYRIAC }, 2 },
    { 0x0640, { SUAS_SCRIPT_ARABIC, SUAS_SCRIPT_SYRIAC }, 2 },
    { 0x064B, { SUAS_SCRIPT_ARABIC, SUAS_SCRIPT_SYRIAC }, 2 },
    { 0x064C, { SUAS_SCRIPT_ARABIC, SUAS_SCRIPT_SYRIAC }, 2 },
    { 0x064D, { SUAS_SCRIPT_ARABIC, SUAS_SCRIPT_SYRIAC }, 2 },
    { 0x064E, { SUAS_SCRIPT_ARABIC, SUAS_SCRIPT_SYRIAC }, 2 },
    { 0x064F, { SUAS_SCRIPT_ARABIC, SUAS_SCRIPT_SYRIAC }, 2 },
    { 0x0650, { SUAS_SCRIPT_ARABIC, SUAS_SCRIPT_SYRIAC }, 2 },
    { 0x0651, { SUAS_SCRIPT_ARABIC, SUAS_SCRIPT_SYRIAC }, 2 },
    { 0x0652, { SUAS_SCRIPT_ARABIC, SUAS_SCRIPT_SYRIAC }, 2 },
    { 0x0653, { SUAS_SCRIPT_ARABIC, SUAS_SCRIPT_SYRIAC }, 2 },
    { 0x0654, { SUAS_SCRIPT_ARABIC, SUAS_SCRIPT_SYRIAC }, 2 },
    { 0x0655, { SUAS_SCRIPT_ARABIC, SUAS_SCRIPT_SYRIAC }, 2 },
    { 0x3001, { SUAS_SCRIPT_HAN, SUAS_SCRIPT_HIRAGANA, SUAS_SCRIPT_KATAKANA }, 3 },
    { 0x3002, { SUAS_SCRIPT_HAN, SUAS_SCRIPT_HIRAGANA, SUAS_SCRIPT_KATAKANA }, 3 },
    { 0x3099, { SUAS_SCRIPT_HIRAGANA, SUAS_SCRIPT_KATAKANA }, 2 },
    { 0x309A, { SUAS_SCRIPT_HIRAGANA, SUAS_SCRIPT_KATAKANA }, 2 },
    { 0x30FC, { SUAS_SCRIPT_HIRAGANA, SUAS_SCRIPT_KATAKANA }, 2 },
};

/* ── RNUR registry carve-out (0x160000-0x16FFFF) ────────────────────
 * The registry is a coordinate system (Set_Number, Code_Point); the high
 * byte of the carve-out offset is the set number. No registry script is
 * mounted yet, so the range currently resolves to Unknown: the RNUR plugin
 * supplies the set -> script mapping when it is mounted (§5.4). */

/* ── Range lookup ─────────────────────────────────────────────────── */
/* Binary search a sorted, non-overlapping range table for cp. Returns the
 * enclosed script, or UNKNOWN when no range matches. */
static suas_script_t ssp_range_lookup(const ssp_range_t* tab, size_t n,
                                      uint32_t cp)
{
    size_t lo = 0, hi = n;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (cp < tab[mid].lo) hi = mid;
        else if (cp > tab[mid].hi) lo = mid + 1;
        else return tab[mid].script;
    }
    return SUAS_SCRIPT_UNKNOWN;
}

static const ssp_scx_entry_t* ssp_scx_find(uint32_t cp)
{
    size_t lo = 0, hi = SSP_NELEM(SSP_SCX_TABLE);
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (cp < SSP_SCX_TABLE[mid].cp) hi = mid;
        else if (cp > SSP_SCX_TABLE[mid].cp) lo = mid + 1;
        else return &SSP_SCX_TABLE[mid];
    }
    return NULL;
}

/* ── The Script property ──────────────────────────────────────────── */
suas_script_t suas_ssp_get_script(sucs_ex_char_t cp)
{
    if (cp <= SUAS_SSP_BRIDGE_MAX) {
        uint32_t c = (uint32_t)cp;
        suas_script_t s = ssp_range_lookup(SSP_SCRIPT_TABLE,
                                           SSP_NELEM(SSP_SCRIPT_TABLE), c);
        if (s != SUAS_SCRIPT_UNKNOWN)
            return s;
        /* Bridge default for an unlisted codepoint: Common when it is a
         * curated shared/symbol/mark range, otherwise Unknown. */
        return ssp_range_lookup(SSP_COMMON_TABLE,
                                SSP_NELEM(SSP_COMMON_TABLE), c);
    }
    if (cp <= SUAS_SSP_SCP_MAX) {
        /* SCP: a script-context directive selects its script; any other
         * control-plane codepoint is not text and carries no script. */
        suas_script_t d;
        if (suas_ssp_is_script_directive(cp, &d))
            return d;
        return SUAS_SCRIPT_UNKNOWN;
    }
    /* Native plane (including the RNUR registry carve-out), terminal 16, and
     * ExtSUCS carry no script until a mounted plugin/registry registers one. */
    return SUAS_SCRIPT_UNKNOWN;
}

bool suas_ssp_matches_script(sucs_ex_char_t cp, suas_script_t target_script)
{
    suas_ssp_scx_t scx;
    if (suas_ssp_get_script(cp) == target_script)
        return true;
    scx = suas_ssp_get_script_extensions(cp);
    return suas_ssp_scx_contains(&scx, target_script);
}

bool suas_ssp_is_implicit(suas_script_t script)
{
    return script == SUAS_SCRIPT_UNKNOWN || script == SUAS_SCRIPT_COMMON;
}

/* ── The Script_Extensions property ───────────────────────────────── */
suas_ssp_scx_t suas_ssp_get_script_extensions(sucs_ex_char_t cp)
{
    suas_ssp_scx_t out;
    out.n = 0;

    if (cp <= SUAS_SSP_BRIDGE_MAX) {
        const ssp_scx_entry_t* e = ssp_scx_find((uint32_t)cp);
        if (e) {
            size_t i;
            for (i = 0; i < e->n && out.n < SUAS_SSP_SCX_MAX; ++i)
                out.scripts[out.n++] = e->scripts[i];
            return out;
        }
    }

    /* Default: the singleton { Script(cp) }. */
    out.scripts[0] = suas_ssp_get_script(cp);
    out.n = 1;
    return out;
}

bool suas_ssp_scx_contains(const suas_ssp_scx_t* set, suas_script_t script)
{
    size_t i;
    if (!set) return false;
    for (i = 0; i < set->n; ++i)
        if (set->scripts[i] == script)
            return true;
    return false;
}

/* ── Context resolution ───────────────────────────────────────────── */
suas_script_t suas_ssp_resolve(sucs_ex_char_t cp, suas_script_t context_script)
{
    suas_script_t s = suas_ssp_get_script(cp);
    if (suas_ssp_is_implicit(s) && !suas_ssp_is_implicit(context_script))
        return context_script;   /* inherit from the preceding base */
    return s;
}

/* ── SCP directives ───────────────────────────────────────────────── */
bool suas_ssp_is_script_directive(sucs_ex_char_t cp, suas_script_t* out)
{
    if (cp >= SUAS_SSP_SCP_SCRIPT_BASE && cp <= SUAS_SSP_SCP_SCRIPT_LAST) {
        if (out) *out = (suas_script_t)((cp - SUAS_SSP_SCP_SCRIPT_BASE) & 0x7FFFULL);
        return true;
    }
    return false;
}

/* ── Names and aliases ────────────────────────────────────────────── */
typedef struct {
    suas_script_t script;
    const char*   long_name;
    const char*   short_name;
} ssp_name_t;

static const ssp_name_t SSP_NAMES[] = {
    { SUAS_SCRIPT_UNKNOWN,    "Unknown",    "Zzzz" },
    { SUAS_SCRIPT_COMMON,     "Common",     "Zyyy" },
    { SUAS_SCRIPT_LATIN,      "Latin",      "Latn" },
    { SUAS_SCRIPT_ARABIC,     "Arabic",     "Arab" },
    { SUAS_SCRIPT_CYRILLIC,   "Cyrillic",   "Cyrl" },
    { SUAS_SCRIPT_GREEK,      "Greek",      "Grek" },
    { SUAS_SCRIPT_HAN,        "Han",        "Hani" },
    { SUAS_SCRIPT_HEBREW,     "Hebrew",     "Hebr" },
    { SUAS_SCRIPT_DEVANAGARI, "Devanagari", "Deva" },
    { SUAS_SCRIPT_HIRAGANA,   "Hiragana",   "Hira" },
    { SUAS_SCRIPT_KATAKANA,   "Katakana",   "Kana" },
    { SUAS_SCRIPT_HANGUL,     "Hangul",     "Hang" },
    { SUAS_SCRIPT_THAI,       "Thai",       "Thai" },
    { SUAS_SCRIPT_LAO,        "Lao",        "Laoo" },
    { SUAS_SCRIPT_TIBETAN,    "Tibetan",    "Tibt" },
    { SUAS_SCRIPT_ARMENIAN,   "Armenian",   "Armn" },
    { SUAS_SCRIPT_GEORGIAN,   "Georgian",   "Geor" },
    { SUAS_SCRIPT_BRAILLE,    "Braille",    "Brai" },
    { SUAS_SCRIPT_SYRIAC,     "Syriac",     "Syrc" },
    { SUAS_SCRIPT_COPTIC,     "Coptic",     "Copt" },
};

static const ssp_name_t* ssp_name_find(suas_script_t script)
{
    size_t i;
    for (i = 0; i < SSP_NELEM(SSP_NAMES); ++i)
        if (SSP_NAMES[i].script == script)
            return &SSP_NAMES[i];
    return NULL;
}

const char* suas_ssp_script_long_name(suas_script_t script)
{
    const ssp_name_t* n = ssp_name_find(script);
    return n ? n->long_name : "Unknown";
}

const char* suas_ssp_script_short_name(suas_script_t script)
{
    const ssp_name_t* n = ssp_name_find(script);
    return n ? n->short_name : "Zzzz";
}

/* Loose, alias-style comparison: ASCII case-insensitive, and SPACE,
 * HYPHEN-MINUS and UNDERSCORE are ignored (UAX #44 §5.9 loose matching). */
static int ssp_tag_eq(const char* a, const char* b)
{
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
        if (ca == ' ' || ca == '-' || ca == '_') { ++a; continue; }
        if (cb == ' ' || cb == '-' || cb == '_') { ++b; continue; }
        if (ca != cb) return 0;
        ++a; ++b;
    }
    while (*a == ' ' || *a == '-' || *a == '_') ++a;
    while (*b == ' ' || *b == '-' || *b == '_') ++b;
    return *a == '\0' && *b == '\0';
}

bool suas_ssp_script_from_tag(const char* tag, suas_script_t* out)
{
    size_t i;
    if (!tag) return false;
    for (i = 0; i < SSP_NELEM(SSP_NAMES); ++i) {
        if (ssp_tag_eq(tag, SSP_NAMES[i].long_name) ||
            ssp_tag_eq(tag, SSP_NAMES[i].short_name)) {
            if (out) *out = SSP_NAMES[i].script;
            return true;
        }
    }
    return false;
}

/* ── Font-engine token packing ────────────────────────────────────── */
uint64_t suas_ssp_tag_codepoint(sucs_ex_char_t cp, suas_script_t script)
{
    return (((uint64_t)cp & 0x7FFFFFFFULL) << 16) |
           ((uint64_t)script & SUAS_SSP_TOKEN_SCRIPT_MASK);
}

suas_script_t suas_ssp_token_script(uint64_t token)
{
    return (suas_script_t)(token & SUAS_SSP_TOKEN_SCRIPT_MASK);
}

sucs_ex_char_t suas_ssp_token_codepoint(uint64_t token)
{
    return (token & SUAS_SSP_TOKEN_CP_MASK) >> 16;
}

/* ── Version ──────────────────────────────────────────────────────── */
const char* suas_ssp_version_string(void)
{
    return "1.0.0";
}
