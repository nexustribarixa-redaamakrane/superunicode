/**
 * SUAS-007 SIS — System Identifiers and Syntax
 *
 * Unit tests for the zero-allocation, single-pass identifier lexical filter
 * (a UAX #31 analog).
 *
 * Covers: the START / CONTINUE classes over the Unicode Bridge (letters from
 * several scripts, combining marks, digits, connector punctuation,
 * Other_ID_Continue), the pattern whitespace / pattern syntax split of §4,
 * SCP invariance, Native / RNUR neography admission, the immutable and
 * hashtag profiles, the streaming state machine, and guard behaviour.
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "suas/suas_sis.h"

static int g_fail = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_fail = 1; \
} } while (0)

static void test_version_and_meta(void)
{
    CHECK(suas_sis_version_string() != NULL);
    CHECK(suas_sis_version_string()[0] == '1');
    CHECK(SUAS_SIS_SPEC_ID[0] == 'S');
    CHECK(suas_sis_class_name(SUAS_SIS_CLASS_START)[0] == 'S');
    CHECK(suas_sis_class_name(SUAS_SIS_CLASS_CONTINUE)[0] == 'C');
    CHECK(suas_sis_class_name(SUAS_SIS_CLASS_INVALID)[0] == 'I');
    CHECK(suas_sis_syntax_name(SUAS_SIS_SYNTAX_OPERATOR)[0] == 'O');
    CHECK(suas_sis_syntax_name(SUAS_SIS_SYNTAX_WHITESPACE)[0] == 'W');
    CHECK(suas_sis_profile_name(SUAS_SIS_PROFILE_DEFAULT)[0] == 'D');
    CHECK(suas_sis_profile_name(SUAS_SIS_PROFILE_HASHTAG)[0] == 'H');
}

static void test_identifier_classes(void)
{
    /* Letters across scripts are Start characters. */
    CHECK(suas_sis_get_class(0x0061) == SUAS_SIS_CLASS_START); /* a  */
    CHECK(suas_sis_get_class(0x005A) == SUAS_SIS_CLASS_START); /* Z  */
    CHECK(suas_sis_get_class(0x00E9) == SUAS_SIS_CLASS_START); /* é  */
    CHECK(suas_sis_get_class(0x03B1) == SUAS_SIS_CLASS_START); /* α  */
    CHECK(suas_sis_get_class(0x0561) == SUAS_SIS_CLASS_START); /* ա  */
    CHECK(suas_sis_get_class(0x05D0) == SUAS_SIS_CLASS_START); /* א  */
    CHECK(suas_sis_get_class(0x4E00) == SUAS_SIS_CLASS_START); /* 一  */
    CHECK(suas_sis_get_class(0x3042) == SUAS_SIS_CLASS_START); /* あ  */
    CHECK(suas_sis_get_class(0xAC00) == SUAS_SIS_CLASS_START); /* 가  */

    /* Underscore and the Other_ID_Start set are Start characters. */
    CHECK(suas_sis_get_class(0x005F) == SUAS_SIS_CLASS_START); /* _ */
    CHECK(suas_sis_get_class(0x2118) == SUAS_SIS_CLASS_START); /* ℘ */
    CHECK(suas_sis_get_class(0x212E) == SUAS_SIS_CLASS_START); /* ℮ */

    /* Continue-only characters. */
    CHECK(suas_sis_get_class(0x0035) == SUAS_SIS_CLASS_CONTINUE); /* 5 */
    CHECK(suas_sis_get_class(0x0301) == SUAS_SIS_CLASS_CONTINUE); /*  ́  */
    CHECK(suas_sis_get_class(0x093F) == SUAS_SIS_CLASS_CONTINUE); /* ि  */
    CHECK(suas_sis_get_class(0x200D) == SUAS_SIS_CLASS_CONTINUE); /* ZWJ */
    CHECK(suas_sis_get_class(0x00B7) == SUAS_SIS_CLASS_CONTINUE); /* ·  */
    CHECK(suas_sis_get_class(0x203F) == SUAS_SIS_CLASS_CONTINUE); /* ‿  */

    /* Whitespace, syntax, controls and emoji are invalid. */
    CHECK(suas_sis_get_class(0x0020) == SUAS_SIS_CLASS_INVALID); /* sp  */
    CHECK(suas_sis_get_class(0x0021) == SUAS_SIS_CLASS_INVALID); /* !   */
    CHECK(suas_sis_get_class(0x002B) == SUAS_SIS_CLASS_INVALID); /* +   */
    CHECK(suas_sis_get_class(0x0023) == SUAS_SIS_CLASS_INVALID); /* #   */
    CHECK(suas_sis_get_class(0x000A) == SUAS_SIS_CLASS_INVALID); /* LF  */
    CHECK(suas_sis_get_class(0x1F600) == SUAS_SIS_CLASS_INVALID);/* 😀 */

    /* Predicates agree with the class. */
    CHECK(suas_sis_is_start(0x0061) && !suas_sis_is_start(0x0035));
    CHECK(suas_sis_is_continue(0x0061) && suas_sis_is_continue(0x0035));
    CHECK(!suas_sis_is_continue(0x0020));
}

