# Talisman V1 - Build Specification

**Status:** Ready for Implementation  
**Project:** Parallax Talisman / Node  
**Build Phase:** V1 Bench Prototype  
**Companion Document:** `Parallax Talisman - Project Overview.md`  
**Created:** 2026-09-03  
**Primary Development Machine:** Soma

---

# 1. Purpose

This document defines the technical scope, architecture, constraints, development sequence, experiments, and acceptance criteria for the first working **Parallax Talisman** prototype.

The companion Project Overview defines what the Talisman is, why it exists, and the broader conceptual direction.

This document defines **how V1 will be built and evaluated**.

V1 is explicitly an experimental system. Decisions should therefore be driven by evidence generated during development rather than premature optimisation.

---

# 2. Decision States

Technical decisions in this specification use four states.

## DECIDED

Part of the current engineering baseline.

A DECIDED choice may still change if evidence demonstrates a significant problem, but should not be casually reopened.

## PROVISIONAL

A reasonable initial implementation that requires validation.

## EXPERIMENTAL

Something V1 specifically exists to investigate.

## DEFERRED

Deliberately excluded from V1 or postponed until sufficient evidence exists.

---

# 3. V1 Mission

## DECIDED

V1 is a **bench prototype** intended to prove that a small autonomous physical device can:

- sense movement;
- recognise deliberate human interaction;
- maintain internal behavioural state;
- preserve selected state across power cycles;
- express itself through light;
- express itself through haptics;
- communicate through Bluetooth Low Energy;
- continue meaningful autonomous behaviour without Parallax;
- communicate with Parallax through a clean abstraction;
- eventually operate untethered from a small rechargeable battery;
- spend most of its inactive life in low-power states;
- provide enough instrumentation to measure its real power requirements.

V1 does **not** need to be wearable.

The priority is:

> **behaviour → reliability → measurement → optimisation → embodiment**

Miniaturisation and aesthetic design come later.

---

# 4. V1 Guiding Question

> **Can a tiny autonomous physical object develop enough continuity, responsiveness, and independence to feel meaningfully different from an ordinary peripheral?**

Every major V1 feature should contribute to answering this question.

---

# 5. V1 Non-Goals

## DECIDED

V1 will not initially include:

- solar charging;
- custom PCB design;
- wearable enclosure;
- final industrial design;
- GPS;
- cellular connectivity;
- Wi-Fi;
- screen or conventional display;
- conventional menu interface;
- continuous sensor logging;
- always-on audio;
- cloud infrastructure;
- conversational AI running on-device;
- large-scale local storage;
- production-grade security architecture;
- over-the-air firmware updates;
- direct modification of the existing Parallax `avatar-v2` codebase.

These capabilities should not be introduced merely because they are technically possible.

---

# 6. Development Philosophy

## DECIDED

Development will proceed through **small, independently testable increments**.

Each meaningful increment should follow:

```text
inspect
   ↓
propose
   ↓
implement
   ↓
test
   ↓
measure
   ↓
document
   ↓
commit
```

Only one significant unknown subsystem should normally be introduced at a time.

Hardware should likewise be added incrementally.

Early development prioritises **observability and debuggability over efficiency**.

The project should resist premature optimisation.

---

# 7. Repository Architecture

## DECIDED

Talisman will exist as a **standalone repository on Soma**, alongside rather than inside the existing `avatar-v2` repository.

Conceptually:

```text
<development-root>/
├── avatar-v2/
└── talisman/
```

The exact parent path should follow the existing development layout on Soma.

The Talisman repository should initially contain:

```text
talisman/
├── firmware/
├── bridge/
└── docs/
```

### `firmware/`

Code executed on the physical Talisman.

### `bridge/`

Standalone host-side software for discovering, connecting to, testing, and communicating with the Talisman.

This becomes the future integration seam with Parallax.

### `docs/`

Canonical project documentation, including:

- Project Overview;
- Build Specification;
- protocol documentation;
- hardware notes;
- experiment results;
- architectural decisions where required.

Additional directories should only be introduced when justified by actual project needs.

---

# 8. Hardware Baseline

## DECIDED

### Primary Controller

**Seeed Studio XIAO nRF52840 Sense**

