#ifndef SUTS_SUTS_SCTS_H
#define SUTS_SUTS_SCTS_H

/* SUTS-012 - System Console Text Service (SCTS)
 *
 * The optional console contract: a caller-supplied cell plane, a cursor, a
 * wrap and scroll model, a sink transport, and a reserved glyph-zone
 * registry. Width comes from SUAS-002 (SGW), break opportunities from
 * SUAS-003 (SBR), and cluster boundaries from SUAS-006 (STS).
 *
 * Zero allocation, freestanding C99.
 *
 * See docs/suts/SUTS-012-scts.md for the normative specification.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "suas/suas_sgw.h"
#include "suas/suas_sbr.h"
#include "suas/suas_sts.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -- Version ---------------------------------------------------------- */

#define SUTS_SCTS_SPEC_ID      "SUTS-012"
#define SUTS_SCTS_VERSION_MAJOR 1
#define SUTS_SCTS_VERSION_MINOR 0
#define SUTS_SCTS_VERSION_PATCH 0

/* -- Reserved markers (section 7.3) ----------------------------------- */
/* Free slots in the tail of the BMP Private Use Area, allocated before any
 * renderable glyph so that a marker can never be mistaken for content. */
#define SCTS_MARK_CONT   0x0000EFE0u /* second cell of a wide glyph */
#define SCTS_MARK_BLANK  0x0000EFE1u /* empty cell                  */

/* -- Fixed capacities ------------------------------------------------- */

#define SCTS_MAX_CLUSTERS     64u  /* cluster records per plane */
#define SCTS_MAX_CLUSTER_CP   8u   /* codepoints per cluster    */
#define SCTS_MAX_ZONES        16u  /* glyph-zone records        */

/* -- Status (section 8) ----------------------------------------------- */

typedef enum {
    SCTS_OK             = 0,
    SCTS_E_NOT_INIT     = 1,
    SCTS_E_NULL         = 2,
    SCTS_E_ROW_RANGE    = 3,
    SCTS_E_COL_RANGE    = 4,
    SCTS_E_CLUSTER      = 5,
    SCTS_E_CONTROL      = 6,
    SCTS_E_ENCODING     = 7,
    SCTS_E_INADMISSIBLE = 8,
    SCTS_E_CAPACITY     = 9,
    SCTS_E_NO_SINK      = 10,
    SCTS_E_SINK         = 11,
    SCTS_E_GEOMETRY     = 12,
    SCTS_E_ZONE         = 13
} scts_status_t;

/* -- Cell attributes -------------------------------------------------- */

#define SCTS_ATTR_FG_MASK    0x000Fu
#define SCTS_ATTR_BG_MASK    0x00F0u
#define SCTS_ATTR_BG_SHIFT   4
#define SCTS_ATTR_WIDE_LEAD  0x0100u
#define SCTS_ATTR_WIDE_TAIL  0x0200u

typedef struct {
    uint32_t cp;     /* codepoint, or SCTS_MARK_CONT / SCTS_MARK_BLANK */
    uint16_t attrs;  /* fg/bg bits plus the wide-pair flags            */
} scts_cell_t;

/* -- Sinks (section 4.5, 7.1) ----------------------------------------- */

typedef enum {
    SCTS_SINK_L0_BYTE    = 0, /* ASCII only, lossy            */
    SCTS_SINK_L1_SUTF8   = 1, /* whole SUTF-8 units           */
    SCTS_SINK_L2_ROW     = 2, /* whole serialized rows        */
    SCTS_SINK_L3_SURFACE = 3  /* cells plus a font (row form) */
} scts_sink_level_t;

/* The sink receives whole SUTF-8 codepoints, never a partial sequence. It
 * must not retain the buffer and must not call back into the plane. */
typedef void (*scts_emit_fn)(uintptr_t token, const uint8_t *sutf8,
                             uint32_t len);

typedef struct {
    scts_emit_fn     emit;
    uintptr_t        token;
    scts_sink_level_t level;
} scts_sink_t;

/* -- Cluster record --------------------------------------------------- */

typedef struct {
    uint16_t row;      /* grid position of the base cell        */
    uint16_t col;
    uint8_t  len;      /* codepoints stored in cps              */
    uint8_t  width;    /* cells occupied (1 or 2)               */
    uint32_t cps[SCTS_MAX_CLUSTER_CP];
} scts_cluster_t;

/* -- Plane (section 4.2) ---------------------------------------------- */

typedef struct {
    scts_cell_t *cells;      /* rows * cols, caller-owned           */
    uint16_t     rows;
    uint16_t     cols;
    uint16_t     cursor_row;
    uint16_t     cursor_col; /* may equal cols as a transient state */
    uint16_t     fg;
    uint16_t     bg;
    uint8_t      tab_width;
    bool         wrap;
    bool         wide_context;
    bool         initialized;

    uint64_t     writes;
    uint64_t     scrolls;

    /* Dirty tracking. Rows below 64 are tracked individually; a plane taller
     * than that reports a full repaint through dirty_all. */
    uint64_t     dirty;
    bool         dirty_all;

    const suas_sgw_options_t *sgw;
    const suas_sbr_options_t *sbr;

    scts_cluster_t clusters[SCTS_MAX_CLUSTERS];
    uint16_t       cluster_count;

    scts_sink_t sink;
    bool        sink_bound;
    uint32_t    lossy_substitutions;
} scts_plane_t;

