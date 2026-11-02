# SUTR-015 - System Input Events (SIE)

**Standard ID:** SUTR-015
**Status:** 0.1.0 - Published
**Scope:** Informative bounded event-queue integration report
**Applies To:** keyboard, UART, input-method, and kernel event producers
**Last Updated:** 2026-11-02

## 1. Purpose

This report documents a portable bounded event queue suitable for OS adapters.

## 2. Method and Evidence Rules

The reference queue is tested for FIFO order, capacity, and null rejection.
An operating system may implement the same contract independently.

## 3. Findings

Fixed caller-owned queues make event delivery predictable and allocation-free.

## 4. Non-Goals

This report does not define a keyboard scan-code format or own device drivers.

## 5. Change Control

Changes that alter queue semantics require a new report revision.

## 6. Implementation Evidence

`include/sutr/sutr_sie.h`, `src/sutr/sutr_sie.c`, and `test_subsystems` provide
the portable reference evidence.

## Terms and Conventions

SIE means System Input Events.

## Revision History

| Version | Date | Change |
|---|---|---|
| 0.1.0 | 2026-11-02 | Initial publication. |