Relevant capabilities include:

- Nordic nRF52840 microcontroller;
- Bluetooth Low Energy;
- USB-C;
- onboard IMU;
- lithium battery charging support;
- GPIO;
- I²C;
- low-power operating modes;
- onboard LEDs.

### Additional V1 Hardware

- protected 3.7 V LiPo battery, approximately 100–150 mAh;
- DRV2605L haptic controller;
- coin vibration motor;
- capacitive touch sensor;
- optional external LEDs;
- breadboard;
- jumper leads;
- thin hookup wire;
- digital multimeter;
- USB power measurement hardware where useful.

## PROVISIONAL

The XIAO's onboard LEDs should be used during initial development before external lighting hardware is introduced.

The standalone capacitive touch module is a prototyping component and may later be replaced by direct capacitive sensing or a custom PCB electrode.

---

# 9. Firmware Framework

## DECIDED

Talisman V1 will use:

**PlatformIO + Arduino framework**

This choice prioritises:

- rapid hardware bring-up;
- strong library availability;
- straightforward BLE development;
- straightforward sensor integration;
- reproducible project configuration;
- relatively low tooling friction;
- compatibility with Codex-assisted development;
- rapid experimentation.

The objective of V1 is to investigate the Talisman concept rather than maximise theoretical firmware efficiency.

## Reassessment Trigger

Zephyr / Nordic nRF Connect SDK should only be reconsidered if V1 demonstrates a concrete limitation involving:

- deep-sleep power consumption;
- BLE control;
- timing requirements;
- memory constraints;
- peripheral support;
- firmware architecture.

Migration should be **evidence-driven**.

---

# 10. High-Level Device Architecture

## PROVISIONAL

```text
                     ┌─────────────────────┐
                     │      TALISMAN       │
                     │                     │
      IMU ──────────►│                     │
                     │                     │
    Touch ──────────►│  Behaviour Engine   │────► Light
                     │                     │
  Battery ──────────►│                     │────► Haptics
                     │                     │
                     └──────────┬──────────┘
                                │
                                │ BLE
                                │
                     ┌──────────▼──────────┐
                     │  TALISMAN BRIDGE    │
                     └──────────┬──────────┘
                                │
                         future interface
                                │
                     ┌──────────▼──────────┐
                     │      PARALLAX       │
                     └─────────────────────┘
```

The device must remain behaviourally functional when BLE and Parallax are unavailable.

Parallax should ultimately become **a participant in the Talisman's environment**, not the sole source of its behaviour.

---

# 11. Firmware Architecture

## PROVISIONAL

Firmware should separate hardware access from behavioural logic.

A likely conceptual structure is:

```text
firmware/
├── src/
│   ├── sensors/
│   │   ├── imu
│   │   ├── touch
│   │   └── battery
│   │
│   ├── behaviour/
│   │   ├── state
│   │   ├── events
│   │   └── decay
│   │
│   ├── expression/
│   │   ├── light
│   │   └── haptics
│   │
│   ├── communication/
│   │   └── ble
│   │
│   ├── persistence/
│   ├── power/
│   └── main
│
└── platformio.ini
```

The exact directory structure should not be created speculatively if PlatformIO or implementation requirements suggest a cleaner arrangement.

The important architectural requirement is **separation of concerns**, not adherence to this exact tree.

---

# 12. Semantic Event Model

## DECIDED

Raw hardware readings should not propagate throughout the behavioural system.

Sensor layers should translate physical input into **semantic events**.

Candidate events include:

```text
DEVICE_WAKE
DEVICE_SLEEP

PICKED_UP
PUT_DOWN

MOVEMENT_STARTED
MOVEMENT_STOPPED

TAP
DOUBLE_TAP
LONG_TOUCH

PARALLAX_CONNECTED
PARALLAX_DISCONNECTED

BATTERY_LOW
BATTERY_CHARGING
BATTERY_FULL

LONG_INACTIVITY
```

## EXPERIMENTAL

V1 should determine:

- which events are genuinely useful;
- which can be detected reliably;
- which generate unacceptable false positives;
- which can be detected while maintaining low power consumption.

The event vocabulary should evolve from observed behaviour rather than expanding indefinitely.

---

