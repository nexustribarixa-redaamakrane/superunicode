/* SUTS-011 - SuperUnicode Set Notation (SUSN)
 *
 * Reference implementation of docs/suts/SUTS-011-susn.md.
 *
 * Freestanding C99, zero allocation. A set is a descriptor plus a static or
 * caller-owned array of inclusive ranges. Membership is a bounded binary
 * search whose depth is a compile-time constant of the image; set algebra is
 * an event sweep over the two operand arrays that writes canonical ranges to
 * caller-owned storage. Nothing in this file grows a table, allocates a node,
 * parses an expression string, or calls into libc.
 *
 * UTS #61 is the model, not the target: it defines a textual notation and a
 * parser. SUSN defines the compiled destination of that notation - the
 * inversion lists and range arrays a kernel can probe in O(1) - and the
 * build-time algebra that produces them.
 */

#include "suts/suts_susn.h"

/* ── Compile-time geometry checks ──────────────────────────────────── */

#define SUSN_STATIC_ASSERT(cond, name) \
    typedef char name[(cond) ? 1 : -1]

SUSN_STATIC_ASSERT(sizeof(susn_codepoint_range_t) == 8u,
                   susn_range_is_8_bytes);
SUSN_STATIC_ASSERT(offsetof(susn_codepoint_range_t, first) == 0u,
                   susn_range_first_at_0);
SUSN_STATIC_ASSERT(offsetof(susn_codepoint_range_t, last) == 4u,
                   susn_range_last_at_4);

SUSN_STATIC_ASSERT(offsetof(susn_set_descriptor_t, set_id) == 0u,
                   susn_set_id_at_0);
SUSN_STATIC_ASSERT(offsetof(susn_set_descriptor_t, set_flags) == 4u,
                   susn_set_flags_at_4);
SUSN_STATIC_ASSERT(offsetof(susn_set_descriptor_t, range_count) == 8u,
                   susn_range_count_at_8);
SUSN_STATIC_ASSERT(offsetof(susn_set_descriptor_t, ranges) >= 12u,
                   susn_ranges_after_counts);
SUSN_STATIC_ASSERT(sizeof(susn_set_descriptor_t) >=
                   offsetof(susn_set_descriptor_t, ranges) + sizeof(void *),
                   susn_descriptor_holds_pointer);

SUSN_STATIC_ASSERT(offsetof(susn_builder_t, flags) == 0u,
                   susn_builder_flags_at_0);
SUSN_STATIC_ASSERT(offsetof(susn_builder_t, count) == 4u,
                   susn_builder_count_at_4);
SUSN_STATIC_ASSERT(sizeof(susn_builder_t) >=
                   2u * sizeof(uint32_t) +
                   SUSN_BUILDER_CAPACITY * sizeof(susn_codepoint_range_t),
                   susn_builder_storage_is_inline);

/* ── Compiled catalog ──────────────────────────────────────────────── */

#include "suts_susn_data.inc"

SUSN_STATIC_ASSERT(SUSN_CATALOG_COUNT <= SUSN_CATALOG_CAPACITY,
                   susn_catalog_fits_capacity);
SUSN_STATIC_ASSERT(SUSN_RNUR_SET_SIZE == (1u << SUSN_RNUR_SET_SHIFT),
                   susn_rnur_block_is_256);

/* The sweep walks the half-open 31-bit universe with an explicit end marker. */
#define SUSN_END 0x80000000UL

/* ── Small helpers ─────────────────────────────────────────────────── */

static bool susn_range_valid(uint32_t first, uint32_t last)
{
    return first <= last && last <= SUSN_UNIVERSE_MAX;
}

static bool susn_operand_ok(const susn_set_descriptor_t *set)
{
    return set == NULL ||
           (set->range_count == 0u || set->ranges != NULL);
}

static bool susn_streq(const char *a, const char *b)
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

/* ── Membership ────────────────────────────────────────────────────── */

static bool susn_contains_raw(const susn_set_descriptor_t *set, uint32_t cp)
{
    uint32_t lo = 0u;
    uint32_t hi;

    if (set == NULL || set->range_count == 0u) {
        return false;
    }
    if (set->ranges == NULL || cp > SUSN_UNIVERSE_MAX) {
        return false;
    }

    hi = set->range_count;
    while (lo < hi) {
        uint32_t mid = lo + ((hi - lo) / 2u);
        const susn_codepoint_range_t *range = &set->ranges[mid];

        if (cp < range->first) {
            hi = mid;
        } else if (cp > range->last) {
            lo = mid + 1u;
        } else {
            return true;
        }
    }
    return false;
}

