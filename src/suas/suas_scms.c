/* SUAS-012 — System Character Manifest & Schema Standard (SCMS)
 *
 * Freestanding C99, zero allocation. The engine is a set of bounded decoders
 * over a caller-supplied line buffer plus a caller-owned streaming state.
 * Nothing is retained beyond a call, nothing is copied into writable storage,
 * and no decoder may exceed SUAS_SCMS_MAX_LINE bytes of input.
 *
 * The grammar is deliberately rigid rather than merely strict: every field has
 * one form, every record has one length, and every deviation has its own
 * outcome code. A character database compiler that rejects a manifest must be
 * able to say why in one word, because the alternative — a silently truncated
 * radical or an inadmissible codepoint reaching a kernel table — is exactly
 * the failure this standard exists to prevent.
 */

#include "suas/suas_scms.h"
#include "suas/suas_ssre.h"

/* ── Grammar self-checks ──────────────────────────────────────────── */

typedef char scms_assert_field_count[
    (SUAS_SCMS_RECORD_FIELD_COUNT == 6u) ? 1 : -1];
typedef char scms_assert_line_budget[
    (SUAS_SCMS_MAX_LINE >= 32U) ? 1 : -1];
typedef char scms_assert_radical[
    (SUAS_SCMS_RADICAL_MAX <= 65535u) ? 1 : -1];
typedef char scms_assert_strokes[
    (SUAS_SCMS_STROKES_MAX <= 255u) ? 1 : -1];
typedef char scms_assert_flags[
    (SUAS_SCMS_FLAGS_MAX <= 0xFFFFu) ? 1 : -1];
typedef char scms_assert_planes[
    (SUAS_SCMS_PLANE_MASK_ALL == 0x0Fu) ? 1 : -1];

/* ── Engine state ─────────────────────────────────────────────────── */

static bool        scms_ready = false;
static scms_error_t scms_last_error = SCMS_OK;

static void scms_arm(void)
{
    scms_ready = true;
}

void suas_scms_init_engine(void)
{
    /* The grammar's consistency is already enforced at compile time by the
     * assertions above; arming is the run-time acknowledgement that a tool is
     * about to consume the grammar, and is idempotent. */
    scms_arm();
    scms_last_error = SCMS_OK;
}

static void scms_fail(scms_error_t error)
{
    scms_last_error = error;
}

scms_error_t suas_scms_last_error(void)
{
    return scms_last_error;
}

const char* suas_scms_error_name(scms_error_t error)
{
    switch (error) {
    case SCMS_OK:                         return "SCMS_OK";
    case SCMS_ERR_NULL_INPUT:             return "SCMS_ERR_NULL_INPUT";
    case SCMS_ERR_LINE_TOO_LONG:          return "SCMS_ERR_LINE_TOO_LONG";
    case SCMS_ERR_EMPTY_FIELD:            return "SCMS_ERR_EMPTY_FIELD";
    case SCMS_ERR_BAD_FIELD_COUNT:        return "SCMS_ERR_BAD_FIELD_COUNT";
    case SCMS_ERR_BAD_CODEPOINT:          return "SCMS_ERR_BAD_CODEPOINT";
    case SCMS_ERR_INADMISSIBLE_CODEPOINT: return "SCMS_ERR_INADMISSIBLE_CODEPOINT";
    case SCMS_ERR_RANGE_ORDER:            return "SCMS_ERR_RANGE_ORDER";
    case SCMS_ERR_RANGE_TOO_WIDE:         return "SCMS_ERR_RANGE_TOO_WIDE";
    case SCMS_ERR_BAD_RADICAL:            return "SCMS_ERR_BAD_RADICAL";
    case SCMS_ERR_BAD_STROKES:            return "SCMS_ERR_BAD_STROKES";
    case SCMS_ERR_BAD_FLAGS:              return "SCMS_ERR_BAD_FLAGS";
    case SCMS_ERR_BAD_WEIGHT:             return "SCMS_ERR_BAD_WEIGHT";
    case SCMS_ERR_BAD_DIRECTIVE:          return "SCMS_ERR_BAD_DIRECTIVE";
    case SCMS_ERR_MISSING_HEADER:         return "SCMS_ERR_MISSING_HEADER";
    case SCMS_ERR_RECORD_COUNT_MISMATCH:  return "SCMS_ERR_RECORD_COUNT_MISMATCH";
    case SCMS_ERR_EMPTY_MANIFEST:         return "SCMS_ERR_EMPTY_MANIFEST";
    default:                              return "SCMS_ERR_UNKNOWN";
    }
}

