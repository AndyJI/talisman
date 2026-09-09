#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <LSM6DS3.h>
#include <Wire.h>
#include <math.h>

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
                       uint32_t pulseDurationMs, uint32_t gapDurationMs) {
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
    success = writeHapticRegister(0x02, 0x40);
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
  Serial.println(success && pulseStopped && standbySet ? "complete"
                                                       : "i2c_error");
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

void handleSemanticEvent(const SemanticEvent& event) {
  switch (event.type) {
    case SemanticEventType::Tap:
      playHapticPattern("tap", 1, kTapHapticPulseMs, 0);
      return;
    case SemanticEventType::DoubleTap:
      playHapticPattern("double_tap", 2, kDoubleTapHapticPulseMs,
                        kDoubleTapHapticGapMs);
      return;
    case SemanticEventType::LongTouch:
      playHapticPattern("long_touch", 1, kLongTouchHapticPulseMs, 0);
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

  handleSemanticEvent(event);
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
  initialiseImu();
  probeHapticController();
  initialiseTouchInput();
  const uint32_t nowMs = millis();
  awakeStartedAtMs = nowMs;
  lastImuSampleAtMs = nowMs;
  startHeartbeat(nowMs);
}

void loop() {
  const uint32_t nowMs = millis();
  updateTouchInput(nowMs);
  updateLongTouch(nowMs);
  updatePendingTap(nowMs);
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