# 13. Behavioural State

## EXPERIMENTAL

The Talisman should eventually maintain a small internal behavioural model.

Initial candidate dimensions are:

```text
energy
arousal
curiosity
familiarity
```

These names and dimensions are deliberately provisional.

Possible influences include:

```text
movement
touch
time
battery state
Parallax presence
interaction frequency
recent event history
```

Example:

```text
movement
   ↓
MOVEMENT_STARTED
   ↓
increase arousal
   ↓
evaluate current behavioural state
   ↓
select expression
   ↓
brief light / haptic response
```

The same event should not necessarily produce an identical response every time.

Any randomness should be:

- bounded;
- inspectable;
- subordinate to behavioural state;
- reproducible during debugging where necessary.

## EXPERIMENTAL

V1 should determine whether the behavioural system is best represented using:

- continuous numerical dimensions;
- finite states;
- event-driven rules;
- or a hybrid approach.

---

# 14. Runtime State

## DECIDED

Active behavioural state should primarily exist in RAM.

Examples include:

```text
energy
arousal
curiosity
familiarity
current movement state
current touch state
Parallax connection state
temporary timers
recent transient events
```

Frequent runtime changes should **not** automatically trigger non-volatile writes.

---

# 15. Persistent State

## DECIDED

V1 will use the nRF52840's internal non-volatile flash.

No SD card, external EEPROM, database, or other storage hardware will be added during V1.

Persistent information may include:

```text
PersistentState
├── schema_version
├── lifetime
│   ├── birth_time
│   ├── wake_count
│   └── interaction_count
├── behaviour
│   ├── energy
│   ├── arousal
│   ├── curiosity
│   └── familiarity
├── configuration
└── event_log
```

The exact schema remains subject to experimentation.

### Schema Versioning

Persistent data must contain a schema version from the beginning of persistent-state development.

Firmware updates should not silently reinterpret incompatible persisted data.

---

# 16. Persistence Strategy

## DECIDED

Flash writes should use **controlled checkpoints**, not continuous persistence.

Potential checkpoint triggers include:

- meaningful behavioural change;
- significant interaction;
- periodic safety checkpoint;
- Parallax disconnection;
- deliberate shutdown;
- transition into a long-duration sleep state.

Sensor noise and minor state fluctuations should never directly cause flash writes.

This protects flash endurance and maintains a clean separation between runtime state and durable memory.

---

# 17. Event Memory

## PROVISIONAL

The Talisman may maintain a small rolling history of **semantic events**.

It should not store continuous raw sensor data.

A conceptual record might contain:

```text
time
event_type
optional_value
```

Example:

```text
08:14 PICKED_UP
09:32 MOVEMENT_STARTED
11:07 LONG_TOUCH
17:54 PARALLAX_CONNECTED
```

A circular buffer is the preferred initial conceptual model.

## EXPERIMENTAL

V1 should determine:

- appropriate buffer size;
- retention period;
- which events deserve persistence;
- write frequency;
- flash impact;
- whether retrieved events remain or are acknowledged/cleared;
- whether absolute timestamps are necessary.

---

# 18. BLE Architecture

## DECIDED

Talisman V1 will expose **one custom Bluetooth Low Energy service** containing a small number of semantically meaningful characteristics.

Initial conceptual characteristics:

```text
TALISMAN SERVICE
├── state
├── events
├── command
├── config
└── info
```

### `state`

Current behavioural/device state.

Likely READ and/or NOTIFY.

### `events`

Semantic events generated by the Talisman.

Likely READ and/or NOTIFY.

### `command`

Semantic instructions sent to the Talisman.

WRITE.

### `config`

Low-frequency configuration.

READ / WRITE where appropriate.

### `info`

Static or slowly changing device information such as firmware version.

READ.

Exact UUIDs and characteristic properties should be defined during BLE implementation.

---

# 19. BLE Message Encoding

## DECIDED

V1 application messages will use **compact human-readable JSON** wherever practical.

Example:

```json
{
  "type": "state",
  "energy": 0.74,
  "arousal": 0.18,
  "familiarity": 0.42,
  "battery": 82
}
```

Early V1 development will primarily use USB power.

At this stage:

