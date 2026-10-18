/* SUAS-007 — System Identifiers and Syntax (SIS)
 *
 * Reference implementation of docs/suas/SUAS-007-sis.md.
 *
 * Freestanding C99, zero allocation. A single-pass lexical filter resolves
 * the identifier class of a codepoint in O(1). Unicode Bridge behaviour
 * follows UAX #31 via curated class predicates; the System Control Plane is
 * strictly rejected inside identifiers; RNUR/RCCR and other Native-plane
 * codepoints are admitted as neography identifier characters.
 *
 * The ID_Start / ID_Continue sets are curated subsets of the UCD: they cover
 * the modern scripts used by the OpenWindows shell, configuration parsers,
 * and the nVAI runtime. Extending them is a bridge-data update (a minor
 * revision) and never changes the API.
 */

#include "suas/suas_sis.h"

/* ── Small range helper ───────────────────────────────────────────── */

static int sis_in(uint32_t c, uint32_t lo, uint32_t hi)
{
    return c >= lo && c <= hi;
}

/* ── ID_Start letters (§2): L* and Nl, plus curated extensions ────── */

static int sis_is_letter(uint32_t c)
{
    return sis_in(c, 0x0041, 0x005A) || sis_in(c, 0x0061, 0x007A) ||
           c == 0x00AA || c == 0x00B5 || c == 0x00BA ||
           sis_in(c, 0x00C0, 0x00D6) || sis_in(c, 0x00D8, 0x00F6) ||
           sis_in(c, 0x00F8, 0x02C1) || sis_in(c, 0x0370, 0x0374) ||
           sis_in(c, 0x0376, 0x0377) || sis_in(c, 0x037A, 0x037D) ||
           c == 0x037F || c == 0x0386 || sis_in(c, 0x0388, 0x038A) ||
           c == 0x038C || sis_in(c, 0x038E, 0x03A1) ||
           sis_in(c, 0x03A3, 0x03F5) || sis_in(c, 0x03F7, 0x0481) ||
           sis_in(c, 0x048A, 0x052F) || sis_in(c, 0x0531, 0x0556) ||
           sis_in(c, 0x0560, 0x0588) || sis_in(c, 0x05D0, 0x05EA) ||
           sis_in(c, 0x05EF, 0x05F2) || sis_in(c, 0x0620, 0x064A) ||
           sis_in(c, 0x066E, 0x066F) || sis_in(c, 0x0671, 0x06D3) ||
           c == 0x06D5 || sis_in(c, 0x06E5, 0x06E6) ||
           sis_in(c, 0x06EE, 0x06EF) || sis_in(c, 0x06FA, 0x06FC) ||
           c == 0x06FF || c == 0x0710 || sis_in(c, 0x0712, 0x072F) ||
           sis_in(c, 0x074D, 0x07A5) || c == 0x07B1 ||
           sis_in(c, 0x0904, 0x0939) || c == 0x093D || c == 0x0950 ||
           sis_in(c, 0x0958, 0x0961) || sis_in(c, 0x0971, 0x0980) ||
           sis_in(c, 0x0985, 0x098C) || sis_in(c, 0x098F, 0x0990) ||
           sis_in(c, 0x0993, 0x09A8) || sis_in(c, 0x09AA, 0x09B0) ||
           c == 0x09B2 || sis_in(c, 0x09B6, 0x09B9) || c == 0x09BD ||
           sis_in(c, 0x09DC, 0x09DD) || sis_in(c, 0x09DF, 0x09E1) ||
           sis_in(c, 0x09F0, 0x09F1) || sis_in(c, 0x0A05, 0x0A0A) ||
           sis_in(c, 0x0A0F, 0x0A10) || sis_in(c, 0x0A13, 0x0A28) ||
           sis_in(c, 0x0A2A, 0x0A30) || sis_in(c, 0x0A32, 0x0A33) ||
           sis_in(c, 0x0A35, 0x0A36) || sis_in(c, 0x0A38, 0x0A39) ||
           sis_in(c, 0x0A59, 0x0A5C) || c == 0x0A5E ||
           sis_in(c, 0x0A72, 0x0A74) || sis_in(c, 0x0A85, 0x0A8D) ||
           sis_in(c, 0x0A8F, 0x0A91) || sis_in(c, 0x0A93, 0x0AA8) ||
           sis_in(c, 0x0AAA, 0x0AB0) || sis_in(c, 0x0AB2, 0x0AB3) ||
           sis_in(c, 0x0AB5, 0x0AB9) || c == 0x0ABD || c == 0x0AD0 ||
           sis_in(c, 0x0B05, 0x0B0C) || sis_in(c, 0x0B0F, 0x0B10) ||
           sis_in(c, 0x0B13, 0x0B28) || sis_in(c, 0x0B2A, 0x0B30) ||
           sis_in(c, 0x0B32, 0x0B33) || sis_in(c, 0x0B35, 0x0B39) ||
           c == 0x0B3D || sis_in(c, 0x0B5C, 0x0B5D) ||
           sis_in(c, 0x0B5F, 0x0B61) || c == 0x0B71 ||
           sis_in(c, 0x0B85, 0x0B8A) || sis_in(c, 0x0B8E, 0x0B90) ||
           sis_in(c, 0x0B92, 0x0B95) || sis_in(c, 0x0B99, 0x0B9A) ||
           c == 0x0B9C || sis_in(c, 0x0B9E, 0x0B9F) ||
           sis_in(c, 0x0BA3, 0x0BA4) || sis_in(c, 0x0BA8, 0x0BAA) ||
           sis_in(c, 0x0BAE, 0x0BB9) || c == 0x0BD0 ||
           sis_in(c, 0x0C05, 0x0C0C) || sis_in(c, 0x0C0E, 0x0C10) ||
           sis_in(c, 0x0C12, 0x0C28) || sis_in(c, 0x0C2A, 0x0C39) ||
           c == 0x0C3D || sis_in(c, 0x0C58, 0x0C5A) ||
           sis_in(c, 0x0C60, 0x0C61) || sis_in(c, 0x0C85, 0x0C8C) ||
           sis_in(c, 0x0C8E, 0x0C90) || sis_in(c, 0x0C92, 0x0CA8) ||
           sis_in(c, 0x0CAA, 0x0CB3) || sis_in(c, 0x0CB5, 0x0CB9) ||
           c == 0x0CBD || c == 0x0CDE || sis_in(c, 0x0CE0, 0x0CE1) ||
           sis_in(c, 0x0D05, 0x0D0C) || sis_in(c, 0x0D0E, 0x0D10) ||
           sis_in(c, 0x0D12, 0x0D3A) || c == 0x0D3D || c == 0x0D4E ||
           sis_in(c, 0x0D54, 0x0D56) || sis_in(c, 0x0D5F, 0x0D61) ||
           sis_in(c, 0x0D7A, 0x0D7F) || sis_in(c, 0x0D85, 0x0D96) ||
           sis_in(c, 0x0D9A, 0x0DB1) || sis_in(c, 0x0DB3, 0x0DBB) ||
           c == 0x0DBD || sis_in(c, 0x0DC0, 0x0DC6) ||
           sis_in(c, 0x0E01, 0x0E30) || c == 0x0E32 || c == 0x0E33 ||
           c == 0x0E40 || sis_in(c, 0x0E41, 0x0E45) || c == 0x0E46 ||
           sis_in(c, 0x0E81, 0x0E82) || c == 0x0E84 ||
           sis_in(c, 0x0E86, 0x0E8A) || c == 0x0E8C ||
           sis_in(c, 0x0E8D, 0x0EA3) || c == 0x0EA5 || c == 0x0EA7 ||
           sis_in(c, 0x0EAA, 0x0EAB) || c == 0x0EBD ||
           sis_in(c, 0x0EC0, 0x0EC4) || c == 0x0EC6 ||
           sis_in(c, 0x0F40, 0x0F47) || sis_in(c, 0x0F49, 0x0F6C) ||
           sis_in(c, 0x0F88, 0x0F8C) || sis_in(c, 0x1000, 0x102A) ||
           sis_in(c, 0x10A0, 0x10C5) || c == 0x10C7 || c == 0x10CD ||
           sis_in(c, 0x10D0, 0x10FA) || sis_in(c, 0x10FC, 0x10FF) ||
           sis_in(c, 0x1100, 0x11FF) || sis_in(c, 0x1200, 0x1248) ||
           sis_in(c, 0x124A, 0x1256) || c == 0x1258 ||
           sis_in(c, 0x1260, 0x1288) || sis_in(c, 0x12C8, 0x12D6) ||
           sis_in(c, 0x1780, 0x17B3) ||
           sis_in(c, 0x1E00, 0x1FFF) || sis_in(c, 0x2C60, 0x2C7F) ||
           c == 0x3005 || c == 0x3006 || c == 0x3007 ||
           sis_in(c, 0x3021, 0x3029) || sis_in(c, 0x3031, 0x3035) ||
           sis_in(c, 0x3038, 0x303C) || sis_in(c, 0x3041, 0x3096) ||
           sis_in(c, 0x309D, 0x309F) || sis_in(c, 0x30A1, 0x30FA) ||
           sis_in(c, 0x30FC, 0x30FF) || sis_in(c, 0x3105, 0x312F) ||
           sis_in(c, 0x3131, 0x318E) || sis_in(c, 0x31A0, 0x31BF) ||
           sis_in(c, 0x31F0, 0x31FF) || sis_in(c, 0x3400, 0x4DBF) ||
           sis_in(c, 0x4E00, 0x9FFF) || sis_in(c, 0xA000, 0xA48C) ||
           sis_in(c, 0xA640, 0xA69F) || sis_in(c, 0xA720, 0xA7FF) ||
           sis_in(c, 0xA960, 0xA97C) || sis_in(c, 0xAC00, 0xD7A3) ||
           sis_in(c, 0xD7B0, 0xD7C6) || sis_in(c, 0xF900, 0xFAFF) ||
           sis_in(c, 0xFB00, 0xFB06) || sis_in(c, 0xFB13, 0xFB17) ||
           sis_in(c, 0xFF21, 0xFF3A) || sis_in(c, 0xFF41, 0xFF5A) ||
           sis_in(c, 0x20000, 0x2FA1F);
}

