# SUTR-018 - System Kernel Console (SKC)

**Standard ID:** SUTR-018
**Status:** 0.1.0 - Published
**Scope:** Informative bounded console-output integration report
**Applies To:** kernel diagnostics, shell output, console adapters
**Last Updated:** 2026-11-02

## 1. Purpose

This report documents a bounded caller-owned console buffer suitable for
kernel and OS adapters.

## 2. Method and Evidence Rules

Evidence covers initialization, append capacity, and explicit lengths.

## 3. Findings

Diagnostic output should fail explicitly rather than silently truncate.

## 4. Non-Goals

This report does not define terminal control, display hardware, or formatting.

## 5. Change Control

Buffer semantics require a revised report when changed.

## 6. Implementation Evidence

`include/sutr/sutr_skc.h`, `src/sutr/sutr_skc.c`, and `test_subsystems` provide
the portable reference buffer.

## Terms and Conventions

SKC means System Kernel Console.

## Revision History

| Version | Date | Change |
|---|---|---|
| 0.1.0 | 2026-11-02 | Initial publication. |