> **inspectability and debugging value are more important than protocol efficiency.**

Human-readable messages make it substantially easier to understand:

- what the Talisman believes;
- what events it is generating;
- what Parallax or the bridge requested;
- where communication failures occur.

---

# 20. BLE Optimisation

## DEFERRED

Binary encoding should not be introduced during early development merely because it is more efficient.

Possible future formats include CBOR or a custom binary representation.

A change away from JSON requires measured evidence of a meaningful limitation involving:

- power consumption;
- BLE packet size;
- latency;
- RAM;
- flash;
- throughput.

The optimisation sequence should be:

> **make it legible → make it reliable → make it frugal**

---

# 21. BLE Semantic Boundary

## DECIDED

Messages should communicate **intent**, not unnecessary hardware implementation details.

Preferred:

```json
{
  "action": "signal",
  "pattern": "attention"
}
```

Avoid exposing low-level implementation unless specifically required:

```json
{
  "led": 1,
  "brightness": 80,
  "motor": 1,
  "duration": 240
}
```

The Talisman firmware should decide how semantic expressions map onto its current physical hardware.

This allows future hardware revisions without unnecessarily changing the Parallax-facing protocol.

---

# 22. Standalone Bridge

## DECIDED

A standalone host-side bridge will be developed inside:

```text
talisman/bridge/
```

The bridge will initially operate independently of `avatar-v2`.

Its responsibilities should eventually include:

- discovering Talisman devices;
- establishing BLE connections;
- handling disconnections;
- reconnecting where appropriate;
- reading device information;
- retrieving behavioural state;
- receiving semantic events;
- sending semantic commands;
- exposing errors in an understandable form;
- hiding raw BLE implementation details from future consumers.

The bridge should itself be usable as a development and diagnostic tool.

---

# 23. Parallax Integration Boundary

## DECIDED

Experimental Talisman BLE logic must **not** initially be added to `avatar-v2`.

Parallax integration begins only after the firmware, protocol, and standalone bridge have reached a sufficiently stable boundary.

The future Parallax-facing interface should expose semantic concepts such as:

```text
discover_talisman()
connect_talisman()

get_talisman_state()
get_talisman_events()

send_talisman_signal()
```

rather than raw BLE characteristics.

## Integration Trigger

Integration work with `avatar-v2` should begin only when:

- BLE discovery works reliably;
- connections are reasonably stable;
- reconnection behaviour is understood;
- state exchange works consistently;
- event exchange works consistently;
- semantic commands work consistently;
- the bridge hides BLE implementation details;
- the Parallax-facing interface can be clearly described;
- integration can occur as a contained addition rather than disrupting active Parallax architecture work.

Until those conditions are met:

> **`avatar-v2` remains untouched by Talisman development.**

---

# 24. Light Language

## EXPERIMENTAL

V1 should explore how little visual vocabulary is necessary to create understandable expression.

Potential dimensions include:

- pulse count;
- pulse duration;
- brightness;
- colour;
- transition speed;
- pauses;
- repetition.

Continuous illumination should generally be avoided.

This reduces power consumption and prevents the Talisman becoming another permanently glowing notification device.

The onboard LEDs should be sufficient for initial behavioural experiments.

---

# 25. Haptic Language

## EXPERIMENTAL

The DRV2605L and coin motor will be used to explore a small tactile vocabulary.

Possible initial patterns include:

```text
short pulse        acknowledgement
double pulse       attention
slow pulse         low-energy behaviour
distinct pattern   Parallax connection
```

These meanings are **not predetermined semantics**.

They should be evaluated through actual interaction.

Haptic activity should remain relatively infrequent because of both power consumption and experiential intrusiveness.

---

# 26. Power Architecture

## DECIDED

Low-power operation is a core architectural requirement, but **not an early optimisation requirement**.

Initial experiments may run entirely over USB.

Once core behaviour is reliable, firmware should increasingly favour:

- deep sleep;
- interrupt-driven wake;
- minimal polling;
- short sensor activity;
- intermittent BLE;
- short LED activity;
- infrequent haptics.

Power optimisation should follow functional validation.

---

# 27. Power Measurement

## EXPERIMENTAL

V1 should eventually measure real consumption for:

- active idle;
- sleep;
- deepest practical sleep state;
- IMU monitoring;
- BLE advertising;
- active BLE connection;
- LED activity;
- touch sensing;
- haptic operation.

Measurements should be recorded in project documentation.

Battery-life projections should use measured behaviour rather than datasheet values alone.

---

# 28. Battery Operation

## PROVISIONAL TARGET

The eventual Talisman should:

- operate for multiple days at minimum;
- preferably operate for one or more weeks between manual charges;
- not assume nightly charging.

V1 should determine whether this is realistic.

The protected LiPo should only be introduced after USB-powered behaviour is stable enough that battery operation provides useful evidence.

---

# 29. Solar Harvesting

## DEFERRED

Solar hardware will **not** be selected or implemented during early V1.

Solar requirements depend on measured average power consumption.

The intended future model is:

> **solar extends endurance rather than replacing the battery**

If implemented later, available energy may deliberately influence Talisman behaviour, allowing energy management to become part of its expressive/metabolic model.

No solar panel dimensions, charging IC, or energy budget should be specified until empirical V1 power measurements exist.

---

# 30. Initial Experiment Sequence

Experiments should be completed sequentially unless there is a clear engineering reason to change the order.

---

## Experiment 01: Board Bring-Up

### Hardware

```text
XIAO nRF52840 Sense
USB-C cable
Soma
```

Nothing else connected.

### Goals

- confirm Soma detects the board;
- establish PlatformIO project;
- compile firmware;
- flash firmware;
- confirm serial/debug output;
- control onboard LED;
- establish deterministic heartbeat behaviour.

### Suggested Initial Serial Output

```text
[TALISMAN]
boot: ok
firmware: 0.1.0-dev
state: awake
```

### Current Development Workflow

From the repository root:

```bash
pio run --project-dir firmware
```

The PlatformIO project pins the Seeed platform revision and board definition. With the pinned Adafruit nRF52 framework, firmware using USB CDC serial must explicitly include `Adafruit_TinyUSB.h`; relying on the transitive declaration from `Arduino.h` compiles but fails at link time because PlatformIO does not discover the TinyUSB library.

With only the XIAO connected by USB, identify its current serial port and then upload while attaching the monitor:

```bash
pio device list
pio run --project-dir firmware --target upload --target monitor --upload-port <serial-port>
```

### Acceptance Criteria

- firmware builds reproducibly;
- board accepts firmware reliably;
- serial/debug output is available;
- onboard LED responds deterministically;
- development workflow is documented sufficiently to repeat it.

### Result

**Completed:** 2026-09-04

Evidence:

- Soma identified the connected board as Seeed XIAO nRF52840 Sense at USB VID:PID `2886:8045`;
- clean firmware builds completed repeatedly with identical `.hex` and `.elf` artefacts;
- two consecutive `nrfutil` DFU uploads completed successfully;
- the complete boot identity and consecutive heartbeat counters were observed over USB CDC serial at 115200 baud;
- the onboard red LED produced a physically confirmed brief pulse approximately once per second;
- the repeatable build, upload, and monitor workflow is recorded above.

### Stop Condition

Do not add additional hardware until Experiment 01 is reliable.

---

## Experiment 02: IMU Bring-Up

### Goals

- access onboard IMU;
- read accelerometer data;
- inspect raw movement data;
- distinguish stationary from moving;
- begin translating raw readings into semantic events.

### Candidate Events

```text
MOVEMENT_STARTED
MOVEMENT_STOPPED
PICKED_UP
PUT_DOWN
```

### Current Development Workflow

Experiment 02 uses the XIAO Sense's onboard LSM6DS3 over its internal I2C connection. No external hardware is connected. The PlatformIO project pins `seeed-studio/Seeed Arduino LSM6DS3` version `2.0.7`.

Firmware samples the accelerometer and gyroscope every 250 ms and prints every raw sample over USB serial. The initial semantic detector uses the peak absolute gyroscope axis, with deliberately visible provisional thresholds:

```text
MOVEMENT_STARTED: at least 12 dps for 2 consecutive samples
MOVEMENT_STOPPED: at most 4 dps for 8 consecutive samples
```