/* Other_ID_Start (§2.5): letters that qualified historically. */
static int sis_is_other_id_start(uint32_t c)
{
    return c == 0x2118 || c == 0x212E || c == 0x309B || c == 0x309C;
}

/* ── ID_Continue marks (§2): Mn and Mc (curated UCD subset) ───────── */

static int sis_is_nonspacing(uint32_t c)
{
    return sis_in(c, 0x0300, 0x036F) || sis_in(c, 0x0483, 0x0489) ||
           sis_in(c, 0x0591, 0x05BD) || c == 0x05BF ||
           sis_in(c, 0x05C1, 0x05C2) || sis_in(c, 0x05C4, 0x05C5) ||
           c == 0x05C7 || sis_in(c, 0x0610, 0x061A) ||
           sis_in(c, 0x064B, 0x065F) || c == 0x0670 ||
           sis_in(c, 0x06D6, 0x06DC) || sis_in(c, 0x06DF, 0x06E4) ||
           sis_in(c, 0x06E7, 0x06E8) || sis_in(c, 0x06EA, 0x06ED) ||
           c == 0x0711 || sis_in(c, 0x0730, 0x074A) ||
           sis_in(c, 0x07A6, 0x07B0) || sis_in(c, 0x07EB, 0x07F3) ||
           sis_in(c, 0x0816, 0x0819) || sis_in(c, 0x081B, 0x0823) ||
           sis_in(c, 0x0825, 0x0827) || sis_in(c, 0x0829, 0x082D) ||
           sis_in(c, 0x0859, 0x085B) || sis_in(c, 0x08D3, 0x08E1) ||
           sis_in(c, 0x08E3, 0x0902) || c == 0x093A || c == 0x093C ||
           sis_in(c, 0x0941, 0x0948) || c == 0x094D ||
           sis_in(c, 0x0951, 0x0957) || sis_in(c, 0x0962, 0x0963) ||
           c == 0x0981 || c == 0x09BC || sis_in(c, 0x09C1, 0x09C4) ||
           c == 0x09CD || sis_in(c, 0x09E2, 0x09E3) || c == 0x09FE ||
           sis_in(c, 0x0A01, 0x0A02) || c == 0x0A3C ||
           sis_in(c, 0x0A41, 0x0A42) || sis_in(c, 0x0A47, 0x0A48) ||
           sis_in(c, 0x0A4B, 0x0A4D) || c == 0x0A51 ||
           sis_in(c, 0x0A70, 0x0A71) || c == 0x0A75 ||
           sis_in(c, 0x0A81, 0x0A82) || c == 0x0ABC ||
           sis_in(c, 0x0AC1, 0x0AC5) || sis_in(c, 0x0AC7, 0x0AC8) ||
           c == 0x0ACD || sis_in(c, 0x0AE2, 0x0AE3) || c == 0x0B01 ||
           c == 0x0B3C || c == 0x0B3F || sis_in(c, 0x0B41, 0x0B44) ||
           c == 0x0B4D || sis_in(c, 0x0B55, 0x0B56) ||
           sis_in(c, 0x0B62, 0x0B63) || c == 0x0B82 || c == 0x0BC0 ||
           c == 0x0BCD || c == 0x0C00 || c == 0x0C04 ||
           sis_in(c, 0x0C3C, 0x0C40) || sis_in(c, 0x0C46, 0x0C48) ||
           sis_in(c, 0x0C4A, 0x0C4D) || sis_in(c, 0x0C55, 0x0C56) ||
           sis_in(c, 0x0C62, 0x0C63) || c == 0x0C81 ||
           sis_in(c, 0x0CBC, 0x0CCD) || sis_in(c, 0x0CE2, 0x0CE3) ||
           sis_in(c, 0x0D00, 0x0D01) || sis_in(c, 0x0D3B, 0x0D3C) ||
           sis_in(c, 0x0D41, 0x0D44) || c == 0x0D4D ||
           sis_in(c, 0x0D62, 0x0D63) || c == 0x0D81 ||
           sis_in(c, 0x0DCA, 0x0DCA) || sis_in(c, 0x0DD2, 0x0DD4) ||
           c == 0x0DD6 || c == 0x0E31 || sis_in(c, 0x0E34, 0x0E3A) ||
           sis_in(c, 0x0E47, 0x0E4E) || c == 0x0EB1 ||
           sis_in(c, 0x0EB4, 0x0EBC) || sis_in(c, 0x0EC8, 0x0ECD) ||
           sis_in(c, 0x0F18, 0x0F19) || c == 0x0F35 || c == 0x0F37 ||
           c == 0x0F39 || sis_in(c, 0x0F71, 0x0F7E) ||
           sis_in(c, 0x0F80, 0x0F84) || sis_in(c, 0x0F86, 0x0F87) ||
           sis_in(c, 0x0F8D, 0x0F97) || sis_in(c, 0x0F99, 0x0FBC) ||
           c == 0x0FC6 || sis_in(c, 0x102D, 0x1030) ||
           sis_in(c, 0x1032, 0x1037) || sis_in(c, 0x1039, 0x103A) ||
           sis_in(c, 0x103D, 0x103E) || sis_in(c, 0x1058, 0x1059) ||
           sis_in(c, 0x105E, 0x1060) || sis_in(c, 0x1071, 0x1074) ||
           c == 0x1082 || sis_in(c, 0x1085, 0x1086) || c == 0x108D ||
           c == 0x109D || sis_in(c, 0x135D, 0x135F) ||
           sis_in(c, 0x1712, 0x1714) || sis_in(c, 0x1732, 0x1734) ||
           sis_in(c, 0x1752, 0x1753) || sis_in(c, 0x1772, 0x1773) ||
           sis_in(c, 0x17B4, 0x17B5) || sis_in(c, 0x17B7, 0x17BD) ||
           c == 0x17C6 || sis_in(c, 0x17C9, 0x17D3) || c == 0x17DD ||
           sis_in(c, 0x180B, 0x180D) || sis_in(c, 0x1885, 0x1886) ||
           c == 0x18A9 || sis_in(c, 0x1920, 0x1922) ||
           sis_in(c, 0x1927, 0x1928) || c == 0x1932 ||
           sis_in(c, 0x1939, 0x193B) || sis_in(c, 0x1A17, 0x1A18) ||
           c == 0x1A1B || c == 0x1A56 || sis_in(c, 0x1A58, 0x1A5E) ||
           c == 0x1A60 || c == 0x1A62 || sis_in(c, 0x1A65, 0x1A6C) ||
           sis_in(c, 0x1A73, 0x1A7C) || c == 0x1A7F ||
           sis_in(c, 0x1AB0, 0x1AFF) || sis_in(c, 0x1B00, 0x1B03) ||
           c == 0x1B34 || sis_in(c, 0x1B36, 0x1B3A) || c == 0x1B3C ||
           c == 0x1B42 || sis_in(c, 0x1B6B, 0x1B73) ||
           sis_in(c, 0x1B80, 0x1B81) || sis_in(c, 0x1BA2, 0x1BA5) ||
           sis_in(c, 0x1BA8, 0x1BA9) || sis_in(c, 0x1BAB, 0x1BAD) ||
           c == 0x1BE6 || sis_in(c, 0x1BE8, 0x1BE9) || c == 0x1BED ||
           sis_in(c, 0x1BEF, 0x1BF1) || sis_in(c, 0x1C2C, 0x1C33) ||
           sis_in(c, 0x1C36, 0x1C37) || sis_in(c, 0x1CD0, 0x1CD2) ||
           sis_in(c, 0x1CD4, 0x1CE0) || sis_in(c, 0x1CE2, 0x1CE8) ||
           c == 0x1CED || c == 0x1CF4 || sis_in(c, 0x1CF8, 0x1CF9) ||
           sis_in(c, 0x1DC0, 0x1DFF) || sis_in(c, 0x20D0, 0x20F0) ||
           c == 0x200C || sis_in(c, 0x2CEF, 0x2CF1) || c == 0x2D7F ||
           sis_in(c, 0x2DE0, 0x2DFF) || sis_in(c, 0x302A, 0x302D) ||
           sis_in(c, 0x3099, 0x309A) || c == 0xA66F ||
           sis_in(c, 0xA674, 0xA67D) || sis_in(c, 0xA69E, 0xA69F) ||
           sis_in(c, 0xA6F0, 0xA6F1) || c == 0xA802 || c == 0xA806 ||
           c == 0xA80B || sis_in(c, 0xA825, 0xA826) || c == 0xA82C ||
           sis_in(c, 0xA8C4, 0xA8C5) || sis_in(c, 0xA8E0, 0xA8F1) ||
           c == 0xA8FF || sis_in(c, 0xA926, 0xA92D) ||
           sis_in(c, 0xA947, 0xA951) || sis_in(c, 0xA980, 0xA982) ||
           c == 0xA9B3 || sis_in(c, 0xA9B6, 0xA9B9) ||
           sis_in(c, 0xA9BC, 0xA9BD) || c == 0xA9E5 ||
           sis_in(c, 0xAA29, 0xAA2E) || sis_in(c, 0xAA31, 0xAA32) ||
           sis_in(c, 0xAA35, 0xAA36) || c == 0xAA43 || c == 0xAA4C ||
           c == 0xAA7C || c == 0xAAB0 || sis_in(c, 0xAAB2, 0xAAB4) ||
           sis_in(c, 0xAAB7, 0xAAB8) || sis_in(c, 0xAABE, 0xAABF) ||
           c == 0xAAC1 || sis_in(c, 0xAAEC, 0xAAED) || c == 0xAAF6 ||
           c == 0xABE5 || c == 0xABE8 || c == 0xABED ||
           c == 0xFB1E || sis_in(c, 0xFE00, 0xFE0F) ||
           sis_in(c, 0xFE20, 0xFE2F) || c == 0x101FD ||
           c == 0x102E0 || sis_in(c, 0x10376, 0x1037A) ||
           c == 0x10A01 || sis_in(c, 0x10A03, 0x10A03) ||
           sis_in(c, 0x10A05, 0x10A06) || sis_in(c, 0x10A0C, 0x10A0F) ||
           sis_in(c, 0x10A38, 0x10A3A) || c == 0x10A3F ||
           sis_in(c, 0x10AE5, 0x10AE6) || c == 0x10D24 ||
           sis_in(c, 0x10D69, 0x10D6D) || sis_in(c, 0x10EAB, 0x10EAC) ||
           c == 0x11001 || sis_in(c, 0x11038, 0x11046) ||
           c == 0x11070 || sis_in(c, 0x11073, 0x11074) ||
           sis_in(c, 0x1107F, 0x11081) || sis_in(c, 0x110B3, 0x110B6) ||
           sis_in(c, 0x110B9, 0x110BA) || c == 0x110C2 ||
           sis_in(c, 0x11100, 0x11102) || sis_in(c, 0x11127, 0x1112B) ||
           sis_in(c, 0x1112D, 0x11134) || c == 0x11173 ||
           sis_in(c, 0x11180, 0x11181) || sis_in(c, 0x111B6, 0x111BE) ||
           sis_in(c, 0x111C9, 0x111CC) || c == 0x111CF ||
           sis_in(c, 0x1122F, 0x11231) || c == 0x11234 ||
           sis_in(c, 0x11236, 0x11237) || c == 0x1123E ||
           c == 0x11241 || sis_in(c, 0x112DF, 0x112DF) ||
           sis_in(c, 0x112E3, 0x112EA) || sis_in(c, 0x11300, 0x11301) ||
           sis_in(c, 0x1133B, 0x1133C) || c == 0x11340 ||
           sis_in(c, 0x11366, 0x1136C) || sis_in(c, 0x11370, 0x11374) ||
           sis_in(c, 0x11438, 0x1143F) || sis_in(c, 0x11442, 0x11444) ||
           c == 0x11446 || c == 0x1145E || sis_in(c, 0x114B3, 0x114B8) ||
           c == 0x114BA || sis_in(c, 0x114BF, 0x114C0) ||
           sis_in(c, 0x114C2, 0x114C3) || sis_in(c, 0x115B2, 0x115B5) ||
           sis_in(c, 0x115BC, 0x115BD) || sis_in(c, 0x115BF, 0x115C0) ||
           sis_in(c, 0x115DC, 0x115DD) || sis_in(c, 0x11633, 0x1163A) ||
           c == 0x1163D || sis_in(c, 0x1163F, 0x11640) || c == 0x116AB ||
           c == 0x116AD || sis_in(c, 0x116B0, 0x116B5) || c == 0x116B7 ||
           sis_in(c, 0x1171D, 0x1171F) || sis_in(c, 0x11722, 0x11725) ||
           sis_in(c, 0x11727, 0x1172B) || sis_in(c, 0x1182F, 0x11837) ||
           sis_in(c, 0x11839, 0x1183A) || c == 0x1193B ||
           c == 0x1193C || c == 0x1193E || c == 0x11943 ||
           sis_in(c, 0x119D4, 0x119D7) || sis_in(c, 0x119DA, 0x119DB) ||
           c == 0x119E0 || sis_in(c, 0x11A01, 0x11A0A) ||
           sis_in(c, 0x11A33, 0x11A38) || sis_in(c, 0x11A3B, 0x11A3E) ||
           c == 0x11A47 || sis_in(c, 0x11A51, 0x11A56) ||
           sis_in(c, 0x11A59, 0x11A5B) || sis_in(c, 0x11A8A, 0x11A96) ||
           sis_in(c, 0x11A98, 0x11A99) || sis_in(c, 0x11C30, 0x11C36) ||
           sis_in(c, 0x11C38, 0x11C3D) || c == 0x11C3F ||
           sis_in(c, 0x11C92, 0x11CA7) || sis_in(c, 0x11CAA, 0x11CB0) ||
           sis_in(c, 0x11CB2, 0x11CB3) || sis_in(c, 0x11CB5, 0x11CB6) ||
           sis_in(c, 0x11D31, 0x11D36) || c == 0x11D3A ||
           sis_in(c, 0x11D3C, 0x11D3D) || sis_in(c, 0x11D3F, 0x11D45) ||
           c == 0x11D47 || sis_in(c, 0x11D90, 0x11D91) || c == 0x11D95 ||
           c == 0x11D97 || c == 0x11EF3 || sis_in(c, 0x11F00, 0x11F01) ||
           c == 0x11F36 || sis_in(c, 0x11F40, 0x11F42) ||
           c == 0x13440 || sis_in(c, 0x13447, 0x13455) ||
           sis_in(c, 0x16AF0, 0x16AF4) || sis_in(c, 0x16B30, 0x16B36) ||
           c == 0x16F4F || sis_in(c, 0x16F8F, 0x16F92) ||
           c == 0x16FE4 ||
           sis_in(c, 0x1BC9D, 0x1BC9E) ||
           sis_in(c, 0x1CF00, 0x1CF2D) || sis_in(c, 0x1CF30, 0x1CF46) ||
           sis_in(c, 0x1D167, 0x1D169) || sis_in(c, 0x1D17B, 0x1D182) ||
           sis_in(c, 0x1D185, 0x1D18B) || sis_in(c, 0x1D1AA, 0x1D1AD) ||
           sis_in(c, 0x1D242, 0x1D244) ||
           sis_in(c, 0x1DA00, 0x1DA36) || sis_in(c, 0x1DA3B, 0x1DA6C) ||
           c == 0x1DA75 || c == 0x1DA84 ||
           sis_in(c, 0x1DA9B, 0x1DA9F) || sis_in(c, 0x1DAA1, 0x1DAAF) ||
           sis_in(c, 0x1E000, 0x1E006) || sis_in(c, 0x1E008, 0x1E018) ||
           sis_in(c, 0x1E01B, 0x1E021) || sis_in(c, 0x1E023, 0x1E024) ||
           sis_in(c, 0x1E026, 0x1E02A) || c == 0x1E08F ||
           sis_in(c, 0x1E130, 0x1E136) || c == 0x1E2AE ||
           sis_in(c, 0x1E2EC, 0x1E2EF) || sis_in(c, 0x1E4EC, 0x1E4EF) ||
           sis_in(c, 0x1E5EE, 0x1E5EF) || sis_in(c, 0x1E8D0, 0x1E8D6) ||
           sis_in(c, 0x1E944, 0x1E94A) || sis_in(c, 0xE0100, 0xE01EF);
}

