/* SUTR-014 - OpenWindows Kernel Text Integration Profile (OKTP)
 *
 * The informative profile as data: a static adoption matrix, three cumulative
 * levels, the observed gaps, and the gates. Every cited identifier is checked
 * against the SUAS-010 registry, so a stale profile fails a test instead of
 * silently drifting.
 *
 * Nothing here allocates, and nothing here creates a requirement.
 *
 * See docs/sutr/SUTR-014-oktp.md for the informative report.
 */

#include "sutr/sutr_oktp.h"

#include "suas/suas_ssre.h"

/* -- Small helpers ----------------------------------------------------- */

static bool oktp_streq(const char *a, const char *b)
{
    size_t i = 0u;

    if (a == NULL || b == NULL) {
        return false;
    }
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return false;
        }
        i++;
    }
    return a[i] == b[i];
}

static bool oktp_prefix(const char *s, const char *prefix)
{
    size_t i = 0u;

    if (s == NULL || prefix == NULL) {
        return false;
    }
    while (prefix[i] != '\0') {
        if (s[i] != prefix[i]) {
            return false;
        }
        i++;
    }
    return true;
}

static size_t oktp_strlen_bounded(const char *s, size_t cap)
{
    size_t n = 0u;

    if (s == NULL) {
        return 0u;
    }
    while (n < cap && s[n] != '\0') {
        n++;
    }
    return n;
}

/* -- Matrix (section 4) ------------------------------------------------ */