typedef struct {
    uint16_t tab_width;    /* 0 -> 8 */
    bool     wrap;         /* false -> fail at the right edge */
    bool     wide_context; /* SGW Ambiguous resolution         */
} scts_plane_options_t;

/* -- Lifecycle -------------------------------------------------------- */

const char   *suts_scts_version_string(void);
const char   *suts_scts_status_name(scts_status_t st);
void          scts_plane_options_default(scts_plane_options_t *o);

/* Construct a plane over caller storage. tab_width 0 defaults to 8. */
scts_status_t scts_plane_init(scts_plane_t *p, scts_cell_t *cells,
                              uint16_t rows, uint16_t cols,
                              const scts_plane_options_t *o);

/* Fill with blanks and reset the cursor. */
scts_status_t scts_plane_clear(scts_plane_t *p);

/* Set the default style applied to subsequent writes. */
scts_status_t scts_plane_set_style(scts_plane_t *p, uint16_t fg, uint16_t bg);

/* -- Writing ---------------------------------------------------------- */

/* Place a SUTF-8 stream. Clusters are delimited by STS; width is SGW's
 * answer for the cluster base; wrap points are SBR opportunities. */
scts_status_t scts_plane_write(scts_plane_t *p, const uint8_t *sutf8,
                               size_t len);

/* Place one codepoint as a one- or two-cell cluster of its own. */
scts_status_t scts_plane_put_cp(scts_plane_t *p, uint32_t cp);

/* Cursor and control operations (section 6). */
scts_status_t scts_plane_newline(scts_plane_t *p);
scts_status_t scts_plane_carriage_return(scts_plane_t *p);
scts_status_t scts_plane_backspace(scts_plane_t *p);
scts_status_t scts_plane_tab(scts_plane_t *p);
scts_status_t scts_plane_scroll(scts_plane_t *p, uint16_t n);

/* -- Reading ---------------------------------------------------------- */

/* Serialize one row to SUTF-8. Fails rather than truncating. */
scts_status_t scts_plane_serialize_row(const scts_plane_t *p, uint16_t row,
                                       uint8_t *buf, size_t cap, bool trim,
                                       size_t *out_len);

/* Copy the codepoints of the cluster whose base is at (row, col). */
scts_status_t scts_plane_cluster_at(const scts_plane_t *p, uint16_t row,
                                    uint16_t col, uint32_t *out_cps,
                                    size_t cap, size_t *out_count);

/* -- Sink transport --------------------------------------------------- */

void          scts_plane_set_sink(scts_plane_t *p, const scts_sink_t *sink);

/* Emit one row. L0/L1 accept scratch == NULL; L2/L3 require a scratch buffer
 * large enough for the serialized row. */
scts_status_t scts_plane_emit_row(scts_plane_t *p, uint16_t row,
                                  uint8_t *scratch, size_t scratch_cap);

/* Emit every dirty row in increasing order and clear the dirty set. */
scts_status_t scts_plane_flush(scts_plane_t *p, uint8_t *scratch,
                               size_t scratch_cap);

bool scts_plane_row_dirty(const scts_plane_t *p, uint16_t row);
void scts_plane_clear_dirty(scts_plane_t *p);

/* -- Glyph-zone registry (section 7.4) -------------------------------- */

typedef struct {
    uint32_t    start;
    uint32_t    end;       /* inclusive */
    const char *owner;
    const char *purpose;
    uint8_t     reusable;
    uint8_t     used;
} scts_zone_t;

typedef struct {
    scts_zone_t zones[SCTS_MAX_ZONES];
    size_t      count;
} scts_zone_table_t;

/* Initialise and reserve SCTS_MARK_CONT and SCTS_MARK_BLANK first (Z4). */
void scts_zones_init(scts_zone_table_t *t);

/* Register a disjoint block (Z1, Z2). Fails with SCTS_E_ZONE on overlap or
 * when the table is full. */
scts_status_t scts_zone_register(scts_zone_table_t *t, uint32_t start,
                                 uint32_t end, const char *owner,
                                 const char *purpose, bool reusable);

const scts_zone_t *scts_zone_find(const scts_zone_table_t *t, uint32_t cp);
bool                scts_zone_overlaps(const scts_zone_table_t *t,
                                       uint32_t start, uint32_t end);
bool                scts_zone_is_reserved_marker(uint32_t cp);

#ifdef __cplusplus
}
#endif

#endif /* SUTS_SUTS_SCTS_H */