static int sis_is_spacingmark(uint32_t c)
{
    return c == 0x0903 || c == 0x093B || sis_in(c, 0x093E, 0x0940) ||
           sis_in(c, 0x0949, 0x094C) || sis_in(c, 0x094E, 0x094F) ||
           sis_in(c, 0x0982, 0x0983) || sis_in(c, 0x09BF, 0x09C0) ||
           sis_in(c, 0x09C7, 0x09C8) || sis_in(c, 0x09CB, 0x09CC) ||
           c == 0x0A03 || sis_in(c, 0x0A3E, 0x0A40) || c == 0x0A83 ||
           sis_in(c, 0x0ABE, 0x0AC0) || c == 0x0AC9 ||
           sis_in(c, 0x0ACB, 0x0ACC) || sis_in(c, 0x0B02, 0x0B03) ||
           c == 0x0B3E || c == 0x0B40 || sis_in(c, 0x0B47, 0x0B48) ||
           sis_in(c, 0x0B4B, 0x0B4C) || sis_in(c, 0x0BBE, 0x0BBF) ||
           sis_in(c, 0x0BC1, 0x0BC2) || sis_in(c, 0x0BC6, 0x0BC8) ||
           sis_in(c, 0x0BCA, 0x0BCC) || c == 0x0BD7 ||
           sis_in(c, 0x0C01, 0x0C03) || sis_in(c, 0x0C41, 0x0C44) ||
           sis_in(c, 0x0C82, 0x0C83) || c == 0x0CBE ||
           sis_in(c, 0x0CC0, 0x0CC4) || sis_in(c, 0x0CC7, 0x0CC8) ||
           sis_in(c, 0x0CCA, 0x0CCB) || sis_in(c, 0x0CD5, 0x0CD6) ||
           sis_in(c, 0x0D02, 0x0D03) || sis_in(c, 0x0D3E, 0x0D40) ||
           sis_in(c, 0x0D46, 0x0D48) || sis_in(c, 0x0D4A, 0x0D4C) ||
           c == 0x0D57 || sis_in(c, 0x0D82, 0x0D83) ||
           sis_in(c, 0x0DCF, 0x0DD1) || sis_in(c, 0x0DD8, 0x0DDF) ||
           sis_in(c, 0x0DF2, 0x0DF3) || c == 0x0E33 || c == 0x0EB3 ||
           sis_in(c, 0x0F3E, 0x0F3F) || sis_in(c, 0x102B, 0x102C) ||
           c == 0x1031 || c == 0x1038 || sis_in(c, 0x103B, 0x103C) ||
           sis_in(c, 0x1056, 0x1057) || sis_in(c, 0x1062, 0x1064) ||
           sis_in(c, 0x1067, 0x106D) || sis_in(c, 0x1083, 0x1084) ||
           sis_in(c, 0x1087, 0x108C) || c == 0x108F ||
           sis_in(c, 0x109A, 0x109C) || c == 0x17B6 ||
           sis_in(c, 0x17BE, 0x17C5) || sis_in(c, 0x17C7, 0x17C8) ||
           sis_in(c, 0x1923, 0x1926) || sis_in(c, 0x1929, 0x192B) ||
           sis_in(c, 0x1930, 0x1931) || sis_in(c, 0x1933, 0x1938) ||
           sis_in(c, 0x1A19, 0x1A1A) || c == 0x1A55 || c == 0x1A57 ||
           c == 0x1A61 || sis_in(c, 0x1A63, 0x1A64) ||
           sis_in(c, 0x1A6D, 0x1A72) || c == 0x1B04 || c == 0x1B35 ||
           c == 0x1B3B || sis_in(c, 0x1B3D, 0x1B41) ||
           sis_in(c, 0x1B43, 0x1B44) || c == 0x1B82 || c == 0x1BA1 ||
           sis_in(c, 0x1BA6, 0x1BA7) || c == 0x1BAA || c == 0x1BE7 ||
           sis_in(c, 0x1BEA, 0x1BEC) || c == 0x1BEE ||
           sis_in(c, 0x1BF2, 0x1BF3) || sis_in(c, 0x1C24, 0x1C2B) ||
           sis_in(c, 0x1C34, 0x1C35) || c == 0x1CE1 || c == 0x1CF7 ||
           sis_in(c, 0x302E, 0x302F) || sis_in(c, 0xA823, 0xA824) ||
           c == 0xA827 || sis_in(c, 0xA880, 0xA881) ||
           sis_in(c, 0xA8B4, 0xA8C3) || c == 0xA952 || c == 0xA953 ||
           c == 0xA983 || c == 0xA9B4 || c == 0xA9B5 ||
           sis_in(c, 0xA9BA, 0xA9BB) || sis_in(c, 0xA9BE, 0xA9C0) ||
           c == 0xAA2F || c == 0xAA30 || sis_in(c, 0xAA33, 0xAA34) ||
           c == 0xAA4D || c == 0xAAEB ||
           sis_in(c, 0xAAEE, 0xAAEF) || c == 0xAAF5 ||
           sis_in(c, 0xABE3, 0xABE4) || c == 0xABE6 || c == 0xABE7 ||
           c == 0xABE9 || c == 0xABEA || c == 0xABEC ||
           c == 0x11000 || c == 0x11002 || c == 0x11082 ||
           sis_in(c, 0x110B0, 0x110B2) || sis_in(c, 0x110B7, 0x110B8) ||
           c == 0x1112C || sis_in(c, 0x11145, 0x11146) || c == 0x11182 ||
           sis_in(c, 0x111B3, 0x111B5) || sis_in(c, 0x111BF, 0x111C0) ||
           c == 0x111CE || sis_in(c, 0x1122C, 0x1122E) ||
           c == 0x11232 || c == 0x11233 || c == 0x11235 ||
           sis_in(c, 0x112E0, 0x112E2) || sis_in(c, 0x11302, 0x11303) ||
           c == 0x1133E || c == 0x1133F || sis_in(c, 0x11341, 0x11344) ||
           sis_in(c, 0x11347, 0x11348) || sis_in(c, 0x1134B, 0x1134D) ||
           c == 0x11357 || sis_in(c, 0x11362, 0x11363) ||
           sis_in(c, 0x11435, 0x11437) || sis_in(c, 0x11440, 0x11441) ||
           c == 0x11445 || sis_in(c, 0x114B0, 0x114B2) ||
           c == 0x114B9 || sis_in(c, 0x114BB, 0x114BE) ||
           c == 0x114C1 || sis_in(c, 0x115AF, 0x115B1) ||
           sis_in(c, 0x115B8, 0x115BB) || c == 0x115BE ||
           sis_in(c, 0x11630, 0x11632) || sis_in(c, 0x1163B, 0x1163C) ||
           c == 0x1163E || c == 0x116AC || c == 0x116AE || c == 0x116AF ||
           c == 0x116B6 || sis_in(c, 0x11720, 0x11721) ||
           c == 0x11726 || sis_in(c, 0x1182C, 0x1182E) || c == 0x11838 ||
           c == 0x11930 || sis_in(c, 0x119D1, 0x119D3) ||
           sis_in(c, 0x119DC, 0x119DF) || c == 0x119E4 ||
           c == 0x11A39 || sis_in(c, 0x11A57, 0x11A58) ||
           c == 0x11A97 || c == 0x11C2F || c == 0x11C3E ||
           c == 0x11CA9 || c == 0x11CB1 || c == 0x11CB4 ||
           sis_in(c, 0x11D8A, 0x11D8E) || sis_in(c, 0x11D93, 0x11D94) ||
           c == 0x11D96 || sis_in(c, 0x11EF5, 0x11EF6) || c == 0x11F03 ||
           c == 0x11F34 || c == 0x11F35 || sis_in(c, 0x11F3E, 0x11F3F) ||
           c == 0x16F51 || sis_in(c, 0x16FF0, 0x16FF1);
}

