# SUTR-016 - System Filesystem (SFS)

**Standard ID:** SUTR-016
**Status:** 0.1.0 - Published
**Scope:** Informative bounded filesystem-name integration report
**Applies To:** OpenWindows VFS, OWFS/USFS adapters, filesystem consumers
**Last Updated:** 2026-11-02

## 1. Purpose

This report documents a portable name-boundary helper; filesystem ownership
remains with an operating system.

## 2. Method and Evidence Rules

Evidence covers bounded length and separator rejection. OS implementations
must perform their own user-pointer and authorization checks.

## 3. Findings

Names should cross the text boundary with an explicit length and no truncation.

## 4. Non-Goals

This report does not implement a filesystem, storage format, or authorization.

## 5. Change Control

Name-policy changes require a revised report.

## 6. Implementation Evidence

`include/sutr/sutr_sfs.h`, `src/sutr/sutr_sfs.c`, and `test_subsystems` provide
the portable helper.

## Terms and Conventions

SFS means System Filesystem.

## Revision History

| Version | Date | Change |
|---|---|---|
| 0.1.0 | 2026-11-02 | Initial publication. |
