# SUTN-004 - Character Data Provenance

**Standard ID:** SUTN-004
**Status:** 0.1.0 - Published
**Scope:** Informative practice for recording source, transformation, and verification of character data
**Applies To:** SUCD consumers, generated tables, data publishers, and release reviewers
**Last Updated:** 2026-11-02

## 1. Purpose and Non-Normative Status

This note is non-normative. Following it does not establish conformance to
SuperUnicode, any SUAS standard, or any SUTS specification.

The note recommends a provenance record for generated character-data artefacts.
It does not add a data-source requirement to SUAS-012 (SUCD).

## 2. Audience and Prerequisites

This note is for data maintainers and release engineers. Readers should know
SUAS-011 (SCMS), SUAS-012 (SUCD), and SUTR-8 (SUCPM).

## 3. Suggested Provenance Record

Record the upstream release, source filenames, retrieval date, transformation
tool version, generator arguments, checksum, and verification result. Keep the
record beside the generated artefact or in the release manifest.

## 4. Review Practice

Reviewers should be able to reproduce the transformation from the recorded
inputs. A changed source, generator, or argument should produce a visible
provenance change rather than an unexplained binary difference.

## 5. Relationship to Normative Documents

SUAS-011 and SUAS-012 define manifest and database behavior. This note only
describes evidence that makes generated data easier to review.

## 6. Lifecycle and Review

Review at every data release and annually. Withdraw the note if provenance is
made a normative requirement in the owning specification.

## 7. Owner and Maintainer

Owner: Character Data Maintainer. Maintainer: SuperUnicode data-release
maintainers.

## 8. Revision History

| Version | Date | Change |
|---|---|---|
| 0.1.0 | 2026-11-02 | Initial publication. |

## 9. Terms and Conventions

Provenance records origin and transformation history; it does not itself
certify semantic correctness.