/* ── ID_Continue digits (§2): Nd (curated UCD subset) ─────────────── */

static int sis_is_digit(uint32_t c)
{
    return sis_in(c, 0x0030, 0x0039) || sis_in(c, 0x0660, 0x0669) ||
           sis_in(c, 0x06F0, 0x06F9) || sis_in(c, 0x07C0, 0x07C9) ||
           sis_in(c, 0x0966, 0x096F) || sis_in(c, 0x09E6, 0x09EF) ||
           sis_in(c, 0x0A66, 0x0A6F) || sis_in(c, 0x0AE6, 0x0AEF) ||
           sis_in(c, 0x0B66, 0x0B6F) || sis_in(c, 0x0BE6, 0x0BEF) ||
           sis_in(c, 0x0C66, 0x0C6F) || sis_in(c, 0x0CE6, 0x0CEF) ||
           sis_in(c, 0x0D66, 0x0D6F) || sis_in(c, 0x0E50, 0x0E59) ||
           sis_in(c, 0x0ED0, 0x0ED9) || sis_in(c, 0x0F20, 0x0F29) ||
           sis_in(c, 0x1040, 0x1049) || sis_in(c, 0x1090, 0x1099) ||
           sis_in(c, 0x17E0, 0x17E9) || sis_in(c, 0x1810, 0x1819) ||
           sis_in(c, 0xFF10, 0xFF19) || sis_in(c, 0x1D7CE, 0x1D7FF);
}