static void test_scp_and_native(void)
{
    /* SCP markers are strictly invalid inside identifiers. */
    CHECK(suas_sis_is_scp(0x00110000ULL));
    CHECK(suas_sis_is_scp(0x0011A080ULL));
    CHECK(!suas_sis_is_scp(0x0010FFFFULL));
    CHECK(suas_sis_get_class(0x00110000ULL) == SUAS_SIS_CLASS_INVALID);
    CHECK(suas_sis_get_class(0x00110040ULL) == SUAS_SIS_CLASS_INVALID);

    /* Native / RNUR / RCCR neography codepoints are identifier characters. */
    CHECK(suas_sis_get_class(0x00120000ULL) == SUAS_SIS_CLASS_START);
    CHECK(suas_sis_get_class(SUAS_SIS_RNUR_MIN) == SUAS_SIS_CLASS_START);
    CHECK(suas_sis_get_class(0x0016FFFFULL) == SUAS_SIS_CLASS_START);

    /* Trap slots and the Sentinel are invalid. */
    CHECK(suas_sis_get_class(0x7FFFFFF5ULL) == SUAS_SIS_CLASS_INVALID);
    CHECK(suas_sis_get_class(0x7FFFFFFFULL) == SUAS_SIS_CLASS_INVALID);
}

static void test_syntax(void)
{
    CHECK(suas_sis_get_syntax(0x0061) == SUAS_SIS_SYNTAX_LITERAL);
    CHECK(suas_sis_get_syntax(0x0009) == SUAS_SIS_SYNTAX_WHITESPACE);
    CHECK(suas_sis_get_syntax(0x0020) == SUAS_SIS_SYNTAX_WHITESPACE);
    CHECK(suas_sis_get_syntax(0x000A) == SUAS_SIS_SYNTAX_LINETERM);
    CHECK(suas_sis_get_syntax(0x000D) == SUAS_SIS_SYNTAX_LINETERM);
    CHECK(suas_sis_get_syntax(0x2028) == SUAS_SIS_SYNTAX_LINETERM);
    CHECK(suas_sis_get_syntax(0x200E) == SUAS_SIS_SYNTAX_IGNORABLE);
    CHECK(suas_sis_get_syntax(0x002B) == SUAS_SIS_SYNTAX_OPERATOR);
    CHECK(suas_sis_get_syntax(0x2192) == SUAS_SIS_SYNTAX_OPERATOR);
    CHECK(suas_sis_get_syntax(0x00110000ULL) == SUAS_SIS_SYNTAX_CONTROL);
    CHECK(suas_sis_get_syntax(0x7FFFFFF5ULL) == SUAS_SIS_SYNTAX_CONTROL);
    CHECK(suas_sis_get_syntax(0x00160000ULL) == SUAS_SIS_SYNTAX_LITERAL);
}