/* ── Bounded character decoders (§4.2) ────────────────────────────── */

static int scms_hex(char c)
{
    if (c >= '0' && c <= '9') return (int)(c - '0');
    if (c >= 'a' && c <= 'f') return (int)(c - 'a') + 10;
    if (c >= 'A' && c <= 'F') return (int)(c - 'A') + 10;
    return -1;
}

static bool scms_is_dec(char c)
{
    return c >= '0' && c <= '9';
}

static bool scms_is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/* "U+4E00" — 'U' or 'u', '+', then 1…8 hex digits and nothing after. */
static bool scms_decode_cp(const char *s, size_t len, sucs_ex_char_t *out)
{
    uint64_t v = 0u;
    size_t i;

    if (len < 3u || len - 2u > SUAS_SCMS_CODEPOINT_HEX_MAX)
        return false;
    if (s[0] != 'U' && s[0] != 'u')
        return false;
    if (s[1] != '+')
        return false;
    for (i = 2u; i < len; i++) {
        int d = scms_hex(s[i]);
        if (d < 0)
            return false;
        v = (v << 4) | (uint64_t)d;
    }
    *out = (sucs_ex_char_t)v;
    return true;
}

/* "0xFFFF" — '0', 'x' or 'X', then 1…4 hex digits and nothing after. */
static bool scms_decode_hex(const char *s, size_t len, uint32_t max,
                            uint32_t *out)
{
    uint64_t v = 0u;
    size_t i;

    if (len < 3u || len - 2u > SUAS_SCMS_FLAGS_HEX_MAX)
        return false;
    if (s[0] != '0')
        return false;
    if (s[1] != 'x' && s[1] != 'X')
        return false;
    for (i = 2u; i < len; i++) {
        int d = scms_hex(s[i]);
        if (d < 0)
            return false;
        v = (v << 4) | (uint64_t)d;
        if (v > 0xFFFFFFFFu)
            return false;
    }
    if (v > (uint64_t)max)
        return false;
    *out = (uint32_t)v;
    return true;
}

/* A bare decimal field of at most 10 digits, capped by the caller's bound. */
static bool scms_decode_dec(const char *s, size_t len, uint32_t max,
                            uint32_t *out)
{
    uint64_t v = 0u;
    size_t i;

    if (len == 0u || len > SUAS_SCMS_DECIMAL_MAX)
        return false;
    for (i = 0u; i < len; i++) {
        if (!scms_is_dec(s[i]))
            return false;
        v = v * 10u + (uint64_t)(s[i] - '0');
        if (v > 0xFFFFFFFFu)
            return false;
    }
    if (v > (uint64_t)max)
        return false;
    *out = (uint32_t)v;
    return true;
}

/* ── Line classification (§3.1) ───────────────────────────────────── */

/* The shared dispatcher. Returns the line kind and, for an invalid line, the
 * outcome that explains it. */
static scms_line_kind_t scms_classify(const char *line, size_t length,
                                      scms_error_t *why)
{
    if (line == NULL) {
        *why = SCMS_ERR_NULL_INPUT;
        return SCMS_LINE_INVALID;
    }
    if (length >= SUAS_SCMS_MAX_LINE) {
        *why = SCMS_ERR_LINE_TOO_LONG;
        return SCMS_LINE_INVALID;
    }
    if (length == 0u) {
        *why = SCMS_OK;
        return SCMS_LINE_BLANK;
    }
    *why = SCMS_OK;
    if (line[0] == '#')
        return SCMS_LINE_COMMENT;
    if (line[0] == '%')
        return SCMS_LINE_DIRECTIVE;
    return SCMS_LINE_RECORD;
}

scms_line_kind_t suas_scms_line_kind(const char *manifest_line, size_t length)
{
    scms_error_t why = SCMS_OK;

    return scms_classify(manifest_line, length, &why);
}

/* ── Tab-delimited field walking (§4.1) ───────────────────────────── */