bool suts_susn_contains(const susn_set_descriptor_t *set, uint32_t codepoint)
{
    bool member;

    if (set == NULL || codepoint > SUSN_UNIVERSE_MAX) {
        return false;
    }
    if (set->range_count > 0u && set->ranges == NULL) {
        return false;
    }

    member = susn_contains_raw(set, codepoint);
    if ((set->set_flags & SUSN_FLAG_NEGATED) != 0u) {
        member = !member;
    }
    return member;
}

bool suts_susn_contains_in(const susn_set_descriptor_t *set, uint32_t codepoint,
                           uint32_t filter_mask)
{
    uint32_t bit;

    if (set == NULL || codepoint > SUSN_UNIVERSE_MAX) {
        return false;
    }
    bit = suts_susn_zone_filter(suts_susn_zone_of(codepoint));
    if (bit == 0u || (filter_mask & bit) == 0u) {
        return false;
    }
    return suts_susn_contains(set, codepoint);
}

uint64_t suts_susn_cardinality(const susn_set_descriptor_t *set)
{
    uint64_t total = 0u;
    uint32_t i;

    if (set == NULL) {
        return 0u;
    }
    if (set->range_count > 0u && set->ranges == NULL) {
        return 0u;
    }
    for (i = 0u; i < set->range_count; i++) {
        const susn_codepoint_range_t *range = &set->ranges[i];
        if (!susn_range_valid(range->first, range->last)) {
            return 0u;
        }
        total += ((uint64_t)range->last - (uint64_t)range->first) + 1u;
    }
    if ((set->set_flags & SUSN_FLAG_NEGATED) != 0u) {
        total = SUSN_UNIVERSE_SIZE - total;
    }
    return total;
}

/* ── Validation and canonicalization ───────────────────────────────── */

bool suts_susn_validate(const susn_set_descriptor_t *set)
{
    uint32_t i;

    if (set == NULL) {
        return false;
    }
    if ((set->set_flags & ~SUSN_FLAG_MASK_DEFINED) != 0u) {
        return false;
    }
    if (set->range_count > 0u && set->ranges == NULL) {
        return false;
    }
    for (i = 0u; i < set->range_count; i++) {
        const susn_codepoint_range_t *range = &set->ranges[i];

        if (!susn_range_valid(range->first, range->last)) {
            return false;
        }
        if (i > 0u) {
            const susn_codepoint_range_t *prev = &set->ranges[i - 1u];
            if ((uint64_t)range->first <= (uint64_t)prev->last + 1u) {
                return false; /* overlap or adjacency; not canonical */
            }
        }
    }
    return true;
}

uint32_t suts_susn_normalize(const susn_codepoint_range_t *in, uint32_t in_count,
                             susn_codepoint_range_t *out, uint32_t capacity)
{
    uint32_t i;
    uint32_t merged;

    if (in_count == 0u) {
        return 0u;
    }
    if (in == NULL || out == NULL || capacity < in_count) {
        return 0u;
    }

    for (i = 0u; i < in_count; i++) {
        if (!susn_range_valid(in[i].first, in[i].last)) {
            return 0u;
        }
    }

    /* Copy, then insertion-sort by `first`. The builder and the catalog are
     * already sorted; this path exists for caller-supplied tables. */
    for (i = 0u; i < in_count; i++) {
        out[i] = in[i];
    }
    for (i = 1u; i < in_count; i++) {
        susn_codepoint_range_t key = out[i];
        uint32_t j = i;

        while (j > 0u && out[j - 1u].first > key.first) {
            out[j] = out[j - 1u];
            j--;
        }
        out[j] = key;
    }

    merged = 0u;
    for (i = 0u; i < in_count; i++) {
        if (merged > 0u &&
            (uint64_t)out[i].first <= (uint64_t)out[merged - 1u].last + 1u) {
            if (out[i].last > out[merged - 1u].last) {
                out[merged - 1u].last = out[i].last;
            }
        } else {
            out[merged] = out[i];
            merged++;
        }
    }
    return merged;
}

bool suts_susn_descriptor_init(susn_set_descriptor_t *out, uint32_t set_id,
                               uint32_t flags, uint32_t range_count,
                               const susn_codepoint_range_t *ranges)
{
    if (out == NULL) {
        return false;
    }
    if (range_count > 0u && ranges == NULL) {
        return false;
    }
    out->set_id = set_id;
    out->set_flags = flags;
    out->range_count = range_count;
    out->ranges = ranges;
    return true;
}