/* ── ID_Continue connector punctuation (§2): Pc ───────────────────── */

static int sis_is_connector(uint32_t c)
{
    return c == 0x005F || sis_in(c, 0x203F, 0x2040) || c == 0x2054 ||
           sis_in(c, 0xFE33, 0xFE34) || sis_in(c, 0xFE4D, 0xFE4F) ||
           c == 0x30FB || c == 0xFF3F || c == 0xFF65;
}

/* Other_ID_Continue (§2.5) and the joining controls / selectors that
 * UAX #31 §2.3 retains in XID_Continue. */
static int sis_is_other_id_continue(uint32_t c)
{
    return c == 0x00B7 || c == 0x0387 || sis_in(c, 0x1369, 0x1371) ||
           c == 0x19DA || c == 0x200D;
}

/* ── Pattern_White_Space (§4.1) ───────────────────────────────────── */

static int sis_is_pattern_ws(uint32_t c)
{
    return sis_in(c, 0x0009, 0x000D) || c == 0x0020 || c == 0x0085 ||
           sis_in(c, 0x200E, 0x200F) || sis_in(c, 0x2028, 0x2029);
}

/* ── Pattern_Syntax (§4.2, curated) ───────────────────────────────── */

static int sis_is_pattern_syntax(uint32_t c)
{
    return sis_in(c, 0x0021, 0x002F) || sis_in(c, 0x003A, 0x0040) ||
           sis_in(c, 0x005B, 0x0060) || sis_in(c, 0x007B, 0x007E) ||
           sis_in(c, 0x00A1, 0x00A7) || c == 0x00A9 ||
           sis_in(c, 0x00AB, 0x00AC) || c == 0x00AE ||
           sis_in(c, 0x00B0, 0x00B1) || c == 0x00B6 || c == 0x00BB ||
           c == 0x00BF || c == 0x00D7 || c == 0x00F7 ||
           sis_in(c, 0x2010, 0x2027) || sis_in(c, 0x2030, 0x203E) ||
           sis_in(c, 0x2041, 0x2053) || sis_in(c, 0x2055, 0x205E) ||
           sis_in(c, 0x2190, 0x245F) || sis_in(c, 0x2500, 0x2775) ||
           sis_in(c, 0x2794, 0x2BFF) || sis_in(c, 0x2E00, 0x2E7F) ||
           sis_in(c, 0x3001, 0x3003) || sis_in(c, 0x3008, 0x3020) ||
           c == 0x3030 || sis_in(c, 0xFD3E, 0xFD3F) ||
           sis_in(c, 0xFE45, 0xFE46);
}

