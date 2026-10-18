/* SUAS-006 — System Text Segmentation (STS)
 *
 * Reference implementation of docs/suas/SUAS-006-sts.md.
 *
 * Freestanding C99, zero allocation. A single-pass deterministic state
 * machine resolves grapheme-cluster, word, and sentence boundaries in O(1)
 * per codepoint. Input codepoints are 64-bit ExtSUCS (sucs_ex_char_t):
 * Unicode Bridge behaviour follows UAX #29 via curated class predicates;
 * the System Control Plane carries explicit segmentation directives at
 * 0x00110040-0x0011004F; Native/plugin codepoints default to an opaque
 * "other" class.
 *
 * The UAX #29 "ignore" rules (WB4, SB5) and the run-length rules (GB9c,
 * GB11, GB12/13, WB15/16) are encoded in the bounded carried-context
 * register of suas_sts_state_t, so no unbounded history is ever required.
 */

#include "suas/suas_sts.h"

/* ── Small range / membership helpers (curated UAX #29 classes) ───── */

static int sts_in(uint32_t c, uint32_t lo, uint32_t hi)
{
    return c >= lo && c <= hi;
}

/* Grapheme_Extend = Yes, or Emoji_Modifier = Yes (excludes ZWJ). */
static int sts_is_extend(uint32_t c)
{
    return sts_in(c, 0x0300, 0x036F) || sts_in(c, 0x0483, 0x0489) ||
           sts_in(c, 0x0591, 0x05BD) || c == 0x05BF ||
           sts_in(c, 0x05C1, 0x05C2) || sts_in(c, 0x05C4, 0x05C5) ||
           c == 0x05C7 || sts_in(c, 0x0610, 0x061A) ||
           sts_in(c, 0x064B, 0x065F) || c == 0x0670 ||
           sts_in(c, 0x06D6, 0x06DC) || sts_in(c, 0x06DF, 0x06E4) ||
           sts_in(c, 0x06E7, 0x06E8) || sts_in(c, 0x06EA, 0x06ED) ||
           c == 0x0711 || sts_in(c, 0x0730, 0x074A) ||
           sts_in(c, 0x07A6, 0x07B0) || sts_in(c, 0x07EB, 0x07F3) ||
           sts_in(c, 0x0816, 0x0819) || sts_in(c, 0x081B, 0x0823) ||
           sts_in(c, 0x0825, 0x0827) || sts_in(c, 0x0829, 0x082D) ||
           sts_in(c, 0x0859, 0x085B) || sts_in(c, 0x08D3, 0x08E1) ||
           sts_in(c, 0x08E3, 0x0902) || c == 0x093A || c == 0x093C ||
           sts_in(c, 0x0941, 0x0948) || c == 0x094D ||
           sts_in(c, 0x0951, 0x0957) || sts_in(c, 0x0962, 0x0963) ||
           c == 0x0981 || c == 0x09BC || sts_in(c, 0x09C1, 0x09C4) ||
           c == 0x09CD || sts_in(c, 0x09E2, 0x09E3) || c == 0x09FE ||
           sts_in(c, 0x0A01, 0x0A02) || c == 0x0A3C ||
           sts_in(c, 0x0A41, 0x0A42) || sts_in(c, 0x0A47, 0x0A48) ||
           sts_in(c, 0x0A4B, 0x0A4D) || c == 0x0A51 ||
           sts_in(c, 0x0A70, 0x0A71) || c == 0x0A75 ||
           sts_in(c, 0x0A81, 0x0A82) || c == 0x0ABC ||
           sts_in(c, 0x0AC1, 0x0AC5) || sts_in(c, 0x0AC7, 0x0AC8) ||
           c == 0x0ACD || sts_in(c, 0x0AE2, 0x0AE3) || c == 0x0B01 ||
           c == 0x0B3C || c == 0x0B3F || sts_in(c, 0x0B41, 0x0B44) ||
           c == 0x0B4D || sts_in(c, 0x0B55, 0x0B56) ||
           sts_in(c, 0x0B62, 0x0B63) || c == 0x0B82 || c == 0x0BC0 ||
           c == 0x0BCD || c == 0x0C00 || c == 0x0C04 ||
           sts_in(c, 0x0C3C, 0x0C40) || sts_in(c, 0x0C46, 0x0C48) ||
           sts_in(c, 0x0C4A, 0x0C4D) || sts_in(c, 0x0C55, 0x0C56) ||
           sts_in(c, 0x0C62, 0x0C63) || c == 0x0C81 ||
           sts_in(c, 0x0CBC, 0x0CCD) || sts_in(c, 0x0CE2, 0x0CE3) ||
           sts_in(c, 0x0D00, 0x0D01) || sts_in(c, 0x0D3B, 0x0D3C) ||
           sts_in(c, 0x0D41, 0x0D44) || c == 0x0D4D ||
           sts_in(c, 0x0D62, 0x0D63) || c == 0x0D81 ||
           sts_in(c, 0x0DCA, 0x0DCA) || sts_in(c, 0x0DD2, 0x0DD4) ||
           c == 0x0DD6 || c == 0x0E31 || sts_in(c, 0x0E34, 0x0E3A) ||
           sts_in(c, 0x0E47, 0x0E4E) || c == 0x0EB1 ||
           sts_in(c, 0x0EB4, 0x0EBC) || sts_in(c, 0x0EC8, 0x0ECD) ||
           sts_in(c, 0x0F18, 0x0F19) || c == 0x0F35 || c == 0x0F37 ||
           c == 0x0F39 || sts_in(c, 0x0F71, 0x0F7E) ||
           sts_in(c, 0x0F80, 0x0F84) || sts_in(c, 0x0F86, 0x0F87) ||
           sts_in(c, 0x0F8D, 0x0F97) || sts_in(c, 0x0F99, 0x0FBC) ||
           c == 0x0FC6 || sts_in(c, 0x102D, 0x1030) ||
           sts_in(c, 0x1032, 0x1037) || sts_in(c, 0x1039, 0x103A) ||
           sts_in(c, 0x103D, 0x103E) || sts_in(c, 0x1058, 0x1059) ||
           sts_in(c, 0x105E, 0x1060) || sts_in(c, 0x1071, 0x1074) ||
           c == 0x1082 || sts_in(c, 0x1085, 0x1086) || c == 0x108D ||
           c == 0x109D || sts_in(c, 0x135D, 0x135F) ||
           sts_in(c, 0x1712, 0x1714) || sts_in(c, 0x1732, 0x1734) ||
           sts_in(c, 0x1752, 0x1753) || sts_in(c, 0x1772, 0x1773) ||
           sts_in(c, 0x17B4, 0x17B5) || sts_in(c, 0x17B7, 0x17BD) ||
           c == 0x17C6 || sts_in(c, 0x17C9, 0x17D3) || c == 0x17DD ||
           sts_in(c, 0x180B, 0x180D) || sts_in(c, 0x1885, 0x1886) ||
           c == 0x18A9 || sts_in(c, 0x1920, 0x1922) ||
           sts_in(c, 0x1927, 0x1928) || c == 0x1932 ||
           sts_in(c, 0x1939, 0x193B) || sts_in(c, 0x1A17, 0x1A18) ||
           c == 0x1A1B || c == 0x1A56 || sts_in(c, 0x1A58, 0x1A5E) ||
           c == 0x1A60 || c == 0x1A62 || sts_in(c, 0x1A65, 0x1A6C) ||
           sts_in(c, 0x1A73, 0x1A7C) || c == 0x1A7F ||
           sts_in(c, 0x1AB0, 0x1AFF) || sts_in(c, 0x1B00, 0x1B03) ||
           c == 0x1B34 || sts_in(c, 0x1B36, 0x1B3A) || c == 0x1B3C ||
           c == 0x1B42 || sts_in(c, 0x1B6B, 0x1B73) ||
           sts_in(c, 0x1B80, 0x1B81) || sts_in(c, 0x1BA2, 0x1BA5) ||
           sts_in(c, 0x1BA8, 0x1BA9) || sts_in(c, 0x1BAB, 0x1BAD) ||
           c == 0x1BE6 || sts_in(c, 0x1BE8, 0x1BE9) || c == 0x1BED ||
           sts_in(c, 0x1BEF, 0x1BF1) || sts_in(c, 0x1C2C, 0x1C33) ||
           sts_in(c, 0x1C36, 0x1C37) || sts_in(c, 0x1CD0, 0x1CD2) ||
           sts_in(c, 0x1CD4, 0x1CE0) || sts_in(c, 0x1CE2, 0x1CE8) ||
           c == 0x1CED || c == 0x1CF4 || sts_in(c, 0x1CF8, 0x1CF9) ||
           sts_in(c, 0x1DC0, 0x1DFF) || sts_in(c, 0x20D0, 0x20F0) ||
           c == 0x200C || sts_in(c, 0x2CEF, 0x2CF1) || c == 0x2D7F ||
           sts_in(c, 0x2DE0, 0x2DFF) || sts_in(c, 0x302A, 0x302F) ||
           sts_in(c, 0x3099, 0x309A) || c == 0xA66F ||
           sts_in(c, 0xA674, 0xA67D) || sts_in(c, 0xA69E, 0xA69F) ||
           sts_in(c, 0xA6F0, 0xA6F1) || c == 0xA802 || c == 0xA806 ||
           c == 0xA80B || sts_in(c, 0xA825, 0xA826) || c == 0xA82C ||
           sts_in(c, 0xA8C4, 0xA8C5) || sts_in(c, 0xA8E0, 0xA8F1) ||
           c == 0xA8FF || sts_in(c, 0xA926, 0xA92D) ||
           sts_in(c, 0xA947, 0xA951) || sts_in(c, 0xA980, 0xA982) ||
           c == 0xA9B3 || sts_in(c, 0xA9B6, 0xA9B9) ||
           sts_in(c, 0xA9BC, 0xA9BD) || c == 0xA9E5 ||
           sts_in(c, 0xAA29, 0xAA2E) || sts_in(c, 0xAA31, 0xAA32) ||
           sts_in(c, 0xAA35, 0xAA36) || c == 0xAA43 || c == 0xAA4C ||
           c == 0xAA7C || c == 0xAAB0 || sts_in(c, 0xAAB2, 0xAAB4) ||
           sts_in(c, 0xAAB7, 0xAAB8) || sts_in(c, 0xAABE, 0xAABF) ||
           c == 0xAAC1 || sts_in(c, 0xAAEC, 0xAAED) || c == 0xAAF6 ||
           c == 0xABE5 || c == 0xABE8 || c == 0xABED ||
           c == 0xFB1E || sts_in(c, 0xFE00, 0xFE0F) ||
           sts_in(c, 0xFE20, 0xFE2F) || sts_in(c, 0x101FD, 0x101FD) ||
           c == 0x102E0 || sts_in(c, 0x10376, 0x1037A) ||
           c == 0x10A01 || sts_in(c, 0x10A03, 0x10A03) ||
           sts_in(c, 0x10A05, 0x10A06) || sts_in(c, 0x10A0C, 0x10A0F) ||
           sts_in(c, 0x10A38, 0x10A3A) || c == 0x10A3F ||
           sts_in(c, 0x10AE5, 0x10AE6) || c == 0x10D24 ||
           sts_in(c, 0x10D69, 0x10D6D) || sts_in(c, 0x10EAB, 0x10EAC) ||
           sts_in(c, 0x11001, 0x11001) || sts_in(c, 0x11038, 0x11046) ||
           c == 0x11070 || sts_in(c, 0x11073, 0x11074) ||
           sts_in(c, 0x1107F, 0x11081) || sts_in(c, 0x110B3, 0x110B6) ||
           sts_in(c, 0x110B9, 0x110BA) || c == 0x110C2 ||
           sts_in(c, 0x11100, 0x11102) || sts_in(c, 0x11127, 0x1112B) ||
           sts_in(c, 0x1112D, 0x11134) || c == 0x11173 ||
           sts_in(c, 0x11180, 0x11181) || sts_in(c, 0x111B6, 0x111BE) ||
           sts_in(c, 0x111C9, 0x111CC) || c == 0x111CF ||
           sts_in(c, 0x1122F, 0x11231) || c == 0x11234 ||
           sts_in(c, 0x11236, 0x11237) || c == 0x1123E ||
           c == 0x11241 || sts_in(c, 0x112DF, 0x112DF) ||
           sts_in(c, 0x112E3, 0x112EA) || sts_in(c, 0x11300, 0x11301) ||
           sts_in(c, 0x1133B, 0x1133C) || c == 0x11340 ||
           sts_in(c, 0x11366, 0x1136C) || sts_in(c, 0x11370, 0x11374) ||
           sts_in(c, 0x11438, 0x1143F) || sts_in(c, 0x11442, 0x11444) ||
           c == 0x11446 || c == 0x1145E || sts_in(c, 0x114B3, 0x114B8) ||
           c == 0x114BA || sts_in(c, 0x114BF, 0x114C0) ||
           sts_in(c, 0x114C2, 0x114C3) || sts_in(c, 0x115B2, 0x115B5) ||
           sts_in(c, 0x115BC, 0x115BD) || sts_in(c, 0x115BF, 0x115C0) ||
           sts_in(c, 0x115DC, 0x115DD) || sts_in(c, 0x11633, 0x1163A) ||
           c == 0x1163D || sts_in(c, 0x1163F, 0x11640) || c == 0x116AB ||
           c == 0x116AD || sts_in(c, 0x116B0, 0x116B5) || c == 0x116B7 ||
           sts_in(c, 0x1171D, 0x1171F) || sts_in(c, 0x11722, 0x11725) ||
           sts_in(c, 0x11727, 0x1172B) || sts_in(c, 0x1182F, 0x11837) ||
           sts_in(c, 0x11839, 0x1183A) || c == 0x1193B ||
           sts_in(c, 0x1193C, 0x1193C) || c == 0x1193E || c == 0x11943 ||
           sts_in(c, 0x119D4, 0x119D7) || sts_in(c, 0x119DA, 0x119DB) ||
           c == 0x119E0 || sts_in(c, 0x11A01, 0x11A0A) ||
           sts_in(c, 0x11A33, 0x11A38) || sts_in(c, 0x11A3B, 0x11A3E) ||
           c == 0x11A47 || sts_in(c, 0x11A51, 0x11A56) ||
           sts_in(c, 0x11A59, 0x11A5B) || sts_in(c, 0x11A8A, 0x11A96) ||
           sts_in(c, 0x11A98, 0x11A99) || sts_in(c, 0x11C30, 0x11C36) ||
           sts_in(c, 0x11C38, 0x11C3D) || c == 0x11C3F ||
           sts_in(c, 0x11C92, 0x11CA7) || sts_in(c, 0x11CAA, 0x11CB0) ||
           sts_in(c, 0x11CB2, 0x11CB3) || sts_in(c, 0x11CB5, 0x11CB6) ||
           sts_in(c, 0x11D31, 0x11D36) || c == 0x11D3A ||
           sts_in(c, 0x11D3C, 0x11D3D) || sts_in(c, 0x11D3F, 0x11D45) ||
           c == 0x11D47 || sts_in(c, 0x11D90, 0x11D91) || c == 0x11D95 ||
           c == 0x11D97 || c == 0x11EF3 || sts_in(c, 0x11F00, 0x11F01) ||
           c == 0x11F36 || sts_in(c, 0x11F40, 0x11F42) ||
           sts_in(c, 0x13440, 0x13440) || sts_in(c, 0x13447, 0x13455) ||
           sts_in(c, 0x16AF0, 0x16AF4) || sts_in(c, 0x16B30, 0x16B36) ||
           sts_in(c, 0x16F4F, 0x16F4F) || sts_in(c, 0x16F8F, 0x16F92) ||
           sts_in(c, 0x16FE4, 0x16FE4) ||
           sts_in(c, 0x1BC9D, 0x1BC9E) ||
           sts_in(c, 0x1CF00, 0x1CF2D) || sts_in(c, 0x1CF30, 0x1CF46) ||
           sts_in(c, 0x1D165, 0x1D169) || sts_in(c, 0x1D16D, 0x1D172) ||
           sts_in(c, 0x1D17B, 0x1D182) || sts_in(c, 0x1D185, 0x1D18B) ||
           sts_in(c, 0x1D1AA, 0x1D1AD) || sts_in(c, 0x1D242, 0x1D244) ||
           sts_in(c, 0x1DA00, 0x1DA36) || sts_in(c, 0x1DA3B, 0x1DA6C) ||
           c == 0x1DA75 || c == 0x1DA84 ||
           sts_in(c, 0x1DA9B, 0x1DA9F) || sts_in(c, 0x1DAA1, 0x1DAAF) ||
           sts_in(c, 0x1E000, 0x1E006) || sts_in(c, 0x1E008, 0x1E018) ||
           sts_in(c, 0x1E01B, 0x1E021) || sts_in(c, 0x1E023, 0x1E024) ||
           sts_in(c, 0x1E026, 0x1E02A) || sts_in(c, 0x1E08F, 0x1E08F) ||
           sts_in(c, 0x1E130, 0x1E136) || sts_in(c, 0x1E2AE, 0x1E2AE) ||
           sts_in(c, 0x1E2EC, 0x1E2EF) || sts_in(c, 0x1E4EC, 0x1E4EF) ||
           sts_in(c, 0x1E5EE, 0x1E5EF) || sts_in(c, 0x1E8D0, 0x1E8D6) ||
           sts_in(c, 0x1E944, 0x1E94A) || sts_in(c, 0x1F3FB, 0x1F3FF) ||
           sts_in(c, 0xE0020, 0xE007F) || sts_in(c, 0xE0100, 0xE01EF);
}