/* ── The event sweep ───────────────────────────────────────────────── */
/* Every set operation is one sweep over the boundaries of the two operand
 * range arrays. A boundary is a range start (membership becomes true) or a
 * range end plus one (membership becomes false). Between two consecutive
 * boundaries the raw membership of each operand is constant, so the operator
 * can be applied once per segment. The effective membership of an operand is
 * its raw membership XOR SUSN_FLAG_NEGATED, which is exactly the inversion-list
 * semantics of §3.1. */

typedef struct {
    const susn_codepoint_range_t *ranges;
    uint32_t count;
    uint32_t index;
    bool     inside;
} susn_cursor_t;

typedef struct {
    susn_codepoint_range_t *out;
    uint32_t capacity;
    uint32_t needed;
    uint32_t last_first;
    uint32_t last_last;
    bool     has_last;
} susn_writer_t;

static uint32_t susn_cursor_next(const susn_cursor_t *cursor)
{
    if (cursor->inside) {
        return (uint32_t)((uint64_t)cursor->ranges[cursor->index].last + 1u);
    }
    if (cursor->index < cursor->count) {
        return cursor->ranges[cursor->index].first;
    }
    return (uint32_t)SUSN_END;
}

static bool susn_cursor_advance(susn_cursor_t *cursor, uint32_t next)
{
    bool moved = false;
    bool changed = true;

    while (changed) {
        changed = false;
        if (cursor->inside) {
            if ((uint64_t)cursor->ranges[cursor->index].last + 1u ==
                (uint64_t)next) {
                cursor->inside = false;
                cursor->index++;
                changed = true;
                moved = true;
            }
        } else if (cursor->index < cursor->count &&
                   cursor->ranges[cursor->index].first == next) {
            cursor->inside = true;
            changed = true;
            moved = true;
        }
    }
    return moved;
}

static void susn_writer_init(susn_writer_t *writer,
                             susn_codepoint_range_t *out, uint32_t capacity)
{
    writer->out = out;
    writer->capacity = capacity;
    writer->needed = 0u;
    writer->last_first = 0u;
    writer->last_last = 0u;
    writer->has_last = false;
}

static void susn_writer_emit(susn_writer_t *writer,
                             uint32_t first, uint32_t last)
{
    if (first > last) {
        return;
    }
    if (writer->has_last &&
        (uint64_t)first <= (uint64_t)writer->last_last + 1u) {
        /* Overlapping or adjacent: extend the previous canonical range. */
        if (last > writer->last_last) {
            writer->last_last = last;
            if (writer->out != NULL && writer->needed > 0u &&
                writer->needed <= writer->capacity) {
                writer->out[writer->needed - 1u].last = last;
            }
        }
        return;
    }

    if (writer->out != NULL && writer->needed < writer->capacity) {
        writer->out[writer->needed].first = first;
        writer->out[writer->needed].last = last;
    }
    writer->needed++;
    writer->last_first = first;
    writer->last_last = last;
    writer->has_last = true;
}

static bool susn_op_emits(susn_set_op_t op, bool in_a, bool in_b)
{
    switch (op) {
    case SUSN_OP_UNION:
        return in_a || in_b;
    case SUSN_OP_INTERSECTION:
        return in_a && in_b;
    case SUSN_OP_DIFFERENCE:
        return in_a && !in_b;
    case SUSN_OP_SYMMETRIC_DIFFERENCE:
        return in_a != in_b;
    default:
        return false;
    }
}

