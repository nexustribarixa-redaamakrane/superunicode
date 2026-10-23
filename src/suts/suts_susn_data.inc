/* SUTS-011 - SuperUnicode Set Notation (SUSN)
 *
 * Compiled catalog data. This file is included exactly once, by
 * src/suts/suts_susn.c, after suts_susn.h. Every array is sorted ascending by
 * `first`, non-overlapping, and separated by at least one non-member
 * codepoint; the unit tests validate the whole catalog at run time.
 *
 * The ranges are curated reference data, not a Unicode property dump. They
 * exist to give the algebra a stable set of named, well-known operands and to
 * demonstrate the zone and neography integration. A consumer that needs a
 * different cut compiles its own table and wraps it in a
 * susn_set_descriptor_t - the engine treats catalog sets and caller sets
 * identically.
 */

#define SUSN_CAT_FLAG   (SUSN_FLAG_RANGE_ARRAY | SUSN_FLAG_COMPILED)
#define SUSN_CAT_PROP   (SUSN_CAT_FLAG | SUSN_FLAG_PROPERTY_BASED)

/* ── Basic Latin and controls ──────────────────────────────────────── */

static const susn_codepoint_range_t susn_ranges_ascii[] = {
    { 0x00000000u, 0x0000007Fu }
};

static const susn_codepoint_range_t susn_ranges_printable_ascii[] = {
    { 0x00000020u, 0x0000007Eu }
};

static const susn_codepoint_range_t susn_ranges_digit[] = {
    { 0x00000030u, 0x00000039u }
};

static const susn_codepoint_range_t susn_ranges_upper[] = {
    { 0x00000041u, 0x0000005Au }
};

static const susn_codepoint_range_t susn_ranges_lower[] = {
    { 0x00000061u, 0x0000007Au }
};

static const susn_codepoint_range_t susn_ranges_ascii_punct[] = {
    { 0x00000021u, 0x0000002Fu },
    { 0x0000003Au, 0x00000040u },
    { 0x0000005Bu, 0x00000060u },
    { 0x0000007Bu, 0x0000007Eu }
};

static const susn_codepoint_range_t susn_ranges_control[] = {
    { 0x00000000u, 0x0000001Fu },
    { 0x0000007Fu, 0x0000009Fu }
};

/* The White_Space property, in its canonical sparse shape. */
static const susn_codepoint_range_t susn_ranges_whitespace[] = {
    { 0x00000009u, 0x0000000Du },
    { 0x00000020u, 0x00000020u },
    { 0x00000085u, 0x00000085u },
    { 0x000000A0u, 0x000000A0u },
    { 0x00001680u, 0x00001680u },
    { 0x00002000u, 0x0000200Au },
    { 0x00002028u, 0x00002029u },
    { 0x0000202Fu, 0x0000202Fu },
    { 0x0000205Fu, 0x0000205Fu },
    { 0x00003000u, 0x00003000u }
};

/* ── Bridge scripts ────────────────────────────────────────────────── */

static const susn_codepoint_range_t susn_ranges_latin1[] = {
    { 0x000000A0u, 0x000000FFu }
};

static const susn_codepoint_range_t susn_ranges_greek[] = {
    { 0x00000370u, 0x000003FFu },
    { 0x00001F00u, 0x00001FFFu }
};

static const susn_codepoint_range_t susn_ranges_cyrillic[] = {
    { 0x00000400u, 0x0000052Fu },
    { 0x00001C80u, 0x00001C8Fu },
    { 0x00002DE0u, 0x00002DFFu },
    { 0x0000A640u, 0x0000A69Fu }
};

static const susn_codepoint_range_t susn_ranges_hebrew[] = {
    { 0x00000590u, 0x000005FFu },
    { 0x0000FB1Du, 0x0000FB4Fu }
};

static const susn_codepoint_range_t susn_ranges_arabic[] = {
    { 0x00000600u, 0x000006FFu },
    { 0x00000750u, 0x0000077Fu },
    { 0x00000870u, 0x000008FFu },
    { 0x0000FB50u, 0x0000FDFFu },
    { 0x0000FE70u, 0x0000FEFFu },
    { 0x0001EE00u, 0x0001EEFFu }
};

static const susn_codepoint_range_t susn_ranges_hiragana[] = {
    { 0x00003040u, 0x0000309Fu },
    { 0x0001B001u, 0x0001B11Fu }
};

static const susn_codepoint_range_t susn_ranges_katakana[] = {
    { 0x000030A0u, 0x000030FFu },
    { 0x000031F0u, 0x000031FFu },
    { 0x0000FF66u, 0x0000FF9Fu },
    { 0x0001B000u, 0x0001B000u }
};

static const susn_codepoint_range_t susn_ranges_han[] = {
    { 0x00003400u, 0x00004DBFu },
    { 0x00004E00u, 0x00009FFFu },
    { 0x0000F900u, 0x0000FAFFu },
    { 0x00020000u, 0x0002A6DFu },
    { 0x0002A700u, 0x0002EBEFu },
    { 0x00030000u, 0x0003134Fu }
};

static const susn_codepoint_range_t susn_ranges_hangul[] = {
    { 0x00001100u, 0x000011FFu },
    { 0x00003130u, 0x0000318Fu },
    { 0x0000A960u, 0x0000A97Fu },
    { 0x0000AC00u, 0x0000D7AFu },
    { 0x0000FFA0u, 0x0000FFDCu }
};