/* ── General_Category blocks used by the Immutable profile (§3) ───── */

static int sis_is_control(uint32_t c)
{
    return sis_in(c, 0x0000, 0x001F) || sis_in(c, 0x007F, 0x009F);
}

static int sis_is_private_use(uint32_t c)
{
    return sis_in(c, 0xE000, 0xF8FF) || sis_in(c, 0xF0000, 0xFFFFD) ||
           sis_in(c, 0x100000, 0x10FFFD);
}

static int sis_is_surrogate(uint32_t c)
{
    return sis_in(c, 0xD800, 0xDFFF);
}

static int sis_is_noncharacter(uint32_t c)
{
    if (sis_in(c, 0xFDD0, 0xFDEF))
        return 1;
    /* Every plane ends in FFFE/FFFF. */
    return (c & 0xFFFEu) == 0xFFFEu && c <= 0x10FFFFu;
}

/* ── Extended_Pictographic / Emoji_Component (hashtag profile, §6) ── */

static int sis_is_extpict(uint32_t c)
{
    if (c == 0x00A9 || c == 0x00AE || c == 0x203C || c == 0x2049 ||
        c == 0x2122 || c == 0x2139)
        return 1;
    if (sis_in(c, 0x2194, 0x2199) || sis_in(c, 0x21A9, 0x21AA) ||
        sis_in(c, 0x231A, 0x231B) || c == 0x2328 ||
        c == 0x23CF || sis_in(c, 0x23E9, 0x23F3) ||
        sis_in(c, 0x23F8, 0x23FA) || c == 0x24C2 ||
        sis_in(c, 0x25AA, 0x25AB) || c == 0x25B6 || c == 0x25C0 ||
        sis_in(c, 0x25FB, 0x25FE) || sis_in(c, 0x2600, 0x27BF) ||
        sis_in(c, 0x2934, 0x2935) || sis_in(c, 0x2B05, 0x2B07) ||
        sis_in(c, 0x2B1B, 0x2B1C) || c == 0x2B50 || c == 0x2B55 ||
        c == 0x3030 || c == 0x303D || c == 0x3297 || c == 0x3299)
        return 1;
    return sis_in(c, 0x1F000, 0x1FAFF);
}

