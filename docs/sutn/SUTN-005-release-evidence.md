# SUTN-005 - Release Evidence Pack

**Standard ID:** SUTN-005
**Status:** 0.1.0 - Published
**Scope:** Informative contents of a repeatable SuperUnicode release evidence pack
**Applies To:** release engineers, maintainers, integrators, and reviewers
**Last Updated:** 2026-11-02

## 1. Purpose and Non-Normative Status

This note is non-normative. Following it does not establish conformance to
SuperUnicode, any SUAS standard, or any SUTS specification.

This note describes a practical release packet so that test results,
documentation state, generated data, and ownership decisions can be reviewed
together.

## 2. Audience and Prerequisites

The audience is anyone preparing or reviewing a repository release. The
reader should be able to run the repository's configured build and tests.

## 3. Suggested Contents

Include the source revision, build configuration, compiler identity,
`verify-docs` output, complete test output, generated-data provenance, changed
specification list, and unresolved questions. Record platform-specific results
separately from portable results.

## 4. Evidence Labels

Use **present**, **built**, **tested**, and **claimed** as separate labels.
“Built” should identify the target; “tested” should identify the test; and
“claimed” should identify the owner who made the claim.

## 5. Relationship to Normative Documents

SUTR-10 (SUCM) describes the conformance model. This note organizes release
evidence but does not create a conformance tier or claim condition.

## 6. Lifecycle and Review

Review the packet format each release cycle and annually. Withdraw this note
if a release process replaces it with a maintained project procedure.

## 7. Owner and Maintainer

Owner: Release Engineering Maintainer. Maintainer: SuperUnicode release
maintainers.

## 8. Revision History

| Version | Date | Change |
|---|---|---|
| 0.1.0 | 2026-11-02 | Initial publication. |

## 9. Terms and Conventions

An evidence pack records observed results; it is not proof beyond the tests and
inputs it identifies.