/* Grapheme_Cluster_Break = SpacingMark (curated). */
static int sts_is_spacingmark(uint32_t c)
{
    return c == 0x0903 || c == 0x093B || sts_in(c, 0x093E, 0x0940) ||
           sts_in(c, 0x0949, 0x094C) || sts_in(c, 0x094E, 0x094F) ||
           sts_in(c, 0x0982, 0x0983) || sts_in(c, 0x09BF, 0x09C0) ||
           sts_in(c, 0x09C7, 0x09C8) || sts_in(c, 0x09CB, 0x09CC) ||
           c == 0x0A03 || sts_in(c, 0x0A3E, 0x0A40) || c == 0x0A83 ||
           sts_in(c, 0x0ABE, 0x0AC0) || c == 0x0AC9 ||
           sts_in(c, 0x0ACB, 0x0ACC) || sts_in(c, 0x0B02, 0x0B03) ||
           c == 0x0B3E || c == 0x0B40 || sts_in(c, 0x0B47, 0x0B48) ||
           sts_in(c, 0x0B4B, 0x0B4C) || sts_in(c, 0x0BBE, 0x0BBF) ||
           sts_in(c, 0x0BC1, 0x0BC2) || sts_in(c, 0x0BC6, 0x0BC8) ||
           sts_in(c, 0x0BCA, 0x0BCC) || c == 0x0BD7 ||
           sts_in(c, 0x0C01, 0x0C03) || sts_in(c, 0x0C41, 0x0C44) ||
           sts_in(c, 0x0C82, 0x0C83) || c == 0x0CBE ||
           sts_in(c, 0x0CC0, 0x0CC4) || sts_in(c, 0x0CC7, 0x0CC8) ||
           sts_in(c, 0x0CCA, 0x0CCB) || sts_in(c, 0x0CD5, 0x0CD6) ||
           sts_in(c, 0x0D02, 0x0D03) || sts_in(c, 0x0D3E, 0x0D40) ||
           sts_in(c, 0x0D46, 0x0D48) || sts_in(c, 0x0D4A, 0x0D4C) ||
           c == 0x0D57 || sts_in(c, 0x0D82, 0x0D83) ||
           sts_in(c, 0x0DCF, 0x0DD1) || sts_in(c, 0x0DD8, 0x0DDF) ||
           sts_in(c, 0x0DF2, 0x0DF3) || c == 0x0E33 || c == 0x0EB3 ||
           sts_in(c, 0x0F3E, 0x0F3F) || sts_in(c, 0x102B, 0x102C) ||
           c == 0x1031 || c == 0x1038 || sts_in(c, 0x103B, 0x103C) ||
           sts_in(c, 0x1056, 0x1057) || sts_in(c, 0x1062, 0x1064) ||
           sts_in(c, 0x1067, 0x106D) || sts_in(c, 0x1083, 0x1084) ||
           sts_in(c, 0x1087, 0x108C) || c == 0x108F ||
           sts_in(c, 0x109A, 0x109C) || c == 0x17B6 ||
           sts_in(c, 0x17BE, 0x17C5) || sts_in(c, 0x17C7, 0x17C8) ||
           sts_in(c, 0x1923, 0x1926) || sts_in(c, 0x1929, 0x192B) ||
           sts_in(c, 0x1930, 0x1931) || sts_in(c, 0x1933, 0x1938) ||
           sts_in(c, 0x1A19, 0x1A1A) || c == 0x1A55 || c == 0x1A57 ||
           c == 0x1A61 || sts_in(c, 0x1A63, 0x1A64) ||
           sts_in(c, 0x1A6D, 0x1A72) || c == 0x1B04 || c == 0x1B35 ||
           c == 0x1B3B || sts_in(c, 0x1B3D, 0x1B41) ||
           sts_in(c, 0x1B43, 0x1B44) || c == 0x1B82 || c == 0x1BA1 ||
           sts_in(c, 0x1BA6, 0x1BA7) || c == 0x1BAA || c == 0x1BE7 ||
           sts_in(c, 0x1BEA, 0x1BEC) || c == 0x1BEE ||
           sts_in(c, 0x1BF2, 0x1BF3) || sts_in(c, 0x1C24, 0x1C2B) ||
           sts_in(c, 0x1C34, 0x1C35) || c == 0x1CE1 || c == 0x1CF7 ||
           sts_in(c, 0x302E, 0x302F) || sts_in(c, 0xA823, 0xA824) ||
           c == 0xA827 || sts_in(c, 0xA880, 0xA881) ||
           sts_in(c, 0xA8B4, 0xA8C3) || c == 0xA952 || c == 0xA953 ||
           sts_in(c, 0xA983, 0xA983) || c == 0xA9B4 || c == 0xA9B5 ||
           sts_in(c, 0xA9BA, 0xA9BB) || sts_in(c, 0xA9BE, 0xA9C0) ||
           c == 0xAA2F || c == 0xAA30 || sts_in(c, 0xAA33, 0xAA34) ||
           c == 0xAA4D || sts_in(c, 0xAAEB, 0xAAEB) ||
           sts_in(c, 0xAAEE, 0xAAEF) || c == 0xAAF5 ||
           sts_in(c, 0xABE3, 0xABE4) || c == 0xABE6 || c == 0xABE7 ||
           c == 0xABE9 || c == 0xABEA || c == 0xABEC ||
           sts_in(c, 0x11000, 0x11000) || sts_in(c, 0x11002, 0x11002) ||
           sts_in(c, 0x11082, 0x11082) || sts_in(c, 0x110B0, 0x110B2) ||
           sts_in(c, 0x110B7, 0x110B8) || sts_in(c, 0x1112C, 0x1112C) ||
           sts_in(c, 0x11145, 0x11146) || sts_in(c, 0x11182, 0x11182) ||
           sts_in(c, 0x111B3, 0x111B5) || sts_in(c, 0x111BF, 0x111C0) ||
           sts_in(c, 0x111CE, 0x111CE) || sts_in(c, 0x1122C, 0x1122E) ||
           c == 0x11232 || c == 0x11233 || c == 0x11235 ||
           sts_in(c, 0x112E0, 0x112E2) || sts_in(c, 0x11302, 0x11303) ||
           c == 0x1133E || c == 0x1133F || sts_in(c, 0x11341, 0x11344) ||
           sts_in(c, 0x11347, 0x11348) || sts_in(c, 0x1134B, 0x1134D) ||
           c == 0x11357 || sts_in(c, 0x11362, 0x11363) ||
           sts_in(c, 0x11435, 0x11437) || sts_in(c, 0x11440, 0x11441) ||
           c == 0x11445 || sts_in(c, 0x114B0, 0x114B2) ||
           c == 0x114B9 || sts_in(c, 0x114BB, 0x114BE) ||
           sts_in(c, 0x114C1, 0x114C1) || sts_in(c, 0x115AF, 0x115B1) ||
           sts_in(c, 0x115B8, 0x115BB) || sts_in(c, 0x115BE, 0x115BE) ||
           sts_in(c, 0x11630, 0x11632) || sts_in(c, 0x1163B, 0x1163C) ||
           c == 0x1163E || c == 0x116AC || c == 0x116AE || c == 0x116AF ||
           c == 0x116B6 || sts_in(c, 0x11720, 0x11721) ||
           c == 0x11726 || sts_in(c, 0x1182C, 0x1182E) || c == 0x11838 ||
           c == 0x11930 || sts_in(c, 0x119D1, 0x119D3) ||
           sts_in(c, 0x119DC, 0x119DF) || c == 0x119E4 ||
           sts_in(c, 0x11A39, 0x11A39) || sts_in(c, 0x11A57, 0x11A58) ||
           c == 0x11A97 || c == 0x11C2F || sts_in(c, 0x11C3E, 0x11C3E) ||
           sts_in(c, 0x11CA9, 0x11CA9) || sts_in(c, 0x11CB1, 0x11CB1) ||
           sts_in(c, 0x11CB4, 0x11CB4) || sts_in(c, 0x11D8A, 0x11D8E) ||
           sts_in(c, 0x11D93, 0x11D94) || c == 0x11D96 ||
           sts_in(c, 0x11EF5, 0x11EF6) || c == 0x11F03 || c == 0x11F34 ||
           c == 0x11F35 || sts_in(c, 0x11F3E, 0x11F3F) ||
           c == 0x16F51 || sts_in(c, 0x16FF0, 0x16FF1) ||
           c == 0x1D166 || c == 0x1D16D;
}