static int sis_is_emoji_component(uint32_t c)
{
    return c == 0x0023 || c == 0x002A || sis_in(c, 0x0030, 0x0039) ||
           c == 0x200D || c == 0x20E3 || c == 0xFE0F ||
           sis_in(c, 0x1F1E6, 0x1F1FF) || sis_in(c, 0x1F3FB, 0x1F3FF) ||
           sis_in(c, 0x1F9B0, 0x1F9B3) || sis_in(c, 0xE0020, 0xE007F);
}

/* ── Zone helpers ─────────────────────────────────────────────────── */

bool suas_sis_is_scp(sucs_ex_char_t cp)
{
    return cp >= SUAS_SIS_ZONE_SCP_MIN && cp <= SUAS_SIS_ZONE_SCP_MAX;
}

/* A codepoint in the Native / plugin planes that is not a trap or the
 * Sentinel. RNUR / RCCR conlang codepoints live here. */
static int sis_is_native(sucs_ex_char_t cp)
{
    if (cp >= SUAS_SIS_ZONE_NATIVE_MIN && cp <= SUAS_SIS_ZONE_NATIVE_LAST)
        return 1;
    if (cp > SUAS_SIS_ZONE_SENTINEL)
        return 1; /* extSUCS plugin plane */
    return 0;
}

/* ── Immutable-profile admissibility (§3) ─────────────────────────── */

static int sis_immutable_ok(sucs_ex_char_t cp)
{
    if (cp > SUAS_SIS_ZONE_UNICODE_MAX)
        return sis_is_native(cp); /* Native ok; traps/sentinel not */
    if (sis_is_pattern_ws((uint32_t)cp) || sis_is_pattern_syntax((uint32_t)cp) ||
        sis_is_control((uint32_t)cp) || sis_is_private_use((uint32_t)cp) ||
        sis_is_surrogate((uint32_t)cp) || sis_is_noncharacter((uint32_t)cp))
        return 0;
    return 1;
}

/* ── Per-profile structural predicates ────────────────────────────── */

static bool sis_start_default(sucs_ex_char_t cp)
{
    if (cp > SUAS_SIS_ZONE_UNICODE_MAX)
        return sis_is_native(cp) != 0;
    return sis_is_letter((uint32_t)cp) || sis_is_other_id_start((uint32_t)cp) ||
           cp == 0x005F; /* Table 3: LOW LINE (underscore) as a start char */
}

static bool sis_continue_default(sucs_ex_char_t cp)
{
    if (cp > SUAS_SIS_ZONE_UNICODE_MAX)
        return sis_is_native(cp) != 0;
    return sis_start_default(cp) ||
           sis_is_nonspacing((uint32_t)cp) ||
           sis_is_spacingmark((uint32_t)cp) ||
           sis_is_digit((uint32_t)cp) ||
           sis_is_connector((uint32_t)cp) ||
           sis_is_other_id_continue((uint32_t)cp);
}

static bool sis_start_immutable(sucs_ex_char_t cp)
{
    return sis_immutable_ok(cp) != 0;
}

static bool sis_start_hashtag(sucs_ex_char_t cp)
{
    return cp == 0x0023 || cp == 0xFE5F || cp == 0xFF03;
}

