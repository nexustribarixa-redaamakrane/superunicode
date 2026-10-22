/*
 * SUTR-10 - SuperUnicode Conformance Model (SUCM)
 *
 * The reference implementation of the compliance surface of SUTR-10: the
 * three cumulative tiers of section 3, the clause registry of section 5, the
 * claim identifier grammar of section 2.3 and the naming rules R1-R4 of
 * section 5.4, the claim row and gate record of sections 7-8, the core
 * engine registry of section 8, and the resource gates of sections 3.2,
 * 3.3 and 6.
 *
 * Architectural notes:
 *
 *   - Freestanding C99. No libc, no <string.h>, no heap. The module
 *     owns nothing: every table is static and every descriptor is a
 *     fixed-size struct the caller supplies or receives by value.
 *   - Bounded. Every table has a compile-time bound; every lookup is a
 *     scan over a fixed-size table, so the cost of a query does not grow
 *     with the size of the corpus (section 4.2).
 *   - Deterministic. No locale, no environment, no clock, no random
 *     source is read (section 4.4).
 *   - Fail closed. A claim that cannot be parsed, that omits a clause of
 *     its tier, or whose gate record is not green is rejected rather
 *     than admitted (section 3.3.5, section 5.1 C6).
 *
 * The normative contract of every function below is the section of
 * docs/sutr/SUTR-10-sucm.md named in its comment.
 */

#include "sutr/sutr_sucm.h"

/* -- Identifiers (sections 1 and 11) ------------------------------- */

const char *sutr_sucm_version_string(void)
{
    return "0.1.0";
}

/* -- A freestanding string kit (no <string.h>) --------------------- */

static size_t sucm_strlen(const char *text)
{
    size_t length = 0u;

    if (text == NULL) {
        return 0u;
    }
    while (text[length] != '\0') {
        length++;
    }
    return length;
}

static bool sucm_streq(const char *left, const char *right)
{
    size_t i;

    if (left == NULL || right == NULL) {
        return false;
    }
    for (i = 0u;; i++) {
        if (left[i] != right[i]) {
            return false;
        }
        if (left[i] == '\0') {
            return true;
        }
    }
}

static bool sucm_streq_ci(const char *left, const char *right)
{
    size_t i;

    if (left == NULL || right == NULL) {
        return false;
    }
    for (i = 0u;; i++) {
        char a = left[i];
        char b = right[i];

        if (a >= 'a' && a <= 'z') {
            a = (char)(a - 'a' + 'A');
        }
        if (b >= 'a' && b <= 'z') {
            b = (char)(b - 'a' + 'A');
        }
        if (a != b) {
            return false;
        }
        if (a == '\0') {
            return true;
        }
    }
}

static bool sucm_streq_ci_n(const char *left, const char *right, size_t count)
{
    size_t i;

    for (i = 0u; i < count; i++) {
        char a = left[i];
        char b = right[i];

        if (a >= 'a' && a <= 'z') {
            a = (char)(a - 'a' + 'A');
        }
        if (b >= 'a' && b <= 'z') {
            b = (char)(b - 'a' + 'A');
        }
        if (a != b) {
            return false;
        }
    }
    return true;
}

