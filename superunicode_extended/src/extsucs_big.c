/**
 * ExtSUCS Arbitrary-Precision Codepoints
 *
 * Little-endian base-2^32 magnitude with caller-owned storage.  No allocation,
 * no libc, no fixed maximum number of limbs.
 */

#include "extsucs_big.h"

/* Inherited Kernel Security Trap Range (64-bit fast-path addresses). */
#define EXTSUCS_TRAP_MIN  0x7FFFFFF0ULL
#define EXTSUCS_TRAP_MAX  0x7FFFFFFEULL
#define EXTSUCS_BASE_MAX  0x7FFFFFFFULL

static void big_trim(sucs_ex_big_t *v) {
    while (v->len > 0 && v->limbs[v->len - 1] == 0) {
        v->len--;
    }
}

bool extsucs_big_init(sucs_ex_big_t *v, uint32_t *storage, size_t cap) {
    if (!v) {
        return false;
    }
    if (!storage && cap != 0) {
        return false;
    }
    v->limbs = storage;
    v->len = 0;
    v->cap = cap;
    return true;
}

void extsucs_big_clear(sucs_ex_big_t *v) {
    if (v) {
        v->len = 0;
    }
}

bool extsucs_big_assign_u64(sucs_ex_big_t *v, uint64_t value) {
    if (!v) {
        return false;
    }
    if (value == 0) {
        v->len = 0;
        return true;
    }
    if (!v->limbs || v->cap < 1) {
        v->len = 0;
        return false;
    }
    v->limbs[0] = (uint32_t)(value & 0xFFFFFFFFULL);
    if (value > 0xFFFFFFFFULL) {
        if (v->cap < 2) {
            v->len = 0;
            return false;
        }
        v->limbs[1] = (uint32_t)(value >> 32);
        v->len = 2;
    } else {
        v->len = 1;
    }
    return true;
}

bool extsucs_big_copy(sucs_ex_big_t *dst, const sucs_ex_big_t *src) {
    if (!dst || !src) {
        return false;
    }
    if (src->len > dst->cap) {
        dst->len = 0;
        return false;
    }
    for (size_t i = 0; i < src->len; i++) {
        dst->limbs[i] = src->limbs[i];
    }
    dst->len = src->len;
    return true;
}

size_t extsucs_big_limbs_for_bytes(size_t byte_count) {
    if (byte_count == 0) {
        return 0;
    }
    if (byte_count > ((size_t)-1) - 3) {
        return (size_t)-1;
    }
    return (byte_count + 3) / 4;
}

bool extsucs_big_assign_bytes_be(sucs_ex_big_t *v,
                                 const uint8_t *bytes, size_t n) {
    if (!v || (!bytes && n != 0)) {
        return false;
    }
    if (n == 0) {
        v->len = 0;
        return true;
    }

    size_t needed = extsucs_big_limbs_for_bytes(n);
    if (needed == (size_t)-1 || needed > v->cap || !v->limbs) {
        v->len = 0;
        return false;
    }

    for (size_t i = 0; i < needed; i++) {
        v->limbs[i] = 0;
    }

    /* Walk from the least-significant byte (end of the input). */
    size_t byte_index = n;
    for (size_t limb = 0; limb < needed; limb++) {
        uint32_t word = 0;
        for (unsigned b = 0; b < 4 && byte_index > 0; b++) {
            --byte_index;
            word |= (uint32_t)bytes[byte_index] << (8u * b);
        }
        v->limbs[limb] = word;
    }

    v->len = needed;
    big_trim(v);
    return true;
}

size_t extsucs_big_bit_length(const sucs_ex_big_t *v) {
    if (!v || v->len == 0) {
        return 0;
    }
    uint32_t top = v->limbs[v->len - 1];
    size_t bits = 0;
    while (top != 0) {
        bits++;
        top >>= 1;
    }
    return (v->len - 1) * 32 + bits;
}

size_t extsucs_big_byte_length(const sucs_ex_big_t *v) {
    size_t bits = extsucs_big_bit_length(v);
    return (bits + 7) / 8;
}