/* Grapheme_Cluster_Break = Prepend (curated). */
static int sts_is_prepend(uint32_t c)
{
    return sts_in(c, 0x0600, 0x0605) || c == 0x06DD || c == 0x070F ||
           sts_in(c, 0x0890, 0x0891) || c == 0x08E2 || c == 0x0D4E ||
           c == 0x110BD || c == 0x110CD ||
           sts_in(c, 0x111C2, 0x111C3) || c == 0x1193F || c == 0x11941 ||
           c == 0x11A3A || sts_in(c, 0x11A84, 0x11A89) || c == 0x11D46;
}

/* Word_Break / Sentence_Break = Format (Cf minus ZWNJ/ZWJ). */
static int sts_is_format(uint32_t c)
{
    return c == 0x00AD || c == 0x061C || c == 0x180E || c == 0x200B ||
           sts_in(c, 0x200E, 0x200F) || sts_in(c, 0x202A, 0x202E) ||
           sts_in(c, 0x2060, 0x2064) || sts_in(c, 0x2066, 0x206F) ||
           c == 0xFEFF || sts_in(c, 0xFFF9, 0xFFFB) ||
           sts_in(c, 0x110BD, 0x110BD) || c == 0x13430;
}

/* Numeric (decimal digits), shared by Word_Break and Sentence_Break. */
static int sts_is_numeric(uint32_t c)
{
    return sts_in(c, 0x0030, 0x0039) || sts_in(c, 0x0660, 0x0669) ||
           sts_in(c, 0x06F0, 0x06F9) || sts_in(c, 0x07C0, 0x07C9) ||
           sts_in(c, 0x0966, 0x096F) || sts_in(c, 0x09E6, 0x09EF) ||
           sts_in(c, 0x0A66, 0x0A6F) || sts_in(c, 0x0AE6, 0x0AEF) ||
           sts_in(c, 0x0B66, 0x0B6F) || sts_in(c, 0x0BE6, 0x0BEF) ||
           sts_in(c, 0x0C66, 0x0C6F) || sts_in(c, 0x0CE6, 0x0CEF) ||
           sts_in(c, 0x0D66, 0x0D6F) || sts_in(c, 0x0E50, 0x0E59) ||
           sts_in(c, 0x0ED0, 0x0ED9) || sts_in(c, 0x0F20, 0x0F29) ||
           sts_in(c, 0x1040, 0x1049) || sts_in(c, 0x1090, 0x1099) ||
           sts_in(c, 0x17E0, 0x17E9) || sts_in(c, 0x1810, 0x1819) ||
           sts_in(c, 0xFF10, 0xFF19);
}

/* ExtendNumLet = connector punctuation (Word_Break). */
static int sts_is_extendnumlet(uint32_t c)
{
    return c == 0x005F || c == 0x202F || sts_in(c, 0x203F, 0x2040) ||
           c == 0x2054 || sts_in(c, 0xFE33, 0xFE34) ||
           sts_in(c, 0xFE4D, 0xFE4F) || c == 0xFF3F;
}

