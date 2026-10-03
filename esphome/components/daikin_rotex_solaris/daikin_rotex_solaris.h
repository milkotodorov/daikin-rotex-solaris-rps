#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace daikin_rotex_solaris {

static const char *const TAG = "daikin_rotex_solaris"; // Used for logging

// ============================================================================
// UART BUFFER CONFIGURATION
// ============================================================================
static constexpr uint8_t BUFFER_SIZE = 128;            // Maximum chars stored per line (RX buffer)
static constexpr uint8_t MIN_LINE_LEN = 22;            // Minimum valid line length (reject shorter lines)
static constexpr uint8_t MAX_LINE_LEN = 48;            // Maximum valid line length (reject longer lines)
static constexpr uint32_t LINE_TIMEOUT_MS = 5000;      // Used for clearing buffer
static constexpr uint32_t OFFLINE_TIMEOUT_MS = 90000;  // Used for detecting Solaris RPS offline/unavailable state (90s of no data)

// ============================================================================
// Tz ACCUMULATOR CONSTANTS
// ============================================================================
static constexpr uint32_t OFF_BASELINE_HOLD_MS = 180000; // Tz automation 1: 3-min off-state hold before snapping baseline
static constexpr uint32_t RISE_HOLD_MS = 90000;          // Tz automation 2: 90s rise hold before counting
static constexpr uint32_t DROP_HOLD_MS = 90000;          // Tz automation 3: 90s drop hold before lowering baseline
static constexpr float    TZ_POWER_ON_KW = 0.01f;        // P > 0.01 kW => solar running
static constexpr int      TZ_RISE_CAP = 2;               // Max countable rise per hold (stale-baseline safety net)

// Boot/Info lines sent by Solaris RPS on startup - the first words for each line
static constexpr char BOOT_LINE1[] = "SOLARIS";
static constexpr char BOOT_LINE2[] = "Zyklus";
static constexpr char BOOT_LINE3[] = "HA;BK;P1";

// ============================================================================
// CONVERSION AND ERROR MESSAGE BUFFERS
// ============================================================================
// Reused for strtof/strtol operations instead of allocating new stack buffers
static constexpr uint8_t CONVERSION_BUFFER_SIZE = 16;     // Temporary buffer for number conversions
static constexpr uint16_t ERROR_MSG_BUFFER_SIZE = 256;    // Error message buffer (for unknown error formatting)

// ============================================================================
// DATA STRUCTURE - Solaris RPS protocol
// ============================================================================
// Field count bounds for one complete data line
// RPS3 sends 11 fields; RPS4 sends 13:
//   field 11 = DeltaT — Sollspreizung (target temp. differential TV−TR during modulation, calculated)
//   field 12 = Zust   — Betriebszustand (operating state): 23=active solar, 21=standby
static constexpr uint8_t MIN_FIELDS = 11;
static constexpr uint8_t TOTAL_FIELDS = 13;

// Enum for field indices in the parsed data array
// Protocol format: "Ha;BK;P1;P2;TK;TR;TS;TV;DF;ERR;PWR[;DeltaT;Zust]"
enum SolarisFields : uint8_t {
  SOLARIS_HA = 0,   // Handbetrieb (Manual Operation flag, 0/1)
  SOLARIS_BK = 1,   // Brennerkontakt (Burner Contact flag, 0/1)
  SOLARIS_P1 = 2,   // Umwälzpumpe (Circulation Pump speed, 0-100%)
  SOLARIS_P2 = 3,   // Boosterpumpe (Boost Pump flag, 0/1)
  SOLARIS_TK = 4,   // Kollektortemperatur (Collector Temperature, °C)
  SOLARIS_TR = 5,   // Rücklauftemperatur (Return Temperature, °C)
  SOLARIS_TS = 6,   // Speichertemperatur (Storage Temperature, °C)
  SOLARIS_TV = 7,   // Vorlauftemperatur (Flow Temperature, °C)
  SOLARIS_DF = 8,   // Durchfluss (Flow Rate, l/min, uses comma as decimal separator)
  SOLARIS_ERR = 9,  // Error code (single character: '', K, R, S, D, V, G, F, W) → published as Fehlercode (raw) + Fehlerbeschreibung (translated)
  SOLARIS_PWR = 10, // Leistung (Power output, Watts)
  SOLARIS_DT = 11   // DeltaT/Sollspreizung (RPS4 field; RPS3 computes it as TV−TR)
};

// ============================================================================
// MAIN COMPONENT CLASS
// ============================================================================
class DaikinRotexSolarisComponent : public Component, public uart::UARTDevice {
  public:
    void setup() override;
    void loop() override;           // Main processing loop (called every cycle)
    void dump_config() override;    // Log configuration at startup
    float get_setup_priority() const override { return setup_priority::DATA; }

    // ========================================================================
    // SENSOR SETTER METHODS - Register sensor pointers from configuration
    // ========================================================================
    // Regular numeric sensors (int/float values)
    void set_solaris_p1_sensor(sensor::Sensor *s) { solaris_p1_sensor_ = s; }
    void set_solaris_tk_sensor(sensor::Sensor *s) { solaris_tk_sensor_ = s; }
    void set_solaris_tr_sensor(sensor::Sensor *s) { solaris_tr_sensor_ = s; }
    void set_solaris_ts_sensor(sensor::Sensor *s) { solaris_ts_sensor_ = s; }
    void set_solaris_tv_sensor(sensor::Sensor *s) { solaris_tv_sensor_ = s; }
    void set_solaris_df_sensor(sensor::Sensor *s) { solaris_df_sensor_ = s; }
    void set_solaris_pwr_sensor(sensor::Sensor *s) { solaris_pwr_sensor_ = s; }
    void set_solaris_deltat_sensor(sensor::Sensor *s) { solaris_deltat_sensor_ = s; }
    void set_solaris_tz_sensor(sensor::Sensor *s) { solaris_tz_sensor_ = s; }