static bool sis_continue_hashtag(sucs_ex_char_t cp)
{
    /* UAX31-R8-1: XID_Continue + Extended_Pictographic + Emoji_Component
     * + '-' + '+' + '_', minus the Start characters. */
    if (cp == 0x002D || cp == 0x002B || cp == 0x005F)
        return true;
    if (cp > SUAS_SIS_ZONE_UNICODE_MAX)
        return sis_is_native(cp) != 0;
    if (sis_start_hashtag(cp))
        return false; /* '#' variants are Start-only */
    return sis_continue_default(cp) || sis_is_extpict((uint32_t)cp) ||
           sis_is_emoji_component((uint32_t)cp);
}

/* ── Public class queries ─────────────────────────────────────────── */

bool suas_sis_is_start_ex(sucs_ex_char_t cp, suas_sis_profile_t profile)
{
    switch (profile) {
    case SUAS_SIS_PROFILE_IMMUTABLE: return sis_start_immutable(cp);
    case SUAS_SIS_PROFILE_HASHTAG:   return sis_start_hashtag(cp);
    case SUAS_SIS_PROFILE_DEFAULT:
    default:                         return sis_start_default(cp);
    }
}

bool suas_sis_is_start(sucs_ex_char_t cp)
{
    return suas_sis_is_start_ex(cp, SUAS_SIS_PROFILE_DEFAULT);
}

bool suas_sis_is_continue_ex(sucs_ex_char_t cp, suas_sis_profile_t profile)
{
    switch (profile) {
    case SUAS_SIS_PROFILE_IMMUTABLE: return sis_start_immutable(cp);
    case SUAS_SIS_PROFILE_HASHTAG:   return sis_continue_hashtag(cp);
    case SUAS_SIS_PROFILE_DEFAULT:
    default:                         return sis_continue_default(cp);
    }
}

bool suas_sis_is_continue(sucs_ex_char_t cp)
{
    return suas_sis_is_continue_ex(cp, SUAS_SIS_PROFILE_DEFAULT);
}

suas_sis_class_t suas_sis_get_class_ex(sucs_ex_char_t cp,
                                       suas_sis_profile_t profile)
{
    if (suas_sis_is_start_ex(cp, profile))
        return SUAS_SIS_CLASS_START;
    if (suas_sis_is_continue_ex(cp, profile))
        return SUAS_SIS_CLASS_CONTINUE;
    return SUAS_SIS_CLASS_INVALID;
}

suas_sis_class_t suas_sis_get_class(sucs_ex_char_t cp)
{
    return suas_sis_get_class_ex(cp, SUAS_SIS_PROFILE_DEFAULT);
}

/* ── Lexical syntax (§4) ──────────────────────────────────────────── */

suas_sis_syntax_t suas_sis_get_syntax(sucs_ex_char_t cp)
{
    uint32_t c;

    if (suas_sis_is_scp(cp))
        return SUAS_SIS_SYNTAX_CONTROL;
    if (cp > SUAS_SIS_ZONE_UNICODE_MAX)
        return (cp >= SUAS_SIS_ZONE_TRAP_MIN &&
                cp <= SUAS_SIS_ZONE_SENTINEL)
                   ? SUAS_SIS_SYNTAX_CONTROL
                   : SUAS_SIS_SYNTAX_LITERAL;
    c = (uint32_t)cp;

    if (c == 0x200E || c == 0x200F)
        return SUAS_SIS_SYNTAX_IGNORABLE;
    if (c == 0x000A || c == 0x000B || c == 0x000C || c == 0x000D ||
        c == 0x0085 || c == 0x2028 || c == 0x2029)
        return SUAS_SIS_SYNTAX_LINETERM;
    if (c == 0x0009 || c == 0x0020)
        return SUAS_SIS_SYNTAX_WHITESPACE;
    if (sis_is_pattern_syntax(c))
        return SUAS_SIS_SYNTAX_OPERATOR;
    return SUAS_SIS_SYNTAX_LITERAL;
}

/* ── Names ────────────────────────────────────────────────────────── */

const char* suas_sis_version_string(void)
{
    return "1.0.0";
}

const char* suas_sis_class_name(suas_sis_class_t klass)
{
    switch (klass) {
    case SUAS_SIS_CLASS_START:    return "Start";
    case SUAS_SIS_CLASS_CONTINUE: return "Continue";
    case SUAS_SIS_CLASS_INVALID:
    default:                      return "Invalid";
    }
}

const char* suas_sis_syntax_name(suas_sis_syntax_t syntax)
{
    switch (syntax) {
    case SUAS_SIS_SYNTAX_WHITESPACE: return "Whitespace";
    case SUAS_SIS_SYNTAX_LINETERM:   return "LineTerm";
    case SUAS_SIS_SYNTAX_IGNORABLE:  return "Ignorable";
    case SUAS_SIS_SYNTAX_OPERATOR:   return "Operator";
    case SUAS_SIS_SYNTAX_CONTROL:    return "Control";
    case SUAS_SIS_SYNTAX_LITERAL:
    default:                         return "Literal";
    }
}

const char* suas_sis_profile_name(suas_sis_profile_t profile)
{
    switch (profile) {
    case SUAS_SIS_PROFILE_IMMUTABLE: return "Immutable";
    case SUAS_SIS_PROFILE_HASHTAG:   return "Hashtag";
    case SUAS_SIS_PROFILE_DEFAULT:
    default:                         return "Default";
    }
}

/* ── State machine ────────────────────────────────────────────────── */

void suas_sis_init(suas_sis_state_t* state)
{
    if (!state)
        return;
    state->is_started = false;
    state->length = 0;
}

bool suas_sis_is_started(const suas_sis_state_t* state)
{
    return state ? state->is_started : false;
}

uint32_t suas_sis_length(const suas_sis_state_t* state)
{
    return state ? state->length : 0u;
}

bool suas_sis_feed(suas_sis_state_t* state, sucs_ex_char_t cp)
{
    return suas_sis_feed_ex(state, cp, SUAS_SIS_PROFILE_DEFAULT);
}

bool suas_sis_feed_ex(suas_sis_state_t* state, sucs_ex_char_t cp,
                      suas_sis_profile_t profile)
{
    if (!state)
        return false;

    if (!state->is_started) {
        if (suas_sis_is_start_ex(cp, profile)) {
            state->is_started = true;
            state->length = 1u;
            return true;
        }
        return false;
    }

    if (suas_sis_is_continue_ex(cp, profile)) {
        state->length++;
        return true;
    }

    /* The codepoint terminates the identifier in progress. */
    state->is_started = false;
    state->length = 0u;
    return false;
}