static void test_state_machine(void)
{
    suas_sis_state_t st;

    /* A well-formed identifier accumulates one codepoint at a time. */
    suas_sis_init(&st);
    CHECK(!suas_sis_is_started(&st));
    CHECK(suas_sis_feed(&st, 0x0061) == true);  /* a */
    CHECK(suas_sis_feed(&st, 0x0062) == true);  /* b */
    CHECK(suas_sis_feed(&st, 0x0031) == true);  /* 1 */
    CHECK(suas_sis_feed(&st, 0x005F) == true);  /* _ */
    CHECK(suas_sis_is_started(&st));
    CHECK(suas_sis_length(&st) == 4u);

    /* An identifier cannot begin with a Continue-only codepoint. */
    suas_sis_init(&st);
    CHECK(suas_sis_feed(&st, 0x0031) == false); /* 1 */
    CHECK(!suas_sis_is_started(&st));

    /* An invalid codepoint terminates the identifier in progress. */
    suas_sis_init(&st);
    CHECK(suas_sis_feed(&st, 0x0061) == true);
    CHECK(suas_sis_feed(&st, 0x0021) == false); /* ! */
    CHECK(!suas_sis_is_started(&st));
    CHECK(suas_sis_length(&st) == 0u);

    /* A terminated identifier is ready to start a new one. */
    CHECK(suas_sis_feed(&st, 0x0062) == true);
    CHECK(suas_sis_length(&st) == 1u);

    /* SCP markers are rejected and terminate, never appended. */
    suas_sis_init(&st);
    CHECK(suas_sis_feed(&st, 0x0061) == true);
    CHECK(suas_sis_feed(&st, 0x00110000ULL) == false);
    CHECK(!suas_sis_is_started(&st));

    /* Guards. */
    suas_sis_init(NULL);
    CHECK(suas_sis_feed(NULL, 0x0061) == false);
    CHECK(!suas_sis_is_started(NULL));
    CHECK(suas_sis_length(NULL) == 0u);
}

static void test_profiles(void)
{
    /* Hashtag profile: '#' starts, emoji continue, a second '#' terminates. */
    CHECK(suas_sis_get_class_ex(0x0023, SUAS_SIS_PROFILE_HASHTAG) ==
          SUAS_SIS_CLASS_START);
    CHECK(suas_sis_get_class_ex(0x1F600, SUAS_SIS_PROFILE_HASHTAG) ==
          SUAS_SIS_CLASS_CONTINUE);

    {
        suas_sis_state_t st;
        const uint32_t cps[] = { 0x0023, 0x0061, 0x002D, 0x0062 };
        int i;
        suas_sis_init(&st);
        for (i = 0; i < 4; i++)
            CHECK(suas_sis_feed_ex(&st, cps[i], SUAS_SIS_PROFILE_HASHTAG)
                  == true);
        CHECK(suas_sis_length(&st) == 4u);
        CHECK(suas_sis_feed_ex(&st, 0x0023, SUAS_SIS_PROFILE_HASHTAG) == false);
        CHECK(!suas_sis_is_started(&st));
    }

    /* Immutable profile: unassigned allowed, controls / PUA / PS forbidden. */
    CHECK(suas_sis_get_class_ex(0x0378, SUAS_SIS_PROFILE_IMMUTABLE) ==
          SUAS_SIS_CLASS_START);
    CHECK(suas_sis_get_class_ex(0x0041, SUAS_SIS_PROFILE_IMMUTABLE) ==
          SUAS_SIS_CLASS_START);
    CHECK(suas_sis_get_class_ex(0x0001, SUAS_SIS_PROFILE_IMMUTABLE) ==
          SUAS_SIS_CLASS_INVALID);
    CHECK(suas_sis_get_class_ex(0x0020, SUAS_SIS_PROFILE_IMMUTABLE) ==
          SUAS_SIS_CLASS_INVALID);
    CHECK(suas_sis_get_class_ex(0x002B, SUAS_SIS_PROFILE_IMMUTABLE) ==
          SUAS_SIS_CLASS_INVALID);
    CHECK(suas_sis_get_class_ex(0xE000, SUAS_SIS_PROFILE_IMMUTABLE) ==
          SUAS_SIS_CLASS_INVALID);
    CHECK(suas_sis_get_class_ex(0xD800, SUAS_SIS_PROFILE_IMMUTABLE) ==
          SUAS_SIS_CLASS_INVALID);
}

int main(void)
{
    test_version_and_meta();
    test_identifier_classes();
    test_scp_and_native();
    test_syntax();
    test_state_machine();
    test_profiles();

    if (g_fail) {
        printf("SUAS-007 SIS tests FAILED\n");
        return 1;
    }
    printf("SUAS-007 SIS tests passed\n");
    return 0;
}