bool extsucs_big_bytes_be_slice(const sucs_ex_big_t *v,
                                size_t offset, uint8_t *out, size_t len) {
    if (!v || (!out && len != 0)) {
        return false;
    }
    size_t total = extsucs_big_byte_length(v);
    if (offset > total || len > total - offset) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        size_t from_msb = offset + i;
        size_t from_lsb = total - 1 - from_msb;
        size_t limb = from_lsb / 4;
        unsigned shift = (unsigned)((from_lsb % 4) * 8);
        out[i] = (uint8_t)((v->limbs[limb] >> shift) & 0xFFu);
    }
    return true;
}

bool extsucs_big_to_bytes_be(const sucs_ex_big_t *v,
                             uint8_t *out, size_t cap, size_t *out_len) {
    if (!v || !out_len) {
        return false;
    }
    size_t need = extsucs_big_byte_length(v);
    *out_len = need;
    if (need == 0) {
        return true;
    }
    if (!out || cap < need) {
        return false;
    }
    return extsucs_big_bytes_be_slice(v, 0, out, need);
}

bool extsucs_big_to_u64(const sucs_ex_big_t *v, uint64_t *out) {
    if (!v || !out) {
        return false;
    }
    if (v->len == 0) {
        *out = 0;
        return true;
    }
    if (v->len > 2) {
        return false;
    }
    uint64_t value = (uint64_t)v->limbs[0];
    if (v->len == 2) {
        value |= ((uint64_t)v->limbs[1]) << 32;
    }
    *out = value;
    return true;
}

int extsucs_big_compare(const sucs_ex_big_t *a, const sucs_ex_big_t *b) {
    if (a == b) {
        return 0;
    }
    if (!a) {
        return (b && b->len != 0) ? -1 : 0;
    }
    if (!b) {
        return (a->len != 0) ? 1 : 0;
    }
    if (a->len != b->len) {
        return a->len < b->len ? -1 : 1;
    }
    for (size_t i = a->len; i > 0; i--) {
        uint32_t wa = a->limbs[i - 1];
        uint32_t wb = b->limbs[i - 1];
        if (wa != wb) {
            return wa < wb ? -1 : 1;
        }
    }
    return 0;
}

bool extsucs_big_is_zero(const sucs_ex_big_t *v) {
    return !v || v->len == 0;
}

bool extsucs_big_is_valid(const sucs_ex_big_t *v) {
    if (!v) {
        return false;
    }
    uint64_t value;
    if (!extsucs_big_to_u64(v, &value)) {
        return true; /* above 2^64-1: always outside the inherited trap range */
    }
    if (value >= EXTSUCS_TRAP_MIN && value <= EXTSUCS_TRAP_MAX) {
        return false;
    }
    return true;
}

bool extsucs_big_is_base_sucs(const sucs_ex_big_t *v) {
    if (!v) {
        return false;
    }
    if (v->len == 0) {
        return true;
    }
    if (v->len > 1) {
        return false;
    }
    return (uint64_t)v->limbs[0] <= EXTSUCS_BASE_MAX;
}

bool extsucs_big_downcast(const sucs_ex_big_t *v, uint32_t *out_cp) {
    if (!out_cp) {
        return false;
    }
    uint64_t value;
    if (!v || !extsucs_big_to_u64(v, &value) ||
        value > EXTSUCS_BASE_MAX ||
        value == EXTSUCS_BASE_MAX ||
        (value >= EXTSUCS_TRAP_MIN && value <= EXTSUCS_TRAP_MAX)) {
        *out_cp = (uint32_t)EXTSUCS_BASE_MAX;
        return false;
    }
    *out_cp = (uint32_t)value;
    return true;
}

bool extsucs_big_add_u32(sucs_ex_big_t *v, uint32_t addend) {
    if (!v) {
        return false;
    }
    if (addend == 0) {
        return true;
    }

    uint64_t carry = addend;
    size_t i = 0;
    while (carry != 0 && i < v->len) {
        uint64_t sum = (uint64_t)v->limbs[i] + carry;
        v->limbs[i] = (uint32_t)(sum & 0xFFFFFFFFULL);
        carry = sum >> 32;
        i++;
    }
    if (carry != 0) {
        if (v->len >= v->cap || !v->limbs) {
            return false;
        }
        v->limbs[v->len++] = (uint32_t)carry;
    }
    return true;
}