/* WSegSpace (Word_Break): space separators excluding NBSP/NNBSP/glue. */
static int sts_is_wsegspace(uint32_t c)
{
    return c == 0x0020 || c == 0x1680 || sts_in(c, 0x2000, 0x2006) ||
           sts_in(c, 0x2008, 0x200A) || c == 0x205F || c == 0x3000;
}

/* Alphabetic letters eligible for Word_Break = ALetter (curated). */
static int sts_is_alpha(uint32_t c)
{
    return sts_in(c, 0x0041, 0x005A) || sts_in(c, 0x0061, 0x007A) ||
           c == 0x00AA || c == 0x00B5 || c == 0x00BA ||
           sts_in(c, 0x00C0, 0x00D6) || sts_in(c, 0x00D8, 0x00F6) ||
           sts_in(c, 0x00F8, 0x02C1) || sts_in(c, 0x0370, 0x0374) ||
           sts_in(c, 0x0376, 0x0377) || sts_in(c, 0x037A, 0x037D) ||
           c == 0x037F || c == 0x0386 || sts_in(c, 0x0388, 0x038A) ||
           c == 0x038C || sts_in(c, 0x038E, 0x03A1) ||
           sts_in(c, 0x03A3, 0x03F5) || sts_in(c, 0x03F7, 0x0481) ||
           sts_in(c, 0x048A, 0x052F) || sts_in(c, 0x0531, 0x0556) ||
           sts_in(c, 0x0560, 0x0588) || sts_in(c, 0x05D0, 0x05EA) ||
           sts_in(c, 0x05EF, 0x05F2) || sts_in(c, 0x0620, 0x064A) ||
           sts_in(c, 0x066E, 0x066F) || sts_in(c, 0x0671, 0x06D3) ||
           c == 0x06D5 || sts_in(c, 0x06E5, 0x06E6) ||
           sts_in(c, 0x06EE, 0x06EF) || sts_in(c, 0x06FA, 0x06FC) ||
           c == 0x06FF || c == 0x0710 || sts_in(c, 0x0712, 0x072F) ||
           sts_in(c, 0x074D, 0x07A5) || c == 0x07B1 ||
           sts_in(c, 0x0904, 0x0939) || c == 0x093D || c == 0x0950 ||
           sts_in(c, 0x0958, 0x0961) || sts_in(c, 0x0971, 0x0980) ||
           sts_in(c, 0x0985, 0x098C) || sts_in(c, 0x098F, 0x0990) ||
           sts_in(c, 0x0993, 0x09A8) || sts_in(c, 0x09AA, 0x09B0) ||
           c == 0x09B2 || sts_in(c, 0x09B6, 0x09B9) || c == 0x09BD ||
           sts_in(c, 0x09DC, 0x09DD) || sts_in(c, 0x09DF, 0x09E1) ||
           sts_in(c, 0x09F0, 0x09F1) || sts_in(c, 0x0A05, 0x0A0A) ||
           sts_in(c, 0x0A0F, 0x0A10) || sts_in(c, 0x0A13, 0x0A28) ||
           sts_in(c, 0x0A2A, 0x0A30) || sts_in(c, 0x0A32, 0x0A33) ||
           sts_in(c, 0x0A35, 0x0A36) || sts_in(c, 0x0A38, 0x0A39) ||
           sts_in(c, 0x0A59, 0x0A5C) || c == 0x0A5E ||
           sts_in(c, 0x0A72, 0x0A74) || sts_in(c, 0x0A85, 0x0A8D) ||
           sts_in(c, 0x0A8F, 0x0A91) || sts_in(c, 0x0A93, 0x0AA8) ||
           sts_in(c, 0x0AAA, 0x0AB0) || sts_in(c, 0x0AB2, 0x0AB3) ||
           sts_in(c, 0x0AB5, 0x0AB9) || c == 0x0ABD || c == 0x0AD0 ||
           sts_in(c, 0x0B05, 0x0B0C) || sts_in(c, 0x0B0F, 0x0B10) ||
           sts_in(c, 0x0B13, 0x0B28) || sts_in(c, 0x0B2A, 0x0B30) ||
           sts_in(c, 0x0B32, 0x0B33) || sts_in(c, 0x0B35, 0x0B39) ||
           c == 0x0B3D || sts_in(c, 0x0B5C, 0x0B5D) ||
           sts_in(c, 0x0B5F, 0x0B61) || c == 0x0B71 ||
           sts_in(c, 0x0B85, 0x0B8A) || sts_in(c, 0x0B8E, 0x0B90) ||
           sts_in(c, 0x0B92, 0x0B95) || sts_in(c, 0x0B99, 0x0B9A) ||
           c == 0x0B9C || sts_in(c, 0x0B9E, 0x0B9F) ||
           sts_in(c, 0x0BA3, 0x0BA4) || sts_in(c, 0x0BA8, 0x0BAA) ||
           sts_in(c, 0x0BAE, 0x0BB9) || c == 0x0BD0 ||
           sts_in(c, 0x0C05, 0x0C0C) || sts_in(c, 0x0C0E, 0x0C10) ||
           sts_in(c, 0x0C12, 0x0C28) || sts_in(c, 0x0C2A, 0x0C39) ||
           c == 0x0C3D || sts_in(c, 0x0C58, 0x0C5A) ||
           sts_in(c, 0x0C60, 0x0C61) || sts_in(c, 0x0C85, 0x0C8C) ||
           sts_in(c, 0x0C8E, 0x0C90) || sts_in(c, 0x0C92, 0x0CA8) ||
           sts_in(c, 0x0CAA, 0x0CB3) || sts_in(c, 0x0CB5, 0x0CB9) ||
           c == 0x0CBD || c == 0x0CDE || sts_in(c, 0x0CE0, 0x0CE1) ||
           sts_in(c, 0x0D05, 0x0D0C) || sts_in(c, 0x0D0E, 0x0D10) ||
           sts_in(c, 0x0D12, 0x0D3A) || c == 0x0D3D || c == 0x0D4E ||
           sts_in(c, 0x0D54, 0x0D56) || sts_in(c, 0x0D5F, 0x0D61) ||
           sts_in(c, 0x0D7A, 0x0D7F) || sts_in(c, 0x0D85, 0x0D96) ||
           sts_in(c, 0x0D9A, 0x0DB1) || sts_in(c, 0x0DB3, 0x0DBB) ||
           c == 0x0DBD || sts_in(c, 0x0DC0, 0x0DC6) ||
           sts_in(c, 0x0F40, 0x0F47) || sts_in(c, 0x0F49, 0x0F6C) ||
           sts_in(c, 0x0F88, 0x0F8C) || sts_in(c, 0x10A0, 0x10C5) ||
           c == 0x10C7 || c == 0x10CD || sts_in(c, 0x10D0, 0x10FA) ||
           sts_in(c, 0x10FC, 0x10FF) || sts_in(c, 0x1E00, 0x1FFF) ||
           sts_in(c, 0x2C60, 0x2C7F) || sts_in(c, 0xA640, 0xA69F) ||
           sts_in(c, 0xA720, 0xA7FF) || sts_in(c, 0xFB00, 0xFB06) ||
           sts_in(c, 0xFB13, 0xFB17) || sts_in(c, 0xFF21, 0xFF3A) ||
           sts_in(c, 0xFF41, 0xFF5A);
}

/* Uppercase letters (Sentence_Break = Upper, curated). */
static int sts_is_upper(uint32_t c)
{
    return sts_in(c, 0x0041, 0x005A) || sts_in(c, 0x00C0, 0x00D6) ||
           sts_in(c, 0x00D8, 0x00DE) || sts_in(c, 0x0391, 0x03A1) ||
           sts_in(c, 0x03A3, 0x03AB) || sts_in(c, 0x0400, 0x042F) ||
           sts_in(c, 0x0531, 0x0556) || sts_in(c, 0x10A0, 0x10C5) ||
           sts_in(c, 0xFF21, 0xFF3A);
}

/* Lowercase letters (Sentence_Break = Lower, curated). */
static int sts_is_lower(uint32_t c)
{
    return sts_in(c, 0x0061, 0x007A) || sts_in(c, 0x00DF, 0x00F6) ||
           sts_in(c, 0x00F8, 0x00FF) || sts_in(c, 0x03B1, 0x03C9) ||
           sts_in(c, 0x03AC, 0x03CE) || sts_in(c, 0x0430, 0x045F) ||
           sts_in(c, 0x0560, 0x0588) || sts_in(c, 0x10D0, 0x10FA) ||
           sts_in(c, 0xFF41, 0xFF5A);
}

/* Hebrew_Letter (Word_Break). */
static int sts_is_hebrew(uint32_t c)
{
    return sts_in(c, 0x05D0, 0x05EA) || sts_in(c, 0x05EF, 0x05F2) ||
           sts_in(c, 0xFB1D, 0xFB4F);
}

/* Katakana (Word_Break). */
static int sts_is_katakana(uint32_t c)
{
    return sts_in(c, 0x3031, 0x3035) || sts_in(c, 0x309B, 0x309C) ||
           sts_in(c, 0x30A0, 0x30FA) || sts_in(c, 0x30FC, 0x30FF) ||
           sts_in(c, 0x31F0, 0x31FF) || sts_in(c, 0x32D0, 0x32FE) ||
           sts_in(c, 0x3300, 0x3357) || sts_in(c, 0xFF66, 0xFF9D) ||
           c == 0x1B000;
}

