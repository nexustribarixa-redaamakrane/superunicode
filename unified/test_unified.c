/**
 * Unified Header Coexistence Test
 *
 * This single translation unit includes EVERY public header from all three
 * modules:
 *
 *   - Base SUCS            : superunicode/superunicode.h (+ sucs_trap.h)
 *   - SUTF transport       : sutf/sutf.h (sucs_types, sucs_mode, sutf8/16/4/2)
 *   - Extended extSUTF     : extsucs_types.h, extsutf_fixed.h, vsutf.h, esutf.h
 *   - Plugin subsystem     : plugin.h, plugin_checksum.h, plugin_stage.h,
 *                            plugin_boot.h, plugin_partition.h
 *
 * Prior to the header-dedupe work, the identical constants (SUCS_INVALID_
 * CODEPOINT, SUCS_TRAP_RANGE_MIN/MAX, SUCS_MAX_CODEPOINT, ...), the
 * sucs_char_t typedef, and sucs_is_valid() were each defined in multiple
 * headers; this file would NOT compile. It now must compile cleanly and run.
 */

#include <stdio.h>
#include <assert.h>
#include "superunicode/superunicode.h"
#include "superunicode/sucs_trap.h"
#include "sutf.h"
#include "extsucs_types.h"
#include "extsutf_fixed.h"
#include "vsutf.h"
#include "esutf.h"
#include "superunicode_extended/plugin.h"
#include "superunicode_extended/plugin_checksum.h"
#include "superunicode_extended/plugin_stage.h"
#include "superunicode_extended/plugin_boot.h"
#include "superunicode_extended/plugin_partition.h"

int main(void) {
    /* --- Duplicated constants agree across modules --- */
    assert(SUCS_MAX_CODEPOINT == 0x7FFFFFFFUL);
    assert(SUCS_INVALID_CODEPOINT == 0x7FFFFFFFUL);
    assert(SUCS_TRAP_RANGE_MIN == SUCS_KERNEL_TRAP_MIN);
    assert(SUCS_TRAP_RANGE_MAX == SUCS_KERNEL_TRAP_MAX);
    assert(SUCS_KERNEL_TRAP_MIN == 0x7FFFFFF0UL);
    assert(SUCS_KERNEL_TRAP_MAX == 0x7FFFFFFEUL);

    /* --- The shared sucs_char_t / sucs_is_valid() coexist --- */
    assert(sucs_is_valid(0x41) == true);
    assert(sucs_is_valid(0x7FFFFFFFUL) == false);   /* sentinel */
    assert(sucs_is_valid(0x7FFFFFF5UL) == false);   /* trap range */
    assert(sucs_is_valid(0x7FFFFFEFUL) == true);

    /* --- ExtSUCS validator distinct and correct --- */
    assert(extsucs_is_valid(0x7FFFFFFFULL) == true);
    assert(extsucs_is_valid(0xFFFFFFFFFFFFFFFFULL) == true);
    assert(extsucs_is_valid(0x7FFFFFF5ULL) == false);   /* inherited trap range */

    /* --- Mode-switch enums from sutf/sucs_mode.h --- */
    assert(SUCS_MODE_BASE == 0);
    assert(SUCS_MODE_EXTENDED == 1);

    /* --- Classifier + trap math work with the shared constants --- */
    assert(sucs_classify_codepoint(0x41) == SUCS_TYPE_UNICODE_COMPAT);
    assert(sucs_classify_codepoint(0x7FFFFFF5UL) == SUCS_TYPE_INVALID);
    assert(sucs_bancode_to_trap(0x0011A080) == 0x7FFFFFF1);

    /* --- Real trap dispatch works from the unified header --- */
    assert(sucs_trap_dispatch(0x0011A005) == false); /* nothing registered */
    assert(sucs_trap_register_handler(14, NULL, NULL) == false); /* NULL handler */
    assert(sucs_trap_unregister_handler(99) == false);

    /* --- An actual SUTF-8 round trip using sutf_static --- */
    uint8_t buf[8];
    assert(sutf8_encode_char(0x110000, buf, sizeof(buf)) == 5);
    sucs_char_t cp = 0;
    assert(sutf8_decode_char(buf, 5, &cp) == 5);
    assert(cp == 0x110000);

    /* --- Extended transports coexist (headers + a quick call) --- */
    sucs_ex_char_t ex_decoded = 0;
    assert(sutf64_encode(0xDEADBEEFCAFEULL, buf, sizeof(buf)) == 8);
    assert(sutf64_decode(buf, 8, &ex_decoded) == 8);
    assert(ex_decoded == 0xDEADBEEFCAFEULL);
    assert(vsutf_encode(0x41ULL, buf, sizeof(buf)) == 1);
    assert(vsutf_decode(buf, 1, &ex_decoded) == 1);
    assert(ex_decoded == 0x41ULL);

    /* --- Plugin subsystem headers coexist with the encodings --- */
    assert(SUCS_PLUGIN_BLOB_MAGIC == 0x53435343UL);
    assert(SUCS_PLUGIN_BASE_LIMIT == 0x7FFFFFFFULL);
    assert(SUCS_PLUGIN_REBOOT_REQUIRED == 13);
    assert(SUCS_PARTITION_FS_OWFS == 1);
    assert(sucs_plugin_partition_fs_is_valid(SUCS_PARTITION_FS_USFS) == true);

    printf("[PASS] test_unified (all module headers coexist in one TU)\n");
    return 0;
}
