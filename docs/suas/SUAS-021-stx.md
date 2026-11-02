# SUAS-021 - System Text Services (STX)

**Standard ID:** SUAS-021
**Status:** Draft - Authoring
**Scope:** Shared text-service entry point over segmentation and text primitives
**Applies To:** SUAS-002, SUAS-003, SUAS-006, SUAS-007, operating-system text paths
**Last Updated:** 2026-11-02

## 1. Overview

STX provides a stable front door for system text processing while preserving
the authority of the existing SUAS modules.

## 2. Scope

STX coordinates bounded state and boundary reporting. Existing SUAS modules
remain the normative owners of their individual algorithms.

## 3. Definitions

- **T1:** A text unit is processed without heap allocation.

## 4. Normative Model

An implementation claiming STX MUST preserve the result of the delegated SUAS
segmentation operation and MUST report final pending state explicitly.

## 5. Conformance Requirements

1. **[C1]** STX MUST initialize deterministic state. (Section 4)
2. **[C2]** STX MUST NOT silently discard a pending final unit. (Section 4)

## 6. Implementation Evidence

`include/suas/suas_stx.h` and `src/suas/suas_stx.c` provide the portable state
adapter; `test_subsystems` covers initialization and finalization.

## 7. Testing

Tests MUST cover empty, one-unit, and multi-unit streams.

## Terms and Conventions

STX is the collision-free subsystem identifier; SUAS-006 STS remains unchanged.
