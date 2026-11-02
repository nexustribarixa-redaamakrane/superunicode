# SuperUnicode Subsystem Classification

**Date:** 2026-11-02  
**Status:** Architectural registry  
**Scope:** The ten proposed system and SuperUnicode subsystems

## Ownership model

SuperUnicode owns the architectural definitions, standards publications,
reference interfaces, and canonical contracts. Operating systems and kernels,
including OpenWindows, may independently implement, extend, integrate, and
maintain those contracts. A reference implementation in this repository is
evidence of a portable interface only; it is not an exclusive implementation
claim.

The registry is implemented by
[`suas_subsystems.h`](../../include/suas/suas_subsystems.h) and
[`suas_subsystems.c`](../../src/suas/suas_subsystems.c). OS-owned entries do
not fabricate source code that is unavailable in this repository.

## Acronym collision table

The proposed names are renamed where they collide with, or are too easily
confused with, an established architecture identifier. Existing specifications
and modules retain their names and meanings.

| Before | After | Reason |
|---|---|---|
| SUCDA | SUCHA | Avoid confusion with established SUCD |
| STS | STX | Existing SUAS-006 STS is text segmentation |
| SUIC | SUICL | SUIC is already the SUTN-001 module label |
| SULP | SULP | No established collision |
| SUTRS | STRS | Avoid the SUTR document-family prefix |
| SFS | SFS | No established collision |
| SIM | SIM | No established collision |
| SIS | SIE | Existing SUAS-007 SIS is identifiers and syntax |
| SKC | SKC | No established collision |
| SKTC | SKTC | No established collision |

The before/after names are subsystem identifiers, not replacements for
SUAS-006, SUAS-007, SUTN-001, or any other established document identifier.

## Classified subsystems

| Identifier | Subsystem | Family | Standards ownership | Reference status |
|---|---|---|---|---|
| SUCHA | SuperUnicode Character Authority | SUAS | SuperUnicode | `suas_sucd` |
| STX | System Text Services | SUAS | SuperUnicode | existing text modules |
| SUICL | SuperUnicode Identifier and Collation Layer | SUTS | SuperUnicode | existing identifier/collation modules |
| SULP | SuperUnicode Locale and Presentation | SUTS | SuperUnicode | existing locale/presentation modules |
| STRS | System Text Rendering Subsystem | SUTS | SuperUnicode | existing rendering/data modules |
| SFS | System Filesystem | SUTR | Shared | OpenWindows implementation |
| SIM | System IPC and Messaging | SUTR | Shared | OpenWindows implementation |
| SIE | System Input Events | SUTR | Shared | OpenWindows implementation |
| SKC | System Kernel Console | SUTR | Shared | OpenWindows implementation |
| SKTC | System Kernel Test and Conformance | SUAS | Shared | aggregate gate pending |

## Dependency order

```text
SUCHA ─┬─> STX ─┬─> SUICL
       │        ├─> SULP
       │        └─> STRS
       ├─> SFS
       └─> SKC

SIM ─────> SFS, existing SIS contract
SIE ─────> SIM, STX
SKTC ────> all entries (verification only)
```

The existing SIS and STS modules remain usable as implementation substrates;
the collision-free subsystem names above are the architectural labels for
their broader integration roles.

## Verification

`test_suas_subsystems` checks the ten-entry count, uniqueness, rejected legacy
aliases, and ownership/reference status. It does not claim that OpenWindows
implements an OS-owned subsystem.