static const oktp_matrix_entry_t g_matrix[] = {
    /* 4.1 Kernel core */
    { "lib/sucs.c mode switching",
      { "SUTR-0", "SUTR-1", "SUTR-10" }, 3u, OKTP_TIER_LINKED,
      "already the only integration point; the SUTR-10 zero-allocation gate applies" },
    { "lib/kstring.c",
      { "SUAS-018" }, 1u, OKTP_TIER_T0,
      "NUL dependence and no capacity is the root defect behind most others" },
    { "lib/kprintf.c",
      { "SUAS-018", "SUAS-002" }, 2u, OKTP_TIER_T0,
      "%s is an unbounded read; width is needed for column arithmetic" },
    { "syscall/dispatcher.c",
      { "SUAS-018" }, 1u, OKTP_TIER_T0,
      "admission exists for one call site; the standard generalizes it" },
    { "vfs/vfs.c names",
      { "SUAS-018", "SUAS-007", "SUAS-004", "SUTS-005" }, 4u, OKTP_TIER_T0,
      "names are resolved and re-resolved; publication immutability closes the race" },
    { "core/alpc.c port names",
      { "SUAS-018", "SUAS-007" }, 2u, OKTP_TIER_T0,
      "same defect as VFS, same fix" },
    { "ps/owx_loader.c image names",
      { "SUAS-018", "SUTS-005" }, 2u, OKTP_TIER_T1,
      "bounded names matter at the trust boundary; no immutability need yet" },
    { "shell/shell.c lexing",
      { "SUAS-018", "SUAS-007", "SUAS-006", "SUAS-003" }, 4u, OKTP_TIER_T1,
      "line editing and token boundaries need STS/SBR once the shell accepts non-ASCII" },
    { "Diagnostics and bancode/",
      { "SUTR-0", "SUAS-018" }, 2u, OKTP_TIER_T0,
      "BANcodes are SCP values and must not enter text objects" },

    /* 4.2 Console and presentation */
    { "DLL/suct plane",
      { "SUTS-012", "SUAS-002" }, 2u, OKTP_TIER_T0,
      "private width table is a direct conflict with the mandatory standard" },
    { "DLL/suct wrapping",
      { "SUTS-012", "SUAS-003" }, 2u, OKTP_TIER_T0,
      "wrap points are break decisions" },
    { "DLL/suct clustering",
      { "SUTS-012", "SUAS-006" }, 2u, OKTP_TIER_T0,
      "combining marks are currently dropped" },
    { "DLL/suct PUA zones",
      { "SUTS-012", "SUTR-8" }, 2u, OKTP_TIER_T1,
      "registry needed before any second consumer of the zones appears" },
    { "Drivers/ttydrv",
      { "SUTS-012" }, 1u, OKTP_TIER_T1,
      "legal, but must be declared lossy and never presented as conformant" },
    { "DLL/owfont64",
      { "SUTR-8", "SUAS-014" }, 2u, OKTP_TIER_T2,
      "rasterizer does not need properties until vertical or complex scripts arrive" },
    { "DLL/bootvid",
      { "SUTS-012" }, 1u, OKTP_TIER_T1,
      "boot glyphs are a PUA consumer; needs zone registration" },
    { "Software/owterm",
      { "SUTS-012", "SUAS-002", "SUAS-003", "SUAS-006" }, 4u, OKTP_TIER_T1,
      "the user-facing console should claim SCTS before it claims anything else" },
    { "Software/owterm input",
      { "SUAS-019", "SUAS-018" }, 2u, OKTP_TIER_T1,
      "key events reach a text sink with no declared event-to-text model" },
    { "Software/owterm emulation",
      { "SUTS-013", "SUTS-012" }, 2u, OKTP_TIER_T1,
      "control sequences are interpreted inline instead of through SCTE" },

    /* 4.3 Text algorithms */
    { "Shell identifier checking",
      { "SUAS-007" }, 1u, OKTP_TIER_T1,
      "only when the shell accepts non-ASCII identifiers" },
    { "Confusable and homograph checks",
      { "SUTS-005" }, 1u, OKTP_TIER_T1,
      "needed once names can contain non-ASCII" },
    { "Canonical equivalence in names",
      { "SUAS-004" }, 1u, OKTP_TIER_T1,
      "needed once names can be canonically equivalent" },
    { "Line breaking",
      { "SUAS-003" }, 1u, OKTP_TIER_T0,
      "T0 for any wrapping surface; T2 otherwise" },
    { "Segmentation",
      { "SUAS-006" }, 1u, OKTP_TIER_T1,
      "line editor, word motion, selection" },
    { "Collation",
      { "SUTS-001" }, 1u, OKTP_TIER_T2,
      "no sorted user-visible list yet" },
    { "Regex",
      { "SUTS-002" }, 1u, OKTP_TIER_T2,
      "no in-kernel regex consumer" },
    { "Locale formatting",
      { "SUTS-003" }, 1u, OKTP_TIER_T2,
      "owlocale64 exists but does not format through a schema" },
    { "Variation sequences",
      { "SUTS-004" }, 1u, OKTP_TIER_T2,
      "needs a font that carries them" },
    { "Emoji sequences",
      { "SUTS-007" }, 1u, OKTP_TIER_T2,
      "no colour glyph path" },
    { "Source-code handling",
      { "SUTS-008" }, 1u, OKTP_TIER_NA,
      "no kernel source pipeline" },
    { "Link detection",
      { "SUTS-009" }, 1u, OKTP_TIER_NA,
      "no user-facing link surface" },
    { "Set notation",
      { "SUTS-011" }, 1u, OKTP_TIER_T1,
      "useful for compiled allow/deny sets in policy checks" },
    { "IDN",
      { "SUTS-006" }, 1u, OKTP_TIER_T2,
      "network exists; no internationalized labels yet" },
    { "BiDi",
      { "SUAS-001" }, 1u, OKTP_TIER_T2,
      "no right-to-left UI" },
    { "Vertical text",
      { "SUAS-014" }, 1u, OKTP_TIER_T2,
      "no vertical surface" },
    { "Arabic marks",
      { "SUAS-015" }, 1u, OKTP_TIER_T2,
      "no shaping pipeline" },
    { "Mongolian",
      { "SUTR-11" }, 1u, OKTP_TIER_T2,
      "no shaping pipeline" },
    { "Cuneiform, hieroglyphs, East Asian script data",
      { "SUAS-016", "SUAS-017", "SUTR-012" }, 3u, OKTP_TIER_T2,
      "data-only; adopt with the fonts that need them" },
    { "East Asian spacing",
      { "SUTR-013" }, 1u, OKTP_TIER_T2,
      "SGW already answers the grid question" },
    { "Mathematical layout",
      { "SUTR-9" }, 1u, OKTP_TIER_T2,
      "no math surface" }
};