/* Extracts the next single-tab-delimited field. A run of two tabs, or a
 * leading or trailing tab, yields a zero-length field and is rejected by the
 * caller as SCMS_ERR_EMPTY_FIELD. */
static bool scms_next_field(const char *s, size_t len, size_t *pos,
                            const char **fstart, size_t *flen)
{
    size_t i;

    if (*pos >= len)
        return false;
    i = *pos;
    while (i < len && s[i] != '\t')
        i++;
    *fstart = s + *pos;
    *flen   = i - *pos;
    *pos    = (i < len) ? i + 1u : i;
    return true;
}

/* ── Range, plane, and admissibility validation (§4.3, §4.4) ──────── */

static bool scms_in(sucs_ex_char_t cp, sucs_ex_char_t lo, sucs_ex_char_t hi)
{
    return cp >= lo && cp <= hi;
}

static bool scms_overlaps(sucs_ex_char_t a0, sucs_ex_char_t a1,
                          sucs_ex_char_t b0, sucs_ex_char_t b1)
{
    return a0 <= b1 && b0 <= a1;
}

bool suas_scms_codepoint_admissible(sucs_ex_char_t codepoint)
{
    /* Everything above the last allocatable Native codepoint is either a trap
     * slot or the Sentinel, and none of it may appear in a manifest. */
    if (codepoint > SUAS_SCMS_ZONE_NATIVE_LAST)
        return false;
    /* The System Control Plane is reserved machinery, never data. */
    if (scms_in(codepoint, SUAS_SCMS_ZONE_SCP_MIN, SUAS_SCMS_ZONE_SCP_MAX))
        return false;
    return true;
}

bool suas_scms_range_admissible(sucs_ex_char_t start, sucs_ex_char_t end)
{
    if (start > end)
        return false;
    if (!suas_scms_codepoint_admissible(start) ||
        !suas_scms_codepoint_admissible(end))
        return false;
    if ((end - start) > SUAS_SCMS_RANGE_MAX_SPAN)
        return false;
    /* Rejecting both endpoints is not enough: a range that begins on the
     * Bridge and ends in the Native plane has admissible endpoints and still
     * swallows every Control Plane codepoint in its interior. A manifest
     * range is a promise that every codepoint it covers is data, so the
     * inadmissible zones must not lie within it either. */
    if (start <= SUAS_SCMS_ZONE_SCP_MAX && end >= SUAS_SCMS_ZONE_SCP_MIN)
        return false;
    if (start <= SUAS_SCMS_ZONE_TRAP_MAX && end >= SUAS_SCMS_ZONE_TRAP_MIN)
        return false;
    return true;
}

uint8_t suas_scms_plane_mask(sucs_ex_char_t start, sucs_ex_char_t end)
{
    uint8_t mask = 0u;

    if (start > end)
        return 0u;
    if (scms_overlaps(start, end, (sucs_ex_char_t)0u,
                      SUAS_SCMS_ZONE_UNICODE_MAX))
        mask |= SUAS_SCMS_PLANE_BRIDGE;
    if (scms_overlaps(start, end, SUAS_SCMS_ZONE_SCP_MIN,
                      SUAS_SCMS_ZONE_SCP_MAX))
        mask |= SUAS_SCMS_PLANE_SCP;
    if (scms_overlaps(start, end, SUAS_SCMS_ZONE_NATIVE_MIN,
                      SUAS_SCMS_ZONE_NATIVE_LAST))
        mask |= SUAS_SCMS_PLANE_NATIVE;
    /* RNUR is a carve-out inside the Native plane, so a range wholly within it
     * reports both bits: it is Native, and specifically neography. */
    if (scms_overlaps(start, end, SUAS_SCMS_RNUR_MIN, SUAS_SCMS_RNUR_MAX))
        mask |= SUAS_SCMS_PLANE_RNUR;
    return mask;
}

bool suas_scms_is_binary_aligned(const scms_record_descriptor_t *desc)
{
    return desc != NULL && desc->start_codepoint == desc->end_codepoint;
}

/* ── Record validation (§4) ───────────────────────────────────────── */

