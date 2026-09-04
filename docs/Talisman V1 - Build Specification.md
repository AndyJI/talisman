# Parallax Talisman V1 - Codex Project Launch

I am beginning implementation of **Parallax Talisman V1**, an experimental low-power physical peripheral / autonomous electronic object that will eventually interact with my existing Parallax system.

Two project documents are attached:

1. `Parallax Talisman - Project Overview.md`
2. `Talisman V1 - Build Specification.md`

Read both documents completely before proposing or making changes.

The **Project Overview** defines the purpose, philosophy, behavioural intent, and longer-term direction of the Talisman.

The **Build Specification** is the governing technical document for V1 and defines the current engineering decisions, constraints, experimental sequence, and acceptance criteria.

Where they differ in technical specificity, treat the Build Specification as authoritative for V1 implementation.

---

## Current Situation

This is a new project.

The Talisman will exist as a standalone repository on **Soma**, the same development machine that hosts the existing `avatar-v2` Parallax repository, but it must remain architecturally separate from `avatar-v2` during V1 development.

The intended initial repository structure is:

```text
talisman/
├── firmware/
├── bridge/
└── docs/
```

The primary hardware is:

**Seeed Studio XIAO nRF52840 Sense**

The V1 firmware framework decision is:

**PlatformIO + Arduino**

Additional hardware has been ordered but should **not** be introduced yet.

Early development will use USB power.

---

# Current Objective: Experiment 01 - Board Bring-Up

Do not begin implementing the wider Talisman architecture yet.

The first objective is deliberately narrow:

```text
XIAO
  ↓
USB
  ↓
Soma
  ↓
PlatformIO
  ↓
firmware
  ↓
serial output
  ↓
heartbeat
```

Experiment 01 should establish that:

- Soma can detect and communicate with the XIAO;
- the PlatformIO project can be created correctly;
- firmware builds reproducibly;
- firmware can be flashed reliably;
- serial/debug output works;
- the onboard LED can be controlled;
- a deterministic heartbeat behaviour can run.

The desired initial serial output is conceptually:

```text
[TALISMAN]
boot: ok
firmware: 0.1.0-dev
state: awake
```

The exact implementation may differ if hardware/framework realities require it.

---

# Working Method

For each development increment:

1. Inspect the current repository and relevant files.
2. Identify what is already known versus what needs verification.
3. Propose the smallest useful implementation step.
4. Explain any assumptions that materially affect the build.
5. Implement only that step.
6. Give me the commands or physical actions I need to perform where hardware interaction is required.
7. Wait for the resulting output, logs, or observations where they are needed to establish whether the experiment succeeded.
8. Diagnose failures from evidence rather than guessing.
9. Update relevant project documentation after meaningful discoveries.
10. Only move to the next experiment when the current experiment's acceptance criteria have been met.

Prefer small, testable changes over large speculative implementations.

---

# Important Constraints

## Do not modify `avatar-v2`

Talisman is being developed independently while significant work continues elsewhere in Parallax.

Do not modify, refactor, or introduce dependencies into `avatar-v2`.

Parallax integration is explicitly deferred until the Talisman firmware, BLE protocol, and standalone bridge provide a stable integration boundary.

---

## Do not prematurely implement future experiments

Do not introduce:

- BLE architecture;
- behavioural state;
- persistence;
- touch;
- haptics;
- battery logic;
- power optimisation;
- solar support;
- Parallax integration;

during Experiment 01 unless something is genuinely required for basic board bring-up.

The Build Specification describes where we are going.

It is **not an instruction to implement everything immediately**.

---

## Preserve observability

V1 prioritises:

> legibility → reliability → efficiency

Prefer implementations that are easy to inspect and debug.

Do not prematurely optimise code, communication, memory, or power consumption.

---

## Treat uncertainty explicitly

If something is unknown, mark it as unknown.

Do not silently turn assumptions into architectural decisions.

Where practical, convert uncertainty into a small experiment.

The Build Specification distinguishes:

- DECIDED
- PROVISIONAL
- EXPERIMENTAL
- DEFERRED

Respect those distinctions.

---

## Hardware safety

Never assume that hardware has been connected correctly merely because software expects it.

When later experiments involve external components or battery power:

- provide clear wiring instructions;
- identify relevant pins;
- identify voltage expectations;
- call out polarity-sensitive connections;
- ask me to verify ambiguous physical details;
- avoid actions that could reasonably damage the XIAO, peripherals, battery, or Soma.

For Experiment 01, nothing except the XIAO and USB connection should be required.

---

# Documentation

The two attached documents should be placed in:

```text
docs/
```

They are living project documents.

Do not rewrite their conceptual intent merely to match an implementation convenience.

When experimental evidence contradicts a provisional assumption, record the evidence and recommend the appropriate documentation change.

Maintain the `Current Build State` section of the Build Specification as development progresses.

---

# First Task

Begin by:

1. reading both attached documents;
2. inspecting the current `talisman` repository/environment if it already exists;
3. confirming the smallest setup required for Experiment 01;
4. identifying anything that should be installed or configured on Soma;
5. proposing the exact first implementation step.

Do **not** proceed beyond Experiment 01.

If the XIAO is not yet physically available, prepare only the repository/tooling work that can safely be completed without it and clearly identify the point at which hardware is required.

The first meaningful milestone is not BLE, autonomy, Parallax integration, or intelligent behaviour.

It is simply:

> **make the board wake up, identify itself, and heartbeat reliably.**