/* Ideographic / other-letter (Sentence_Break = OLetter, curated). */
static int sts_is_oleter(uint32_t c)
{
    if ((sts_is_alpha(c) && !sts_is_upper(c) && !sts_is_lower(c)))
        return 1;
    return sts_in(c, 0x3041, 0x3096) || sts_in(c, 0x30A1, 0x30FA) ||
           sts_in(c, 0x3400, 0x4DBF) || sts_in(c, 0x4E00, 0x9FFF) ||
           sts_in(c, 0xAC00, 0xD7A3) || sts_in(c, 0x1100, 0x11FF) ||
           sts_in(c, 0x3130, 0x318F) || sts_in(c, 0x0E01, 0x0E30) ||
           sts_in(c, 0x0E40, 0x0E46) || sts_in(c, 0xF900, 0xFAFF) ||
           sts_in(c, 0x20000, 0x2FA1F);
}

/* Extended_Pictographic (curated emoji set). */
static int sts_is_extpict(uint32_t c)
{
    if (c == 0x00A9 || c == 0x00AE || c == 0x203C || c == 0x2049 ||
        c == 0x2122 || c == 0x2139)
        return 1;
    if (sts_in(c, 0x2194, 0x2199) || sts_in(c, 0x21A9, 0x21AA) ||
        sts_in(c, 0x231A, 0x231B) || c == 0x2328 ||
        sts_in(c, 0x23CF, 0x23CF) || sts_in(c, 0x23E9, 0x23F3) ||
        sts_in(c, 0x23F8, 0x23FA) || c == 0x24C2 ||
        sts_in(c, 0x25AA, 0x25AB) || c == 0x25B6 || c == 0x25C0 ||
        sts_in(c, 0x25FB, 0x25FE) || sts_in(c, 0x2600, 0x27BF) ||
        sts_in(c, 0x2934, 0x2935) || sts_in(c, 0x2B05, 0x2B07) ||
        sts_in(c, 0x2B1B, 0x2B1C) || c == 0x2B50 || c == 0x2B55 ||
        c == 0x3030 || c == 0x303D || c == 0x3297 || c == 0x3299)
        return 1;
    return sts_in(c, 0x1F000, 0x1FAFF);
}

/* Sentence_Break = ATerm (ambiguous terminator). */
static int sts_is_aterm(uint32_t c)
{
    return c == 0x002E || c == 0x2024 || c == 0xFE52 || c == 0xFF0E;
}

/* Sentence_Break = STerm (strong terminator, curated). */
static int sts_is_sterm(uint32_t c)
{
    return c == 0x0021 || c == 0x003F || c == 0x0589 ||
           sts_in(c, 0x061D, 0x061F) || sts_in(c, 0x0700, 0x0702) ||
           c == 0x07F9 || sts_in(c, 0x0837, 0x0837) || c == 0x0839 ||
           c == 0x083D || c == 0x083E || c == 0x0964 || c == 0x0965 ||
           sts_in(c, 0x104A, 0x104B) || c == 0x1362 ||
           sts_in(c, 0x1367, 0x1368) || c == 0x166E ||
           sts_in(c, 0x1735, 0x1736) || c == 0x1803 || c == 0x1809 ||
           sts_in(c, 0x1944, 0x1945) || sts_in(c, 0x1AA8, 0x1AAB) ||
           c == 0x1B5A || c == 0x1B5B || c == 0x1B5E || c == 0x1B5F ||
           sts_in(c, 0x1C3B, 0x1C3C) || sts_in(c, 0x1C7E, 0x1C7F) ||
           sts_in(c, 0x203C, 0x203D) || sts_in(c, 0x2047, 0x2049) ||
           c == 0x2E2E || c == 0x2E3C || sts_in(c, 0x2E53, 0x2E54) ||
           c == 0x3002 || c == 0xA4FF || sts_in(c, 0xA60E, 0xA60F) ||
           c == 0xA6F3 || c == 0xA6F7 || sts_in(c, 0xA876, 0xA877) ||
           sts_in(c, 0xA8CE, 0xA8CF) || c == 0xA92F ||
           sts_in(c, 0xA9C8, 0xA9C9) || sts_in(c, 0xAA5D, 0xAA5F) ||
           c == 0xAAF0 || c == 0xAAF1 || c == 0xABEB ||
           sts_in(c, 0xFE56, 0xFE57) || c == 0xFF01 || c == 0xFF1F ||
           c == 0xFF61;
}

/* Sentence_Break = SContinue (curated). */
static int sts_is_scontinue(uint32_t c)
{
    return c == 0x002C || c == 0x002D || c == 0x003A || c == 0x003B ||
           c == 0x037E || c == 0x055D || sts_in(c, 0x060C, 0x060D) ||
           c == 0x07F8 || c == 0x1802 || c == 0x1808 ||
           sts_in(c, 0x2013, 0x2014) || c == 0x3001 || c == 0xFE10 ||
           c == 0xFE11 || c == 0xFE13 || c == 0xFE14 || c == 0xFE31 ||
           c == 0xFE32 || c == 0xFE50 || c == 0xFE51 || c == 0xFE54 ||
           c == 0xFE55 || c == 0xFE58 || c == 0xFE63 || c == 0xFF0C ||
           c == 0xFF0D || c == 0xFF1A || c == 0xFF1B || c == 0xFF64;
}

/* Sentence_Break = Close (curated). */
static int sts_is_close(uint32_t c)
{
    return c == 0x0029 || c == 0x005D || c == 0x007D || c == 0x0022 ||
           c == 0x0027 || c == 0x2019 || c == 0x201D || c == 0x00BB ||
           c == 0x3009 || c == 0x300B || c == 0x300D || c == 0x300F ||
           c == 0x3011 || c == 0x3015 || c == 0x3017 || c == 0x3019 ||
           c == 0x301B || sts_in(c, 0x301E, 0x301F) || c == 0xFD3E ||
           c == 0xFE17 || c == 0xFE18 || sts_in(c, 0xFE35, 0xFE44) ||
           sts_in(c, 0xFE47, 0xFE48) || c == 0xFF09 || c == 0xFF3D ||
           c == 0xFF5D || c == 0xFF60 || c == 0xFF63;
}

/* Sentence_Break = Sp (White_Space minus Sep/CR/LF). */
static int sts_is_sbspace(uint32_t c)
{
    return c == 0x0009 || c == 0x000B || c == 0x000C || c == 0x0020 ||
           c == 0x00A0 || c == 0x1680 || sts_in(c, 0x2000, 0x200A) ||
           c == 0x202F || c == 0x205F || c == 0x3000;
}

/* Indic_Conjunct_Break = Linker (curated virama set). */
static int sts_is_incb_linker(uint32_t c)
{
    return c == 0x094D || c == 0x09CD || c == 0x0ACD || c == 0x0B4D ||
           c == 0x0C4D || c == 0x0CCD || c == 0x0D4D || c == 0x0DCA ||
           c == 0x0E3A || c == 0x0EBA || c == 0x0F84 ||
           sts_in(c, 0x1039, 0x103A) || c == 0x1714 || c == 0x1734 ||
           c == 0x17D2 || c == 0x1A60 || c == 0x1B44 ||
           sts_in(c, 0x1BAA, 0x1BAB) || sts_in(c, 0x1BF2, 0x1BF3) ||
           c == 0x2D7F || c == 0xA806 || c == 0xA8C4 || c == 0xA953 ||
           c == 0xA9C0 || c == 0xAAF6 || c == 0xABED || c == 0x10A3F ||
           c == 0x11046 || c == 0x11070 || sts_in(c, 0x110B9, 0x110BA) ||
           sts_in(c, 0x11133, 0x11134) || c == 0x111C0 || c == 0x11235 ||
           c == 0x112EA || c == 0x1134D || c == 0x11442 || c == 0x114C2 ||
           c == 0x115BF || c == 0x1163F || c == 0x116B6 || c == 0x1172B ||
           c == 0x11A34 || c == 0x11A47 || c == 0x11A99 || c == 0x11C3F ||
           c == 0x11D44 || c == 0x11D45 || c == 0x11D97;
}

/* Indic_Conjunct_Break = Consonant (curated, main Indic blocks). */
static int sts_is_incb_consonant(uint32_t c)
{
    return sts_in(c, 0x0915, 0x0939) || sts_in(c, 0x0958, 0x095F) ||
           sts_in(c, 0x0978, 0x097F) || sts_in(c, 0x0995, 0x09A8) ||
           sts_in(c, 0x09AA, 0x09B0) || c == 0x09B2 ||
           sts_in(c, 0x09B6, 0x09B9) || sts_in(c, 0x09DC, 0x09DD) ||
           sts_in(c, 0x09DF, 0x09DF) || sts_in(c, 0x09F0, 0x09F1) ||
           sts_in(c, 0x0A95, 0x0AA8) || sts_in(c, 0x0AAA, 0x0AB0) ||
           sts_in(c, 0x0AB2, 0x0AB3) || sts_in(c, 0x0AB5, 0x0AB9) ||
           sts_in(c, 0x0AF9, 0x0AF9) || sts_in(c, 0x0B15, 0x0B28) ||
           sts_in(c, 0x0B2A, 0x0B30) || sts_in(c, 0x0B32, 0x0B33) ||
           sts_in(c, 0x0B35, 0x0B39) || sts_in(c, 0x0B5C, 0x0B5D) ||
           sts_in(c, 0x0B5F, 0x0B5F) || sts_in(c, 0x0B71, 0x0B71) ||
           sts_in(c, 0x0C15, 0x0C28) || sts_in(c, 0x0C2A, 0x0C39) ||
           sts_in(c, 0x0C58, 0x0C5A) || sts_in(c, 0x0D15, 0x0D3A) ||
           sts_in(c, 0x1000, 0x1021) || sts_in(c, 0x1050, 0x1055);
}

