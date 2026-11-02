# SUTN-006 - Zero-Allocation Integration Patterns

**Standard ID:** SUTN-006
**Status:** 0.1.0 - Published
**Scope:** Informative implementation patterns for integrating bounded text services without heap allocation
**Applies To:** kernels, freestanding adapters, embedded systems, and portable SuperUnicode references
**Last Updated:** 2026-11-02

## 1. Purpose and Non-Normative Status

This note is non-normative. Following it does not establish conformance to
SuperUnicode, any SUAS standard, or any SUTS specification.

This note is not a Unicode-analogy note. It records implementation patterns
that also apply to non-Unicode subsystems such as IPC queues, console buffers,
and filesystem boundaries.

## 2. Audience and Prerequisites

The audience is C implementers working with caller-owned storage. Readers
should understand explicit lengths, capacities, and failure returns.

## 3. Patterns

Prefer fixed-capacity records, explicit lengths, caller-owned buffers, bounded
loops, and status returns. Keep allocation policy outside the primitive so a
kernel, embedded target, or hosted application can choose its own ownership
model.

## 4. Review Questions

Reviewers should ask whether every write has a capacity check, whether every
queue has a full condition, whether partial results are distinguishable from
success, and whether a null or overflow input has an explicit result.

## 5. Relationship to Normative Documents

This note may inform implementations of SUAS-018 (SKTO), SUTS-012 (SCTS),
SUTR-015 (SIE), SUTR-017 (SIM), and SUTR-018 (SKC). Those documents govern
their own requirements and behavior.

## 6. Lifecycle and Review

Review annually and when a shared allocation or buffer policy changes. Withdraw
this note if its patterns become obsolete or are moved into a maintained
integration guide.

## 7. Owner and Maintainer

Owner: Systems Integration Maintainer. Maintainer: SuperUnicode portability
maintainers.

## 8. Revision History

| Version | Date | Change |
|---|---|---|
| 0.1.0 | 2026-11-02 | Initial publication. |

## 9. Terms and Conventions

Zero-allocation means the described primitive performs no dynamic allocation
on its execution path; it does not prohibit caller-provided storage.
