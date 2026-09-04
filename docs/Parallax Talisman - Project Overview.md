---
Status: Prototype planning  
Project: Parallax
Type: Embodied interface / peripheral intelligence / experimental wearable
Current Phase: V1 bench prototype
Created: 2026-09-03
---

# Parallax Talisman / Node
## 1. Project Overview

The **Parallax Talisman** is an experimental physical peripheral for Parallax: a small, low-power electronic object capable of sensing interaction, maintaining its own persistent internal state, expressing itself through light and haptics, and communicating with Parallax over Bluetooth Low Energy.

The project originated from the idea of creating functional electronic jewellery, but the underlying concept is deliberately broader than a necklace.

The first prototype does **not** need to be wearable.

Its purpose is to explore what happens when Parallax gains access to a small autonomous physical object that can:

- sense aspects of its environment;
    
- recognise deliberate human interaction;
    
- maintain state independently;
    
- communicate through minimal non-verbal signals;
    
- operate for extended periods on battery power;
    
- connect to Parallax when available;
    
- continue behaving meaningfully when Parallax is unavailable.
    

The eventual pendant is therefore only one possible **body** for the system.

---

## 2. Core Concept

The project combines two initially separate ideas.

### Parallax Node

A physical extension of the Parallax ecosystem.

When connected to Parallax, the device can act as a low-bandwidth peripheral through which Parallax can:

- signal presence;
    
- communicate attention or acknowledgement;
    
- receive touch or gesture input;
    
- exchange device state;
    
- trigger light or haptic patterns;
    
- potentially participate in wider Parallax presence behaviours.
    

### Electronic Talisman

When disconnected from Parallax, the device remains independently active rather than becoming a useless remote control.

It maintains a small internal behavioural model influenced by factors such as:

- movement;
    
- touch;
    
- time;
    
- battery state;
    
- interaction frequency;
    
- potentially environmental conditions.
    

This gives the object a primitive sense of continuity and temperament.

The long-term intention is not to simulate a miniature conversational AI. The Talisman should instead communicate through **behaviour**.

---

## 3. Design Principle

> **The device should have metabolism rather than merely battery life.**

Power, activity and behaviour should form part of the object's character.

The Talisman should spend most of its existence in extremely low-power states, waking briefly in response to meaningful events.

Conceptually:

```
sleep
  ↓
event detected
  ↓
wake
  ↓
sense / update state
  ↓
respond
  ↓
record significant event
  ↓
sleep
```

This approach serves both technical and experiential goals.

Technically, it allows long battery life.

Experientially, the object feels dormant rather than switched off.

---

## 4. Interaction Philosophy

The Talisman should resist becoming a miniature smartwatch.

The preferred expressive vocabulary is deliberately narrow:

**Light + touch + movement + haptics**

There should initially be:

- no screen;
    
- no conventional user interface;
    
- no menu system;
    
- no continuous notification stream;
    
- no expectation of constant connectivity.
    

Its behaviour should initially be somewhat **legible but not completely explicit**.

The user learns what the object does through interaction rather than reading status messages from it.

This ambiguity is intentional.

---

## 5. V1 Objective

V1 is a **bench prototype**.

It exists to prove the behavioural and technical architecture before attempting miniaturisation, custom PCB design, solar harvesting or wearable packaging.

The prototype can therefore use:

- breadboards;
    
- jumper wires;
    
- breakout boards;
    
- externally accessible components;
    
- USB power during development.
    

Aesthetics are irrelevant at this stage.

The objective is to discover whether the underlying interaction model is interesting enough to deserve a body.

---

## 6. V1 Hardware

### Primary Controller

**Seeed Studio XIAO nRF52840 Sense**

The board provides:

- Nordic nRF52840 microcontroller;
    
- Bluetooth Low Energy;
    
- USB-C;
    
- lithium battery charging support;
    
- onboard 6-axis IMU;
    
- onboard RGB/user LEDs;
    
- PDM microphone;
    
- very low-power operating modes;
    
- GPIO, ADC, I²C, SPI and UART interfaces.
    

Its approximately 21 × 17.8 mm footprint also leaves open the possibility of later wearable development.

### Planned Peripheral Hardware

|Component|Purpose|
|---|---|
|XIAO nRF52840 Sense|Core processor, BLE, motion sensing|
|3.7 V protected LiPo (~100–150 mAh)|Portable power|
|DRV2605L haptic controller|Programmable tactile output|
|Coin vibration motor|Physical/haptic expression|
|Capacitive touch sensor|Deliberate human interaction|
|LEDs / RGB LED|Visual expression|
|Breadboard and jumper wiring|V1 construction|