static uint32_t susn_sweep(const susn_set_descriptor_t *a,
                           const susn_set_descriptor_t *b,
                           susn_set_op_t op,
                           const susn_codepoint_range_t *spans,
                           uint32_t span_count,
                           susn_codepoint_range_t *out, uint32_t capacity)
{
    susn_cursor_t ca;
    susn_cursor_t cb;
    susn_writer_t writer;
    bool neg_a = (a != NULL) &&
                 ((a->set_flags & SUSN_FLAG_NEGATED) != 0u);
    bool neg_b = (b != NULL) &&
                 ((b->set_flags & SUSN_FLAG_NEGATED) != 0u);
    uint32_t pos = 0u;

    ca.ranges = (a != NULL && a->range_count > 0u && a->ranges != NULL)
                    ? a->ranges : NULL;
    ca.count = (ca.ranges != NULL) ? a->range_count : 0u;
    ca.index = 0u;
    ca.inside = false;

    cb.ranges = (b != NULL && b->range_count > 0u && b->ranges != NULL)
                    ? b->ranges : NULL;
    cb.count = (cb.ranges != NULL) ? b->range_count : 0u;
    cb.index = 0u;
    cb.inside = false;

    susn_writer_init(&writer, out, capacity);

    while (pos < (uint32_t)SUSN_END) {
        uint32_t next = (uint32_t)SUSN_END;
        uint32_t next_a = susn_cursor_next(&ca);
        uint32_t next_b = susn_cursor_next(&cb);

        if (next_a < next) {
            next = next_a;
        }
        if (next_b < next) {
            next = next_b;
        }
        if (next < pos) {
            next = pos; /* defensive: malformed input must not go backwards */
        }

        if (pos < next) {
            bool in_a = (ca.inside != neg_a);
            bool in_b = (cb.inside != neg_b);

            if (susn_op_emits(op, in_a, in_b)) {
                uint32_t segment_last = next - 1u;

                if (spans == NULL || span_count == 0u) {
                    susn_writer_emit(&writer, pos, segment_last);
                } else {
                    uint32_t i;
                    for (i = 0u; i < span_count; i++) {
                        uint32_t lo = spans[i].first > pos
                                          ? spans[i].first : pos;
                        uint32_t hi = spans[i].last < segment_last
                                          ? spans[i].last : segment_last;
                        if (lo <= hi) {
                            susn_writer_emit(&writer, lo, hi);
                        }
                    }
                }
            }
        }

        if (next >= (uint32_t)SUSN_END) {
            break;
        }

        bool moved_a = susn_cursor_advance(&ca, next);
        bool moved_b = susn_cursor_advance(&cb, next);

        if (next <= pos && !moved_a && !moved_b) {
            break; /* no progress: malformed input, stop deterministically */
        }
        pos = next;
    }

    return writer.needed;
}

/* ── Set algebra ───────────────────────────────────────────────────── */

uint32_t suts_susn_combine(susn_set_op_t op,
                           const susn_set_descriptor_t *a,
                           const susn_set_descriptor_t *b,
                           susn_codepoint_range_t *out, uint32_t capacity)
{
    if (op != SUSN_OP_UNION && op != SUSN_OP_INTERSECTION &&
        op != SUSN_OP_DIFFERENCE && op != SUSN_OP_SYMMETRIC_DIFFERENCE) {
        return 0u;
    }
    if (capacity > 0u && out == NULL) {
        return 0u;
    }
    if (!susn_operand_ok(a) || !susn_operand_ok(b)) {
        return 0u;
    }
    return susn_sweep(a, b, op, NULL, 0u, out, capacity);
}

uint32_t suts_susn_union(const susn_set_descriptor_t *a,
                         const susn_set_descriptor_t *b,
                         susn_codepoint_range_t *out, uint32_t capacity)
{
    return suts_susn_combine(SUSN_OP_UNION, a, b, out, capacity);
}

uint32_t suts_susn_intersection(const susn_set_descriptor_t *a,
                                const susn_set_descriptor_t *b,
                                susn_codepoint_range_t *out, uint32_t capacity)
{
    return suts_susn_combine(SUSN_OP_INTERSECTION, a, b, out, capacity);
}

uint32_t suts_susn_difference(const susn_set_descriptor_t *a,
                              const susn_set_descriptor_t *b,
                              susn_codepoint_range_t *out, uint32_t capacity)
{
    return suts_susn_combine(SUSN_OP_DIFFERENCE, a, b, out, capacity);
}

uint32_t suts_susn_symmetric_difference(const susn_set_descriptor_t *a,
                                        const susn_set_descriptor_t *b,
                                        susn_codepoint_range_t *out,
                                        uint32_t capacity)
{
    return suts_susn_combine(SUSN_OP_SYMMETRIC_DIFFERENCE, a, b, out, capacity);
}

uint32_t suts_susn_complement(const susn_set_descriptor_t *set,
                              susn_codepoint_range_t *out, uint32_t capacity)
{
    susn_set_descriptor_t inverted;

    if (capacity > 0u && out == NULL) {
        return 0u;
    }
    if (!susn_operand_ok(set)) {
        return 0u;
    }

    if (set == NULL) {
        inverted.set_id = 0u;
        inverted.set_flags = SUSN_FLAG_NEGATED;
        inverted.range_count = 0u;
        inverted.ranges = NULL;
    } else {
        inverted = *set;
        inverted.set_flags ^= SUSN_FLAG_NEGATED;
    }
    return susn_sweep(&inverted, NULL, SUSN_OP_UNION, NULL, 0u,
                      out, capacity);
}