The separate start and stop thresholds provide hysteresis. The consecutive-sample requirements reject isolated spikes and require two seconds of stillness before declaring movement stopped. These values are experimental observations for board bring-up, not settled behavioural architecture.

### Acceptance Criteria

- movement is detected consistently;
- stationary periods do not generate excessive false events;
- raw readings remain observable during debugging.

### Result

**Completed:** 2026-09-04

Evidence:

- the onboard LSM6DS3 initialised successfully at I2C address `0x6A` and reported `imu: ok` at boot;
- stationary accelerometer readings showed approximately 1 g total acceleration and the stationary gyroscope remained well below the movement-start threshold during the observed test window;
- deliberate board movement produced clearly differentiated raw gyroscope readings, including peaks above 200 dps;
- controlled movement generated `MOVEMENT_STARTED`, and returning the board to rest generated `MOVEMENT_STOPPED`;
- an isolated gyroscope spike above the start threshold did not create a false start because it did not persist for two samples;
- raw acceleration and gyroscope readings remained available alongside semantic events and the existing heartbeat output.

`PICKED_UP` and `PUT_DOWN` remain candidate events and were not implemented in this experiment. Their semantics require more evidence than the initial movement boundary.

During several tests, USB serial briefly disconnected while the board was being handled and then recovered. At least one reconnect resumed with a continuing heartbeat counter, indicating that the firmware had not reset. The USB cable did not visibly come out or significantly move, although slight movement within the socket was possible, and no interruption to the LED heartbeat was observed. The physical cause is not yet established and must be observed in future handling tests.

---

## Experiment 03: Sleep and Wake

### Goals

- establish practical low-power sleep;
- wake reliably;
- preserve appropriate runtime behaviour;
- obtain first approximate power measurements.

### Acceptance Criteria

- device enters sleep reliably;
- device wakes reliably;
- repeated sleep/wake cycles remain stable;
- no unexpected state corruption occurs.

---

## Experiment 04: Touch

### Hardware Added

Capacitive touch module.

### Goals

- detect intentional touch;
- generate semantic touch events;
- investigate short versus sustained interaction.

### Candidate Events

```text
TAP
DOUBLE_TAP
LONG_TOUCH
```

### Acceptance Criteria

- deliberate interactions are detected consistently;
- false triggers remain manageable;
- behaviour remains observable through debugging output.

---

## Experiment 05: Haptics

### Hardware Added

- DRV2605L;
- coin vibration motor.

### Goals

- establish I²C communication;
- activate motor safely;
- explore repeatable tactile patterns.

### Acceptance Criteria

- haptic effects are repeatable;
- motor activity does not destabilise the controller;
- useful patterns can be distinguished by touch.

---

## Experiment 06: Semantic Event Layer

### Goals

Separate hardware readings from behavioural interpretation.

Create an event pipeline conceptually resembling:

```text
sensor
   ↓
hardware interpretation
   ↓
semantic event
   ↓
behaviour engine
```

### Acceptance Criteria

- behavioural code does not depend directly on raw sensor implementation;
- events can be logged and inspected;
- event vocabulary remains small and meaningful.

---

## Experiment 07: Behaviour Engine

### Goals

- introduce first internal behavioural dimensions;
- update them in response to events;
- introduce time-based change or decay;
- generate light/haptic responses based on state.

### Acceptance Criteria

- internal state changes over time;
- events modify state predictably enough to debug;
- expression depends on behavioural state;
- behaviour is more sophisticated than direct input/output mapping.

---

## Experiment 08: Persistence

### Goals

- define versioned persistent schema;
- checkpoint selected state;
- restore state after restart;
- begin rolling semantic event history.

### Acceptance Criteria

- selected state survives power cycle/reset;
- schema version is recorded;
- writes are controlled rather than continuous;
- event history remains bounded.

---

## Experiment 09: BLE Bring-Up

### Goals

- advertise custom Talisman service;
- discover device from Soma;
- establish connection;
- read device information;
- exchange first human-readable JSON.

### Acceptance Criteria

- device is reliably discoverable;
- connection can be established repeatedly;
- at least one state message can be retrieved;
- at least one semantic command can be sent.

---

## Experiment 10: BLE Semantic Interface

### Goals

Implement initial conceptual characteristics:

```text
state
events
command
config
info
```

### Acceptance Criteria

- state can be read;
- events can be observed;
- commands can trigger semantic expressions;
- BLE implementation details remain isolated from behavioural logic.

---

## Experiment 11: Standalone Bridge

### Goals

Create the first host-side bridge inside:

```text
bridge/
```

The bridge should:

- discover the Talisman;
- connect;
- inspect device information;
- retrieve state;
- display events;
- send a semantic signal.

### Acceptance Criteria

The complete path works:

```text
Talisman
   ⇅ BLE
Bridge
   ⇅
human-readable development interface
```

No `avatar-v2` modification is required.

---

## Experiment 12: Battery Operation

### Hardware Added

Protected LiPo.

### Goals

- verify safe battery operation;
- verify charging behaviour;
- transition between USB and battery operation;
- begin untethered testing.

### Acceptance Criteria

- device operates reliably from battery;
- charging works as expected;
- no unexpected resets occur during normal operation.

---

## Experiment 13: Power Characterisation

### Goals

Measure representative operating modes.

Use real measurements to estimate realistic endurance.

### Acceptance Criteria

Produce an initial power budget containing measured or appropriately qualified values for major operating states.

This experiment determines whether optimisation is necessary before wearable development.

---

# 31. V1 Completion Criteria

V1 can be considered successful when:

- firmware builds reproducibly;
- the device operates reliably;
- movement generates meaningful semantic events;
- touch generates meaningful semantic events;
- behavioural state changes over time;
- selected state survives restart;
- light communicates state;
- haptics communicate state;
- BLE communication works reliably;
- messages remain inspectable;
- the standalone bridge communicates successfully;
- the device remains behaviourally functional without the bridge;
- battery operation works;
- representative power consumption has been measured;
- there is enough evidence to decide whether wearable V2 development is worthwhile.

---

# 32. Deferred V2 Decisions

## DEFERRED

Do not prematurely design:

- custom PCB;
- final MCU choice;
- final sensor choice;
- solar panel;
- solar charging controller;
- final battery chemistry;
- final battery capacity;
- pendant dimensions;
- enclosure;
- PCB-integrated touch electrode;
- final LED arrangement;
- antenna placement;
- waterproofing;
- wearable mechanical protection;
- production power optimisation;
- over-the-air updates;
- richer long-term behavioural development;
- multi-node networking;
- final Parallax integration architecture.

V1 evidence should inform these decisions.

---

# 33. Safety Constraints

The eventual device may be worn against the body.

Battery handling should therefore be treated conservatively from the beginning.

V1 should:

- use a protected LiPo;
- verify battery polarity before connection;
- avoid soldering directly to LiPo cells;
- avoid puncturing, bending, crushing, or mechanically stressing pouch cells;
- disconnect damaged or unexpectedly heating cells;
- keep exposed conductive objects away from battery terminals;
- verify charging behaviour before unattended operation.

Wearable battery packaging requires separate evaluation during later development.

---

# 34. Documentation Discipline

Documentation should capture **evidence**, not merely intended architecture.

Unexpected behaviour should be recorded rather than immediately designed away.

Examples include:

- unreliable gesture recognition;
- false touch events;
- surprisingly high power consumption;
- BLE instability;
- interesting accidental light/haptic patterns;
- behavioural responses that feel unusually effective;
- behavioural responses that feel irritating or mechanical.

Unexpected behaviour is experimental data.

---

# 35. Current Build State

This section should be updated at the end of each meaningful development session.

