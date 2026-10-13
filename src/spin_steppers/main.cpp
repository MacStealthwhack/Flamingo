#include <Arduino.h>
#include <TMCStepper.h>
#include <AccelStepper.h>
#include "config.hpp"

// =============================================================================
//  Flamingo Neck — Steady Spin Test
//  src/main.cpp
//
//  Spins both steppers at a constant speed after a brief UART connection check.
//  Useful for:
//    - Confirming both drivers and motors are wired and working
//    - Continuous-tension tests where you want the cable under steady pull
//
//  SERIAL COMMANDS (115200 baud)
//    p+ / p-   primary forward / reverse
//    s+ / s-   secondary forward / reverse
//    a+ / a-   both forward / reverse
//    px / sx / ax   stop primary / secondary / both
//    p<n> / s<n> / a<n>   set speed steps/sec, e.g. "p800"  "a1200"
//    t         print current speed and direction for both motors
//    ?         print help
// =============================================================================

// ---------- Driver objects ----------
TMC2209Stepper drvP(&Serial1, R_SENSE, UART_ADDR_PRIMARY);
TMC2209Stepper drvS(&Serial1, R_SENSE, UART_ADDR_SECONDARY);

// ---------- Stepper objects ----------
AccelStepper stepP(AccelStepper::DRIVER, PIN_P_STEP, PIN_P_DIR);
AccelStepper stepS(AccelStepper::DRIVER, PIN_S_STEP, PIN_S_DIR);

// ---------- Runtime state ----------
// Speed is stored as a signed value: positive = forward, negative = reverse.
float speedP = 600.0f;
float speedS = 600.0f;
bool  runP   = false;
bool  runS   = false;

String inputLine;

// ---------- Helpers ----------

void configureDriver(TMC2209Stepper &drv) {
  drv.begin();
  drv.toff(4);
  drv.blank_time(24);
  drv.pdn_disable(true);
  drv.I_scale_analog(false);
  drv.rms_current(STEPPER_RUN_MA);
  drv.microsteps(STEPPER_MICROSTEPS);
  drv.en_spreadCycle(false);
  drv.TCOOLTHRS(TCOOLTHRS_VALUE);
}

bool checkUart(TMC2209Stepper &drv, const char* name) {
  uint8_t result = drv.test_connection();
  Serial.print(name);
  if (result == 0) {
    Serial.print(F(": UART OK  version=0x"));
    Serial.println(drv.version(), HEX);
    return true;
  }
  Serial.print(F(": UART FAIL code="));
  Serial.println(result);
  return false;
}

// Apply the current speed/run state to an AccelStepper in "run forever" mode.
// AccelStepper doesn't have a native constant-velocity mode, so we keep
// re-targeting a large position in the current direction each loop iteration.
void applySpeed(AccelStepper &step, float speed, bool running) {
  if (!running) {
    step.setMaxSpeed(0);
    step.stop();
    return;
  }
  float absSpeed = abs(speed);
  step.setMaxSpeed(absSpeed);
  step.setAcceleration(MOTION_ACCEL_SPS2);
  // Target a position far away in the direction of travel so it never stops.
  step.moveTo(speed > 0 ? 2000000000L : -2000000000L);
}

void printStatus() {
  Serial.print(F("PRIMARY   speed="));
  Serial.print(speedP);
  Serial.print(F(" sps  "));
  Serial.println(runP ? (speedP >= 0 ? F("FORWARD") : F("REVERSE")) : F("STOPPED"));

  Serial.print(F("SECONDARY speed="));
  Serial.print(speedS);
  Serial.print(F(" sps  "));
  Serial.println(runS ? (speedS >= 0 ? F("FORWARD") : F("REVERSE")) : F("STOPPED"));
}

void printHelp() {
  Serial.println(F("p+/p- s+/s- a+/a-   start fwd/rev"));
  Serial.println(F("px  sx  ax           stop"));
  Serial.println(F("p<n> s<n> a<n>       set speed (e.g. a800)"));
  Serial.println(F("t   status    ?  help"));
}

void handleCommand(const String &cmd) {
  if (cmd.length() < 2) { printHelp(); return; }

  char target = tolower(cmd[0]);
  char action = tolower(cmd[1]);

  if (target != 'p' && target != 's' && target != 'a' && target != 't' && target != '?') {
    printHelp();
    return;
  }

  if (target == 't' || cmd == "t") { printStatus(); return; }
  if (target == '?' || cmd == "?") { printHelp();   return; }

  bool doP = (target == 'p' || target == 'a');
  bool doS = (target == 's' || target == 'a');

  if (action == '+') {
    if (doP) { speedP =  abs(speedP); runP = true; }
    if (doS) { speedS =  abs(speedS); runS = true; }
  } else if (action == '-') {
    if (doP) { speedP = -abs(speedP); runP = true; }
    if (doS) { speedS = -abs(speedS); runS = true; }
  } else if (action == 'x') {
    if (doP) runP = false;
    if (doS) runS = false;
  } else if (isDigit(action)) {
    // Numeric argument: set speed magnitude, preserve current direction sign.
    float val = cmd.substring(1).toFloat();
    if (val < 50 || val > 4000) {
      Serial.println(F("Speed must be 50-4000 sps."));
      return;
    }
    if (doP) speedP = (speedP >= 0 ? 1 : -1) * val;
    if (doS) speedS = (speedS >= 0 ? 1 : -1) * val;
    Serial.print(F("Speed set to ")); Serial.println(val);
  } else {
    printHelp();
    return;
  }

  if (doP) applySpeed(stepP, speedP, runP);
  if (doS) applySpeed(stepS, speedS, runS);
}

// =============================================================================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}

  Serial1.begin(UART_BAUD);

  // Enable drivers
  pinMode(PIN_P_EN, OUTPUT); digitalWrite(PIN_P_EN, LOW);
  pinMode(PIN_S_EN, OUTPUT); digitalWrite(PIN_S_EN, LOW);

  configureDriver(drvP);
  configureDriver(drvS);

  Serial.println(F("=== Flamingo steady-spin test ==="));
  bool okP = checkUart(drvP, "PRIMARY");
  bool okS = checkUart(drvS, "SECONDARY");

  if (!okP || !okS) {
    Serial.println(F("UART failure — check wiring. Motors will not run."));
    // Leave EN low so drivers are live but no steps will be commanded.
    while (true) {}
  }

  // Both steppers start stopped; AccelStepper needs a non-zero accel even
  // when standing by, otherwise moveTo() calls are silently ignored.
  stepP.setAcceleration(MOTION_ACCEL_SPS2);
  stepS.setAcceleration(MOTION_ACCEL_SPS2);
  stepP.setMaxSpeed(MOTION_MAX_SPEED_SPS);
  stepS.setMaxSpeed(MOTION_MAX_SPEED_SPS);

  printHelp();
  Serial.println(F("Both motors stopped. Send 'a+' to spin both forward."));
}

void loop() {
  // Serial command handling
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      inputLine.trim();
      if (inputLine.length()) {
        handleCommand(inputLine);
        inputLine = "";
      }
    } else {
      inputLine += c;
    }
  }

  // Step both motors every loop iteration
  stepP.run();
  stepS.run();
}