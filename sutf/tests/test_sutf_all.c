#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include "sucs_types.h"
#include "sutf8.h"
#include "sutf16.h"
#include "sutf4.h"
#include "sutf2.h"
#include "sucs_mode.h"

void test_validator(void) {
    assert(sucs_is_valid(0x00000000) == true);
    assert(sucs_is_valid(0x00000041) == true);
    assert(sucs_is_valid(0x0010FFFF) == true);
    assert(sucs_is_valid(0x7FFFFFFE) == false); /* Trap range max */
    assert(sucs_is_valid(0x7FFFFFF0) == false); /* Trap range min */
    assert(sucs_is_valid(SUCS_INVALID_CODEPOINT) == false);
    assert(sucs_is_valid(0x80000000UL) == false);
    printf("[PASS] test_validator\n");
}

void test_sutf8(void) {
    uint8_t buf[16];
    sucs_char_t decoded = 0;

    /* 1 Byte ASCII */
    size_t w = sutf8_encode_char(0x41, buf, sizeof(buf));
    assert(w == 1);
    size_t r = sutf8_decode_char(buf, w, &decoded);
    assert(r == 1 && decoded == 0x41);

    /* 3 Bytes (PUA 0xD800 direct) */
    w = sutf8_encode_char(0xD800, buf, sizeof(buf));
    assert(w == 3);
    r = sutf8_decode_char(buf, w, &decoded);
    assert(r == 3 && decoded == 0xD800);

    /* 4 Bytes (SCP boundary 0x10000) */
    w = sutf8_encode_char(0x10000, buf, sizeof(buf));
    assert(w == 4);
    r = sutf8_decode_char(buf, w, &decoded);
    assert(r == 4 && decoded == 0x10000);

    /* 5 Bytes (0x200000) */
    w = sutf8_encode_char(0x200000, buf, sizeof(buf));
    assert(w == 5);
    r = sutf8_decode_char(buf, w, &decoded);
    assert(r == 5 && decoded == 0x200000);

    /* 6 Bytes (0x4000000) */
    /* 6 Bytes (0x7FFFFFEF - max valid codepoint before trap range) */
    w = sutf8_encode_char(0x7FFFFFEF, buf, sizeof(buf));
    assert(w == 6);
    r = sutf8_decode_char(buf, w, &decoded);
    assert(r == 6 && decoded == 0x7FFFFFEF);

    /* Trap range rejection */
    w = sutf8_encode_char(0x7FFFFFF5UL, buf, sizeof(buf));
    assert(w == 0);

    /* Sentinel rejection */
    w = sutf8_encode_char(SUCS_INVALID_CODEPOINT, buf, sizeof(buf));
    assert(w == 0);

    printf("[PASS] test_sutf8 (1..6 Bytes)\n");
}

void test_sutf16(void) {
    uint16_t words[4];
    sucs_char_t decoded = 0;

    /* 1 Word BMP */
    size_t w = sutf16_encode_char(0x1234, words, 4);
    assert(w == 1 && words[0] == 0x1234);
    size_t r = sutf16_decode_char(words, w, &decoded);
    assert(r == 1 && decoded == 0x1234);

    /* 1 Word PUA (0xD800 direct value) */
    w = sutf16_encode_char(0xD800, words, 4);
    assert(w == 1 && words[0] == 0xD800);
    r = sutf16_decode_char(words, w, &decoded);
    assert(r == 1 && decoded == 0xD800);

    /* 2 Words (0x10000) */
    w = sutf16_encode_char(0x10000, words, 4);
    assert(w == 2);
    r = sutf16_decode_char(words, w, &decoded);
    assert(r == 2 && decoded == 0x10000);

    /* 2 Words (0x7FFFFFEF - highest valid codepoint before trap range) */
    w = sutf16_encode_char(0x7FFFFFEF, words, 4);
    assert(w == 2);
    r = sutf16_decode_char(words, w, &decoded);
    assert(r == 2 && decoded == 0x7FFFFFEF);

    /* Sentinel SUCS_INVALID_CODEPOINT (0x7FFFFFFF) returns 0 */
    w = sutf16_encode_char(0x7FFFFFFF, words, 4);
    assert(w == 0);

    printf("[PASS] test_sutf16 (1..2 Words)\n");
}

void test_sutf4(void) {
    uint8_t buf[8];
    sucs_char_t decoded = 0;

    sucs_char_t cp = 0x12345678;
    size_t w = sutf4_encode_char(cp, buf, sizeof(buf));
    assert(w == 4);
    size_t r = sutf4_decode_char(buf, w, &decoded);
    assert(r == 4 && decoded == cp);

    printf("[PASS] test_sutf4 (Packed 4-Bit Nibbles)\n");
}

void test_sutf2(void) {
    uint8_t buf[8];
    sucs_char_t decoded = 0;

    sucs_char_t cp = 0x3ABCDEF0;
    size_t w = sutf2_encode_char(cp, buf, sizeof(buf));
    assert(w == 4);
    size_t r = sutf2_decode_char(buf, w, &decoded);
    assert(r == 4 && decoded == cp);

    printf("[PASS] test_sutf2 (2-Bit Symbol Bitstream)\n");
}

void test_kernel_mode_switch(void) {
    sucs_kernel_boot_config_t cfg;
    sucs_init_boot_config(&cfg, SUCS_MODE_BASE);

    assert(cfg.active_mode == SUCS_MODE_BASE);
    assert(cfg.pending_mode == SUCS_MODE_BASE);
    assert(cfg.reboot_required == false);

    /* Request switch to ExtSUCS mode */
    sucs_switch_status_t status = sucs_request_mode_switch(SUCS_MODE_EXTENDED);
    assert(status == SUCS_SWITCH_REBOOT_REQUIRED);
    assert(sucs_get_pending_mode() == SUCS_MODE_EXTENDED);
    assert(sucs_is_reboot_required() == true);
    /* Active mode remains Base until system restart commit */
    assert(sucs_get_active_mode() == SUCS_MODE_BASE);

    /* Simulate early kernel boot commit */
    bool committed = sucs_commit_mode_on_boot(NULL);
    assert(committed == true);
    assert(sucs_get_active_mode() == SUCS_MODE_EXTENDED);
    assert(sucs_is_reboot_required() == false);

    /* Requesting same mode when already active returns error */
    status = sucs_request_mode_switch(SUCS_MODE_EXTENDED);
    assert(status == SUCS_SWITCH_ERR_ALREADY_ACTIVE);

    printf("[PASS] test_kernel_mode_switch (Restart & Boot Commit)\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING ALL SUTF SERIALIZATION TESTS   \n");
    printf("=========================================\n");
    test_validator();
    test_sutf8();
    test_sutf16();
    test_sutf4();
    test_sutf2();
    test_kernel_mode_switch();
    printf("=========================================\n");
    printf(" ALL SUTF SERIALIZATION TESTS PASSED!   \n");
    printf("=========================================\n");
    return 0;
}