```text
BUILD STATE

Hardware available:
- 1 Seeed Studio XIAO nRF52840 Sense
- HEEPD haptic motor controller using a DRV2605L haptic driver (marked Vin/Logic 2-5V, GC-2, 94V-0; pin headers not yet attached)
- WS2812B-8 LED modules
- 5 flat coin vibration motors (10 x 2.7 mm, DC 3-5V, specified 63 mA, described by supplier as micro brushless)
- 30 TTP223 capacitive touch button modules (15 x 11 mm, 2.5-5.5V, configurable self-lock/momentary and high/low-level output modes)
- breadboards
- jumper leads
- digital multimeter

Hardware currently connected:
- Seeed Studio XIAO nRF52840 Sense by USB only

Firmware version: 0.1.0-dev
Firmware status:
- Experiments 01 and 02 complete
- PlatformIO build succeeds from a clean build directory
- repeated clean builds produce identical `.hex` and `.elf` artefacts; the generated `.zip` hash changes with archive metadata
- Soma detects the board as Seeed XIAO nRF52840 Sense at USB VID:PID 2886:8045
- two consecutive nrfutil DFU uploads completed successfully through /dev/cu.usbmodem21101
- USB serial output confirmed at 115200 baud, including the complete boot identity and consecutive heartbeat counters
- onboard red LED heartbeat physically confirmed as a brief pulse approximately once per second
- onboard LSM6DS3 initialises over internal I2C at address 0x6A
- accelerometer and gyroscope samples are emitted every 250 ms over USB serial
- provisional hysteretic movement detection emits MOVEMENT_STARTED and MOVEMENT_STOPPED

Bridge status: Initial uv scaffold only; intentionally untouched during Experiments 01 and 02

Last completed experiment: Experiment 02 - IMU Bring-Up
Current experiment: None; Experiment 02 complete and Experiment 03 not started

Known issues:
- The pinned Seeed platform requires an explicit Adafruit_TinyUSB include for USB CDC symbols to link
- USB serial briefly disconnected and recovered during several board-handling tests; the cable remained connected, slight movement within the socket was possible, and neither the observed LED heartbeat nor the continuing serial heartbeat count indicated a firmware reset

Power measurements: None

Confirmed decisions:
- PlatformIO Core 6.1.19
- Arduino framework using the Adafruit nRF52 core
- Seeed PlatformIO platform pinned to commit a33b13986ff07f32c9940006518fa130bb3ffd7f
- Board definition seeed-xiao-afruitnrf52-nrf52840-sense
- Serial monitor speed 115200 baud
- XIAO USB detection, nrfutil upload, and USB CDC serial communication work on Soma
- the onboard red LED is active-low and the configured 100 ms pulse at a 1,000 ms period is visibly correct
- Seeed Arduino LSM6DS3 version 2.0.7 provides working access to the onboard IMU at I2C address 0x6A
- initial IMU sampling period is 250 ms, with raw samples retained for observability

Open decisions:
- tune or replace the provisional 12 dps start and 4 dps stop movement thresholds using evidence from later physical use
- determine whether handling-related USB interruptions come from cable or connector movement, host behaviour, or firmware
- confirm the DRV2605L controller pinout and connection details from the physical board before Experiment 05 wiring
- confirm the coin motors' actual drive technology and compatibility with the DRV2605L before Experiment 05 wiring
- confirm the TTP223 modules' default solder-jumper configuration before Experiment 04 wiring
- confirm the WS2812B-8 module pinout and electrical details before any future external-light experiment

Next experiment: Experiment 03 - Sleep and Wake (not started)
```

This section should provide a rapid re-entry point for future Codex sessions.

---

# 36. Change Discipline

Changes to this specification should distinguish between:

### Implementation Discovery

A detail learned while implementing an existing decision.

Update the relevant section.

### Experimental Evidence

A result that changes a provisional assumption.

Record the evidence and update the specification.

### Architectural Change

A change to a DECIDED principle.

Document:

- what changed;
- why;
- what evidence justified the change;
- what downstream components are affected.

The specification should remain a **living engineering document**, not an immutable contract.

---

# 37. Codex Working Principle

Codex should treat this specification and the companion Project Overview as the project's governing context.

When implementing work:

1. inspect the current repository and build state;
2. identify the current experiment;
3. propose the smallest useful increment;
4. avoid unrelated architectural expansion;
5. implement;
6. test;
7. report observed results;
8. update relevant documentation;
9. identify the next clean experiment.

Unknowns should remain explicit.

Do not invent complexity merely to make the architecture appear complete.

---

# 38. First Milestone

The first milestone is deliberately small.

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

Success means:

```text
[TALISMAN]
boot: ok
firmware: 0.1.0-dev
state: awake
```

followed by a deterministic visible heartbeat from the onboard LED.

At that point:

> **Talisman V1 is alive.**

Everything afterwards is evolution.
