#ifndef SUTF_MASTER_H
#define SUTF_MASTER_H

/**
 * SuperUnicode Transformation Format (SUTF) Master Transport Header
 *
 * SUTF and extSUTF are strictly TEXT FORMATTING AND SERIALIZATION TRANSPORTS.
 * They define the physical byte-packing, bit-alignment, memory layouts, and
 * stream framing rules for storing and transmitting SUCS and ExtSUCS codepoints.
 *
 * Base SUTF Transports (Included):
 * - SUTF-8:  1..6 Byte Stream Transport
 * - SUTF-16: 1..2 16-Bit Word Stream Transport
 * - SUTF-4:  4-Bit Hex Nibble Stream Transport
 * - SUTF-2:  2-Bit Symbol Frame Stream Transport
 * - Kernel Mode-Switching: <sucs_mode.h> (Base <-> ExtSUCS system restart controller)
 *
 * Extended extSUTF Transports (Referenced / Forwarded if on include path):
 * - SUTF-32/64/128/256/512/N: Fixed-Width Vector Transports (<extsutf_fixed.h>)
 * - vSUTF: Variable Multi-Byte Streaming Transport (<vsutf.h>)
 * - e-SUTF: Hypervisor Page-Mapped Virtual IPC Transport (<esutf.h>)
 */

#include "sucs_types.h"
#include "sucs_mode.h"
#include "sutf8.h"
#include "sutf16.h"
#include "sutf4.h"
#include "sutf2.h"

/* Forwarding references for extSUTF transports if superunicode_extended is on include path */
#if defined(__has_include)
  #if __has_include("extsutf_fixed.h")
    #include "extsutf_fixed.h"
  #endif
  #if __has_include("vsutf.h")
    #include "vsutf.h"
  #endif
  #if __has_include("esutf.h")
    #include "esutf.h"
  #endif
#endif

#endif /* SUTF_MASTER_H */