/* Hangul syllable type (§9): L/V/T/LV/LVT, or 0 when not Hangul. */
static int sts_hangul_type(uint32_t c, int* out)
{
    if (sts_in(c, 0x1100, 0x115F) || sts_in(c, 0xA960, 0xA97C)) {
        *out = SUAS_STS_GCB_L;
        return 1;
    }
    if (sts_in(c, 0x1160, 0x11A7) || sts_in(c, 0xD7B0, 0xD7C6)) {
        *out = SUAS_STS_GCB_V;
        return 1;
    }
    if (sts_in(c, 0x11A8, 0x11FF) || sts_in(c, 0xD7CB, 0xD7FB)) {
        *out = SUAS_STS_GCB_T;
        return 1;
    }
    if (sts_in(c, 0xAC00, 0xD7A3)) {
        *out = ((c - 0xAC00) % 28u == 0u) ? SUAS_STS_GCB_LV
                                          : SUAS_STS_GCB_LVT;
        return 1;
    }
    return 0;
}

/* ── Public classification ────────────────────────────────────────── */

suas_sts_gcb_t suas_sts_classify_grapheme(sucs_ex_char_t cp)
{
    uint32_t c;
    int htype;

    if (cp > SUAS_STS_ZONE_UNICODE_MAX)
        return SUAS_STS_GCB_OTHER;
    c = (uint32_t)cp;

    if (c == 0x000D) return SUAS_STS_GCB_CR;
    if (c == 0x000A) return SUAS_STS_GCB_LF;
    if (c == 0x200D) return SUAS_STS_GCB_ZWJ;
    if (sts_in(c, 0x1F1E6, 0x1F1FF)) return SUAS_STS_GCB_RI;
    if (sts_hangul_type(c, &htype)) return (suas_sts_gcb_t)htype;
    if (sts_is_extend(c)) return SUAS_STS_GCB_EXTEND;
    if (sts_is_spacingmark(c)) return SUAS_STS_GCB_SPACINGMARK;
    if (sts_is_prepend(c)) return SUAS_STS_GCB_PREPEND;
    if (sts_in(c, 0x0000, 0x001F) || sts_in(c, 0x007F, 0x009F) ||
        c == 0x00AD || c == 0x061C || c == 0x180E || c == 0x200B ||
        sts_in(c, 0x200E, 0x200F) || sts_in(c, 0x2028, 0x2029) ||
        sts_in(c, 0x202A, 0x202E) || sts_in(c, 0x2060, 0x2064) ||
        sts_in(c, 0x2066, 0x206F) || c == 0xFEFF ||
        sts_in(c, 0xFFF9, 0xFFFB))
        return SUAS_STS_GCB_CONTROL;
    return SUAS_STS_GCB_OTHER;
}

suas_sts_wb_t suas_sts_classify_word(sucs_ex_char_t cp)
{
    uint32_t c;

    if (cp > SUAS_STS_ZONE_UNICODE_MAX)
        return SUAS_STS_WB_OTHER;
    c = (uint32_t)cp;

    if (c == 0x000D) return SUAS_STS_WB_CR;
    if (c == 0x000A) return SUAS_STS_WB_LF;
    if (c == 0x000B || c == 0x000C || c == 0x0085 || c == 0x2028 ||
        c == 0x2029)
        return SUAS_STS_WB_NEWLINE;
    if (c == 0x200D) return SUAS_STS_WB_ZWJ;
    if (sts_in(c, 0x1F1E6, 0x1F1FF)) return SUAS_STS_WB_RI;
    if (sts_is_extend(c) || sts_is_spacingmark(c))
        return SUAS_STS_WB_EXTEND;
    if (sts_is_format(c)) return SUAS_STS_WB_FORMAT;
    if (sts_is_wsegspace(c)) return SUAS_STS_WB_WSEGSPACE;
    if (sts_is_numeric(c)) return SUAS_STS_WB_NUMERIC;
    if (sts_is_katakana(c)) return SUAS_STS_WB_KATAKANA;
    if (sts_is_hebrew(c)) return SUAS_STS_WB_HEBREW;
    if (c == 0x0027) return SUAS_STS_WB_SINGLE_QUOTE;
    if (c == 0x0022) return SUAS_STS_WB_DOUBLE_QUOTE;
    if (c == 0x002E || c == 0x2018 || c == 0x2019 || c == 0x2024 ||
        c == 0xFE52 || c == 0xFF07 || c == 0xFF0E)
        return SUAS_STS_WB_MIDNUMLET;
    if (c == 0x003A || c == 0x00B7 || c == 0x0387 || c == 0x05F4 ||
        c == 0x2027 || c == 0xFE13 || c == 0xFE55 || c == 0xFF1A)
        return SUAS_STS_WB_MIDLETTER;
    if (c == 0x002C || c == 0x003B || c == 0x066C || c == 0xFE50 ||
        c == 0xFE54 || c == 0xFF0C || c == 0xFF1B)
        return SUAS_STS_WB_MIDNUM;
    if (sts_is_extendnumlet(c)) return SUAS_STS_WB_EXTENDNUMLET;
    if (sts_is_alpha(c)) return SUAS_STS_WB_ALETTER;
    return SUAS_STS_WB_OTHER;
}

suas_sts_sb_t suas_sts_classify_sentence(sucs_ex_char_t cp)
{
    uint32_t c;

    if (cp > SUAS_STS_ZONE_UNICODE_MAX)
        return SUAS_STS_SB_OTHER;
    c = (uint32_t)cp;

    if (c == 0x000D) return SUAS_STS_SB_CR;
    if (c == 0x000A) return SUAS_STS_SB_LF;
    if (c == 0x0085 || c == 0x2028 || c == 0x2029) return SUAS_STS_SB_SEP;
    if (c == 0x200D) return SUAS_STS_SB_EXTEND;
    if (sts_is_extend(c) || sts_is_spacingmark(c))
        return SUAS_STS_SB_EXTEND;
    if (sts_is_format(c)) return SUAS_STS_SB_FORMAT;
    if (sts_is_sbspace(c)) return SUAS_STS_SB_SP;
    if (sts_is_aterm(c)) return SUAS_STS_SB_ATERM;
    if (sts_is_sterm(c)) return SUAS_STS_SB_STERM;
    if (sts_is_scontinue(c)) return SUAS_STS_SB_SCONTINUE;
    if (sts_is_close(c)) return SUAS_STS_SB_CLOSE;
    if (sts_is_numeric(c)) return SUAS_STS_SB_NUMERIC;
    if (sts_is_lower(c)) return SUAS_STS_SB_LOWER;
    if (sts_is_upper(c)) return SUAS_STS_SB_UPPER;
    if (sts_is_oleter(c)) return SUAS_STS_SB_OLETTER;
    return SUAS_STS_SB_OTHER;
}

/* ── Carried-context constants ────────────────────────────────────── */
#define STS_SB_TERM_NONE  0u /* no terminator run in progress          */
#define STS_SB_TERM_AFTER 1u /* SATerm seen                            */
#define STS_SB_TERM_CLOSE 2u /* SATerm Close* seen                     */
#define STS_SB_TERM_SPACE 3u /* SATerm Close* Sp* seen                 */

static int sts_wb_is_ah(uint8_t b)
{
    return b == SUAS_STS_WB_ALETTER || b == SUAS_STS_WB_HEBREW;
}

static int sts_wb_is_mid(uint8_t b)
{
    return b == SUAS_STS_WB_MIDLETTER || b == SUAS_STS_WB_MIDNUMLET ||
           b == SUAS_STS_WB_MIDNUM || b == SUAS_STS_WB_SINGLE_QUOTE ||
           b == SUAS_STS_WB_DOUBLE_QUOTE;
}

static int sts_sb_is_scan(uint8_t b)
{
    return b == SUAS_STS_SB_OTHER || b == SUAS_STS_SB_NUMERIC;
}

/* ── Window + version helpers ─────────────────────────────────────── */

static void sts_push(suas_sts_state_t* st, sucs_ex_char_t cp)
{
    if (st->count < SUAS_STS_WINDOW_SIZE) {
        st->codepoints[(st->head + st->count) % SUAS_STS_WINDOW_SIZE] = cp;
        st->count++;
    } else {
        st->codepoints[st->head] = cp;
        st->head = (uint8_t)((st->head + 1u) % SUAS_STS_WINDOW_SIZE);
    }
}

const char* suas_sts_version_string(void)
{
    return "1.0.0";
}

const char* suas_sts_level_name(suas_sts_level_t level)
{
    switch (level) {
    case SUAS_STS_LEVEL_GRAPHEME: return "Grapheme";
    case SUAS_STS_LEVEL_WORD:     return "Word";
    case SUAS_STS_LEVEL_SENTENCE: return "Sentence";
    default:                      return "Unknown";
    }
}

suas_sts_level_t suas_sts_level(const suas_sts_state_t* st)
{
    return st ? st->level : SUAS_STS_LEVEL_GRAPHEME;
}

void suas_sts_init(suas_sts_state_t* st, suas_sts_level_t level)
{
    uint8_t i;

    if (!st)
        return;
    for (i = 0; i < SUAS_STS_WINDOW_SIZE; i++)
        st->codepoints[i] = 0;
    st->head = 0;
    st->count = 0;
    st->level = level;

    st->gcb_prev = SUAS_STS_GCB_OTHER;
    st->gcb_ri = 0;
    st->gcb_linker = 0;
    st->gcb_epi = 0;
    st->gcb_zwj = 0;

    st->wb_prev = SUAS_STS_WB_OTHER;
    st->wb_before = SUAS_STS_WB_OTHER;
    st->wb_ri = 0;
    st->wb_zwj = 0;

    st->sb_prev = SUAS_STS_SB_OTHER;
    st->sb_before = SUAS_STS_SB_OTHER;
    st->sb_term = STS_SB_TERM_NONE;
    st->sb_aterm = 0;
}