bool suas_scms_validate_record(const char *manifest_line, size_t length,
                               scms_record_descriptor_t *out_desc)
{
    const char *f;
    size_t flen;
    size_t pos = 0u;
    sucs_ex_char_t start = 0u;
    sucs_ex_char_t end   = 0u;
    uint32_t radical = 0u;
    uint32_t strokes = 0u;
    uint32_t flags   = 0u;
    uint32_t weight  = 0u;

    if (!scms_ready)
        scms_arm();

    if (manifest_line == NULL || out_desc == NULL) {
        scms_fail(SCMS_ERR_NULL_INPUT);
        return false;
    }
    if (length >= SUAS_SCMS_MAX_LINE) {
        scms_fail(SCMS_ERR_LINE_TOO_LONG);
        return false;
    }
    /* A blank, comment, or directive line is not a record; the caller
     * dispatches on line kind before validating. */
    if (length == 0u || manifest_line[0] == '#' || manifest_line[0] == '%') {
        scms_fail(SCMS_ERR_BAD_FIELD_COUNT);
        return false;
    }

    /* Column 0 — start codepoint. A leading tab or space leaves this field
     * malformed, which surfaces as SCMS_ERR_BAD_CODEPOINT: leading whitespace
     * is not permitted anywhere in a record. */
    if (!scms_next_field(manifest_line, length, &pos, &f, &flen)) {
        scms_fail(SCMS_ERR_BAD_FIELD_COUNT);
        return false;
    }
    if (flen == 0u) {
        scms_fail(SCMS_ERR_EMPTY_FIELD);
        return false;
    }
    if (!scms_decode_cp(f, flen, &start)) {
        scms_fail(SCMS_ERR_BAD_CODEPOINT);
        return false;
    }

    /* Column 1 — end codepoint. */
    if (!scms_next_field(manifest_line, length, &pos, &f, &flen)) {
        scms_fail(SCMS_ERR_BAD_FIELD_COUNT);
        return false;
    }
    if (flen == 0u) {
        scms_fail(SCMS_ERR_EMPTY_FIELD);
        return false;
    }
    if (!scms_decode_cp(f, flen, &end)) {
        scms_fail(SCMS_ERR_BAD_CODEPOINT);
        return false;
    }

    /* Column 2 — radical. */
    if (!scms_next_field(manifest_line, length, &pos, &f, &flen)) {
        scms_fail(SCMS_ERR_BAD_FIELD_COUNT);
        return false;
    }
    if (flen == 0u) {
        scms_fail(SCMS_ERR_EMPTY_FIELD);
        return false;
    }
    if (!scms_decode_dec(f, flen, SUAS_SCMS_RADICAL_MAX, &radical)) {
        scms_fail(SCMS_ERR_BAD_RADICAL);
        return false;
    }

    /* Column 3 — stroke count. */
    if (!scms_next_field(manifest_line, length, &pos, &f, &flen)) {
        scms_fail(SCMS_ERR_BAD_FIELD_COUNT);
        return false;
    }
    if (flen == 0u) {
        scms_fail(SCMS_ERR_EMPTY_FIELD);
        return false;
    }
    if (!scms_decode_dec(f, flen, SUAS_SCMS_STROKES_MAX, &strokes)) {
        scms_fail(SCMS_ERR_BAD_STROKES);
        return false;
    }

    /* Column 4 — flag word. */
    if (!scms_next_field(manifest_line, length, &pos, &f, &flen)) {
        scms_fail(SCMS_ERR_BAD_FIELD_COUNT);
        return false;
    }
    if (flen == 0u) {
        scms_fail(SCMS_ERR_EMPTY_FIELD);
        return false;
    }
    if (!scms_decode_hex(f, flen, SUAS_SCMS_FLAGS_MAX, &flags)) {
        scms_fail(SCMS_ERR_BAD_FLAGS);
        return false;
    }

    /* Column 5 — collation weight. */
    if (!scms_next_field(manifest_line, length, &pos, &f, &flen)) {
        scms_fail(SCMS_ERR_BAD_FIELD_COUNT);
        return false;
    }
    if (flen == 0u) {
        scms_fail(SCMS_ERR_EMPTY_FIELD);
        return false;
    }
    if (!scms_decode_dec(f, flen, SUAS_SCMS_WEIGHT_MAX, &weight)) {
        scms_fail(SCMS_ERR_BAD_WEIGHT);
        return false;
    }

    /* Exactly six columns: a seventh makes the line a malformed record. */
    if (pos < length) {
        scms_fail(SCMS_ERR_BAD_FIELD_COUNT);
        return false;
    }

    /* Range validation, in strict order so the outcome names the first thing
     * that is wrong: ordering, then the endpoints, then the interior, then
     * the span ceiling. */
    if (start > end) {
        scms_fail(SCMS_ERR_RANGE_ORDER);
        return false;
    }
    if (!suas_scms_codepoint_admissible(start) ||
        !suas_scms_codepoint_admissible(end)) {
        scms_fail(SCMS_ERR_INADMISSIBLE_CODEPOINT);
        return false;
    }
    /* Rejecting both endpoints is not enough: a range that begins on the
     * Bridge and ends in the Native plane has admissible endpoints and still
     * swallows every Control Plane codepoint in its interior. A manifest
     * range is a promise that every codepoint it covers is data. */
    if ((start <= SUAS_SCMS_ZONE_SCP_MAX && end >= SUAS_SCMS_ZONE_SCP_MIN) ||
        (start <= SUAS_SCMS_ZONE_TRAP_MAX && end >= SUAS_SCMS_ZONE_TRAP_MIN)) {
        scms_fail(SCMS_ERR_INADMISSIBLE_CODEPOINT);
        return false;
    }
    if ((end - start) > SUAS_SCMS_RANGE_MAX_SPAN) {
        scms_fail(SCMS_ERR_RANGE_TOO_WIDE);
        return false;
    }

    out_desc->start_codepoint = start;
    out_desc->end_codepoint   = end;
    out_desc->field_count     = (uint16_t)SUAS_SCMS_RECORD_FIELD_COUNT;
    out_desc->is_binary_aligned = (start == end);

    /* The four payload columns are decoded for their bounds checks and then
     * deliberately not carried into the descriptor: the descriptor reports
     * structure, and a compiler walks the columns themselves through
     * suas_scms_field_at(). */
    (void)radical;
    (void)strokes;
    (void)flags;
    (void)weight;

    scms_fail(SCMS_OK);
    return true;
}