/* A broad emoji-presentation block set, used by the worked examples. */
static const susn_codepoint_range_t susn_ranges_emoji[] = {
    { 0x00002600u, 0x000027BFu },
    { 0x00002B00u, 0x00002BFFu },
    { 0x0001F000u, 0x0001F1FFu },
    { 0x0001F300u, 0x0001FAFFu }
};

/* The explicit bidi controls, matching SUAS-001 (SDF) section 4. */
static const susn_codepoint_range_t susn_ranges_bidi_control[] = {
    { 0x0000061Cu, 0x0000061Cu },
    { 0x0000200Eu, 0x0000200Fu },
    { 0x0000202Au, 0x0000202Eu },
    { 0x00002066u, 0x00002069u }
};

/* ── Address-space zones ───────────────────────────────────────────── */

static const susn_codepoint_range_t susn_ranges_bridge[] = {
    { 0x00000000u, 0x0010FFFFu }
};

static const susn_codepoint_range_t susn_ranges_scp[] = {
    { 0x00110000u, 0x0011FFFFu }
};

static const susn_codepoint_range_t susn_ranges_rnur[] = {
    { 0x00160000u, 0x0016FFFFu }
};

static const susn_codepoint_range_t susn_ranges_native[] = {
    { 0x00120000u, 0x7FFFFFEFu }
};

static const susn_codepoint_range_t susn_ranges_trap[] = {
    { 0x7FFFFFF0u, 0x7FFFFFFEu }
};

static const susn_codepoint_range_t susn_ranges_sentinel[] = {
    { 0x7FFFFFFFu, 0x7FFFFFFFu }
};

/* ── The curated catalog ───────────────────────────────────────────── */

static const susn_set_descriptor_t susn_catalog[] = {
    { (uint32_t)SUSN_SETID_ASCII,           SUSN_CAT_FLAG, 1u, susn_ranges_ascii },
    { (uint32_t)SUSN_SETID_PRINTABLE_ASCII, SUSN_CAT_PROP, 1u, susn_ranges_printable_ascii },
    { (uint32_t)SUSN_SETID_DIGIT,           SUSN_CAT_PROP, 1u, susn_ranges_digit },
    { (uint32_t)SUSN_SETID_UPPER,           SUSN_CAT_PROP, 1u, susn_ranges_upper },
    { (uint32_t)SUSN_SETID_LOWER,           SUSN_CAT_PROP, 1u, susn_ranges_lower },
    { (uint32_t)SUSN_SETID_ASCII_PUNCT,     SUSN_CAT_PROP, 4u, susn_ranges_ascii_punct },
    { (uint32_t)SUSN_SETID_CONTROL,         SUSN_CAT_PROP, 2u, susn_ranges_control },
    { (uint32_t)SUSN_SETID_WHITESPACE,      SUSN_CAT_PROP, 10u, susn_ranges_whitespace },
    { (uint32_t)SUSN_SETID_LATIN1,          SUSN_CAT_FLAG, 1u, susn_ranges_latin1 },
    { (uint32_t)SUSN_SETID_GREEK,           SUSN_CAT_FLAG, 2u, susn_ranges_greek },
    { (uint32_t)SUSN_SETID_CYRILLIC,        SUSN_CAT_FLAG, 4u, susn_ranges_cyrillic },
    { (uint32_t)SUSN_SETID_HEBREW,          SUSN_CAT_FLAG, 2u, susn_ranges_hebrew },
    { (uint32_t)SUSN_SETID_ARABIC,          SUSN_CAT_FLAG, 6u, susn_ranges_arabic },
    { (uint32_t)SUSN_SETID_HIRAGANA,        SUSN_CAT_FLAG, 2u, susn_ranges_hiragana },
    { (uint32_t)SUSN_SETID_KATAKANA,        SUSN_CAT_FLAG, 4u, susn_ranges_katakana },
    { (uint32_t)SUSN_SETID_HAN,             SUSN_CAT_PROP, 6u, susn_ranges_han },
    { (uint32_t)SUSN_SETID_HANGUL,          SUSN_CAT_FLAG, 5u, susn_ranges_hangul },
    { (uint32_t)SUSN_SETID_EMOJI,           SUSN_CAT_PROP, 4u, susn_ranges_emoji },
    { (uint32_t)SUSN_SETID_BIDI_CONTROL,    SUSN_CAT_PROP, 4u, susn_ranges_bidi_control },
    { (uint32_t)SUSN_SETID_BRIDGE,          SUSN_CAT_FLAG, 1u, susn_ranges_bridge },
    { (uint32_t)SUSN_SETID_SCP,             SUSN_CAT_FLAG, 1u, susn_ranges_scp },
    { (uint32_t)SUSN_SETID_RNUR,            SUSN_CAT_FLAG, 1u, susn_ranges_rnur },
    { (uint32_t)SUSN_SETID_NATIVE,          SUSN_CAT_FLAG, 1u, susn_ranges_native },
    { (uint32_t)SUSN_SETID_TRAP,            SUSN_CAT_FLAG, 1u, susn_ranges_trap },
    { (uint32_t)SUSN_SETID_SENTINEL,        SUSN_CAT_FLAG, 1u, susn_ranges_sentinel }
};

#define SUSN_CATALOG_COUNT \
    ((uint32_t)(sizeof susn_catalog / sizeof susn_catalog[0]))

#undef SUSN_CAT_PROP
#undef SUSN_CAT_FLAG
