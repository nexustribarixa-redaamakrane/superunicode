# SUTS-013 - SuperUnicode Identifier and Collation Layer (SUICL)

**Standard ID:** SUTS-013
**Status:** Draft - Authoring
**Scope:** Optional identifier validation and deterministic codepoint ordering
**Applies To:** SUAS-007 (SIS), SUTS-001 (SUCA), SUTS-005, SUTS-008, SUTS-006
**Last Updated:** 2026-11-02

An implementation that claims SUTS-013 conformance MUST satisfy every MUST in
this document, and the claim is false if it does not.

## 1. Overview

SUICL combines existing identifier and collation primitives without redefining them.

## 2. Scope

SUICL covers bounded identifier validation and total ordering.

## 3. Relationship to the Authoritative Standards

SUAS-007 owns identifier classes. SUTS-001 owns collation behavior. SUICL
delegates to those authorities.

## 4. Model

Validation MUST reject null and empty input and MUST require a valid start
followed only by valid continuation codepoints.

## 5. Conformance Requirements

1. **[C1]** SUICL MUST reject invalid identifier starts. (Section 4)
2. **[C2]** SUICL MUST return a deterministic comparison result. (Section 4)

## 6. Testing

Tests MUST cover valid, invalid, equal, and ordered inputs.

## 7. Implementation Evidence

`include/suts/suts_suicl.h` and `src/suts/suts_suicl.c` are the reference implementation.

## Terms and Conventions

SUICL is distinct from SUTN-001's historical SUIC label.