bool suas_sts_is_directive(sucs_ex_char_t cp, suas_sts_boundary_t* out)
{
    if (cp == SCP_STS_BREAK_FORCE) {
        if (out) *out = SUAS_STS_BOUNDARY_MUST;
        return true;
    }
    if (cp == SCP_STS_BREAK_BLOCK) {
        if (out) *out = SUAS_STS_BOUNDARY_NONE;
        return true;
    }
    if (cp == SCP_STS_BREAK_ALLOW) {
        if (out) *out = SUAS_STS_BOUNDARY_ALLOW;
        return true;
    }
    return false;
}

/* ── Start-of-stream seeding ──────────────────────────────────────── */

static void sts_seed(suas_sts_state_t* st, sucs_ex_char_t cp)
{
    uint8_t g = (uint8_t)suas_sts_classify_grapheme(cp);
    uint8_t w = (uint8_t)suas_sts_classify_word(cp);
    uint8_t s = (uint8_t)suas_sts_classify_sentence(cp);
    int bridge = (cp <= SUAS_STS_ZONE_UNICODE_MAX);

    st->gcb_prev = g;
    st->gcb_ri = (g == SUAS_STS_GCB_RI) ? 1u : 0u;
    st->gcb_linker = (bridge && sts_is_incb_linker((uint32_t)cp)) ? 1u : 0u;
    st->gcb_epi = (bridge && sts_is_extpict((uint32_t)cp)) ? 1u : 0u;
    st->gcb_zwj = 0u;

    st->wb_prev = w;
    st->wb_before = SUAS_STS_WB_OTHER;
    st->wb_ri = (w == SUAS_STS_WB_RI) ? 1u : 0u;
    st->wb_zwj = 0u;

    st->sb_prev = s;
    st->sb_before = SUAS_STS_SB_OTHER;
    st->sb_term = (s == SUAS_STS_SB_ATERM || s == SUAS_STS_SB_STERM)
                      ? STS_SB_TERM_AFTER : STS_SB_TERM_NONE;
    st->sb_aterm = (s == SUAS_STS_SB_ATERM) ? 1u : 0u;
}

/* ── Grapheme-cluster boundaries (GB1-GB999) ──────────────────────── */

static suas_sts_boundary_t sts_grapheme_boundary(const suas_sts_state_t* st,
                                                 uint8_t a, uint8_t b,
                                                 sucs_ex_char_t cp)
{
    int bridge = (cp <= SUAS_STS_ZONE_UNICODE_MAX);

    if (a == SUAS_STS_GCB_CR && b == SUAS_STS_GCB_LF)
        return SUAS_STS_BOUNDARY_NONE;                       /* GB3 */
    if (a == SUAS_STS_GCB_CONTROL || a == SUAS_STS_GCB_CR ||
        a == SUAS_STS_GCB_LF)
        return SUAS_STS_BOUNDARY_MUST;                       /* GB4 */
    if (b == SUAS_STS_GCB_CONTROL || b == SUAS_STS_GCB_CR ||
        b == SUAS_STS_GCB_LF)
        return SUAS_STS_BOUNDARY_MUST;                       /* GB5 */
    if (a == SUAS_STS_GCB_L && (b == SUAS_STS_GCB_L ||
        b == SUAS_STS_GCB_V || b == SUAS_STS_GCB_LV ||
        b == SUAS_STS_GCB_LVT))
        return SUAS_STS_BOUNDARY_NONE;                       /* GB6 */
    if ((a == SUAS_STS_GCB_LV || a == SUAS_STS_GCB_V) &&
        (b == SUAS_STS_GCB_V || b == SUAS_STS_GCB_T))
        return SUAS_STS_BOUNDARY_NONE;                       /* GB7 */
    if ((a == SUAS_STS_GCB_LVT || a == SUAS_STS_GCB_T) &&
        b == SUAS_STS_GCB_T)
        return SUAS_STS_BOUNDARY_NONE;                       /* GB8 */
    if (b == SUAS_STS_GCB_EXTEND || b == SUAS_STS_GCB_ZWJ)
        return SUAS_STS_BOUNDARY_NONE;                       /* GB9 */
    if (b == SUAS_STS_GCB_SPACINGMARK)
        return SUAS_STS_BOUNDARY_NONE;                       /* GB9a */
    if (a == SUAS_STS_GCB_PREPEND)
        return SUAS_STS_BOUNDARY_NONE;                       /* GB9b */
    if (bridge && b == SUAS_STS_GCB_OTHER &&
        sts_is_incb_consonant((uint32_t)cp) && st->gcb_linker)
        return SUAS_STS_BOUNDARY_NONE;                       /* GB9c */
    if (bridge && sts_is_extpict((uint32_t)cp) && st->gcb_zwj)
        return SUAS_STS_BOUNDARY_NONE;                       /* GB11 */
    if (a == SUAS_STS_GCB_RI && b == SUAS_STS_GCB_RI &&
        (st->gcb_ri & 1u))
        return SUAS_STS_BOUNDARY_NONE;                       /* GB12/13 */
    return SUAS_STS_BOUNDARY_ALLOW;                          /* GB999 */
}

static void sts_grapheme_update(suas_sts_state_t* st, uint8_t a, uint8_t b,
                                sucs_ex_char_t cp)
{
    int bridge = (cp <= SUAS_STS_ZONE_UNICODE_MAX);

    if (b == SUAS_STS_GCB_RI) {
        if (a == SUAS_STS_GCB_RI ||
            (a == SUAS_STS_GCB_EXTEND && st->gcb_ri))
            st->gcb_ri = (uint8_t)((st->gcb_ri + 1u) & 1u);
        else
            st->gcb_ri = 1u;
    } else if (b != SUAS_STS_GCB_EXTEND) {
        st->gcb_ri = 0u;
    }

    if (bridge && sts_is_incb_linker((uint32_t)cp))
        st->gcb_linker = 1u;
    else if (b != SUAS_STS_GCB_EXTEND)
        st->gcb_linker = 0u;

    if (bridge && sts_is_extpict((uint32_t)cp)) {
        st->gcb_epi = 1u;
        st->gcb_zwj = 0u;
    } else if (b == SUAS_STS_GCB_EXTEND && st->gcb_epi) {
        /* keep the ExtPict run alive across Extend */
    } else if (b == SUAS_STS_GCB_ZWJ && st->gcb_epi) {
        st->gcb_zwj = 1u;
        st->gcb_epi = 0u;
    } else {
        st->gcb_epi = 0u;
        st->gcb_zwj = 0u;
    }

    st->gcb_prev = b;
}

static suas_sts_boundary_t sts_eval_grapheme(suas_sts_state_t* st,
                                             sucs_ex_char_t cp)
{
    uint8_t a = st->gcb_prev;
    uint8_t b = (uint8_t)suas_sts_classify_grapheme(cp);
    suas_sts_boundary_t r = sts_grapheme_boundary(st, a, b, cp);

    sts_grapheme_update(st, a, b, cp);
    sts_push(st, cp);
    return r;
}

/* ── Word boundaries (WB1-WB999) ──────────────────────────────────── */

static suas_sts_boundary_t sts_word_boundary(const suas_sts_state_t* st,
                                             uint8_t a, uint8_t b,
                                             sucs_ex_char_t cp)
{
    if (a == SUAS_STS_WB_CR && b == SUAS_STS_WB_LF)
        return SUAS_STS_BOUNDARY_NONE;                       /* WB3 */
    if (a == SUAS_STS_WB_NEWLINE || a == SUAS_STS_WB_CR ||
        a == SUAS_STS_WB_LF)
        return SUAS_STS_BOUNDARY_MUST;                       /* WB3a */
    if (b == SUAS_STS_WB_NEWLINE || b == SUAS_STS_WB_CR ||
        b == SUAS_STS_WB_LF)
        return SUAS_STS_BOUNDARY_MUST;                       /* WB3b */
    if (st->wb_zwj && cp <= SUAS_STS_ZONE_UNICODE_MAX &&
        sts_is_extpict((uint32_t)cp))
        return SUAS_STS_BOUNDARY_NONE;                       /* WB3c */
    if (a == SUAS_STS_WB_WSEGSPACE && b == SUAS_STS_WB_WSEGSPACE)
        return SUAS_STS_BOUNDARY_NONE;                       /* WB3d */
    if (sts_wb_is_ah(a) && sts_wb_is_ah(b))
        return SUAS_STS_BOUNDARY_NONE;                       /* WB5 */
    if ((b == SUAS_STS_WB_MIDLETTER || b == SUAS_STS_WB_MIDNUMLET ||
         b == SUAS_STS_WB_MIDNUM || b == SUAS_STS_WB_SINGLE_QUOTE) &&
        (sts_wb_is_ah(a) || a == SUAS_STS_WB_NUMERIC))
        return SUAS_STS_BOUNDARY_NONE;                       /* WB6/WB12 */
    if (b == SUAS_STS_WB_DOUBLE_QUOTE && a == SUAS_STS_WB_HEBREW)
        return SUAS_STS_BOUNDARY_NONE;                       /* WB7b */
    if ((a == SUAS_STS_WB_MIDLETTER || a == SUAS_STS_WB_MIDNUMLET ||
         a == SUAS_STS_WB_SINGLE_QUOTE) && sts_wb_is_ah(b) &&
        sts_wb_is_ah(st->wb_before))
        return SUAS_STS_BOUNDARY_NONE;                       /* WB7 */
    if (a == SUAS_STS_WB_DOUBLE_QUOTE && b == SUAS_STS_WB_HEBREW &&
        st->wb_before == SUAS_STS_WB_HEBREW)
        return SUAS_STS_BOUNDARY_NONE;                       /* WB7c */
    if (a == SUAS_STS_WB_HEBREW && b == SUAS_STS_WB_SINGLE_QUOTE)
        return SUAS_STS_BOUNDARY_NONE;                       /* WB7a */
    if ((a == SUAS_STS_WB_MIDNUM || a == SUAS_STS_WB_MIDNUMLET ||
         a == SUAS_STS_WB_SINGLE_QUOTE) && b == SUAS_STS_WB_NUMERIC &&
        st->wb_before == SUAS_STS_WB_NUMERIC)
        return SUAS_STS_BOUNDARY_NONE;                       /* WB11 */
    if (a == SUAS_STS_WB_NUMERIC && b == SUAS_STS_WB_NUMERIC)
        return SUAS_STS_BOUNDARY_NONE;                       /* WB8 */
    if (sts_wb_is_ah(a) && b == SUAS_STS_WB_NUMERIC)
        return SUAS_STS_BOUNDARY_NONE;                       /* WB9 */
    if (a == SUAS_STS_WB_NUMERIC && sts_wb_is_ah(b))
        return SUAS_STS_BOUNDARY_NONE;                       /* WB10 */
    if (a == SUAS_STS_WB_KATAKANA && b == SUAS_STS_WB_KATAKANA)
        return SUAS_STS_BOUNDARY_NONE;                       /* WB13 */
    if ((sts_wb_is_ah(a) || a == SUAS_STS_WB_NUMERIC ||
         a == SUAS_STS_WB_KATAKANA || a == SUAS_STS_WB_EXTENDNUMLET) &&
        b == SUAS_STS_WB_EXTENDNUMLET)
        return SUAS_STS_BOUNDARY_NONE;                       /* WB13a */
    if (a == SUAS_STS_WB_EXTENDNUMLET &&
        (sts_wb_is_ah(b) || b == SUAS_STS_WB_NUMERIC ||
         b == SUAS_STS_WB_KATAKANA))
        return SUAS_STS_BOUNDARY_NONE;                       /* WB13b */
    if (a == SUAS_STS_WB_RI && b == SUAS_STS_WB_RI && (st->wb_ri & 1u))
        return SUAS_STS_BOUNDARY_NONE;                       /* WB15/16 */
    return SUAS_STS_BOUNDARY_ALLOW;                          /* WB999 */
}