#define OKTP_MATRIX_COUNT (sizeof g_matrix / sizeof g_matrix[0])

/* -- Levels (section 5) ------------------------------------------------ */

static const oktp_level_t g_levels[OKTP_LEVEL_COUNT] = {
    { "KT-0", "Encoding only",
      { "SUTR-0", "SUTR-1" }, 2u, true,
      "OpenWindows links SUTF-8 and SUCS mode" },
    { "KT-1", "Bounded text",
      { "SUTR-0", "SUTR-1", "SUAS-018" }, 3u, false,
      "syscall_copy_name implements part of the admission pattern; no text object exists" },
    { "KT-2", "Conformant console",
      { "SUTR-0", "SUTR-1", "SUAS-018", "SUTS-012", "SUAS-002",
        "SUAS-003", "SUAS-006" }, 7u, false,
      "suct is close in shape and conflicts in authority" }
};

/* -- Gaps (section 6) -------------------------------------------------- */

static const oktp_gap_t g_gaps[] = {
    { "G-1", "Kernel text has no length independent of a terminator",
      "SUAS-018", "lib/kstring.c ow_strlen" },
    { "G-2", "No ownership or lifetime classes",
      "SUAS-018", "inc/ow_string.h is all char *" },
    { "G-3", "Name publication is mutable",
      "SUAS-018", "vfs/vfs.c writes e->Name in place" },
    { "G-4", "Resolve-then-use races on names",
      "SUAS-018", "vfs/vfs.c compares, then acts" },
    { "G-5", "Console owns a second width table",
      "SUTS-012", "suct.c zero_width_ranges, wide_ranges" },
    { "G-6", "Console drops combining marks",
      "SUTS-012", "plane_emit_cp returns OK for w == 0" },
    { "G-7", "PUA zones are unregistered and overlap markers",
      "SUTS-012", "suct.h 0xEE00/0xEE01 inside the OWM block" },
    { "G-8", "Legacy ASCII sink is indistinguishable from a conformant one",
      "SUTS-012", "ttydrv.h uint8_t ch" },
    { "G-9", "No SCP filtering in text",
      "SUAS-018", "no kernel path rejects 0x110000-0x11FFFF" },
    { "G-10", "Key events reach a text sink without a declared model",
      "SUAS-019", "owterm input path has no composition or repeat contract" },
    { "G-11", "Console control sequences are interpreted without a terminal model",
      "SUTS-013", "owterm parses escapes inline; no SCTS/SCTE separation" }
};

#define OKTP_GAP_COUNT (sizeof g_gaps / sizeof g_gaps[0])

/* -- Gates (section 7) ------------------------------------------------- */

static const oktp_gate_t g_gates[] = {
    { "D-1", "cmake --build build --target verify-docs",
      "the four family indexes exist" },
    { "D-2", "ctest --test-dir build",
      "the unit suites pass, including registry self-consistency and header coexistence" },
    { "D-3", "ctest --test-dir build -R test_unified",
      "every public header compiles in one translation unit" },
    { "D-4", "ctest --test-dir build -R test_suas_ssre",
      "the curated registry agrees with its own tests" },
    { "K-1", "OpenWindows hosttest",
      "the kernel boots and its integration assertions pass" },
    { "K-2", "OpenWindows make qemu",
      "the boot path and SUCS commit assertions pass" }
};