bool suas_scms_field_at(const char *manifest_line, size_t length,
                        uint16_t index, uint32_t *out_value)
{
    const char *f;
    size_t flen;
    size_t pos = 0u;
    uint16_t n = 0u;
    sucs_ex_char_t cp = 0u;

    if (manifest_line == NULL || out_value == NULL) {
        scms_fail(SCMS_ERR_NULL_INPUT);
        return false;
    }
    if (length >= SUAS_SCMS_MAX_LINE) {
        scms_fail(SCMS_ERR_LINE_TOO_LONG);
        return false;
    }
    if (index >= (uint16_t)SUAS_SCMS_RECORD_FIELD_COUNT) {
        scms_fail(SCMS_ERR_BAD_FIELD_COUNT);
        return false;
    }

    while (scms_next_field(manifest_line, length, &pos, &f, &flen)) {
        if (n == index) {
            bool ok;
            switch (index) {
            case 0u: /* start codepoint */
            case 1u: /* end codepoint   */
                ok = scms_decode_cp(f, flen, &cp);
                if (ok)
                    *out_value = (uint32_t)cp;
                break;
            case 2u: ok = scms_decode_dec(f, flen, SUAS_SCMS_RADICAL_MAX, out_value); break;
            case 3u: ok = scms_decode_dec(f, flen, SUAS_SCMS_STROKES_MAX, out_value); break;
            case 4u: ok = scms_decode_hex(f, flen, SUAS_SCMS_FLAGS_MAX, out_value);   break;
            default: ok = scms_decode_dec(f, flen, SUAS_SCMS_WEIGHT_MAX, out_value);  break;
            }
            if (!ok)
                scms_fail(SCMS_ERR_BAD_CODEPOINT);
            else
                scms_fail(SCMS_OK);
            return ok;
        }
        n++;
    }

    scms_fail(SCMS_ERR_BAD_FIELD_COUNT);
    return false;
}

/* ── Directive validation and folding (§3.2) ──────────────────────── */

/* Well-formedness only: '%' name WSP value, with name letters-only and value
 * free of whitespace. An unknown but well-formed name is accepted and
 * ignored, so a manifest may carry toolchain-private directives. */
