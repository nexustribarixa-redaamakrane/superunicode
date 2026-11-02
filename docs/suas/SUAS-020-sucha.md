# SUAS-020 - SuperUnicode Character Authority (SUCHA)

**Standard ID:** SUAS-020
**Status:** Draft - Authoring
**Scope:** Authoritative access to the SUCD character-data image
**Applies To:** SUAS-012 (SUCD), character-data consumers, operating-system integrations
**Last Updated:** 2026-11-02

## 1. Overview

SUCHA defines the single authority boundary for character data. It delegates
records and properties to SUCD rather than copying tables.

## 2. Scope

SUCHA owns authority and query routing. It does not define a second database
format or replace SUAS-012.

## 3. Definitions

- **A1:** The SUCD image is the authoritative character-data source.

## 4. Normative Model

An implementation claiming SUCHA MUST route character-data queries through
SUCD and MUST NOT publish a conflicting duplicate table.

## 5. Conformance Requirements

1. **[C1]** SUCHA MUST expose the SUCD record and property query results. (Section 4)
2. **[C2]** SUCHA MUST report unavailable data explicitly. (Section 4)

## 6. Implementation Evidence

The portable reference is `include/suas/suas_sucha.h` and
`src/suas/suas_sucha.c`; `test_subsystems` exercises the boundary.

## 7. Testing

Tests MUST cover loaded and unavailable SUCD images and representative planes.

## Terms and Conventions

SUCHA means SuperUnicode Character Authority; SUCD means SuperUnicode Character Database.