#define OKTP_GATE_COUNT (sizeof g_gates / sizeof g_gates[0])

/* -- Introspection ----------------------------------------------------- */

const char *sutr_oktp_version_string(void)
{
    return "SUTR-014 OKTP 1.0.0";
}

const char *sutr_oktp_status_name(oktp_status_t st)
{
    switch (st) {
    case OKTP_OK:         return "OKTP_OK";
    case OKTP_E_NULL:     return "OKTP_E_NULL";
    case OKTP_E_RANGE:    return "OKTP_E_RANGE";
    case OKTP_E_CITATION: return "OKTP_E_CITATION";
    case OKTP_E_KIND:     return "OKTP_E_KIND";
    case OKTP_E_PROFILE:  return "OKTP_E_PROFILE";
    default:              return "OKTP_E_UNKNOWN";
    }
}

const char *sutr_oktp_adoption_name(oktp_adoption_t a)
{
    switch (a) {
    case OKTP_ADOPT_LINKED:    return "Linked";
    case OKTP_ADOPT_PRESENT:   return "Present";
    case OKTP_ADOPT_SPECIFIED: return "Specified";
    case OKTP_ADOPT_ABSENT:    return "Absent";
    default:                   return "Unknown";
    }
}

const char *sutr_oktp_adoption_meaning(oktp_adoption_t a)
{
    switch (a) {
    case OKTP_ADOPT_LINKED:
        return "the build compiles and links the translation unit";
    case OKTP_ADOPT_PRESENT:
        return "the translation unit or header exists but is not on the build path";
    case OKTP_ADOPT_SPECIFIED:
        return "the governing document exists but no implementation exists";
    case OKTP_ADOPT_ABSENT:
        return "neither document nor implementation exists";
    default:
        return "unknown adoption status";
    }
}

const char *sutr_oktp_tier_name(oktp_tier_t t)
{
    switch (t) {
    case OKTP_TIER_LINKED: return "Linked";
    case OKTP_TIER_T0:     return "T0";
    case OKTP_TIER_T1:     return "T1";
    case OKTP_TIER_T2:     return "T2";
    case OKTP_TIER_NA:     return "N/A";
    default:               return "Unknown";
    }
}

const char *sutr_oktp_tier_meaning(oktp_tier_t t)
{
    switch (t) {
    case OKTP_TIER_LINKED:
        return "already on the build path";
    case OKTP_TIER_T0:
        return "adopt now: observable code already violates it, or safety requires it";
    case OKTP_TIER_T1:
        return "adopt with the feature that makes it binding";
    case OKTP_TIER_T2:
        return "deferred: useful, not blocking, and no consumer exists";
    case OKTP_TIER_NA:
        return "out of scope for this kernel";
    default:
        return "unknown adoption tier";
    }
}

const char *sutr_oktp_level_name(oktp_level_id_t l)
{
    switch (l) {
    case OKTP_LEVEL_KT0: return "KT-0";
    case OKTP_LEVEL_KT1: return "KT-1";
    case OKTP_LEVEL_KT2: return "KT-2";
    default:             return "Unknown";
    }
}

/* -- Matrix ------------------------------------------------------------ */

size_t sutr_oktp_matrix_count(void)
{
    return (size_t)OKTP_MATRIX_COUNT;
}

const oktp_matrix_entry_t *sutr_oktp_matrix_at(size_t index)
{
    if (index >= (size_t)OKTP_MATRIX_COUNT) {
        return NULL;
    }
    return &g_matrix[index];
}

const oktp_matrix_entry_t *sutr_oktp_matrix_find(const char *subsystem)
{
    size_t i;

    if (subsystem == NULL) {
        return NULL;
    }
    for (i = 0u; i < (size_t)OKTP_MATRIX_COUNT; i++) {
        if (oktp_streq(g_matrix[i].subsystem, subsystem)) {
            return &g_matrix[i];
        }
    }
    return NULL;
}