/* ── Range filtering and probe masks ───────────────────────────────── */

uint32_t suts_susn_filter(const susn_set_descriptor_t *set, uint32_t zone_mask,
                          susn_codepoint_range_t *out, uint32_t capacity)
{
    susn_codepoint_range_t spans[6];
    uint32_t count = 0u;

    if (capacity > 0u && out == NULL) {
        return 0u;
    }
    if (!susn_operand_ok(set)) {
        return 0u;
    }

    zone_mask &= SUSN_FILTER_ALL;
    if (zone_mask == 0u) {
        return 0u;
    }

    if ((zone_mask & SUSN_FILTER_BRIDGE) != 0u) {
        spans[count].first = 0x00000000u;
        spans[count].last = 0x0010FFFFu;
        count++;
    }
    if ((zone_mask & SUSN_FILTER_SCP) != 0u) {
        spans[count].first = 0x00110000u;
        spans[count].last = 0x0011FFFFu;
        count++;
    }
    if ((zone_mask & SUSN_FILTER_NATIVE) != 0u) {
        /* The Native span already contains RNUR, so selecting both must not
         * emit the neography block twice. */
        spans[count].first = 0x00120000u;
        spans[count].last = 0x7FFFFFEFu;
        count++;
    } else if ((zone_mask & SUSN_FILTER_RNUR) != 0u) {
        spans[count].first = SUSN_RNUR_MIN;
        spans[count].last = SUSN_RNUR_MAX;
        count++;
    }
    if ((zone_mask & SUSN_FILTER_TRAP) != 0u) {
        spans[count].first = 0x7FFFFFF0u;
        spans[count].last = 0x7FFFFFFEu;
        count++;
    }
    if ((zone_mask & SUSN_FILTER_SENTINEL) != 0u) {
        spans[count].first = 0x7FFFFFFFu;
        spans[count].last = 0x7FFFFFFFu;
        count++;
    }

    if (count == 0u) {
        return 0u;
    }
    return susn_sweep(set, NULL, SUSN_OP_UNION, spans, count, out, capacity);
}

uint32_t suts_susn_probe_mask(const susn_set_descriptor_t *set, uint32_t first,
                              uint32_t stride, uint32_t count)
{
    uint32_t mask = 0u;
    uint32_t i;

    if (set == NULL || count == 0u) {
        return 0u;
    }
    if (count > 32u) {
        count = 32u;
    }
    if (stride == 0u) {
        return suts_susn_contains(set, first) ? 1u : 0u;
    }
    for (i = 0u; i < count; i++) {
        uint64_t cp = (uint64_t)first + (uint64_t)i * (uint64_t)stride;
        if (cp > (uint64_t)SUSN_UNIVERSE_MAX) {
            break;
        }
        if (suts_susn_contains(set, (uint32_t)cp)) {
            mask |= (1u << i);
        }
    }
    return mask;
}

/* ── Set relations ─────────────────────────────────────────────────── */

bool suts_susn_is_empty(const susn_set_descriptor_t *set)
{
    if (set == NULL || (set->range_count > 0u && set->ranges == NULL)) {
        return true;
    }
    if ((set->set_flags & SUSN_FLAG_NEGATED) != 0u) {
        return set->range_count == 1u &&
               set->ranges[0].first == 0u &&
               set->ranges[0].last == SUSN_UNIVERSE_MAX;
    }
    return set->range_count == 0u;
}

bool suts_susn_is_universe(const susn_set_descriptor_t *set)
{
    if (set == NULL || (set->range_count > 0u && set->ranges == NULL)) {
        return false;
    }
    if ((set->set_flags & SUSN_FLAG_NEGATED) != 0u) {
        return set->range_count == 0u;
    }
    return set->range_count == 1u &&
           set->ranges[0].first == 0u &&
           set->ranges[0].last == SUSN_UNIVERSE_MAX;
}

bool suts_susn_is_subset(const susn_set_descriptor_t *a,
                         const susn_set_descriptor_t *b)
{
    if (!susn_operand_ok(a) || !susn_operand_ok(b)) {
        return false;
    }
    return susn_sweep(a, b, SUSN_OP_DIFFERENCE, NULL, 0u, NULL, 0u) == 0u;
}

bool suts_susn_is_disjoint(const susn_set_descriptor_t *a,
                           const susn_set_descriptor_t *b)
{
    if (!susn_operand_ok(a) || !susn_operand_ok(b)) {
        return false;
    }
    return susn_sweep(a, b, SUSN_OP_INTERSECTION, NULL, 0u, NULL, 0u) == 0u;
}

