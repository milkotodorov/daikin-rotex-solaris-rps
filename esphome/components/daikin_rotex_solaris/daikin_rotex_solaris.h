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
static constexpr uint8_t BUFFER_SIZE = 128;          // Maximum chars stored per line (RX buffer)
static constexpr uint8_t MIN_LINE_LEN = 22;          // Minimum valid line length (reject shorter lines)
static constexpr uint8_t MAX_LINE_LEN = 48;          // Maximum valid line length (reject longer lines)
static constexpr uint32_t LINE_TIMEOUT_MS = 5000;    // Clear buffer if no newline received within 5s

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
// Total number of semicolon-delimited fields in one complete data line
static constexpr uint8_t TOTAL_FIELDS = 11;

// Enum for field indices in the parsed data array
// Protocol format: "Ha;BK;P1;P2;TK;TR;TS;TV;DF;ERR;PWR"
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
  SOLARIS_ERR = 9,  // Fehlerstatus (Error code, single character: '', K, R, S, D, V, G, F, W)
  SOLARIS_PWR = 10  // Leistung (Power output, Watts)
};

// ============================================================================
// MAIN COMPONENT CLASS
// ============================================================================
class DaikinRotexSolarisComponent : public Component, public uart::UARTDevice {
  public:
    void setup() override {}
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

    // Binary sensors (on/off states)
    void set_solaris_ha_sensor(binary_sensor::BinarySensor *s) { solaris_ha_sensor_ = s; }
    void set_solaris_bk_sensor(binary_sensor::BinarySensor *s) { solaris_bk_sensor_ = s; }
    void set_solaris_p2_sensor(binary_sensor::BinarySensor *s) { solaris_p2_sensor_ = s; }

    // Text sensor (error status messages)
    void set_solaris_err_sensor(text_sensor::TextSensor *s) { solaris_err_sensor_ = s; }

  protected:
    // ========================================================================
    // INTERNAL PROCESSING METHODS - Core parsing and data handling
    // ========================================================================
    // Parses a complete UART line into individual fields and validates data
    void parse_line_(const char *line, size_t len);

    // Publishes parsed values to all registered sensor entities
    void publish_values_(const int int_values[], float solaris_df, char error_code, uint8_t num_tokens);

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

    // Binary sensors (on/off states)
    binary_sensor::BinarySensor *solaris_ha_sensor_{nullptr};
    binary_sensor::BinarySensor *solaris_bk_sensor_{nullptr};
    binary_sensor::BinarySensor *solaris_p2_sensor_{nullptr};

    // Text sensor (error status)
    text_sensor::TextSensor *solaris_err_sensor_{nullptr};

    // ========================================================================
    // UART BUFFER STATE - Tracks incoming character stream
    // ========================================================================
    char buffer_[BUFFER_SIZE];      // Circular RX buffer for one complete line
    uint8_t buffer_idx_{0};         // Current write position in buffer
    uint32_t last_char_time_{0};    // Timestamp of last received character (used for timeout)

    // ========================================================================
    // REUSABLE TEMPORARY BUFFERS
    // ========================================================================
    // Reused for strtof/strtol operations instead of creating new buffers per conversion
    char conversion_buffer_[CONVERSION_BUFFER_SIZE];   // For numeric parsing (int/float)
    char error_msg_buffer_[ERROR_MSG_BUFFER_SIZE];     // For formatting unknown error messages
};

} // namespace daikin_rotex_solaris
} // namespace esphome