bool sutr_oktp_entry_cites(const oktp_matrix_entry_t *entry,
                           const char *doc_id)
{
    uint8_t i;

    if (entry == NULL || doc_id == NULL) {
        return false;
    }
    for (i = 0u; i < entry->doc_count; i++) {
        if (oktp_streq(entry->docs[i], doc_id)) {
            return true;
        }
    }
    return false;
}

size_t sutr_oktp_matrix_doc_count(const char *doc_id)
{
    size_t i;
    size_t n = 0u;

    if (doc_id == NULL) {
        return 0u;
    }
    for (i = 0u; i < (size_t)OKTP_MATRIX_COUNT; i++) {
        if (sutr_oktp_entry_cites(&g_matrix[i], doc_id)) {
            n++;
        }
    }
    return n;
}

/* -- Levels, gaps, gates ---------------------------------------------- */

const oktp_level_t *sutr_oktp_level(oktp_level_id_t level)
{
    if ((unsigned)level >= OKTP_LEVEL_COUNT) {
        return NULL;
    }
    return &g_levels[(unsigned)level];
}

const oktp_level_t *sutr_oktp_level_find(const char *id)
{
    unsigned i;

    if (id == NULL) {
        return NULL;
    }
    for (i = 0u; i < OKTP_LEVEL_COUNT; i++) {
        if (oktp_streq(g_levels[i].id, id)) {
            return &g_levels[i];
        }
    }
    return NULL;
}

bool sutr_oktp_level_met(oktp_level_id_t level)
{
    if ((unsigned)level >= OKTP_LEVEL_COUNT) {
        return false;
    }
    return g_levels[(unsigned)level].met;
}

size_t sutr_oktp_gap_count(void)
{
    return (size_t)OKTP_GAP_COUNT;
}

const oktp_gap_t *sutr_oktp_gap_at(size_t index)
{
    if (index >= (size_t)OKTP_GAP_COUNT) {
        return NULL;
    }
    return &g_gaps[index];
}

const oktp_gap_t *sutr_oktp_gap_find(const char *id)
{
    size_t i;

    if (id == NULL) {
        return NULL;
    }
    for (i = 0u; i < (size_t)OKTP_GAP_COUNT; i++) {
        if (oktp_streq(g_gaps[i].id, id)) {
            return &g_gaps[i];
        }
    }
    return NULL;
}

size_t sutr_oktp_gate_count(void)
{
    return (size_t)OKTP_GATE_COUNT;
}

const oktp_gate_t *sutr_oktp_gate_at(size_t index)
{
    if (index >= (size_t)OKTP_GATE_COUNT) {
        return NULL;
    }
    return &g_gates[index];
}

const oktp_gate_t *sutr_oktp_gate_find(const char *id)
{
    size_t i;

    if (id == NULL) {
        return NULL;
    }
    for (i = 0u; i < (size_t)OKTP_GATE_COUNT; i++) {
        if (oktp_streq(g_gates[i].id, id)) {
            return &g_gates[i];
        }
    }
    return NULL;
}

/* -- Verification ------------------------------------------------------ */

oktp_status_t sutr_oktp_check_citation(const char *doc_id)
{
    const ssre_spec_record_t *rec;
    uint8_t expected;

    if (doc_id == NULL) {
        return OKTP_E_NULL;
    }

    if (oktp_prefix(doc_id, "SUAS-")) {
        expected = SUAS_SSRE_KIND_MANDATORY;
    } else if (oktp_prefix(doc_id, "SUTS-")) {
        expected = SUAS_SSRE_KIND_OPTIONAL;
    } else if (oktp_prefix(doc_id, "SUTR-")) {
        expected = SUAS_SSRE_KIND_REFERENCE;
    } else {
        return OKTP_E_CITATION;
    }

    rec = suas_ssre_lookup(doc_id);
    if (rec == NULL) {
        return OKTP_E_CITATION;
    }
    if (rec->kind != expected) {
        return OKTP_E_KIND;
    }
    return OKTP_OK;
}