bool suts_susn_equals(const susn_set_descriptor_t *a,
                      const susn_set_descriptor_t *b)
{
    return suts_susn_is_subset(a, b) && suts_susn_is_subset(b, a);
}

/* ── Zones and neography ───────────────────────────────────────────── */

susn_zone_t suts_susn_zone_of(uint32_t codepoint)
{
    if (codepoint <= 0x0010FFFFu) {
        return SUSN_ZONE_BRIDGE;
    }
    if (codepoint >= 0x00110000u && codepoint <= 0x0011FFFFu) {
        return SUSN_ZONE_SCP;
    }
    if (codepoint >= SUSN_RNUR_MIN && codepoint <= SUSN_RNUR_MAX) {
        return SUSN_ZONE_RNUR;
    }
    if (codepoint >= 0x00120000u && codepoint <= 0x7FFFFFEFu) {
        return SUSN_ZONE_NATIVE;
    }
    if (codepoint >= 0x7FFFFFF0u && codepoint <= 0x7FFFFFFEu) {
        return SUSN_ZONE_TRAP;
    }
    if (codepoint == SUSN_UNIVERSE_MAX) {
        return SUSN_ZONE_SENTINEL;
    }
    return SUSN_ZONE_EXTSUCS;
}

uint32_t suts_susn_zone_filter(susn_zone_t zone)
{
    switch (zone) {
    case SUSN_ZONE_BRIDGE:   return SUSN_FILTER_BRIDGE;
    case SUSN_ZONE_SCP:      return SUSN_FILTER_SCP;
    case SUSN_ZONE_RNUR:     return SUSN_FILTER_RNUR;
    case SUSN_ZONE_NATIVE:   return SUSN_FILTER_NATIVE;
    case SUSN_ZONE_TRAP:     return SUSN_FILTER_TRAP;
    case SUSN_ZONE_SENTINEL: return SUSN_FILTER_SENTINEL;
    case SUSN_ZONE_EXTSUCS:  return 0u;
    default:                 return 0u;
    }
}

bool suts_susn_is_neography(uint32_t codepoint)
{
    return codepoint >= SUSN_RNUR_MIN && codepoint <= SUSN_RNUR_MAX;
}

bool suts_susn_neography_set_of(uint32_t codepoint, uint32_t *out_set)
{
    if (out_set == NULL || !suts_susn_is_neography(codepoint)) {
        return false;
    }
    *out_set = (codepoint - SUSN_RNUR_MIN) >> SUSN_RNUR_SET_SHIFT;
    return true;
}

bool suts_susn_neography_bounds(uint32_t set_number, uint32_t *out_first,
                                uint32_t *out_last)
{
    uint32_t first;
    uint32_t last;

    if (set_number >= SUSN_RNUR_SET_COUNT) {
        return false;
    }
    first = SUSN_RNUR_MIN + (set_number << SUSN_RNUR_SET_SHIFT);
    last = first + (SUSN_RNUR_SET_SIZE - 1u);
    if (last > SUSN_RNUR_MAX) {
        return false;
    }
    if (out_first != NULL) {
        *out_first = first;
    }
    if (out_last != NULL) {
        *out_last = last;
    }
    return true;
}

bool suts_susn_neography_contains(uint32_t set_number, uint32_t codepoint)
{
    uint32_t first;
    uint32_t last;

    if (!suts_susn_neography_bounds(set_number, &first, &last)) {
        return false;
    }
    return codepoint >= first && codepoint <= last;
}

bool suts_susn_neography_set(uint32_t set_number,
                             susn_codepoint_range_t *storage,
                             susn_set_descriptor_t *out)
{
    uint32_t first;
    uint32_t last;

    if (storage == NULL || out == NULL) {
        return false;
    }
    if (!suts_susn_neography_bounds(set_number, &first, &last)) {
        return false;
    }

    storage->first = first;
    storage->last = last;

    out->set_id = (uint32_t)SUSN_SETID_NEOSCRIPT_BASE + set_number;
    out->set_flags = SUSN_FLAG_RANGE_ARRAY | SUSN_FLAG_NEOSCRIPT;
    out->range_count = 1u;
    out->ranges = storage;
    return true;
}

/* ── Builder ───────────────────────────────────────────────────────── */