The onboard IMU means a separate accelerometer is unnecessary during V1.

---

## 7. Proposed Internal State

Rather than implementing fixed input/output rules alone, the Talisman should maintain a small persistent behavioural state.

Initial experimental dimensions might include:

```
energy
curiosity
familiarity
arousal
```

These names are provisional.

Sensors and events modify these dimensions over time.

For example:

```
movement → arousal increases

long inactivity → arousal decreases

interaction → familiarity increases

novel gesture → curiosity changes

low battery → energy decreases

charging → energy increases
```

The resulting state influences how the device responds.

This creates a feedback loop:

```
environment
     ↓
 sensors
     ↓
 internal state
     ↓
 behaviour
     ↓
 human interaction
     ↓
 new internal state
```

The important distinction is that the same input does not necessarily need to produce the same output every time.

---

## 8. Parallax Integration

Communication with Parallax should use **Bluetooth Low Energy (BLE)**.

The nRF52840 is specifically designed for low-energy wireless applications and supports BLE alongside other wireless protocols.

The first integration should remain deliberately small.

Parallax might initially be able to:

```
GET state
GET battery
GET recent_events

SEND signal
SEND haptic_pattern
SEND light_pattern

SET parallax_present
```

The exact protocol should be designed during implementation rather than assumed here.

The device should remain functional if Parallax disappears.

This creates two operating contexts:

```
             ┌────────────────────┐
             │      TALISMAN      │
             │                    │
             │ sensors            │
             │ persistent state   │
             │ behaviour          │
             └─────────┬──────────┘
                       │
                       │ BLE
                       │
              ┌────────▼─────────┐
              │     PARALLAX     │
              └──────────────────┘
```

### Connected

Parallax becomes another participant in the Talisman's environment.

### Disconnected

The Talisman continues operating autonomously.

Parallax should therefore **interact with the device rather than simply control it**.

---

## 9. Event Memory

The device may eventually maintain a small rolling event history.

This is not intended to become another Neurocodex or conversational memory store.

It should contain only compact device experiences, for example:

```
08:14 awakened
09:32 sustained movement
11:07 touch interaction
12:41 strong light exposure
15:26 prolonged inactivity
17:54 interaction
18:17 Parallax connected
```

When the device reconnects, Parallax could retrieve and interpret these events.

This creates the possibility of the Talisman functioning as a tiny **embodied satellite memory** that temporarily leaves the wider Parallax environment and later returns with a minimal record of its experience.

Privacy implications should be considered before adding richer sensing or event recording.

---

## 10. Power Philosophy

Low power consumption is a fundamental requirement rather than a later optimisation.

The eventual wearable should **not require nightly charging**.

The target architecture should therefore favour:

- deep sleep;
    
- interrupt-driven wake events;
    
- short LED activity;
    
- rare haptic activity;
    
- intermittent BLE communication;
    
- minimal continuous sensing.
    

The XIAO nRF52840 Sense is appropriate for experimentation because Seeed specifies standby consumption below 5 μA, although actual system consumption will depend heavily on firmware and attached peripherals.

Battery performance should be **measured during V1** rather than predicted from component specifications alone.

---

## 11. Solar Power

Solar energy harvesting is a possible later feature, but deliberately excluded from the initial prototype.

The intended model is:

> **Solar extends battery endurance rather than replacing the battery.**

Outdoor daylight could provide meaningful charging from a pendant-sized photovoltaic surface, while ordinary indoor illumination is unlikely to provide enough energy to act as the primary power source.

Solar therefore introduces an interesting behavioural possibility.

The device's available energy could affect its temperament.

High energy might produce more expressive behaviour.

Low energy could progressively reduce activity.

Very low energy could trigger dormancy.

Sunlight then becomes something analogous to **feeding the device**.

This transforms energy management from an invisible engineering concern into part of the object's interaction language.

Solar hardware should only be selected after V1 provides real measurements of average energy consumption.

---

## 12. Development Sequence

### Phase 1: Bring the Brain Online

Use only the XIAO.

Goals:

- establish development environment;
    
- flash firmware;
    
- read IMU data;
    
- control onboard LEDs;
    
- verify BLE communication;
    
- understand sleep/wake behaviour.
    

### Phase 2: Sensory Behaviour

Add touch and movement interpretation.

Explore:

- tap;
    
- double tap;
    
- hold;
    
- pickup;
    
- orientation;
    
- movement;
    
- prolonged inactivity.
    

### Phase 3: Expression

