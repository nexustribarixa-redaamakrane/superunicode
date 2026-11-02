# SUAS-022 - System Kernel Test and Conformance (SKTC)

**Standard ID:** SUAS-022
**Status:** Draft - Authoring
**Scope:** Aggregate result accounting for SuperUnicode and kernel conformance gates
**Applies To:** SUAS, SUTS, SUTR reference tests, operating-system host tests
**Last Updated:** 2026-11-02

## 1. Overview

SKTC defines a small result accumulator for aggregate conformance runners.

## 2. Scope

SKTC records results and does not replace any individual specification test.

## 3. Definitions

- **K1:** A complete report has at least one passing check and no failures.

## 4. Normative Model

An implementation claiming SKTC MUST account for every executed check and MUST
fail the aggregate when any check fails.

## 5. Conformance Requirements

1. **[C1]** SKTC MUST reject a report containing a failure. (Section 4)
2. **[C2]** SKTC MUST reject an empty report. (Section 4)

## 6. Implementation Evidence

`include/suas/suas_sktc.h` and `src/suas/suas_sktc.c` provide the portable
counter; OS runners may feed their own results.

## 7. Testing

Tests MUST cover empty, passing, and failing reports.

## Terms and Conventions

SKTC is an aggregate gate, not a runtime text subsystem.