bool suas_scms_validate_directive(const char *manifest_line, size_t length)
{
    size_t i = 1u;         /* past the '%' */
    size_t name_len;
    size_t value_len;

    if (!scms_ready)
        scms_arm();
    if (manifest_line == NULL) {
        scms_fail(SCMS_ERR_NULL_INPUT);
        return false;
    }
    if (length >= SUAS_SCMS_MAX_LINE) {
        scms_fail(SCMS_ERR_LINE_TOO_LONG);
        return false;
    }
    if (length < 3u || manifest_line[0] != '%') {
        scms_fail(SCMS_ERR_BAD_DIRECTIVE);
        return false;
    }
    while (i < length && scms_is_alpha(manifest_line[i]))
        i++;
    name_len = i - 1u;
    if (name_len == 0u || name_len > 16u) {
        scms_fail(SCMS_ERR_BAD_DIRECTIVE);
        return false;
    }
    if (i >= length || (manifest_line[i] != ' ' && manifest_line[i] != '\t')) {
        scms_fail(SCMS_ERR_BAD_DIRECTIVE);
        return false;
    }
    while (i < length && (manifest_line[i] == ' ' || manifest_line[i] == '\t'))
        i++;
    value_len = length - i;
    if (value_len == 0u) {
        scms_fail(SCMS_ERR_BAD_DIRECTIVE);
        return false;
    }
    for (; i < length; i++) {
        char c = manifest_line[i];
        if (c <= 0x20 || c > 0x7E) {
            scms_fail(SCMS_ERR_BAD_DIRECTIVE);
            return false;
        }
    }
    scms_fail(SCMS_OK);
    return true;
}

static bool scms_name_is(const char *line, size_t len, const char *name)
{
    size_t i = 1u;
    size_t j = 0u;

    while (i < len && line[i] != ' ' && line[i] != '\t') {
        if (name[j] == '\0' || line[i] != name[j])
            return false;
        i++;
        j++;
    }
    return name[j] == '\0';
}

/* ── Streaming manifest reader (§3.3) ─────────────────────────────── */

void suas_scms_begin(scms_manifest_state_t *state)
{
    if (state == NULL) {
        scms_fail(SCMS_ERR_NULL_INPUT);
        return;
    }
    if (!scms_ready)
        scms_arm();
    state->header.manifest_version = NULL;
    state->header.total_records    = 0u;
    state->header.plane_mask       = 0u;
    state->lines          = 0u;
    state->records        = 0u;
    state->comments       = 0u;
    state->directives     = 0u;
    state->errors         = 0u;
    state->first_error_line = 0u;
    state->last_error     = (uint8_t)SCMS_OK;
    state->observed_planes = 0u;
    state->saw_manifest   = false;
    state->saw_records    = false;
    scms_fail(SCMS_OK);
}

static void scms_note_error(scms_manifest_state_t *state, scms_error_t why)
{
    state->errors++;
    if (state->first_error_line == 0u) {
        state->first_error_line = state->lines;
        state->last_error = (uint8_t)why;
    }
    scms_fail(why);
}

/* The byte offset of a directive's value: past the name and its whitespace
 * run. Assumes the line already passed suas_scms_validate_directive. */
static size_t scms_value_offset(const char *line, size_t len)
{
    size_t i = 1u;

    while (i < len && line[i] != ' ' && line[i] != '\t')
        i++;
    while (i < len && (line[i] == ' ' || line[i] == '\t'))
        i++;
    return i;
}

static bool scms_fold_directive(scms_manifest_state_t *state,
                                const char *line, size_t len)
{
    if (!suas_scms_validate_directive(line, len)) {
        scms_note_error(state, (scms_error_t)scms_last_error);
        return false;
    }
    if (scms_name_is(line, len, "manifest")) {
        /* The version string borrows the caller's buffer; a build tool keeps
         * the manifest alive for the duration of the compile. */
        state->header.manifest_version = line + scms_value_offset(line, len);
        state->saw_manifest = true;
    } else if (scms_name_is(line, len, "records")) {
        uint32_t total = 0u;
        size_t i = scms_value_offset(line, len);
        if (!scms_decode_dec(line + i, len - i, 0xFFFFFFFFu, &total)) {
            scms_note_error(state, SCMS_ERR_BAD_DIRECTIVE);
            return false;
        }
        state->header.total_records = total;
        state->saw_records = true;
    } else if (scms_name_is(line, len, "planes")) {
        uint32_t mask = 0u;
        size_t i = scms_value_offset(line, len);
        if (!scms_decode_hex(line + i, len - i, SUAS_SCMS_PLANE_MASK_ALL, &mask)) {
            scms_note_error(state, SCMS_ERR_BAD_DIRECTIVE);
            return false;
        }
        state->header.plane_mask = (uint32_t)mask;
    }
    /* Any other well-formed name is reserved for tooling and ignored. */
    return true;
}