void suts_susn_builder_init(susn_builder_t *builder, uint32_t flags)
{
    if (builder == NULL) {
        return;
    }
    builder->flags = flags & SUSN_FLAG_MASK_DEFINED;
    builder->count = 0u;
}

static bool susn_builder_merge_at(susn_builder_t *builder, uint32_t pos)
{
    while (pos + 1u < builder->count &&
           (uint64_t)builder->ranges[pos + 1u].first <=
               (uint64_t)builder->ranges[pos].last + 1u) {
        if (builder->ranges[pos + 1u].last > builder->ranges[pos].last) {
            builder->ranges[pos].last = builder->ranges[pos + 1u].last;
        }
        {
            uint32_t i;
            for (i = pos + 1u; i + 1u < builder->count; i++) {
                builder->ranges[i] = builder->ranges[i + 1u];
            }
        }
        builder->count--;
    }
    return true;
}

bool suts_susn_builder_add_range(susn_builder_t *builder, uint32_t first,
                                 uint32_t last)
{
    uint32_t pos;
    uint32_t i;

    if (builder == NULL || !susn_range_valid(first, last)) {
        return false;
    }
    if (builder->count >= SUSN_BUILDER_CAPACITY) {
        return false;
    }

    pos = 0u;
    while (pos < builder->count && builder->ranges[pos].first < first) {
        pos++;
    }

    for (i = builder->count; i > pos; i--) {
        builder->ranges[i] = builder->ranges[i - 1u];
    }
    builder->ranges[pos].first = first;
    builder->ranges[pos].last = last;
    builder->count++;

    if (pos > 0u &&
        (uint64_t)builder->ranges[pos].first <=
            (uint64_t)builder->ranges[pos - 1u].last + 1u) {
        if (builder->ranges[pos].last > builder->ranges[pos - 1u].last) {
            builder->ranges[pos - 1u].last = builder->ranges[pos].last;
        }
        for (i = pos; i + 1u < builder->count; i++) {
            builder->ranges[i] = builder->ranges[i + 1u];
        }
        builder->count--;
        pos--;
    }

    return susn_builder_merge_at(builder, pos);
}

bool suts_susn_builder_add_codepoint(susn_builder_t *builder, uint32_t codepoint)
{
    if (codepoint > SUSN_UNIVERSE_MAX) {
        return false;
    }
    return suts_susn_builder_add_range(builder, codepoint, codepoint);
}

bool suts_susn_builder_add_set(susn_builder_t *builder,
                               const susn_set_descriptor_t *set)
{
    uint32_t i;

    if (builder == NULL || !susn_operand_ok(set) || set == NULL) {
        return false;
    }

    if ((set->set_flags & SUSN_FLAG_NEGATED) == 0u) {
        for (i = 0u; i < set->range_count; i++) {
            if (!suts_susn_builder_add_range(builder, set->ranges[i].first,
                                             set->ranges[i].last)) {
                return false;
            }
        }
        return true;
    }

    /* Add the complement of the inversion list, gap by gap. */
    {
        uint32_t cursor = 0u;

        for (i = 0u; i < set->range_count; i++) {
            const susn_codepoint_range_t *range = &set->ranges[i];

            if (!susn_range_valid(range->first, range->last)) {
                return false;
            }
            if (range->last < cursor) {
                continue;
            }
            if (range->first > cursor) {
                if (!suts_susn_builder_add_range(builder, cursor,
                                                 range->first - 1u)) {
                    return false;
                }
            }
            if (range->last >= SUSN_UNIVERSE_MAX) {
                return true;
            }
            cursor = range->last + 1u;
        }
        return suts_susn_builder_add_range(builder, cursor, SUSN_UNIVERSE_MAX);
    }
}

bool suts_susn_builder_finish(susn_builder_t *builder, uint32_t set_id,
                              susn_set_descriptor_t *out)
{
    if (builder == NULL || out == NULL) {
        return false;
    }
    out->set_id = set_id;
    out->set_flags = builder->flags | SUSN_FLAG_RANGE_ARRAY;
    out->range_count = builder->count;
    out->ranges = builder->ranges;
    return true;
}

/* ── Catalog ───────────────────────────────────────────────────────── */

uint32_t suts_susn_catalog_count(void)
{
    return SUSN_CATALOG_COUNT;
}

const susn_set_descriptor_t *suts_susn_catalog_at(uint32_t index)
{
    if (index >= SUSN_CATALOG_COUNT) {
        return NULL;
    }
    return &susn_catalog[index];
}