static void sucm_copy(char *dst, size_t capacity, const char *src)
{
    size_t i = 0u;

    if (dst == NULL || capacity == 0u) {
        return;
    }
    if (src != NULL) {
        while (src[i] != '\0' && i + 1u < capacity) {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = '\0';
}

static bool sucm_is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool sucm_is_alpha(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static bool sucm_is_alnum(char c)
{
    return sucm_is_alpha(c) || sucm_is_digit(c);
}

/* -- The tiers of section 3 ---------------------------------------- */

const char *sutr_sucm_tier_name(sucm_tier_t tier)
{
    switch (tier) {
        case SUCM_TIER_L0: return "L0";
        case SUCM_TIER_L1: return "L1";
        case SUCM_TIER_L2: return "L2";
        default:           return "unknown";
    }
}

sucm_tier_t sutr_sucm_tier_from_name(const char *name)
{
    if (sucm_streq_ci(name, "L0")) {
        return SUCM_TIER_L0;
    }
    if (sucm_streq_ci(name, "L1")) {
        return SUCM_TIER_L1;
    }
    if (sucm_streq_ci(name, "L2")) {
        return SUCM_TIER_L2;
    }
    return SUCM_TIER_COUNT;
}

bool sutr_sucm_tier_is_valid(sucm_tier_t tier)
{
    return (tier == SUCM_TIER_L0) || (tier == SUCM_TIER_L1) ||
           (tier == SUCM_TIER_L2);
}

/*
 * The tier inclusion of section 3.4: L2 subsumes L1 subsumes L0. A unit
 * that claims a tier asserts every clause of the tiers below it, so the
 * higher tier always includes the lower one.
 */
bool sutr_sucm_tier_includes(sucm_tier_t higher, sucm_tier_t lower)
{
    if (!sutr_sucm_tier_is_valid(higher) || !sutr_sucm_tier_is_valid(lower)) {
        return false;
    }
    return (int)higher >= (int)lower;
}

/* -- The clause registry of section 5.2 ----------------------------- */

/*
 * The identifiers are stable: a clause is never renumbered and a
 * withdrawn clause is marked and never reused (sections 5.2 and 11). The
 * registry is the machine-readable form of sections 5 and 6.
 */
static const sucm_clause_t g_sucm_clauses[SUCM_CLAUSE_COUNT] = {
    { "SUCM-D1", SUCM_CLAUSE_DEFINITION,  1u, 0u, "Conformance unit" },
    { "SUCM-D2", SUCM_CLAUSE_DEFINITION,  2u, 0u, "Conformance claim" },
    { "SUCM-D3", SUCM_CLAUSE_DEFINITION,  3u, 0u, "Tier" },
    { "SUCM-D4", SUCM_CLAUSE_DEFINITION,  4u, 0u, "Core validation engine" },
    { "SUCM-D5", SUCM_CLAUSE_DEFINITION,  5u, 0u, "Resource ceiling" },
    { "SUCM-D6", SUCM_CLAUSE_DEFINITION,  6u, 0u, "Verification gate" },
    { "SUCM-D7", SUCM_CLAUSE_DEFINITION,  7u, 0u, "Gate record" },
    { "SUCM-D8", SUCM_CLAUSE_DEFINITION,  8u, 0u, "System image" },

    { "SUCM-C1", SUCM_CLAUSE_CONFORMANCE, 1u, 0u, "Claim by tier name" },
    { "SUCM-C2", SUCM_CLAUSE_CONFORMANCE, 2u, 0u, "Tiers are cumulative" },
    { "SUCM-C3", SUCM_CLAUSE_CONFORMANCE, 3u, 0u, "Zero-allocation mandate" },
    { "SUCM-C4", SUCM_CLAUSE_CONFORMANCE, 4u, 0u, "Bridge 1:1 guarantee" },
    { "SUCM-C5", SUCM_CLAUSE_CONFORMANCE, 5u, 0u, "Native and integrity gates" },
    { "SUCM-C6", SUCM_CLAUSE_CONFORMANCE, 6u, 0u, "Unproven claim is void" },

    { "SUCM-R1", SUCM_CLAUSE_REQUIREMENT,  1u, 0u, "Identifier alphabet" },
    { "SUCM-R2", SUCM_CLAUSE_REQUIREMENT,  2u, 0u, "Identifier uniqueness" },
    { "SUCM-R3", SUCM_CLAUSE_REQUIREMENT,  3u, 0u, "Identifier equivalence" },
    { "SUCM-R4", SUCM_CLAUSE_REQUIREMENT,  4u, 0u, "Identifier provenance" },
    { "SUCM-R5", SUCM_CLAUSE_REQUIREMENT,  5u, 0u, "Freestanding" },
    { "SUCM-R6", SUCM_CLAUSE_REQUIREMENT,  6u, 0u, "Bounded" },
    { "SUCM-R7", SUCM_CLAUSE_REQUIREMENT,  7u, 0u, "Single pass" },
    { "SUCM-R8", SUCM_CLAUSE_REQUIREMENT,  8u, 0u, "SCP invariance" },
    { "SUCM-R9", SUCM_CLAUSE_REQUIREMENT,  9u, 0u, "Bridge 1:1" },
    { "SUCM-R10", SUCM_CLAUSE_REQUIREMENT, 10u, 0u, "Native gate" },
    { "SUCM-R11", SUCM_CLAUSE_REQUIREMENT, 11u, 0u, "Integrity gate" },
    { "SUCM-R12", SUCM_CLAUSE_REQUIREMENT, 12u, 0u, "Neography mapping" },
    { "SUCM-R13", SUCM_CLAUSE_REQUIREMENT, 13u, 0u, "Totality" },
    { "SUCM-R14", SUCM_CLAUSE_REQUIREMENT, 14u, 0u, "Determinism" },
    { "SUCM-R15", SUCM_CLAUSE_REQUIREMENT, 15u, 0u, "Gated claim" }
};

const char *sutr_sucm_clause_kind_name(sucm_clause_kind_t kind)
{
    switch (kind) {
        case SUCM_CLAUSE_DEFINITION:  return "definition";
        case SUCM_CLAUSE_CONFORMANCE: return "conformance";
        case SUCM_CLAUSE_REQUIREMENT: return "requirement";
        default:                      return "unknown";
    }
}

uint32_t sutr_sucm_clause_count(void)
{
    return SUCM_CLAUSE_COUNT;
}

bool sutr_sucm_clause_at(uint32_t index, sucm_clause_t *out)
{
    if (out == NULL || index >= SUCM_CLAUSE_COUNT) {
        return false;
    }
    *out = g_sucm_clauses[index];
    return true;
}

bool sutr_sucm_clause_by_id(const char *id, sucm_clause_t *out)
{
    uint32_t i;

    if (id == NULL) {
        return false;
    }
    for (i = 0u; i < SUCM_CLAUSE_COUNT; i++) {
        if (sucm_streq(g_sucm_clauses[i].id, id)) {
            if (out != NULL) {
                *out = g_sucm_clauses[i];
            }
            return true;
        }
    }
    return false;
}

sucm_clause_kind_t sutr_sucm_clause_kind_of(const char *id)
{
    sucm_clause_t clause;

    if (!sutr_sucm_clause_by_id(id, &clause)) {
        return SUCM_CLAUSE_KIND_COUNT;
    }
    return clause.kind;
}

uint32_t sutr_sucm_clause_number_of(const char *id)
{
    sucm_clause_t clause;

    if (!sutr_sucm_clause_by_id(id, &clause)) {
        return 0u;
    }
    return clause.number;
}

bool sutr_sucm_clause_is_withdrawn(const char *id)
{
    sucm_clause_t clause;

    if (!sutr_sucm_clause_by_id(id, &clause)) {
        return false;
    }
    return clause.withdrawn != 0u;
}

/* -- The clause sets of sections 5 and 8 --------------------------- */

/*
 * The registry index of a clause: the definitions occupy bits 0-7, the
 * conformance clauses bits 8-13, and the requirements bits 14-28.
 */
static bool sucm_clause_index(sucm_clause_kind_t kind, uint32_t number,
                               uint32_t *index)
{
    uint32_t base;
    uint32_t count;

    switch (kind) {
        case SUCM_CLAUSE_DEFINITION:
            base  = 0u;
            count = 8u;
            break;
        case SUCM_CLAUSE_CONFORMANCE:
            base  = 8u;
            count = 6u;
            break;
        case SUCM_CLAUSE_REQUIREMENT:
            base  = 14u;
            count = 15u;
            break;
        default:
            return false;
    }
    if (number < 1u || number > count) {
        return false;
    }
    *index = base + number - 1u;
    return true;
}

static uint32_t sucm_clause_bit(sucm_clause_kind_t kind, uint32_t number)
{
    uint32_t index;

    if (!sucm_clause_index(kind, number, &index)) {
        return 0u;
    }
    return 1u << index;
}

/*
 * The clause set a tier requires. The tiers are cumulative (section 3.4),
 * so the Level-1 set is the Level-0 set plus the bridge clauses, and the
 * Level-2 set is the Level-1 set plus the plugin clauses.
 *
 * The two gated-claim clauses - C6 and R15 - are enforced by the gate
 * record itself rather than by the clause column, which is why they do
 * not appear in any tier set.
 */
sucm_clause_set_t sutr_sucm_tier_clause_set(sucm_tier_t tier)
{
    sucm_clause_set_t set;
    uint32_t bits;

    set.bits = 0u;

    bits  = sucm_clause_bit(SUCM_CLAUSE_CONFORMANCE, 1u);
    bits |= sucm_clause_bit(SUCM_CLAUSE_CONFORMANCE, 2u);
    bits |= sucm_clause_bit(SUCM_CLAUSE_CONFORMANCE, 3u);
    bits |= sucm_clause_bit(SUCM_CLAUSE_REQUIREMENT, 5u);
    bits |= sucm_clause_bit(SUCM_CLAUSE_REQUIREMENT, 6u);
    bits |= sucm_clause_bit(SUCM_CLAUSE_REQUIREMENT, 7u);
    bits |= sucm_clause_bit(SUCM_CLAUSE_REQUIREMENT, 8u);
    bits |= sucm_clause_bit(SUCM_CLAUSE_REQUIREMENT, 13u);
    bits |= sucm_clause_bit(SUCM_CLAUSE_REQUIREMENT, 14u);
    set.bits = bits;

    if (tier == SUCM_TIER_L1 || tier == SUCM_TIER_L2) {
        set.bits |= sucm_clause_bit(SUCM_CLAUSE_CONFORMANCE, 4u);
        set.bits |= sucm_clause_bit(SUCM_CLAUSE_REQUIREMENT, 9u);
    }
    if (tier == SUCM_TIER_L2) {
        set.bits |= sucm_clause_bit(SUCM_CLAUSE_CONFORMANCE, 5u);
        set.bits |= sucm_clause_bit(SUCM_CLAUSE_REQUIREMENT, 10u);
        set.bits |= sucm_clause_bit(SUCM_CLAUSE_REQUIREMENT, 11u);
        set.bits |= sucm_clause_bit(SUCM_CLAUSE_REQUIREMENT, 12u);
    }
    return set;
}

bool sutr_sucm_clause_set_has(sucm_clause_set_t set, const char *id)
{
    sucm_clause_t clause;
    uint32_t index;

    if (!sutr_sucm_clause_by_id(id, &clause)) {
        return false;
    }
    if (!sucm_clause_index(clause.kind, clause.number, &index)) {
        return false;
    }
    return (set.bits & (1u << index)) != 0u;
}

bool sutr_sucm_clause_set_contains(sucm_clause_set_t set,
                                   sucm_clause_set_t other)
{
    return (set.bits & other.bits) == other.bits;
}

uint32_t sutr_sucm_clause_set_count(sucm_clause_set_t set)
{
    uint32_t bits = set.bits;
    uint32_t count = 0u;

    while (bits != 0u) {
        bits &= bits - 1u;
        count++;
    }
    return count;
}

/*
 * The clause list of a claim row (section 8). The grammar accepts the
 * full identifier (`SUCM-C1`), the kind-qualified shorthand (`C1`), and
 * the range shorthand (`R5-R8`, `R5-C8`, `SUCM-R5-R8`). A range whose
 * endpoints are not registered clauses is rejected, so a range can never
 * smuggle an unregistered identifier into a claim.
 */
bool sutr_sucm_clause_list_parse(const char *list, sucm_clause_set_t *out)
{
    sucm_clause_set_t set;
    size_t index = 0u;

    set.bits = 0u;
    if (list == NULL || out == NULL) {
        return false;
    }

    while (list[index] != '\0') {
        sucm_clause_kind_t kind;
        uint32_t first = 0u;
        uint32_t last  = 0u;
        uint32_t number;
        size_t digits = 0u;
        size_t j;

        /* Skip the separators and the surrounding space. */
        while (list[index] == ' ' || list[index] == '\t' ||
               list[index] == ',' || list[index] == ';') {
            index++;
        }
        if (list[index] == '\0') {
            break;
        }

        /* The optional "SUCM-" prefix. */
        if (list[index] == 'S' || list[index] == 's') {
            size_t remaining = 0u;
            char probe[6];

            while (list[index + remaining] != '\0') {
                remaining++;
            }
            if (remaining >= 5u) {
                for (j = 0u; j < 5u; j++) {
                    probe[j] = list[index + j];
                }
                probe[5] = '\0';
                if (sucm_streq_ci("SUCM-", probe)) {
                    index += 5u;
                }
            }
        }

        /* The kind letter. */
        if (list[index] == 'C' || list[index] == 'c') {
            kind = SUCM_CLAUSE_CONFORMANCE;
        } else if (list[index] == 'D' || list[index] == 'd') {
            kind = SUCM_CLAUSE_DEFINITION;
        } else if (list[index] == 'R' || list[index] == 'r') {
            kind = SUCM_CLAUSE_REQUIREMENT;
        } else {
            return false;
        }
        index++;

        /* The clause number. */
        while (sucm_is_digit(list[index]) != false) {
            first = first * 10u + (uint32_t)(list[index] - '0');
            index++;
            digits++;
        }
        if (digits == 0u) {
            return false;
        }
        last = first;

        /* The optional range. */
        if (list[index] == '-') {
            index++;
            if (list[index] == 'C' || list[index] == 'c' ||
                list[index] == 'D' || list[index] == 'd' ||
                list[index] == 'R' || list[index] == 'r') {
                if (list[index] == 'C' || list[index] == 'c') {
                    kind = SUCM_CLAUSE_CONFORMANCE;
                } else if (list[index] == 'D' || list[index] == 'd') {
                    kind = SUCM_CLAUSE_DEFINITION;
                } else {
                    kind = SUCM_CLAUSE_REQUIREMENT;
                }
                index++;
            }
            digits = 0u;
            last   = 0u;
            while (sucm_is_digit(list[index]) != false) {
                last = last * 10u + (uint32_t)(list[index] - '0');
                index++;
                digits++;
            }
            if (digits == 0u) {
                return false;
            }
        }

        if (last < first) {
            return false;
        }
        for (number = first; number <= last; number++) {
            uint32_t bit = sucm_clause_bit(kind, number);

            if (bit == 0u) {
                /* An unregistered clause: the whole list is rejected. */
                return false;
            }
            set.bits |= bit;
        }

        /* The token must end here. */
        while (list[index] == ' ' || list[index] == '\t') {
            index++;
        }
        if (list[index] != '\0' && list[index] != ',' && list[index] != ';') {
            return false;
        }
    }

    *out = set;
    return true;
}

/* -- The claim identifier of section 2.3 and the rules of 5.4 ------- */

/*
 * SUCM-R1: the unit component is built from Latin letters, digits, SPACE
 * and HYPHEN-MINUS, and a word may not begin with a digit.
 *
 * A word is a run of characters delimited by SPACE. HYPHEN-MINUS is an
 * ordinary character inside a word, so `sis-3` is a valid unit whose
 * word begins with a letter, while `3sis` and `sis 3id` are not.
 *
 * Implementation note. The report describes the claim identifier as an
 * "uppercase ASCII string" in section 2.3 while every example in the
 * report - `L1/sucf/0.1.0`, `L0/sis/0.1.0`, and the claim row of section
 * 8 - writes the unit in lower case. The identifier grammar therefore
 * accepts letters in either case; the tier component is the one part
 * every example writes in upper case, and it is matched
 * case-insensitively for the same reason.
 */
bool sutr_sucm_claim_id_unit_is_valid(const char *unit)
{
    size_t length;
    size_t i;
    bool at_word_start = true;

    if (unit == NULL) {
        return false;
    }
    length = sucm_strlen(unit);
    if (length == 0u || length >= SUCM_MAX_UNIT) {
        return false;
    }
    for (i = 0u; i < length; i++) {
        char c = unit[i];

        if (c == ' ') {
            /* A SPACE ends the current word. */
            at_word_start = true;
            continue;
        }
        if (c == '-') {
            /* A HYPHEN-MINUS is an ordinary character inside a word. */
            continue;
        }
        if (sucm_is_alpha(c) != false) {
            at_word_start = false;
            continue;
        }
        if (sucm_is_digit(c) != false) {
            if (at_word_start != false) {
                /* R1: a digit may not come first in a word. */
                return false;
            }
            continue;
        }
        return false;
    }
    return true;
}

/*
 * The version component.
 *
 * Implementation note. SUCM-R1 enumerates the alphabet of "the unit and
 * version components", but the only version forms the report exhibits are
 * the dotted-numeric ones (`0.1.0`), and the dot is not in R1's
 * alphabet. The dotted-numeric form is therefore taken as the canonical
 * shape of a version component: one or more decimal groups separated by
 * single dots.
 */
bool sutr_sucm_claim_id_version_is_valid(const char *version)
{
    size_t length;
    size_t i;
    bool digits = false;

    if (version == NULL) {
        return false;
    }
    length = sucm_strlen(version);
    if (length == 0u || length >= SUCM_MAX_VERSION) {
        return false;
    }
    for (i = 0u; i < length; i++) {
        char c = version[i];

        if (sucm_is_digit(c) != false) {
            digits = true;
            continue;
        }
        if (c == '.') {
            if (digits == false) {
                /* An empty group. */
                return false;
            }
            digits = false;
            continue;
        }
        return false;
    }
    return digits;
}

/*
 * The claim identifier of section 2.3: `<tier>/<unit>/<version>`. The
 * parser is a single pass over the string and copies into the caller's
 * struct; it allocates nothing.
 */
bool sutr_sucm_claim_id_parse(const char *text, sucm_claim_id_t *out)
{
    size_t length;
    size_t first = 0u;
    size_t second = 0u;
    size_t i;
    char tier_text[8];
    sucm_tier_t tier;

    if (text == NULL || out == NULL) {
        return false;
    }
    length = sucm_strlen(text);

    /* The two separators. */
    for (i = 0u; i < length; i++) {
        if (text[i] == '/') {
            if (first == 0u) {
                first = i + 1u;
            } else {
                second = i + 1u;
                break;
            }
        }
    }
    if (first == 0u || second == 0u || second <= first) {
        return false;
    }
    if (first - 1u >= sizeof(tier_text)) {
        return false;
    }
    if ((second - 1u - first) >= SUCM_MAX_UNIT) {
        return false;
    }
    if ((length - second) >= SUCM_MAX_VERSION) {
        return false;
    }

    for (i = 0u; i < first - 1u; i++) {
        tier_text[i] = text[i];
    }
    tier_text[first - 1u] = '\0';

    tier = sutr_sucm_tier_from_name(tier_text);
    if (tier == SUCM_TIER_COUNT) {
        return false;
    }

    out->tier   = tier;
    out->valid  = 1u;
    out->unit[0]    = '\0';
    out->version[0] = '\0';
    for (i = first; i < second - 1u; i++) {
        out->unit[i - first] = text[i];
    }
    out->unit[second - 1u - first] = '\0';
    for (i = second; i < length; i++) {
        out->version[i - second] = text[i];
    }
    out->version[length - second] = '\0';

    if (out->unit[0] == '\0' || out->version[0] == '\0') {
        return false;
    }
    if (sutr_sucm_claim_id_unit_is_valid(out->unit) == false) {
        return false;
    }
    if (sutr_sucm_claim_id_version_is_valid(out->version) == false) {
        return false;
    }
    return true;
}

bool sutr_sucm_claim_id_is_valid(const char *text)
{
    sucm_claim_id_t id;

    if (sutr_sucm_claim_id_parse(text, &id) == false) {
        return false;
    }
    return id.valid != 0u;
}

bool sutr_sucm_claim_id_build(sucm_tier_t tier, const char *unit,
                              const char *version, char *out, size_t capacity)
{
    const char *tier_name;

    if (out == NULL || capacity == 0u) {
        return false;
    }
    if (sutr_sucm_tier_is_valid(tier) == false) {
        return false;
    }
    if (sutr_sucm_claim_id_unit_is_valid(unit) == false) {
        return false;
    }
    if (sutr_sucm_claim_id_version_is_valid(version) == false) {
        return false;
    }
    tier_name = sutr_sucm_tier_name(tier);
    if (sucm_strlen(tier_name) + 1u + sucm_strlen(unit) + 1u +
        sucm_strlen(version) + 1u > capacity) {
        return false;
    }
    out[0] = '\0';
    sucm_copy(out, capacity, tier_name);
    {
        size_t at = sucm_strlen(out);
        out[at] = '/';
        out[at + 1u] = '\0';
    }
    {
        size_t at = sucm_strlen(out);
        sucm_copy(out + at, capacity - at, unit);
    }
    {
        size_t at = sucm_strlen(out);
        out[at] = '/';
        out[at + 1u] = '\0';
    }
    {
        size_t at = sucm_strlen(out);
        sucm_copy(out + at, capacity - at, version);
    }
    return true;
}

/*
 * SUCM-R3: two claim identifiers are the same if they are equal after
 * SPACE and medial HYPHEN-MINUS are ignored and the words LEVEL, TIER
 * and CORE are ignored.
 *
 * The key is built per component, so the tuple structure of section 2.3
 * is preserved: a component that reduces to nothing compares equal only
 * to another empty component, and a separator is never swallowed by a
 * neighbouring word.
 *
 * Within a component the atoms are the maximal runs of letters and
 * digits and the single characters between them. The words LEVEL, TIER
 * and CORE are matched as whole atoms - a maximal run - so `SCORE` is not
 * the word `CORE` - and they are matched without regard to case, because
 * sections 2.3 and 5.4 exhibit identifiers in both cases. A HYPHEN-MINUS
 * that touches one of those words goes with the word. A HYPHEN-MINUS that
 * is neither the first nor the last surviving atom is medial and is
 * dropped, so `s-u-c-f` and `sucf` agree while `-sucf` and `sucf-` do
 * not.
 */
#define SUCM_MAX_ATOMS 64U

typedef struct {
    uint16_t offset;
    uint16_t length;
    uint8_t  word;   /* 1 when the atom is LEVEL, TIER, or CORE */
    uint8_t  drop;   /* 1 when the atom does not survive */
} sucm_atom_t;

static bool sucm_normalize_component(const char *text, size_t length,
                                      char *out, size_t capacity)
{
    sucm_atom_t atoms[SUCM_MAX_ATOMS];
    uint32_t count = 0u;
    size_t index = 0u;
    size_t written = 0u;
    uint32_t k;

    while (index < length) {
        if (text[index] == ' ' || text[index] == '\t') {
            index++;
            continue;
        }
        if (count >= SUCM_MAX_ATOMS) {
            return false;
        }
        if (sucm_is_alnum(text[index]) != false) {
            size_t start = index;

            while (index < length && sucm_is_alnum(text[index]) != false) {
                index++;
            }
            atoms[count].offset = (uint16_t)start;
            atoms[count].length = (uint16_t)(index - start);
            atoms[count].word   = 0u;
            atoms[count].drop   = 0u;
            if (atoms[count].length == 5u &&
                sucm_streq_ci_n("LEVEL", text + start, 5u) == true) {
                atoms[count].word = 1u;
            } else if (atoms[count].length == 4u &&
                       sucm_streq_ci_n("TIER", text + start, 4u) == true) {
                atoms[count].word = 1u;
            } else if (atoms[count].length == 4u &&
                       sucm_streq_ci_n("CORE", text + start, 4u) == true) {
                atoms[count].word = 1u;
            }
            count++;
            continue;
        }
        atoms[count].offset = (uint16_t)index;
        atoms[count].length = 1u;
        atoms[count].word   = 0u;
        atoms[count].drop   = 0u;
        count++;
        index++;
    }

    /* The words go, and so does the hyphen that touches them. */
    for (k = 0u; k < count; k++) {
        if (atoms[k].word == 0u) {
            continue;
        }
        atoms[k].drop = 1u;
        if (k > 0u && atoms[k - 1u].length == 1u &&
            text[atoms[k - 1u].offset] == '-') {
            atoms[k - 1u].drop = 1u;
        }
        if (k + 1u < count && atoms[k + 1u].length == 1u &&
            text[atoms[k + 1u].offset] == '-') {
            atoms[k + 1u].drop = 1u;
        }
    }

    /* The medial hyphens go. */
    {
        uint32_t first = 0u;
        uint32_t last  = 0u;
        uint8_t  seen  = 0u;

        for (k = 0u; k < count; k++) {
            if (atoms[k].drop != 0u) {
                continue;
            }
            if (seen == 0u) {
                first = k;
            }
            last = k;
            seen  = 1u;
        }
        if (seen != 0u) {
            for (k = first + 1u; k < last; k++) {
                if (atoms[k].length == 1u &&
                    text[atoms[k].offset] == '-') {
                    atoms[k].drop = 1u;
                }
            }
        }
    }

    for (k = 0u; k < count; k++) {
        uint16_t j;

        if (atoms[k].drop != 0u) {
            continue;
        }
        if (written + atoms[k].length + 1u > capacity) {
            return false;
        }
        for (j = 0u; j < atoms[k].length; j++) {
            out[written] = text[atoms[k].offset + j];
            written++;
        }
    }
    out[written] = '\0';
    return true;
}

bool sutr_sucm_claim_id_normalize(const char *text, char *out, size_t capacity)
{
    size_t length;
    size_t index;
    size_t start = 0u;
    size_t written = 0u;
    char component[SUCM_MAX_UNIT * 2u];

    if (text == NULL || out == NULL || capacity == 0u) {
        return false;
    }
    length = sucm_strlen(text);
    if (length > 128u) {
        return false;
    }
    out[0] = '\0';

    for (index = 0u; index <= length; index++) {
        size_t at;

        if (index != length && text[index] != '/') {
            continue;
        }
        if (sucm_normalize_component(text + start, index - start, component,
                                      sizeof(component)) == false) {
            return false;
        }
        at = sucm_strlen(component);
        if (written + at + 2u > capacity) {
            return false;
        }
        {
            size_t j;

            for (j = 0u; j < at; j++) {
                out[written] = component[j];
                written++;
            }
        }
        if (index != length) {
            out[written] = '/';
            written++;
        }
        out[written] = '\0';
        start = index + 1u;
    }
    return true;
}

bool sutr_sucm_claim_id_equal(const char *left, const char *right)
{
    char left_key[160];
    char right_key[160];

    if (sutr_sucm_claim_id_normalize(left, left_key, sizeof(left_key)) ==
        false) {
        return false;
    }
    if (sutr_sucm_claim_id_normalize(right, right_key, sizeof(right_key)) ==
        false) {
        return false;
    }
    return sucm_streq(left_key, right_key);
}

/* -- The gate records of section 7 --------------------------------- */

const char *sutr_sucm_gate_name(sucm_gate_t gate)
{
    switch (gate) {
        case SUCM_GATE_TEST:  return "ctest";
        case SUCM_GATE_DOCS:  return "verify-docs";
        case SUCM_GATE_BUILD: return "build";
        default:              return "unknown";
    }
}

sucm_gate_t sutr_sucm_gate_from_name(const char *name)
{
    if (sucm_streq(name, "ctest")) {
        return SUCM_GATE_TEST;
    }
    if (sucm_streq(name, "verify-docs")) {
        return SUCM_GATE_DOCS;
    }
    if (sucm_streq(name, "build")) {
        return SUCM_GATE_BUILD;
    }
    return SUCM_GATE_COUNT;
}

const char *sutr_sucm_result_name(sucm_result_t result)
{
    switch (result) {
        case SUCM_RESULT_GREEN: return "green";
        case SUCM_RESULT_RED:   return "red";
        case SUCM_RESULT_UNKNOWN:
        default:                return "unknown";
    }
}

sucm_result_t sutr_sucm_result_from_name(const char *name)
{
    if (sucm_streq(name, "green")) {
        return SUCM_RESULT_GREEN;
    }
    if (sucm_streq(name, "red")) {
        return SUCM_RESULT_RED;
    }
    return SUCM_RESULT_UNKNOWN;
}

/*
 * The compact gate record of section 8: `<harness>:<result>`, for example
 * `ctest:green`. An unknown harness is rejected: the gate set is closed,
 * and a claim that names a gate the model does not define cannot be
 * backed by it.
 */
bool sutr_sucm_gate_record_parse(const char *text, sucm_gate_record_t *out)
{
    size_t length;
    size_t i;
    sucm_gate_t gate;
    sucm_result_t result;
    char harness[SUCM_MAX_HARNESS];

    if (text == NULL || out == NULL) {
        return false;
    }
    length = sucm_strlen(text);
    if (length == 0u || length >= SUCM_MAX_GATE) {
        return false;
    }

    for (i = 0u; i < length; i++) {
        if (text[i] == ':') {
            break;
        }
    }
    if (i == 0u || i >= length) {
        return false;
    }
    if (i >= sizeof(harness)) {
        return false;
    }
    for (length = 0u; length < i; length++) {
        harness[length] = text[length];
    }
    harness[i] = '\0';

    gate = sutr_sucm_gate_from_name(harness);
    if (gate == SUCM_GATE_COUNT) {
        return false;
    }
    result = sutr_sucm_result_from_name(text + i + 1u);
    if (result == SUCM_RESULT_UNKNOWN) {
        return false;
    }

    out->gate        = gate;
    out->result      = result;
    out->suite_count = 0u;
    out->harness[0]  = '\0';
    out->version[0]  = '\0';
    for (length = 0u; length < i; length++) {
        out->harness[length] = harness[length];
    }
    out->harness[i] = '\0';
    return true;
}

bool sutr_sucm_gate_record_is_valid(const sucm_gate_record_t *record)
{
    if (record == NULL) {
        return false;
    }
    if (record->gate >= SUCM_GATE_COUNT) {
        return false;
    }
    if (record->result != SUCM_RESULT_GREEN &&
        record->result != SUCM_RESULT_RED) {
        return false;
    }
    if (record->harness[0] == '\0') {
        return false;
    }
    return true;
}

/*
 * A gate record backs a claim only when it passed (section 5.1 C6,
 * section 6 R15). A red record is a valid record and an invalid claim.
 */
bool sutr_sucm_gate_record_passes(const sucm_gate_record_t *record)
{
    if (sutr_sucm_gate_record_is_valid(record) == false) {
        return false;
    }
    return record->result == SUCM_RESULT_GREEN;
}

bool sutr_sucm_gate_record_add_suite(sucm_gate_record_t *record,
                                    const char *suite)
{
    size_t length;

    if (record == NULL || suite == NULL) {
        return false;
    }
    if (record->suite_count >= SUCM_MAX_SUITES) {
        return false;
    }
    length = sucm_strlen(suite);
    if (length == 0u || length >= SUCM_MAX_SUITE) {
        return false;
    }
    sucm_copy(record->suites[record->suite_count], SUCM_MAX_SUITE, suite);
    record->suite_count++;
    return true;
}

uint32_t sutr_sucm_gate_record_suite_count(const sucm_gate_record_t *record)
{
    if (record == NULL) {
        return 0u;
    }
    return record->suite_count;
}

const char *sutr_sucm_gate_record_suite_at(const sucm_gate_record_t *record,
                                           uint32_t index)
{
    if (record == NULL || index >= record->suite_count) {
        return NULL;
    }
    return record->suites[index];
}

/* -- The claim rows of section 8 ----------------------------------- */

/*
 * The published claims. Every row is `tier;unit;version;clauses;gate`.
 *
 * The Level-0 clause column is the one the report itself publishes in
 * section 8; the Level-1 and Level-2 columns extend it with the bridge
 * and the plugin clauses of sections 3.2 and 3.3. The gate column is the
 * compact gate record of section 7.
 */
#define SUCM_CLAUSES_L0 "SUCM-C1,SUCM-C2,SUCM-C3,R5-R8,R13-R14"
#define SUCM_CLAUSES_L1 "SUCM-C1,SUCM-C2,SUCM-C3,SUCM-C4,R5-R8,R9,R13-R14"
#define SUCM_CLAUSES_L2 "SUCM-C1,SUCM-C2,SUCM-C3,SUCM-C4,SUCM-C5,R5-R8,R9-R12,R13-R14"
#define SUCM_GATE_GREEN "ctest:green"

static const sucm_claim_t g_sucm_claims[SUCM_CLAIM_COUNT] = {
    { SUCM_TIER_L0, "sis",     "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "sdf",     "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "sgw",     "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "sbr",     "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "ssp",     "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "sts",     "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "sncs",    "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "sip",     "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "ssre",    "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "scms",    "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "sucd",    "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "sisr",    "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "suvtl",   "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "suamr",   "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "suca",    "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "sre",     "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "sldf",    "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "sivd",    "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "scss",    "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "sidn",    "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "sueps",   "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "susch",   "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "sucpm",   "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "sumnls",  "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "sucm",    "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L0, "sums",    "0.1.0", SUCM_CLAUSES_L0, SUCM_GATE_GREEN },
    { SUCM_TIER_L1, "sucf",    "0.1.0", SUCM_CLAUSES_L1, SUCM_GATE_GREEN },
    { SUCM_TIER_L1, "sucem",   "0.1.0", SUCM_CLAUSES_L1, SUCM_GATE_GREEN },
    { SUCM_TIER_L2, "plugin",  "0.1.0", SUCM_CLAUSES_L2, SUCM_GATE_GREEN }
};

uint32_t sutr_sucm_claim_count(void)
{
    return SUCM_CLAIM_COUNT;
}

bool sutr_sucm_claim_at(uint32_t index, sucm_claim_t *out)
{
    if (out == NULL || index >= SUCM_CLAIM_COUNT) {
        return false;
    }
    *out = g_sucm_claims[index];
    return true;
}

/*
 * SUCM-R2: only one claim is published per `(tier, unit, version)`, so a
 * lookup is unique.
 */
bool sutr_sucm_claim_find(sucm_tier_t tier, const char *unit,
                          const char *version, sucm_claim_t *out)
{
    uint32_t i;

    if (unit == NULL || version == NULL) {
        return false;
    }
    for (i = 0u; i < SUCM_CLAIM_COUNT; i++) {
        if (g_sucm_claims[i].tier != tier) {
            continue;
        }
        if (sucm_streq(g_sucm_claims[i].unit, unit) == false) {
            continue;
        }
        if (sucm_streq(g_sucm_claims[i].version, version) == false) {
            continue;
        }
        if (out != NULL) {
            *out = g_sucm_claims[i];
        }
        return true;
    }
    return false;
}

/*
 * The claim row of section 8: five semicolon-separated fields. The parser
 * is bounded and allocates nothing; the fields are copied into the
 * caller's struct.
 */
bool sutr_sucm_claim_row_parse(const char *row, sucm_claim_t *out)
{
    size_t length;
    size_t field_start = 0u;
    size_t field = 0u;
    char tier_text[8];
    sucm_tier_t tier;

    if (row == NULL || out == NULL) {
        return false;
    }
    length = sucm_strlen(row);
    if (length == 0u) {
        return false;
    }

    out->unit[0]    = '\0';
    out->version[0] = '\0';
    out->clauses[0] = '\0';
    out->gate[0]    = '\0';

    while (field <= 4u) {
        size_t i;

        for (i = field_start; i < length && row[i] != ';'; i++) {
            /* find the end of the field */
        }
        switch (field) {
            case 0u:
                if (i - field_start >= sizeof(tier_text)) {
                    return false;
                }
                {
                    size_t j;

                    for (j = 0u; j < i - field_start; j++) {
                        tier_text[j] = row[field_start + j];
                    }
                    tier_text[i - field_start] = '\0';
                }
                tier = sutr_sucm_tier_from_name(tier_text);
                if (tier == SUCM_TIER_COUNT) {
                    return false;
                }
                out->tier = tier;
                break;
            case 1u:
                if (i - field_start >= SUCM_MAX_UNIT) {
                    return false;
                }
                {
                    size_t j;

                    for (j = 0u; j < i - field_start; j++) {
                        out->unit[j] = row[field_start + j];
                    }
                    out->unit[i - field_start] = '\0';
                }
                break;
            case 2u:
                if (i - field_start >= SUCM_MAX_VERSION) {
                    return false;
                }
                {
                    size_t j;

                    for (j = 0u; j < i - field_start; j++) {
                        out->version[j] = row[field_start + j];
                    }
                    out->version[i - field_start] = '\0';
                }
                break;
            case 3u:
                if (i - field_start >= SUCM_MAX_CLAUSE_LIST) {
                    return false;
                }
                {
                    size_t j;

                    for (j = 0u; j < i - field_start; j++) {
                        out->clauses[j] = row[field_start + j];
                    }
                    out->clauses[i - field_start] = '\0';
                }
                break;
            default:
                if (i - field_start >= SUCM_MAX_GATE) {
                    return false;
                }
                {
                    size_t j;

                    for (j = 0u; j < i - field_start; j++) {
                        out->gate[j] = row[field_start + j];
                    }
                    out->gate[i - field_start] = '\0';
                }
                break;
        }
        if (i >= length) {
            break;
        }
        field_start = i + 1u;
        field++;
    }
    return field == 4u;
}

bool sutr_sucm_claim_row_format(const sucm_claim_t *claim, char *out,
                                size_t capacity)
{
    size_t needed;

    if (claim == NULL || out == NULL || capacity == 0u) {
        return false;
    }
    needed = sucm_strlen(sutr_sucm_tier_name(claim->tier)) + 1u +
             sucm_strlen(claim->unit) + 1u +
             sucm_strlen(claim->version) + 1u +
             sucm_strlen(claim->clauses) + 1u +
             sucm_strlen(claim->gate) + 1u;
    if (needed > capacity) {
        return false;
    }
    out[0] = '\0';
    sucm_copy(out, capacity, sutr_sucm_tier_name(claim->tier));
    {
        size_t at = sucm_strlen(out);
        out[at] = ';';
        out[at + 1u] = '\0';
    }
    {
        size_t at = sucm_strlen(out);
        sucm_copy(out + at, capacity - at, claim->unit);
        at = sucm_strlen(out);
        out[at] = ';';
        out[at + 1u] = '\0';
    }
    {
        size_t at = sucm_strlen(out);
        sucm_copy(out + at, capacity - at, claim->version);
        at = sucm_strlen(out);
        out[at] = ';';
        out[at + 1u] = '\0';
    }
    {
        size_t at = sucm_strlen(out);
        sucm_copy(out + at, capacity - at, claim->clauses);
        at = sucm_strlen(out);
        out[at] = ';';
        out[at + 1u] = '\0';
    }
    {
        size_t at = sucm_strlen(out);
        sucm_copy(out + at, capacity - at, claim->gate);
    }
    return true;
}

/*
 * A claim is valid when it names a tier it can hold, when it satisfies
 * every clause of that tier and of every lower tier (section 3.4), and
 * when it is backed by a passing gate record (section 5.1 C6, section 6
 * R15).
 */
bool sutr_sucm_claim_is_valid(const sucm_claim_t *claim)
{
    sucm_clause_set_t set;
    sucm_gate_record_t record;

    if (claim == NULL) {
        return false;
    }
    if (sutr_sucm_tier_is_valid(claim->tier) == false) {
        return false;
    }
    if (sutr_sucm_claim_id_unit_is_valid(claim->unit) == false) {
        return false;
    }
    if (sutr_sucm_claim_id_version_is_valid(claim->version) == false) {
        return false;
    }
    if (sutr_sucm_clause_list_parse(claim->clauses, &set) == false) {
        return false;
    }
    if (sutr_sucm_clause_set_contains(set,
            sutr_sucm_tier_clause_set(claim->tier)) == false) {
        return false;
    }
    if (sutr_sucm_gate_record_parse(claim->gate, &record) == false) {
        return false;
    }
    return sutr_sucm_gate_record_passes(&record);
}

/* -- The core engine registry of section 8 ------------------------- */

/*
 * The conformance units of section 1.3.1: the fifteen SUAS engines, the
 * eight SUTS engines, the five SUTR reference implementations, and the
 * plugin lifecycle of SUTR-5. Each row names the module, the unit
 * component of its claim identifier (SUCM-R4), the owning specification,
 * and the memory and time classes the unit declares (section 4.2).
 */
static const sucm_engine_t g_sucm_engines[SUCM_ENGINE_COUNT] = {
    { "suas_sdf",   "sdf",     "SUAS-001", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "suas_sgw",   "sgw",     "SUAS-002", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "suas_sbr",   "sbr",     "SUAS-003", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "suas_sucf",  "sucf",    "SUAS-004", "O(1) stack", "O(1) per unit", SUCM_TIER_L1 },
    { "suas_ssp",   "ssp",     "SUAS-005", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "suas_sts",   "sts",     "SUAS-006", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "suas_sis",   "sis",     "SUAS-007", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "suas_sncs",  "sncs",    "SUAS-008", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "suas_sip",   "sip",     "SUAS-009", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "suas_ssre",  "ssre",    "SUAS-010", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "suas_scms",  "scms",    "SUAS-011", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "suas_sucd",  "sucd",    "SUAS-012", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "suas_sisr",  "sisr",    "SUAS-013", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "suas_suvtl", "suvtl",   "SUAS-014", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "suas_suamr", "suamr",   "SUAS-015", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "suts_suca",  "suca",    "SUTS-001", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "suts_sre",   "sre",     "SUTS-002", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "suts_sldf",  "sldf",    "SUTS-003", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "suts_sivd",  "sivd",    "SUTS-004", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "suts_scss",  "scss",    "SUTS-005", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "suts_sidn",  "sidn",    "SUTS-006", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "suts_sueps", "sueps",   "SUTS-007", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "suts_susch", "susch",   "SUTS-008", "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "sutr_sucem", "sucem",   "SUTR-7",   "O(1) stack", "O(1) per unit", SUCM_TIER_L1 },
    { "sutr_sucpm", "sucpm",   "SUTR-8",   "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "sutr_sumnls","sumnls",  "SUTR-9",   "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "sutr_sucm",  "sucm",    "SUTR-10",  "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "sutr_sums",  "sums",    "SUTR-11",  "O(1) stack", "O(1) per unit", SUCM_TIER_L0 },
    { "sucs_plugin","plugin",  "SUTR-5",   "O(1) stack", "O(1) per unit", SUCM_TIER_L2 }
};

uint32_t sutr_sucm_engine_count(void)
{
    return SUCM_ENGINE_COUNT;
}

bool sutr_sucm_engine_at(uint32_t index, sucm_engine_t *out)
{
    if (out == NULL || index >= SUCM_ENGINE_COUNT) {
        return false;
    }
    *out = g_sucm_engines[index];
    return true;
}

bool sutr_sucm_engine_by_module(const char *module, sucm_engine_t *out)
{
    uint32_t i;

    if (module == NULL) {
        return false;
    }
    for (i = 0u; i < SUCM_ENGINE_COUNT; i++) {
        if (sucm_streq(g_sucm_engines[i].module, module) != false) {
            if (out != NULL) {
                *out = g_sucm_engines[i];
            }
            return true;
        }
    }
    return false;
}

bool sutr_sucm_engine_by_unit(const char *unit, sucm_engine_t *out)
{
    uint32_t i;

    if (unit == NULL) {
        return false;
    }
    for (i = 0u; i < SUCM_ENGINE_COUNT; i++) {
        if (sucm_streq(g_sucm_engines[i].unit, unit) != false) {
            if (out != NULL) {
                *out = g_sucm_engines[i];
            }
            return true;
        }
    }
    return false;
}

/* -- The resource gates of sections 3.2, 3.3 and 6 ----------------- */

/*
 * The Level-1 bridge gate of section 3.2 and SUCM-R9: the bridge range is
 * `0x000000-0x10FFFF`. A codepoint above it is Native, never a Bridge
 * character.
 */
bool sutr_sucm_bridge_gate_allows(uint64_t codepoint)
{
    return codepoint <= SUCM_BRIDGE_MAX;
}

/*
 * The Level-2 native-plane gate of section 3.3 and SUCM-R10: plugin
 * codepoints are allocated only in `0x00120000-0x7FFFFFEF` or strictly
 * above `0x7FFFFFFF`. The trap slots `0x7FFFFFF0-0x7FFFFFFE` and the
 * Sentinel `0x7FFFFFFF` are unallocatable, and the gate fails closed.
 */
bool sutr_sucm_native_gate_allows(uint64_t codepoint)
{
    if (codepoint >= SUCM_NATIVE_MIN && codepoint <= SUCM_NATIVE_MAX) {
        return true;
    }
    if (codepoint > SUCM_SENTINEL) {
        return true;
    }
    return false;
}

/*
 * The neography carve-out of section 3.3 and SUCM-R12: PUA neography
 * resolves to `0x00160000-0x0016FFFF`.
 */
bool sutr_sucm_neography_gate_allows(uint64_t codepoint)
{
    return codepoint >= SUCM_NEOGRAPHY_MIN &&
           codepoint <= SUCM_NEOGRAPHY_MAX;
}

bool sutr_sucm_is_trap(uint64_t codepoint)
{
    return codepoint >= SUCM_TRAP_SLOT_LOW &&
           codepoint <= SUCM_TRAP_SLOT_HIGH;
}

bool sutr_sucm_is_sentinel(uint64_t codepoint)
{
    return codepoint == SUCM_SENTINEL;
}