    void reset_tz_daily();          // Called from YAML on_time: at 00:00:00
    void set_tz_acc(int v);         // Called from YAML api: services: to manually restore Tz after reboot

    // Binary sensors (on/off states)
    void set_solaris_ha_sensor(binary_sensor::BinarySensor *s) { solaris_ha_sensor_ = s; }
    void set_solaris_bk_sensor(binary_sensor::BinarySensor *s) { solaris_bk_sensor_ = s; }
    void set_solaris_p2_sensor(binary_sensor::BinarySensor *s) { solaris_p2_sensor_ = s; }

    // Text sensors (error code + error description)
    void set_solaris_errcode_sensor(text_sensor::TextSensor *s) { solaris_errcode_sensor_ = s; }
    void set_solaris_errdesc_sensor(text_sensor::TextSensor *s) { solaris_errdesc_sensor_ = s; }

    // Git hash sensor (source code version)
    void set_git_hash_sensor(text_sensor::TextSensor *s, const std::string &hash) {
      git_hash_sensor_ = s;
      git_hash_ = hash;
    }

  protected:
    // ========================================================================
    // INTERNAL PROCESSING METHODS - Core parsing and data handling
    // ========================================================================
    // Parses a complete UART line into individual fields and validates data
    void parse_line_(const char *line, size_t len);

    // Invalidates all sensors states and sets them to N/A
    // Called when RPS is likely offline/unavailable - after OFFLINE_TIMEOUT_MS time of no data received
    void invalidate_all_sensors_();

    // Publishes parsed values to all registered sensor entities
    void publish_values_(const int int_values[], float solaris_df, char error_code, uint8_t token_count);

    // Gets error code description from error code character
    const char *get_error_text_(char error_code);

    // ========================================================================
    // SENSOR STORAGE - Pointers to sensor instances created by ESPHome
    // ========================================================================
    // Numeric sensors (temperatures, flow rates, power)
    sensor::Sensor *solaris_p1_sensor_{nullptr};
    sensor::Sensor *solaris_tk_sensor_{nullptr};
    sensor::Sensor *solaris_tr_sensor_{nullptr};
    sensor::Sensor *solaris_ts_sensor_{nullptr};
    sensor::Sensor *solaris_tv_sensor_{nullptr};
    sensor::Sensor *solaris_df_sensor_{nullptr};
    sensor::Sensor *solaris_pwr_sensor_{nullptr};
    sensor::Sensor *solaris_deltat_sensor_{nullptr};
    sensor::Sensor *solaris_tz_sensor_{nullptr};  // Daily solar storage temp gain (Tz, °C)

    // Binary sensors (on/off states)
    binary_sensor::BinarySensor *solaris_ha_sensor_{nullptr};
    binary_sensor::BinarySensor *solaris_bk_sensor_{nullptr};
    binary_sensor::BinarySensor *solaris_p2_sensor_{nullptr};

    // Text sensors (error code = raw letter; error description = localized text)
    text_sensor::TextSensor *solaris_errcode_sensor_{nullptr};
    text_sensor::TextSensor *solaris_errdesc_sensor_{nullptr};

    // Git hash sensor
    text_sensor::TextSensor *git_hash_sensor_{nullptr};
    std::string git_hash_;

    // ========================================================================
    // UART BUFFER STATE - Tracks incoming character stream
    // ========================================================================
    char buffer_[BUFFER_SIZE];      // Circular RX buffer for one complete line
    uint8_t buffer_idx_{0};         // Current write position in buffer
    uint32_t last_char_time_{0};    // Timestamp of last received character (used for timeout)
    uint32_t p2_off_time_{0};       // Timestamp when P2 last turned off (RPS3 DeltaT hold-off)

    // ========================================================================
    // Tz ACCUMULATOR STATE — ports 4 HA automations + 2 input_number helpers
    // ========================================================================
    int      tz_acc_{0};                    // Published Tz (whole °C, accumulated since midnight)
    int      last_ts_{0};                   // Reference baseline temperature (whole °C)
    uint32_t off_since_{0};                 // Automation 1 hold timer (0 = not holding)
    uint32_t rise_since_{0};                // Automation 2 hold timer
    uint32_t drop_since_{0};                // Automation 3 hold timer
    bool     tz_rebaseline_pending_{false}; // midnight/offline => next reading re-anchors last_ts_

    // ========================================================================
    // REUSABLE TEMPORARY BUFFERS
    // ========================================================================
    // Reused for strtof/strtol operations instead of creating new buffers per conversion
    char conversion_buffer_[CONVERSION_BUFFER_SIZE];   // For numeric parsing (int/float)
    char error_msg_buffer_[ERROR_MSG_BUFFER_SIZE];     // For formatting unknown error messages
};

} // namespace daikin_rotex_solaris
} // namespace esphome