static bool oktp_level_cites(const oktp_level_t *level, const char *doc_id)
{
    uint8_t i;

    for (i = 0u; i < level->doc_count; i++) {
        if (oktp_streq(level->docs[i], doc_id)) {
            return true;
        }
    }
    return false;
}

oktp_status_t sutr_oktp_verify(const char **out_doc, size_t *out_index)
{
    size_t i;
    uint8_t d;
    unsigned l;
    oktp_status_t st;

    if (out_doc != NULL) {
        *out_doc = NULL;
    }
    if (out_index != NULL) {
        *out_index = 0u;
    }

    /* Every matrix citation must be registered with the right class. */
    for (i = 0u; i < (size_t)OKTP_MATRIX_COUNT; i++) {
        const oktp_matrix_entry_t *e = &g_matrix[i];
        if (e->doc_count == 0u || e->doc_count > OKTP_MAX_DOCS_PER_ROW ||
            e->subsystem == NULL || e->why == NULL) {
            if (out_index != NULL) {
                *out_index = i;
            }
            return OKTP_E_PROFILE;
        }
        if (e->tier > (uint8_t)OKTP_TIER_NA) {
            if (out_index != NULL) {
                *out_index = i;
            }
            return OKTP_E_PROFILE;
        }
        for (d = 0u; d < e->doc_count; d++) {
            st = sutr_oktp_check_citation(e->docs[d]);
            if (st != OKTP_OK) {
                if (out_doc != NULL) {
                    *out_doc = e->docs[d];
                }
                if (out_index != NULL) {
                    *out_index = i;
                }
                return st;
            }
        }
    }

    /* Levels are cumulative and each names at least one document. */
    for (l = 0u; l < OKTP_LEVEL_COUNT; l++) {
        const oktp_level_t *lv = &g_levels[l];
        if (lv->doc_count == 0u || lv->id == NULL || lv->title == NULL) {
            return OKTP_E_PROFILE;
        }
        for (d = 0u; d < lv->doc_count; d++) {
            st = sutr_oktp_check_citation(lv->docs[d]);
            if (st != OKTP_OK) {
                if (out_doc != NULL) {
                    *out_doc = lv->docs[d];
                }
                return st;
            }
        }
        if (l > 0u) {
            const oktp_level_t *prev = &g_levels[l - 1u];
            for (d = 0u; d < prev->doc_count; d++) {
                if (!oktp_level_cites(lv, prev->docs[d])) {
                    if (out_doc != NULL) {
                        *out_doc = prev->docs[d];
                    }
                    return OKTP_E_PROFILE;
                }
            }
        }
    }

    /* A higher level cannot be met while a lower one is not. */
    for (l = 1u; l < OKTP_LEVEL_COUNT; l++) {
        if (g_levels[l].met && !g_levels[l - 1u].met) {
            return OKTP_E_PROFILE;
        }
    }

    /* Every gap's closing document must exist. */
    for (i = 0u; i < (size_t)OKTP_GAP_COUNT; i++) {
        st = sutr_oktp_check_citation(g_gaps[i].closed_by);
        if (st != OKTP_OK) {
            if (out_doc != NULL) {
                *out_doc = g_gaps[i].closed_by;
            }
            return st;
        }
    }

    /* Every gate must name a command and the claim it proves. */
    for (i = 0u; i < (size_t)OKTP_GATE_COUNT; i++) {
        if (g_gates[i].id == NULL || g_gates[i].command == NULL ||
            g_gates[i].proves == NULL ||
            oktp_strlen_bounded(g_gates[i].command, 256u) == 0u) {
            return OKTP_E_PROFILE;
        }
    }

    return OKTP_OK;
}