static suas_sts_boundary_t sts_eval_word(suas_sts_state_t* st,
                                         sucs_ex_char_t cp)
{
    uint8_t a = st->wb_prev;
    uint8_t b = (uint8_t)suas_sts_classify_word(cp);
    suas_sts_boundary_t r;

    if (b == SUAS_STS_WB_EXTEND || b == SUAS_STS_WB_FORMAT ||
        b == SUAS_STS_WB_ZWJ) {
        /* WB4: ignore, except after sot/CR/LF/Newline (WB3a/3b). */
        r = (a == SUAS_STS_WB_CR || a == SUAS_STS_WB_LF ||
             a == SUAS_STS_WB_NEWLINE)
                ? SUAS_STS_BOUNDARY_MUST : SUAS_STS_BOUNDARY_NONE;
        if (b == SUAS_STS_WB_ZWJ)
            st->wb_zwj = 1u;
        sts_push(st, cp);
        return r;
    }

    r = sts_word_boundary(st, a, b, cp);

    if (sts_wb_is_mid(b))
        st->wb_before = a;
    st->wb_prev = b;
    st->wb_zwj = 0u;
    if (b == SUAS_STS_WB_RI) {
        st->wb_ri = (a == SUAS_STS_WB_RI)
                        ? (uint8_t)(st->wb_ri ^ 1u) : 1u;
    } else {
        st->wb_ri = 0u;
    }
    sts_push(st, cp);
    return r;
}

/* ── Sentence boundaries (SB1-SB998) ──────────────────────────────── */

static suas_sts_boundary_t sts_sentence_boundary(const suas_sts_state_t* st,
                                                 uint8_t a, uint8_t b)
{
    if (a == SUAS_STS_SB_CR && b == SUAS_STS_SB_LF)
        return SUAS_STS_BOUNDARY_NONE;                       /* SB3 */
    if (a == SUAS_STS_SB_SEP || a == SUAS_STS_SB_CR ||
        a == SUAS_STS_SB_LF)
        return SUAS_STS_BOUNDARY_MUST;                       /* SB4 */
    if (a == SUAS_STS_SB_ATERM && b == SUAS_STS_SB_NUMERIC)
        return SUAS_STS_BOUNDARY_NONE;                       /* SB6 */
    if (a == SUAS_STS_SB_ATERM && b == SUAS_STS_SB_UPPER &&
        (st->sb_before == SUAS_STS_SB_UPPER ||
         st->sb_before == SUAS_STS_SB_LOWER))
        return SUAS_STS_BOUNDARY_NONE;                       /* SB7 */
    if (st->sb_aterm && st->sb_term != STS_SB_TERM_NONE &&
        b == SUAS_STS_SB_LOWER)
        return SUAS_STS_BOUNDARY_NONE;                       /* SB8 */
    if (st->sb_term != STS_SB_TERM_NONE &&
        (b == SUAS_STS_SB_SCONTINUE || b == SUAS_STS_SB_ATERM ||
         b == SUAS_STS_SB_STERM))
        return SUAS_STS_BOUNDARY_NONE;                       /* SB8a */
    if ((st->sb_term == STS_SB_TERM_AFTER ||
         st->sb_term == STS_SB_TERM_CLOSE) &&
        (b == SUAS_STS_SB_CLOSE || b == SUAS_STS_SB_SP ||
         b == SUAS_STS_SB_SEP))
        return SUAS_STS_BOUNDARY_NONE;                       /* SB9 */
    if (st->sb_term != STS_SB_TERM_NONE &&
        (b == SUAS_STS_SB_SP || b == SUAS_STS_SB_SEP))
        return SUAS_STS_BOUNDARY_NONE;                       /* SB10 */
    if (st->sb_term != STS_SB_TERM_NONE && !sts_sb_is_scan(b))
        return SUAS_STS_BOUNDARY_ALLOW;                      /* SB11 */
    return SUAS_STS_BOUNDARY_NONE;                           /* SB998 */
}

static suas_sts_boundary_t sts_eval_sentence(suas_sts_state_t* st,
                                             sucs_ex_char_t cp)
{
    uint8_t a = st->sb_prev;
    uint8_t b = (uint8_t)suas_sts_classify_sentence(cp);
    suas_sts_boundary_t r;

    if (b == SUAS_STS_SB_EXTEND || b == SUAS_STS_SB_FORMAT) {
        /* SB5: ignore, except after sot/ParaSep (SB4). */
        r = (a == SUAS_STS_SB_SEP || a == SUAS_STS_SB_CR ||
             a == SUAS_STS_SB_LF)
                ? SUAS_STS_BOUNDARY_MUST : SUAS_STS_BOUNDARY_NONE;
        sts_push(st, cp);
        return r;
    }

    r = sts_sentence_boundary(st, a, b);

    if (b == SUAS_STS_SB_ATERM) {
        st->sb_before = a;
        st->sb_aterm = 1u;
        st->sb_term = STS_SB_TERM_AFTER;
    } else if (b == SUAS_STS_SB_STERM) {
        st->sb_before = a;
        st->sb_aterm = 0u;
        st->sb_term = STS_SB_TERM_AFTER;
    } else if (st->sb_term == STS_SB_TERM_AFTER &&
               b == SUAS_STS_SB_CLOSE) {
        st->sb_term = STS_SB_TERM_CLOSE;
    } else if ((st->sb_term == STS_SB_TERM_AFTER ||
                st->sb_term == STS_SB_TERM_CLOSE) &&
               b == SUAS_STS_SB_SP) {
        st->sb_term = STS_SB_TERM_SPACE;
    } else if (st->sb_term == STS_SB_TERM_SPACE &&
               b == SUAS_STS_SB_SP) {
        /* further spaces keep the run alive */
    } else if (st->sb_term != STS_SB_TERM_NONE &&
               (b == SUAS_STS_SB_CLOSE || sts_sb_is_scan(b))) {
        /* SB8 scan window: keep the terminator run alive */
    } else {
        st->sb_term = STS_SB_TERM_NONE;
        st->sb_aterm = 0u;
    }

    if (r == SUAS_STS_BOUNDARY_MUST || r == SUAS_STS_BOUNDARY_ALLOW) {
        st->sb_term = STS_SB_TERM_NONE;
        st->sb_aterm = 0u;
    }

    st->sb_prev = b;
    sts_push(st, cp);
    return r;
}

/* ── Public stream engine ─────────────────────────────────────────── */

suas_sts_boundary_t suas_sts_evaluate(suas_sts_state_t* st, sucs_ex_char_t cp)
{
    suas_sts_boundary_t d;

    if (!st)
        return SUAS_STS_BOUNDARY_NONE;
    if (suas_sts_is_directive(cp, &d))
        return d;

    if (st->count == 0) {
        sts_push(st, cp);
        sts_seed(st, cp);
        return SUAS_STS_BOUNDARY_MUST;                       /* GB1/WB1/SB1 */
    }

    switch (st->level) {
    case SUAS_STS_LEVEL_WORD:
        return sts_eval_word(st, cp);
    case SUAS_STS_LEVEL_SENTENCE:
        return sts_eval_sentence(st, cp);
    case SUAS_STS_LEVEL_GRAPHEME:
    default:
        return sts_eval_grapheme(st, cp);
    }
}

suas_sts_boundary_t suas_sts_flush(suas_sts_state_t* st)
{
    (void)st;
    return SUAS_STS_BOUNDARY_MUST;                           /* GB2/WB2/SB2 */
}

