#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <LSM6DS3.h>
#include <Wire.h>
#include <stddef.h>
#include <math.h>

using namespace Adafruit_LittleFS_Namespace;

namespace {

constexpr char kFirmwareVersion[] = "0.1.0-dev";

constexpr uint32_t kSerialReadyTimeoutMs = 3000;
constexpr uint32_t kHeartbeatPeriodMs = 1000;
constexpr uint32_t kHeartbeatPulseMs = 100;
constexpr uint32_t kImuSamplePeriodMs = 250;
constexpr uint32_t kAwakePeriodMs = 10000;
constexpr uint32_t kSleepPeriodMs = 5000;
constexpr bool kScheduledSleepEnabled = false;
constexpr uint32_t kTouchDebounceMs = 25;
constexpr uint32_t kTapMaximumMs = 500;
constexpr uint32_t kDoubleTapGapMaximumMs = 350;
constexpr uint32_t kLongTouchThresholdMs = 1500;
constexpr uint32_t kTapHapticPulseMs = 150;
constexpr uint32_t kDoubleTapHapticPulseMs = 90;
constexpr uint32_t kDoubleTapHapticGapMs = 100;
constexpr uint32_t kLongTouchHapticPulseMs = 350;
constexpr uint32_t kBehaviourDecayPeriodMs = 5000;
constexpr uint8_t kArousalDecayAmount = 2;
constexpr uint8_t kLowArousalHapticDrive = 0x28;
constexpr uint8_t kHighArousalHapticDrive = 0x58;
constexpr uint8_t kHighArousalThreshold = 50;
constexpr uint32_t kPersistenceMagic = 0x54414C31;
constexpr uint16_t kPersistenceSchemaVersion = 1;
constexpr uint8_t kPersistentEventCapacity = 8;
constexpr uint32_t kCheckpointDelayMs = 10000;
constexpr char kPersistenceSlotA[] = "/state-a.bin";
constexpr char kPersistenceSlotB[] = "/state-b.bin";

constexpr uint8_t kImuI2cAddress = 0x6A;
constexpr uint8_t kHapticI2cAddress = 0x5A;
constexpr uint8_t kTouchPin = D1;

constexpr float kMovementStartThresholdDps = 12.0F;
constexpr float kMovementStopThresholdDps = 4.0F;
constexpr uint8_t kMovementStartSamples = 2;
constexpr uint8_t kMovementStopSamples = 8;

// The XIAO nRF52840 Sense user LED is active-low.
constexpr uint8_t kLedOn = LOW;
constexpr uint8_t kLedOff = HIGH;

uint32_t heartbeatStartedAtMs = 0;
uint32_t heartbeatCount = 0;
bool heartbeatLedIsOn = false;
uint32_t awakeStartedAtMs = 0;
uint32_t sleepCycleCount = 0;
bool touchInputIsHigh = false;
bool touchCandidateIsHigh = false;
uint32_t touchCandidateStartedAtMs = 0;
uint32_t touchPressedAtMs = 0;
bool longTouchWasEmitted = false;
bool tapIsPending = false;
uint32_t firstTapReleasedAtMs = 0;
bool secondTapIsInProgress = false;
bool hapticControllerIsAvailable = false;
uint32_t lastBehaviourDecayAtMs = 0;
bool persistenceIsAvailable = false;
bool persistenceIsDirty = false;
uint32_t persistenceDirtyAtMs = 0;
uint32_t persistenceGeneration = 0;
uint32_t interactionCount = 0;
uint32_t nextPersistentEventSequence = 1;

LSM6DS3 imu(I2C_MODE, kImuI2cAddress);
bool imuIsAvailable = false;
uint32_t lastImuSampleAtMs = 0;

bool movementIsActive = false;
uint8_t movementStartSampleCount = 0;
uint8_t movementStopSampleCount = 0;

struct ImuSample {
  float accelXG;
  float accelYG;
  float accelZG;
  float gyroXDps;
  float gyroYDps;
  float gyroZDps;
};

enum class SemanticEventType : uint8_t {
  MovementStarted,
  MovementStopped,
  Tap,
  DoubleTap,
  LongTouch,
};

struct SemanticEvent {
  SemanticEventType type;
  uint32_t occurredAtMs;
  uint32_t durationMs;
};

struct BehaviourState {
  uint8_t arousal;
  uint8_t familiarity;
};

BehaviourState behaviourState = {20, 0};

struct PersistentEventRecord {
  uint32_t sequence;
  uint32_t durationMs;
  uint8_t type;
  uint8_t reserved[3];
};

struct PersistentStateV1 {
  uint32_t magic;
  uint16_t schemaVersion;
  uint16_t structSize;
  uint32_t generation;
  uint32_t interactionCount;
  uint32_t nextEventSequence;
  uint8_t familiarity;
  uint8_t eventCount;
  uint8_t nextEventIndex;
  uint8_t reserved;
  PersistentEventRecord events[kPersistentEventCapacity];
  uint32_t checksum;
};

PersistentEventRecord persistentEvents[kPersistentEventCapacity] = {};
uint8_t persistentEventCount = 0;
uint8_t nextPersistentEventIndex = 0;

void printIdentity() {
  Serial.println("[TALISMAN]");
  Serial.println("boot: ok");
  Serial.print("firmware: ");
  Serial.println(kFirmwareVersion);
  Serial.println("state: awake");
}

void startHeartbeat(uint32_t nowMs) {
  heartbeatStartedAtMs = nowMs;
  heartbeatLedIsOn = true;
  digitalWrite(LED_BUILTIN, kLedOn);

  ++heartbeatCount;
  Serial.print("heartbeat: ");
  Serial.println(heartbeatCount);
}

void initialiseImu() {
  if (imu.begin() != 0) {
    Serial.println("imu: error");
    return;
  }

  imuIsAvailable = true;
  Serial.println("imu: ok");
  Serial.println("motion: start=12.0dps/2_samples stop=4.0dps/8_samples");
}

bool writeHapticRegister(uint8_t registerAddress, uint8_t value) {
  Wire.beginTransmission(kHapticI2cAddress);
  Wire.write(registerAddress);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

void probeHapticController() {
  // D4/D5 use Wire. The onboard IMU is on the separate Wire1 bus.
  Wire.begin();
  Wire.setClock(100000);
  Wire.beginTransmission(kHapticI2cAddress);
  const uint8_t result = Wire.endTransmission();

  hapticControllerIsAvailable = result == 0;
  Serial.print("haptic: address=0x5A status=");
  Serial.println(hapticControllerIsAvailable ? "found" : "not_found");
}

void playHapticPattern(const char* patternName, uint8_t pulseCount,
                       uint32_t pulseDurationMs, uint32_t gapDurationMs,
                       uint8_t driveStrength) {
  if (!hapticControllerIsAvailable) {
    Serial.print("haptic: pattern=");
    Serial.print(patternName);
    Serial.println(" status=skipped_controller_unavailable");
    return;
  }

  // The DRV2605L defaults to ERM closed-loop configuration. Enter real-time
  // playback mode and apply a low drive value briefly, then explicitly stop
  // and return the controller to standby. This intentionally avoids the
  // effect library's stronger overdrive patterns while the motor rating is
  // still provisional.
  bool success = writeHapticRegister(0x01, 0x05);
  for (uint8_t pulse = 0; success && pulse < pulseCount; ++pulse) {
    success = writeHapticRegister(0x02, driveStrength);
    if (success) {
      delay(pulseDurationMs);
      success = writeHapticRegister(0x02, 0x00);
    }
    if (success && pulse + 1 < pulseCount) {
      delay(gapDurationMs);
    }
  }
  const bool pulseStopped = writeHapticRegister(0x02, 0x00);
  const bool standbySet = writeHapticRegister(0x01, 0x40);

  Serial.print("haptic: pattern=");
  Serial.print(patternName);
  Serial.print(" status=");
  Serial.print(success && pulseStopped && standbySet ? "complete"
                                                      : "i2c_error");
  Serial.print(" drive=");
  Serial.println(driveStrength);
}

const char* semanticEventName(SemanticEventType type) {
  switch (type) {
    case SemanticEventType::MovementStarted:
      return "MOVEMENT_STARTED";
    case SemanticEventType::MovementStopped:
      return "MOVEMENT_STOPPED";
    case SemanticEventType::Tap:
      return "TAP";
    case SemanticEventType::DoubleTap:
      return "DOUBLE_TAP";
    case SemanticEventType::LongTouch:
      return "LONG_TOUCH";
  }

  return "UNKNOWN";
}

uint32_t persistenceChecksum(const PersistentStateV1& state) {
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&state);
  uint32_t checksum = 2166136261UL;
  for (size_t index = 0; index < offsetof(PersistentStateV1, checksum);
       ++index) {
    checksum ^= bytes[index];
    checksum *= 16777619UL;
  }
  return checksum;
}

bool persistentStateIsValid(const PersistentStateV1& state) {
  return state.magic == kPersistenceMagic &&
         state.schemaVersion == kPersistenceSchemaVersion &&
         state.structSize == sizeof(PersistentStateV1) &&
         state.eventCount <= kPersistentEventCapacity &&
         state.nextEventIndex < kPersistentEventCapacity &&
         state.checksum == persistenceChecksum(state);
}

bool readPersistenceSlot(const char* path, PersistentStateV1& state) {
  File file(InternalFS);
  if (!file.open(path, FILE_O_READ)) {
    return false;
  }

  const bool correctSize = file.size() == sizeof(PersistentStateV1);
  const int bytesRead =
      correctSize ? file.read(&state, sizeof(PersistentStateV1)) : 0;
  file.close();
  return bytesRead == sizeof(PersistentStateV1) &&
         persistentStateIsValid(state);
}

void printPersistentEventHistory() {
  Serial.print("persistence: history_count=");
  Serial.println(persistentEventCount);
  const uint8_t oldestIndex =
      persistentEventCount < kPersistentEventCapacity
          ? 0
          : nextPersistentEventIndex;
  for (uint8_t offset = 0; offset < persistentEventCount; ++offset) {
    const uint8_t index =
        (oldestIndex + offset) % kPersistentEventCapacity;
    const PersistentEventRecord& record = persistentEvents[index];
    Serial.print("persistence: event sequence=");
    Serial.print(record.sequence);
    Serial.print(" type=");
    Serial.print(
        semanticEventName(static_cast<SemanticEventType>(record.type)));
    if (record.durationMs > 0) {
      Serial.print(" duration_ms=");
      Serial.print(record.durationMs);
    }
    Serial.println();
  }
}

void initialisePersistence() {
  persistenceIsAvailable = InternalFS.begin();
  if (!persistenceIsAvailable) {
    Serial.println("persistence: status=unavailable defaults=used");
    return;
  }

  PersistentStateV1 slotA = {};
  PersistentStateV1 slotB = {};
  const bool slotAIsValid = readPersistenceSlot(kPersistenceSlotA, slotA);
  const bool slotBIsValid = readPersistenceSlot(kPersistenceSlotB, slotB);

  if (!slotAIsValid && !slotBIsValid) {
    Serial.println("persistence: status=no_valid_snapshot defaults=used");
    return;
  }

  const PersistentStateV1& restored =
      slotAIsValid && (!slotBIsValid || slotA.generation >= slotB.generation)
          ? slotA
          : slotB;
  persistenceGeneration = restored.generation;
  interactionCount = restored.interactionCount;
  nextPersistentEventSequence = restored.nextEventSequence;
  behaviourState.familiarity = restored.familiarity;
  persistentEventCount = restored.eventCount;
  nextPersistentEventIndex = restored.nextEventIndex;
  memcpy(persistentEvents, restored.events, sizeof(persistentEvents));

  Serial.print("persistence: status=restored schema=");
  Serial.print(restored.schemaVersion);
  Serial.print(" generation=");
  Serial.print(restored.generation);
  Serial.print(" interaction_count=");
  Serial.print(interactionCount);
  Serial.print(" familiarity=");
  Serial.println(behaviourState.familiarity);
  printPersistentEventHistory();
}

void recordPersistentInteraction(const SemanticEvent& event) {
  switch (event.type) {
    case SemanticEventType::Tap:
    case SemanticEventType::DoubleTap:
    case SemanticEventType::LongTouch:
      break;
    case SemanticEventType::MovementStarted:
    case SemanticEventType::MovementStopped:
      return;
  }

  ++interactionCount;
  persistentEvents[nextPersistentEventIndex] = {
      nextPersistentEventSequence++, event.durationMs,
      static_cast<uint8_t>(event.type), {0, 0, 0}};
  nextPersistentEventIndex =
      (nextPersistentEventIndex + 1) % kPersistentEventCapacity;
  if (persistentEventCount < kPersistentEventCapacity) {
    ++persistentEventCount;
  }
  if (!persistenceIsDirty) {
    persistenceDirtyAtMs = event.occurredAtMs;
  }
  persistenceIsDirty = true;

  Serial.print("persistence: dirty interaction_count=");
  Serial.print(interactionCount);
  Serial.print(" history_count=");
  Serial.println(persistentEventCount);
}

bool writePersistenceSlot(const char* path, const PersistentStateV1& state) {
  InternalFS.remove(path);
  File file(InternalFS);
  if (!file.open(path, FILE_O_WRITE)) {
    return false;
  }

  const size_t bytesWritten = file.write(
      reinterpret_cast<const uint8_t*>(&state), sizeof(PersistentStateV1));
  file.flush();
  file.close();
  return bytesWritten == sizeof(PersistentStateV1);
}

void checkpointPersistence(uint32_t nowMs) {
  if (!persistenceIsAvailable || !persistenceIsDirty ||
      nowMs - persistenceDirtyAtMs < kCheckpointDelayMs) {
    return;
  }

  PersistentStateV1 state = {};
  state.magic = kPersistenceMagic;
  state.schemaVersion = kPersistenceSchemaVersion;
  state.structSize = sizeof(PersistentStateV1);
  state.generation = persistenceGeneration + 1;
  state.interactionCount = interactionCount;
  state.nextEventSequence = nextPersistentEventSequence;
  state.familiarity = behaviourState.familiarity;
  state.eventCount = persistentEventCount;
  state.nextEventIndex = nextPersistentEventIndex;
  memcpy(state.events, persistentEvents, sizeof(persistentEvents));
  state.checksum = persistenceChecksum(state);

  const char* targetPath =
      state.generation % 2 == 1 ? kPersistenceSlotA : kPersistenceSlotB;
  if (!writePersistenceSlot(targetPath, state)) {
    Serial.print("persistence: checkpoint=failed generation=");
    Serial.println(state.generation);
    persistenceDirtyAtMs = nowMs;
    return;
  }

  persistenceGeneration = state.generation;
  persistenceIsDirty = false;
  Serial.print("persistence: checkpoint=complete schema=");
  Serial.print(state.schemaVersion);
  Serial.print(" generation=");
  Serial.print(state.generation);
  Serial.print(" interaction_count=");
  Serial.print(state.interactionCount);
  Serial.print(" familiarity=");
  Serial.print(state.familiarity);
  Serial.print(" history_count=");
  Serial.println(state.eventCount);
}

uint8_t boundedAdd(uint8_t value, int8_t change) {
  const int16_t result = static_cast<int16_t>(value) + change;
  return static_cast<uint8_t>(constrain(result, 0, 100));
}

void printBehaviourState(const char* reason) {
  Serial.print("behaviour: reason=");
  Serial.print(reason);
  Serial.print(" arousal=");
  Serial.print(behaviourState.arousal);
  Serial.print(" familiarity=");
  Serial.println(behaviourState.familiarity);
}

void updateBehaviourForEvent(const SemanticEvent& event) {
  int8_t arousalChange = 0;
  int8_t familiarityChange = 0;

  switch (event.type) {
    case SemanticEventType::MovementStarted:
      arousalChange = 20;
      break;
    case SemanticEventType::MovementStopped:
      break;
    case SemanticEventType::Tap:
      arousalChange = 5;
      familiarityChange = 2;
      break;
    case SemanticEventType::DoubleTap:
      arousalChange = 10;
      familiarityChange = 3;
      break;
    case SemanticEventType::LongTouch:
      arousalChange = -10;
      familiarityChange = 4;
      break;
  }

  behaviourState.arousal =
      boundedAdd(behaviourState.arousal, arousalChange);
  behaviourState.familiarity =
      boundedAdd(behaviourState.familiarity, familiarityChange);
  printBehaviourState(semanticEventName(event.type));
}

uint8_t currentHapticDriveStrength() {
  return behaviourState.arousal >= kHighArousalThreshold
             ? kHighArousalHapticDrive
             : kLowArousalHapticDrive;
}

void expressSemanticEvent(const SemanticEvent& event) {
  const uint8_t driveStrength = currentHapticDriveStrength();

  switch (event.type) {
    case SemanticEventType::Tap:
      playHapticPattern("tap", 1, kTapHapticPulseMs, 0, driveStrength);
      return;
    case SemanticEventType::DoubleTap:
      playHapticPattern("double_tap", 2, kDoubleTapHapticPulseMs,
                        kDoubleTapHapticGapMs, driveStrength);
      return;
    case SemanticEventType::LongTouch:
      playHapticPattern("long_touch", 1, kLongTouchHapticPulseMs, 0,
                        driveStrength);
      return;
    case SemanticEventType::MovementStarted:
    case SemanticEventType::MovementStopped:
      return;
  }
}

void emitSemanticEvent(SemanticEventType type, uint32_t durationMs = 0) {
  const SemanticEvent event = {type, millis(), durationMs};

  Serial.print("event: ");
  Serial.print(semanticEventName(event.type));
  Serial.print(" occurred_at_ms=");
  Serial.print(event.occurredAtMs);
  if (event.durationMs > 0) {
    Serial.print(" duration_ms=");
    Serial.print(event.durationMs);
  }
  Serial.println();

  recordPersistentInteraction(event);
  updateBehaviourForEvent(event);
  expressSemanticEvent(event);
}

void updateBehaviourDecay(uint32_t nowMs) {
  if (nowMs - lastBehaviourDecayAtMs < kBehaviourDecayPeriodMs) {
    return;
  }

  lastBehaviourDecayAtMs = nowMs;
  if (behaviourState.arousal == 0) {
    return;
  }

  behaviourState.arousal =
      boundedAdd(behaviourState.arousal, -kArousalDecayAmount);
  printBehaviourState("TIME_DECAY");
}

ImuSample readImuSample() {
  return {
      imu.readFloatAccelX(), imu.readFloatAccelY(), imu.readFloatAccelZ(),
      imu.readFloatGyroX(), imu.readFloatGyroY(), imu.readFloatGyroZ(),
  };
}

void printImuSample(const ImuSample& sample) {
  Serial.print("imu: accel_g=[");
  Serial.print(sample.accelXG, 4);
  Serial.print(',');
  Serial.print(sample.accelYG, 4);
  Serial.print(',');
  Serial.print(sample.accelZG, 4);
  Serial.print("] gyro_dps=[");
  Serial.print(sample.gyroXDps, 4);
  Serial.print(',');
  Serial.print(sample.gyroYDps, 4);
  Serial.print(',');
  Serial.print(sample.gyroZDps, 4);
  Serial.println(']');
}

float peakGyroMagnitudeDps(const ImuSample& sample) {
  return max(fabsf(sample.gyroXDps),
             max(fabsf(sample.gyroYDps), fabsf(sample.gyroZDps)));
}

void updateMovement(const ImuSample& sample) {
  const float peakGyroDps = peakGyroMagnitudeDps(sample);

  if (!movementIsActive) {
    movementStopSampleCount = 0;

    if (peakGyroDps >= kMovementStartThresholdDps) {
      ++movementStartSampleCount;
    } else {
      movementStartSampleCount = 0;
    }

    if (movementStartSampleCount >= kMovementStartSamples) {
      movementIsActive = true;
      movementStartSampleCount = 0;
      emitSemanticEvent(SemanticEventType::MovementStarted);
    }

    return;
  }

  movementStartSampleCount = 0;

  if (peakGyroDps <= kMovementStopThresholdDps) {
    ++movementStopSampleCount;
  } else {
    movementStopSampleCount = 0;
  }

  if (movementStopSampleCount >= kMovementStopSamples) {
    movementIsActive = false;
    movementStopSampleCount = 0;
    emitSemanticEvent(SemanticEventType::MovementStopped);
  }
}

void initialiseTouchInput() {
  pinMode(kTouchPin, INPUT);
  touchInputIsHigh = digitalRead(kTouchPin) == HIGH;
  touchCandidateIsHigh = touchInputIsHigh;
  touchCandidateStartedAtMs = millis();

  Serial.println(
      "touch: pin=D1 mode=momentary_active_high debounce_ms=25 "
      "tap_max_ms=500 double_tap_gap_ms=350 long_touch_ms=1500");
  Serial.print("touch: initial=");
  Serial.println(touchInputIsHigh ? "pressed" : "released");
  if (!kScheduledSleepEnabled) {
    Serial.println("sleep: disabled reason=haptic_experiment");
  }
}

void updateTouchInput(uint32_t nowMs) {
  const bool inputIsHigh = digitalRead(kTouchPin) == HIGH;

  if (inputIsHigh != touchCandidateIsHigh) {
    touchCandidateIsHigh = inputIsHigh;
    touchCandidateStartedAtMs = nowMs;
    return;
  }

  if (touchCandidateIsHigh == touchInputIsHigh ||
      nowMs - touchCandidateStartedAtMs < kTouchDebounceMs) {
    return;
  }

  touchInputIsHigh = touchCandidateIsHigh;

  if (touchInputIsHigh) {
    touchPressedAtMs = nowMs;
    longTouchWasEmitted = false;
    secondTapIsInProgress =
        tapIsPending && nowMs - firstTapReleasedAtMs <= kDoubleTapGapMaximumMs;
    Serial.println("touch: pressed");
    return;
  }

  const uint32_t touchDurationMs = nowMs - touchPressedAtMs;
  Serial.print("touch: released duration_ms=");
  Serial.println(touchDurationMs);

  if (!longTouchWasEmitted && touchDurationMs <= kTapMaximumMs) {
    if (secondTapIsInProgress) {
      tapIsPending = false;
      secondTapIsInProgress = false;
      emitSemanticEvent(SemanticEventType::DoubleTap);
    } else {
      tapIsPending = true;
      firstTapReleasedAtMs = nowMs;
    }
  } else if (!longTouchWasEmitted) {
    if (secondTapIsInProgress && tapIsPending) {
      tapIsPending = false;
      emitSemanticEvent(SemanticEventType::Tap);
    }
    secondTapIsInProgress = false;
    Serial.println("touch: unclassified");
  }
}

void updatePendingTap(uint32_t nowMs) {
  if (!tapIsPending || touchInputIsHigh ||
      nowMs - firstTapReleasedAtMs <= kDoubleTapGapMaximumMs) {
    return;
  }

  tapIsPending = false;
  emitSemanticEvent(SemanticEventType::Tap);
}

void updateLongTouch(uint32_t nowMs) {
  if (!touchInputIsHigh || longTouchWasEmitted ||
      nowMs - touchPressedAtMs < kLongTouchThresholdMs) {
    return;
  }

  longTouchWasEmitted = true;
  if (secondTapIsInProgress && tapIsPending) {
    tapIsPending = false;
    secondTapIsInProgress = false;
    emitSemanticEvent(SemanticEventType::Tap);
  }
  emitSemanticEvent(SemanticEventType::LongTouch,
                    nowMs - touchPressedAtMs);
}

void sleepAndWake() {
  ++sleepCycleCount;

  Serial.print("sleep: entering cycle=");
  Serial.print(sleepCycleCount);
  Serial.print(" duration_ms=");
  Serial.println(kSleepPeriodMs);
  Serial.println("state: sleeping");
  Serial.flush();

  heartbeatLedIsOn = false;
  digitalWrite(LED_BUILTIN, kLedOff);

  // In this core, delay() blocks the loop task and allows FreeRTOS tickless
  // idle to put the CPU into event-wait sleep until the scheduled wake time.
  delay(kSleepPeriodMs);

  const uint32_t nowMs = millis();
  awakeStartedAtMs = nowMs;
  lastImuSampleAtMs = nowMs;

  Serial.print("wake: cycle=");
  Serial.println(sleepCycleCount);
  Serial.println("state: awake");
  startHeartbeat(nowMs);
}

}  // namespace

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, kLedOff);

  Serial.begin(115200);

  const uint32_t serialWaitStartedAtMs = millis();
  while (!Serial && millis() - serialWaitStartedAtMs < kSerialReadyTimeoutMs) {
    delay(10);
  }

  printIdentity();
  initialisePersistence();
  initialiseImu();
  probeHapticController();
  initialiseTouchInput();
  const uint32_t nowMs = millis();
  awakeStartedAtMs = nowMs;
  lastImuSampleAtMs = nowMs;
  lastBehaviourDecayAtMs = nowMs;
  printBehaviourState("INITIAL");
  startHeartbeat(nowMs);
}

void loop() {
  const uint32_t nowMs = millis();
  updateTouchInput(nowMs);
  updateLongTouch(nowMs);
  updatePendingTap(nowMs);
  updateBehaviourDecay(nowMs);
  checkpointPersistence(nowMs);
  const uint32_t heartbeatElapsedMs = nowMs - heartbeatStartedAtMs;

  if (heartbeatLedIsOn && heartbeatElapsedMs >= kHeartbeatPulseMs) {
    heartbeatLedIsOn = false;
    digitalWrite(LED_BUILTIN, kLedOff);
  }

  if (heartbeatElapsedMs >= kHeartbeatPeriodMs) {
    startHeartbeat(nowMs);
  }

  if (imuIsAvailable && nowMs - lastImuSampleAtMs >= kImuSamplePeriodMs) {
    lastImuSampleAtMs = nowMs;
    const ImuSample sample = readImuSample();
    printImuSample(sample);
    updateMovement(sample);
  }

  if (kScheduledSleepEnabled &&
      nowMs - awakeStartedAtMs >= kAwakePeriodMs && !touchInputIsHigh &&
      !tapIsPending) {
    sleepAndWake();
  }
}
