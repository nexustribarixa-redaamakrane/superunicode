# SUTS-015 - System Text Rendering Subsystem (STRS)

**Standard ID:** SUTS-015
**Status:** Draft - Authoring
**Scope:** Shared rendering metrics and vertical orientation routing
**Applies To:** SUAS-002, SUAS-014, SUAS-015, SUAS-016, SUAS-017, SUAS-009
**Last Updated:** 2026-11-02

An implementation that claims SUTS-015 conformance MUST satisfy every MUST in
this document, and the claim is false if it does not.

## 1. Overview

STRS provides one rendering query surface while retaining specialized data ownership.

## 2. Scope

STRS covers cell width and orientation lookup; shaping and font selection remain
owned by their respective specifications.

## 3. Relationship to the Authoritative Standards

SGW owns width and SUVTL owns vertical orientation. STRS MUST defer to both.

## 4. Model

STRS queries are bounded, allocation-free, and deterministic.

## 5. Conformance Requirements

1. **[C1]** STRS MUST return SGW's resolved width. (Section 4)
2. **[C2]** STRS MUST return SUVTL's orientation. (Section 4)

## 6. Testing

Tests MUST cover narrow, wide, and ambiguous width contexts.

## 7. Implementation Evidence

`include/suts/suts_strs.h` and `src/suts/suts_strs.c` are the reference implementation.

## Terms and Conventions

STRS is distinct from the SUTR document-family prefix.
