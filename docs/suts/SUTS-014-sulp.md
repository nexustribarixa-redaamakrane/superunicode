# SUTS-014 - SuperUnicode Locale and Presentation (SULP)

**Standard ID:** SUTS-014
**Status:** Draft - Authoring
**Scope:** Locale lookup and presentability routing
**Applies To:** SUTS-003 (SLDF), SUTS-007 (SUEPS), locale and presentation consumers
**Last Updated:** 2026-11-02

An implementation that claims SUTS-014 conformance MUST satisfy every MUST in
this document, and the claim is false if it does not.

## 1. Overview

SULP joins locale and presentation services at a portable boundary.

## 2. Scope

SULP does not duplicate locale tables or emoji data.

## 3. Relationship to the Authoritative Standards

SLDF owns locale records and SUEPS owns emoji presentation classification.

## 4. Model

SULP MUST return the delegated locale record and MUST report unsupported
presentation values as false.

## 5. Conformance Requirements

1. **[C1]** SULP MUST preserve SLDF locale identity. (Section 4)
2. **[C2]** SULP MUST preserve SUEPS presentation decisions. (Section 4)

## 6. Testing

Tests MUST cover unknown locale tags and representative presentation values.

## 7. Implementation Evidence

`include/suts/suts_sulp.h` and `src/suts/suts_sulp.c` are the reference implementation.

## Terms and Conventions

SULP means SuperUnicode Locale and Presentation.
