# SUTR-017 - System IPC and Messaging (SIM)

**Standard ID:** SUTR-017
**Status:** 0.1.0 - Published
**Scope:** Informative bounded message-channel integration report
**Applies To:** kernel IPC, ALPC adapters, process message channels
**Last Updated:** 2026-11-02

## 1. Purpose

This report documents a caller-owned FIFO message channel for portable
reference testing.

## 2. Method and Evidence Rules

Evidence covers bounded payloads, FIFO order, and destination-capacity checks.
Kernel implementations must validate handles and user memory independently.

## 3. Findings

Explicit payload lengths prevent implicit string and buffer assumptions.

## 4. Non-Goals

This report does not define scheduling, handle tables, or a kernel syscall ABI.

## 5. Change Control

Message semantics require a revised report when changed.

## 6. Implementation Evidence

`include/sutr/sutr_sim.h`, `src/sutr/sutr_sim.c`, and `test_subsystems` provide
the portable reference channel.

## Terms and Conventions

SIM means System IPC and Messaging.

## Revision History

| Version | Date | Change |
|---|---|---|
| 0.1.0 | 2026-11-02 | Initial publication. |