bool suas_scms_feed_line(scms_manifest_state_t *state,
                         const char *manifest_line, size_t length)
{
    scms_error_t why = SCMS_OK;
    scms_line_kind_t kind;

    if (state == NULL) {
        scms_fail(SCMS_ERR_NULL_INPUT);
        return false;
    }
    if (!scms_ready)
        scms_arm();

    state->lines++;
    kind = scms_classify(manifest_line, length, &why);

    switch (kind) {
    case SCMS_LINE_BLANK:
        return true;

    case SCMS_LINE_COMMENT:
        state->comments++;
        return true;

    case SCMS_LINE_DIRECTIVE:
        state->directives++;
        return scms_fold_directive(state, manifest_line, length);

    case SCMS_LINE_RECORD: {
        scms_record_descriptor_t desc;
        if (!suas_scms_validate_record(manifest_line, length, &desc)) {
            scms_note_error(state, (scms_error_t)scms_last_error);
            return false;
        }
        state->records++;
        state->observed_planes |=
            suas_scms_plane_mask(desc.start_codepoint, desc.end_codepoint);
        return true;
    }

    default:
        scms_note_error(state, why);
        return false;
    }
}

bool suas_scms_end(const scms_manifest_state_t *state)
{
    if (state == NULL) {
        scms_fail(SCMS_ERR_NULL_INPUT);
        return false;
    }
    scms_fail(SCMS_OK);
    if (!state->saw_manifest) {
        scms_fail(SCMS_ERR_MISSING_HEADER);
        return false;
    }
    if (state->records == 0u) {
        scms_fail(SCMS_ERR_EMPTY_MANIFEST);
        return false;
    }
    if (state->errors != 0u) {
        scms_fail((scms_error_t)state->last_error);
        return false;
    }
    if (state->saw_records && state->header.total_records != state->records) {
        scms_fail(SCMS_ERR_RECORD_COUNT_MISMATCH);
        return false;
    }
    return true;
}

bool suas_scms_planes_declared(const scms_manifest_state_t *state)
{
    if (state == NULL || !state->saw_manifest)
        return false;
    /* Every plane the records touched must appear in the declared mask. A
     * manifest that omits %planes declares nothing, so nothing it wrote can
     * satisfy the check. */
    return (state->observed_planes & ~(uint8_t)state->header.plane_mask) == 0u;
}

/* ── SUAS-011 (SSRE) linkage gate (§6) ────────────────────────────── */

bool suas_scms_verify_schema_link(const scms_schema_link_t *link)
{
    ssre_dependency_descriptor_t dep;

    if (link == NULL || link->schema_spec_id == NULL) {
        scms_fail(SCMS_ERR_NULL_INPUT);
        return false;
    }
    dep.spec_id           = link->schema_spec_id;
    dep.min_version_major = link->schema_major;
    dep.min_version_minor = link->schema_minor;
    dep.is_mandatory      = link->schema_mandatory;

    if (!suas_ssre_verify_dependency(&dep)) {
        /* A manifest aimed at an unregistered or unsatisfiable schema is a
         * linkage error, reported in SCMS's own vocabulary. */
        scms_fail(SCMS_ERR_MISSING_HEADER);
        return false;
    }
    scms_fail(SCMS_OK);
    return true;
}

/* ── Setup ────────────────────────────────────────────────────────── */

#define SCMS_STR2(x) #x
#define SCMS_STR(x)  SCMS_STR2(x)

const char* suas_scms_version_string(void)
{
    return SUAS_SCMS_SPEC_ID "/"
           SCMS_STR(SUAS_SCMS_VERSION_MAJOR) "."
           SCMS_STR(SUAS_SCMS_VERSION_MINOR) "."
           SCMS_STR(SUAS_SCMS_VERSION_PATCH);
}