Add haptic output and develop a minimal signalling vocabulary.

Examples might include:

```
short pulse       acknowledgement
double pulse      attention
slow pulse        low-energy/dormant state
distinct pattern  Parallax connection
```

Patterns should emerge through experimentation rather than being treated as final semantics.

### Phase 4: Persistent State

Implement the first Talisman behavioural model.

Test whether internal variables produce behaviour that feels coherent without becoming predictable or irritating.

### Phase 5: Parallax Link

Implement the BLE interface between Parallax and the Talisman.

Parallax should initially:

- discover the device;
    
- establish connection;
    
- retrieve state;
    
- retrieve battery information;
    
- send simple expressive commands.
    

### Phase 6: Untethered Operation

Move from USB development power to LiPo.

Measure:

- active consumption;
    
- sleep consumption;
    
- BLE cost;
    
- LED cost;
    
- haptic cost;
    
- realistic battery endurance.
    

### Phase 7: Evaluate the Creature

Before progressing further, ask:

> Is interacting with this object actually interesting?

If not, change the behavioural model.

If yes, begin designing its physical embodiment.

---

## 13. Possible V2 Direction

If V1 proves successful, V2 can begin moving toward a self-contained object.

Potential developments include:

- custom PCB;
    
- integrated capacitive touch electrode;
    
- lower-power motion sensing;
    
- smaller protected battery;
    
- photovoltaic energy harvesting;
    
- custom enclosure;
    
- wearable pendant format;
    
- PCB traces used as aesthetic elements;
    
- dedicated Parallax presence indicator;
    
- improved event persistence;
    
- over-the-air firmware updates;
    
- richer BLE protocol.
    

The eventual PCB should ideally avoid hiding its electronics.

Circuit traces, antenna geometry, electrodes and components can form part of the object's visual language.

The electronics should not merely sit beneath the design.

**The electronics should be the design.**

---

## 14. Possible Future Bodies

The architecture should remain independent of pendant form.

The same behavioural system could potentially inhabit:

- necklace;
    
- desk object;
    
- keyring;
    
- pocket talisman;
    
- wrist object;
    
- bag attachment;
    
- room node;
    
- robot-mounted node;
    
- other Parallax physical interfaces.
    

The software entity and its physical embodiment should therefore remain conceptually separable.

---

## 15. Non-Goals

At this stage, the project is **not** intended to become:

- a smartwatch;
    
- a fitness tracker;
    
- a phone replacement;
    
- a general notification device;
    
- an always-listening microphone;
    
- a GPS tracker;
    
- a conversational AI running on the pendant;
    
- a miniature copy of Parallax.
    

Adding capabilities merely because the hardware permits them should be resisted.

Every feature should justify its effect on the relationship between:

**human ↔ object ↔ Parallax ↔ environment**

---

## 16. Open Questions

The prototype exists partly to answer questions that should not yet be prematurely designed away.

### Behaviour

- How complex should the internal state become?
    
- Should behaviour contain controlled randomness?
    
- How quickly should familiarity develop?
    
- Should the device develop long-term behavioural differences?
    
- How transparent should its state be to the user?
    

### Parallax Relationship

- Does Parallax command the device, influence it, or negotiate with it?
    
- Can the Talisman initiate interactions with Parallax?
    
- Should Parallax remember previous Talisman states?
    
- Is the Talisman conceptually part of Parallax or an entity Parallax encounters?
    

### Embodiment

- Pendant, pocket object or something else?
    
- How much visible electronics should remain?
    
- Should touch areas be visually obvious?
    
- How should light be diffused?
    
- Should haptics be perceptible only while worn/held?
    

### Energy

- What is real-world V1 average consumption?
    
- What battery capacity provides acceptable endurance?
    
- How much photovoltaic area would offset typical daily consumption?
    
- Should low energy deliberately alter behaviour?
    

### Memory

- Which events deserve persistence?
    
- How long should event history survive?
    
- What should happen to history after Parallax retrieves it?
    
- Which sensing should be explicitly excluded for privacy reasons?
    

---

## 17. Guiding Principle

The success criterion is not:

> **Can Parallax control a Bluetooth gadget?**

That is technically straightforward and conceptually uninteresting.

The more useful question is:

> **Can a tiny physical object develop enough continuity, responsiveness and autonomy that interacting with it feels meaningfully different from interacting with conventional electronics?**

If successful, the Talisman becomes an experiment in **embodied peripheral intelligence**: something existing at the boundary between interface, artefact, sensor, memory and electronic familiar.

The necklace is only one possible shell.

First, build the creature.