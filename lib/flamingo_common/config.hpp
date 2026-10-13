#pragma once

#include <Arduino.h>

// =============================================================================
//  Flamingo Configuration
//
//  All pin assignments, driver constants, motion parameters, and homing
//  tuning values live here.
// =============================================================================


// -----------------------------------------------------------------------------
//  Primary stepper (lower neck segment)
//  UART address 0: MS1=GND, MS2=GND
// -----------------------------------------------------------------------------
constexpr uint8_t PIN_P_STEP = 5;
constexpr uint8_t PIN_P_DIR  = 6;
constexpr uint8_t PIN_P_EN   = 9;

// -----------------------------------------------------------------------------
//  Secondary stepper (upper neck segment)
//  UART address 1: MS1=3.3V, MS2=GND
//  NOTE: PIN_S_EN (D13) is also the Feather 32u4 onboard LED.
//        It will glow whenever the secondary driver is enabled. This is normal.
// -----------------------------------------------------------------------------
constexpr uint8_t PIN_S_STEP = 11;
constexpr uint8_t PIN_S_DIR  = 12;
constexpr uint8_t PIN_S_EN   = 13;  // TODO: move to a free pin if LED conflicts

// -----------------------------------------------------------------------------
//  Servo (mouth)
//  D10 is provisional from the original handoff — confirm before final wiring.
// -----------------------------------------------------------------------------
constexpr uint8_t PIN_SERVO = 10;  // TODO: verify

// -----------------------------------------------------------------------------
//  UART (shared single-wire half-duplex to both TMC2209 drivers)
//  Uses hardware Serial1 (D0=RX, D1=TX) on the Feather 32u4.
//  Wiring: TX --[1kΩ]--+-- PDN_UART (both drivers)
//                       |
//          RX ----------+
// -----------------------------------------------------------------------------
constexpr uint32_t UART_BAUD = 115200;

constexpr uint8_t UART_ADDR_PRIMARY   = 0;
constexpr uint8_t UART_ADDR_SECONDARY = 1;

// -----------------------------------------------------------------------------
//  TMC2209 driver constants
// -----------------------------------------------------------------------------

// Confirmed 0.11Ω (110mΩ) for the Adafruit TMC2209 breakout boards in this build.
constexpr float R_SENSE = 0.11f;

// Motor: STEPPERONLINE 17HS15-1504S-X1, rated 1500mA / phase
// Running at 800mA initially; raise toward 1200mA once thermals are confirmed.
// Hard ceiling is 1500mA — do not exceed the motor's rated current.
constexpr uint16_t STEPPER_RUN_MA = 800;   // TODO: raise to 1100-1200 after thermal validation

constexpr uint16_t STEPPER_MICROSTEPS = 16;

// -----------------------------------------------------------------------------
//  Motion parameters
// -----------------------------------------------------------------------------
constexpr float MOTION_MAX_SPEED_SPS = 1500.0f;  // steps/sec, normal operation
constexpr float MOTION_ACCEL_SPS2    = 3000.0f;  // steps/sec²

// -----------------------------------------------------------------------------
//  Homing parameters
//
//  StallGuard signal (SG_RESULT) is speed-dependent — always home at this
//  fixed speed. Do not change HOME_SPEED_SPS without re-tuning SGTHRS.
// -----------------------------------------------------------------------------
constexpr float HOME_SPEED_SPS  = 600.0f;
constexpr float HOME_ACCEL_SPS2 = 2000.0f;

// Direction toward the hard stop. Flip to -1 if a motor homes the wrong way.
constexpr int HOME_DIR_PRIMARY   =  1;   // TODO: verify physical direction
constexpr int HOME_DIR_SECONDARY =  1;   // TODO: verify physical direction

// Steps to retreat from the hard stop after a confirmed stall, before zeroing.
constexpr long HOME_BACKOFF_STEPS = 200;

// Safety cutoff: if SG_RESULT never drops below threshold, stop after this
// many steps rather than grinding indefinitely against the stop.
constexpr long HOME_MAX_STEPS = 20000;

// Number of consecutive low SG_RESULT readings required to confirm a stall.
// Filters single-sample noise without adding meaningful latency.
constexpr uint8_t HOME_CONFIRM_COUNT = 3;

// StallGuard thresholds — tune from live SG_RESULT output with the homing
// test sketch. Lower SG_RESULT = more load; a stall should read clearly below
// the free-spinning baseline. Start high and reduce until reliable.
constexpr uint16_t SGTHRS_PRIMARY   = 100;  // TODO: tune experimentally
constexpr uint16_t SGTHRS_SECONDARY = 100;  // TODO: tune experimentally

// TCOOLTHRS: velocity register below which StallGuard is disabled by the chip.
// 0xFFFFF activates it across the full speed range (safe for testing).
constexpr uint32_t TCOOLTHRS_VALUE = 0xFFFFF;

// -----------------------------------------------------------------------------
//  Motor coil wiring reference (not used in code — here for build reference)
//
//  STEPPERONLINE 17HS15-1504S-X1:
//    Coil A:  Red (A+)  and Yellow (A-)   → driver A1 / A2
//    Coil B:  Green (B+) and Gray (B-)    → driver B1 / B2
//  Swapping A+/A- or B+/B- reverses direction for that coil (safe).
//  Swapping A wires into B terminals causes the "wiggle" fault — don't mix coils.
// -----------------------------------------------------------------------------