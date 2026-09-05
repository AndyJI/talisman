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

constexpr uint8_t kImuI2cAddress = 0x6A;

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
      Serial.println("event: MOVEMENT_STARTED");
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
    Serial.println("event: MOVEMENT_STOPPED");
  }
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
  const uint32_t nowMs = millis();
  awakeStartedAtMs = nowMs;
  lastImuSampleAtMs = nowMs;
  startHeartbeat(nowMs);
}

void loop() {
  const uint32_t nowMs = millis();
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

  if (nowMs - awakeStartedAtMs >= kAwakePeriodMs) {
    sleepAndWake();
  }
}