const susn_set_descriptor_t *suts_susn_catalog_find(uint32_t set_id)
{
    uint32_t i;

    for (i = 0u; i < SUSN_CATALOG_COUNT; i++) {
        if (susn_catalog[i].set_id == set_id) {
            return &susn_catalog[i];
        }
    }
    return NULL;
}

const susn_set_descriptor_t *suts_susn_catalog_find_name(const char *name)
{
    uint32_t i;

    if (name == NULL) {
        return NULL;
    }
    for (i = 0u; i < SUSN_CATALOG_COUNT; i++) {
        if (susn_streq(suts_susn_set_name(susn_catalog[i].set_id), name)) {
            return &susn_catalog[i];
        }
    }
    return NULL;
}

/* ── Version and setup ─────────────────────────────────────────────── */

#define SUSN_STR2(x) #x
#define SUSN_STR(x)  SUSN_STR2(x)

const char* suts_susn_version_string(void)
{
    return SUSN_SPEC_ID "/"
           SUSN_STR(SUSN_VERSION_MAJOR) "."
           SUSN_STR(SUSN_VERSION_MINOR);
}

void suts_susn_init_engine(void)
{
    /* The engine is fully static; initialization exists so a consumer can
     * force a deterministic point of first use and so a future revision with
     * additional build-time tables keeps a stable entry point. */
}

/* ── Taxonomy names ────────────────────────────────────────────────── */

const char* suts_susn_op_name(susn_set_op_t op)
{
    switch (op) {
    case SUSN_OP_UNION:                return "Union";
    case SUSN_OP_INTERSECTION:         return "Intersection";
    case SUSN_OP_DIFFERENCE:           return "Difference";
    case SUSN_OP_SYMMETRIC_DIFFERENCE: return "Symmetric Difference";
    default:                           return "Unknown";
    }
}

const char* suts_susn_zone_name(susn_zone_t zone)
{
    switch (zone) {
    case SUSN_ZONE_BRIDGE:   return "Bridge";
    case SUSN_ZONE_SCP:      return "SCP";
    case SUSN_ZONE_RNUR:     return "RNUR";
    case SUSN_ZONE_NATIVE:   return "Native";
    case SUSN_ZONE_TRAP:     return "Trap";
    case SUSN_ZONE_SENTINEL: return "Sentinel";
    case SUSN_ZONE_EXTSUCS:  return "ExtSUCS";
    default:                 return "Unknown";
    }
}

const char* suts_susn_set_name(uint32_t set_id)
{
    if (set_id >= (uint32_t)SUSN_SETID_NEOSCRIPT_BASE &&
        set_id < (uint32_t)SUSN_SETID_NEOSCRIPT_BASE + SUSN_RNUR_SET_COUNT) {
        return "Neography Block";
    }
    switch (set_id) {
    case SUSN_SETID_NONE:            return "Empty";
    case SUSN_SETID_ASCII:           return "ASCII";
    case SUSN_SETID_PRINTABLE_ASCII: return "Printable ASCII";
    case SUSN_SETID_DIGIT:           return "Digit";
    case SUSN_SETID_UPPER:           return "Uppercase";
    case SUSN_SETID_LOWER:           return "Lowercase";
    case SUSN_SETID_ASCII_PUNCT:     return "ASCII Punctuation";
    case SUSN_SETID_CONTROL:         return "Control";
    case SUSN_SETID_WHITESPACE:      return "Whitespace";
    case SUSN_SETID_LATIN1:          return "Latin-1 Supplement";
    case SUSN_SETID_GREEK:           return "Greek";
    case SUSN_SETID_CYRILLIC:        return "Cyrillic";
    case SUSN_SETID_HEBREW:          return "Hebrew";
    case SUSN_SETID_ARABIC:          return "Arabic";
    case SUSN_SETID_HIRAGANA:        return "Hiragana";
    case SUSN_SETID_KATAKANA:        return "Katakana";
    case SUSN_SETID_HAN:             return "Han";
    case SUSN_SETID_HANGUL:          return "Hangul";
    case SUSN_SETID_EMOJI:           return "Emoji";
    case SUSN_SETID_BIDI_CONTROL:    return "Bidi Control";
    case SUSN_SETID_BRIDGE:          return "Bridge";
    case SUSN_SETID_SCP:             return "SCP";
    case SUSN_SETID_RNUR:            return "RNUR";
    case SUSN_SETID_NATIVE:          return "Native";
    case SUSN_SETID_TRAP:            return "Trap";
    case SUSN_SETID_SENTINEL:        return "Sentinel";
    default:                         return "Unknown";
    }
}
