#include "daikin_rotex_solaris.h"
#include "esphome/core/log.h"
#include "solaris_error_codes.h"

namespace esphome {
namespace daikin_rotex_solaris {

void DaikinRotexSolarisComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "DAIKIN/ROTEX Solaris RPS Configuration:");
  LOG_SENSOR("  ", "solaris_tk", solaris_tk_sensor_);
  LOG_SENSOR("  ", "solaris_tr", solaris_tr_sensor_);
  LOG_SENSOR("  ", "solaris_ts", solaris_ts_sensor_);
  LOG_SENSOR("  ", "solaris_tv", solaris_tv_sensor_);
  LOG_SENSOR("  ", "solaris_df", solaris_df_sensor_);
  LOG_SENSOR("  ", "solaris_pwr", solaris_pwr_sensor_);
  LOG_SENSOR("  ", "solaris_p1", solaris_p1_sensor_);
  LOG_BINARY_SENSOR("  ", "solaris_p2", solaris_p2_sensor_);
  LOG_BINARY_SENSOR("  ", "solaris_ha", solaris_ha_sensor_);
  LOG_BINARY_SENSOR("  ", "solaris_bk", solaris_bk_sensor_);
  LOG_TEXT_SENSOR("  ", "solaris_err", solaris_err_sensor_);
}

void DaikinRotexSolarisComponent::loop() {
  uint32_t now = millis();

  // ========================================================================
  // DEBUG SIMULATION
  // ========================================================================
  // FOR DEBUGGING/TESTING PURPOSES ONLY:
  // Uncomment this block to simulate incoming UART data for testing
  // Parses debug string every 10 seconds
  //
  // static uint32_t last_debug_time = 0;
  // if (now - last_debug_time >= 10000) { // Parse every 10s
  //   last_debug_time = now;
  //   // Format: Ha;BK;P1;P2;TK;TR;TS;TV;DF;Err;P
  //   // Err codes: ''/K/R/S/V/D/G/F/W (empty = no error)
  //   static const char DEBUG_STRING[] = "0;1;75;0;84;58;61;63;3,2;;3500";
  //   parse_line_(DEBUG_STRING, sizeof(DEBUG_STRING) - 1);
  // }
  // return;
  // ========================================================================
  // END DEBUG SIMULATION
  // ========================================================================

  // ========================================================================
  // BUFFER TIMEOUT HANDLING - Prevent corrupted data accumulation
  // ========================================================================
  // If we have partial data and haven't received anything for LINE_TIMEOUT_MS
  // discard the incomplete line to allow recovery from transmission errors
  if (buffer_idx_ > 0 && (now - last_char_time_ > LINE_TIMEOUT_MS)) {
    ESP_LOGW(TAG, "Line timeout(%us), clearing buffer (%u chars)", 
      LINE_TIMEOUT_MS / 1000, buffer_idx_);
    buffer_idx_ = 0;
    last_char_time_ = now;
  }

  // ========================================================================
  // PROCESS AVAILABLE UART DATA
  // ========================================================================
  while (available()) {
    uint8_t c;
    if (!read_byte(&c)) break;

    last_char_time_ = now;

    if (c == '\n') {
      // ====================================================================
      // COMPLETE LINE RECEIVED (newline detected)
      // ====================================================================
      if (buffer_idx_ > 0) {
        buffer_[buffer_idx_] = '\0';  // Null-terminate for string functions
        
        #if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_DEBUG
        ESP_LOGD(TAG, "Complete line (%u chars): %s", buffer_idx_, buffer_);
        #endif

        parse_line_(buffer_, buffer_idx_);
        buffer_idx_ = 0;  // Reset for next line
      }

    } else if (c != '\r' && buffer_idx_ < BUFFER_SIZE - 1) {
      // ====================================================================
      // APPEND CHARACTER TO BUFFER (skip carriage return, prevent overflow)
      // ====================================================================
      // Store character if it's not CR and buffer has space remaining
      buffer_[buffer_idx_++] = c;
    }
  }
}

const char *DaikinRotexSolarisComponent::get_error_text_(char error_code) {
  // ========================================================================
  // ERROR CODE LOOKUP - Search for known error code in generated header
  // ========================================================================
  // Search all entries except the last one (the "unknown error")
  for (size_t i = 0; i < UNKNOWN_ERROR_INDEX; i++) {
    if (ERROR_CODES[i].code == error_code) {
      if (error_code != '\0') {
        ESP_LOGE(TAG, "Solaris Error: Code %c; Description: %s", 
          error_code, ERROR_CODES[i].description);
      }
      return ERROR_CODES[i].description;
    }
  }

  // ========================================================================
  // UNKNOWN ERROR CODE - Format fallback message with the actual code
  // ========================================================================
  // Unknown error code - We do not have concrete error message. Return the  
  // actual code/message for logging and troubleshooting.
  const char *base_msg = ERROR_CODES[UNKNOWN_ERROR_INDEX].description;
  snprintf(error_msg_buffer_, sizeof(error_msg_buffer_), "%s ('%c')", base_msg, error_code);
  ESP_LOGE(TAG, "Unknown error code: %c", error_code);
  return error_msg_buffer_;
}

void DaikinRotexSolarisComponent::parse_line_(const char *line, size_t len) {
  // ========================================================================
  // LINE VALIDATION - Check length is within acceptable range
  // ========================================================================
  #if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_INFO
  ESP_LOGI(TAG, "Parsing line: '%s' (len=%u)", line, len);
  #endif

  if (len < MIN_LINE_LEN || len > MAX_LINE_LEN) {
    // Boot/Info lines on startup - log and ignore these
    if (strncmp(line, BOOT_LINE1, 7) == 0 || strncmp(line, BOOT_LINE2, 6) == 0) {
      #if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_INFO
      ESP_LOGI(TAG, "Boot/info line detected, ignoring.");
      #endif
      return;
    }
    if (strncmp(line, BOOT_LINE3, 8) == 0) {
      #if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_INFO
      const char BOOT_LINE_UTF8[] = "HA;BK;P1 /%;P2;TK /°C;TR /°C;TS /°C;TV /°C;V /l/min;ERROR;P/W";
      ESP_LOGI(TAG, "Boot/info line detected, ignoring.");
      ESP_LOGI(TAG, "Solaris line UTF-8: '%s'", BOOT_LINE_UTF8);
      #endif
      return;
    }

    ESP_LOGE(TAG, "Parsing line: '%s'", line);
    ESP_LOGE(TAG, "Invalid line length (%u), expected %u-%u", 
      len, MIN_LINE_LEN, MAX_LINE_LEN);
    return;
  }

  // ========================================================================
  // INITIALIZE PARSING STATE - Prepare for tokenization
  // ========================================================================
  // Parse: "Ha;BK;P1;P2;TK;TR;TS;TV;DF;Err;P"
  int int_values[TOTAL_FIELDS] = {0};   // Array to store parsed integer values
  float solaris_df = 0.0f;              // Flow rate (stored separately as float)
  char error_code = '\0';               // Error code character
  uint8_t token_idx = 0;                // Current field index being parsed
  const char *token_start = line;       // Pointer to start of current token

  // ========================================================================
  // SINGLE-PASS TOKENIZATION - Split by semicolon delimiter
  // ========================================================================
  for (size_t i = 0; i <= len; i++) {
    bool is_delimiter = (i == len || line[i] == ';');  // Delimiter or end of string

    if (is_delimiter && token_idx < TOTAL_FIELDS) {
      size_t token_len = &line[i] - token_start;  // Calculate token length

      // ====================================================================
      // PARSE FLOW RATE (SOLARIS_DF) - Float value with comma decimal separator
      // ====================================================================
      if (token_idx == SOLARIS_DF) {
        if (token_len > 0 && token_len < CONVERSION_BUFFER_SIZE) {
          // Copy token to buffer and null-terminate
          strncpy(conversion_buffer_, token_start, token_len);
          conversion_buffer_[token_len] = '\0';

          // Replace comma with decimal point (Solaris uses comma as decimal separator)
          for (char *c = conversion_buffer_; *c; c++) {
            if (*c == ',') *c = '.';
          }

          char *endptr;
          solaris_df = strtof(conversion_buffer_, &endptr);
          if (endptr == conversion_buffer_) {
            // Conversion failed (no valid number found)
            ESP_LOGE(TAG, "Failed to parse solaris_df at token %u", token_idx);
            solaris_df = 0.0f;
          }

          #if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_DEBUG
          ESP_LOGD(TAG, "Token[%u] = %.1f", token_idx, solaris_df);
          #endif
        } else {
          solaris_df = 0.0f;
        }

      // ====================================================================
      // PARSE ERROR CODE (SOLARIS_ERR) - Single character value
      // ====================================================================
      } else if (token_idx == SOLARIS_ERR) {
        // Error code field: single character (empty, K, R, S, V, D, G, F, W)
        error_code = (token_len > 0) ? token_start[0] : '\0';

        if (error_code != '\0') {
          ESP_LOGE(TAG, "Token[%u] = '%c'", token_idx, error_code);
        }

        #if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_DEBUG
        if (error_code == '\0') {
          ESP_LOGD(TAG, "Token[%u] = '%c'", token_idx, error_code);
        }
        #endif

      // ====================================================================
      // PARSE INTEGER FIELDS - All other fields are integers
      // ====================================================================
      } else {
        if (token_len > 0) {
          // Copy token to buffer and null-terminate
          strncpy(conversion_buffer_, token_start, token_len);
          conversion_buffer_[token_len] = '\0';

          char *endptr;
          int_values[token_idx] = static_cast<int>(strtol(conversion_buffer_, &endptr, 10));
          if (endptr == conversion_buffer_) {
            // Conversion failed (no valid number found)
            ESP_LOGE(TAG, "Failed to parse token %u, using 0", token_idx);
            int_values[token_idx] = 0;
          }
        } else {
          int_values[token_idx] = 0;  // Empty token defaults to 0
        }

        #if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_DEBUG
        ESP_LOGD(TAG, "Token[%u] = %d", token_idx, int_values[token_idx]);
        #endif
      }

      // Move to the next token
      token_start = &line[i + 1];  // Move start pointer past delimiter
      token_idx++;                 // Increment field counter
    }
  }

  // ========================================================================
  // VALIDATE FIELD COUNT - Ensure all 11 fields were received
  // ========================================================================
  if (token_idx != 11) {
    ESP_LOGE(TAG, "Incomplete data: only %u tokens found, expected 11", token_idx);
    return;
  }

  // ========================================================================
  // PUBLISH PARSED DATA FOR ALL SENSORS
  // ========================================================================
  publish_values_(int_values, solaris_df, error_code, token_idx);

  #if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_DEBUG
  ESP_LOGD(TAG, "Parse complete: %u tokens processed successfully", token_idx);
  #endif
}

void DaikinRotexSolarisComponent::publish_values_(const int int_values[], 
  float solaris_df, char error_code, uint8_t num_tokens) {
  // ========================================================================
  // PUBLISH BINARY SENSORS - on/off states (true/false)
  // ========================================================================
  if (solaris_ha_sensor_) {
    // Manual operation flag (solaris_ha)
    solaris_ha_sensor_->publish_state(int_values[SOLARIS_HA] != 0);
  }

  if (solaris_bk_sensor_) {
    // Burner contact flag (solaris_bk)
    solaris_bk_sensor_->publish_state(int_values[SOLARIS_BK] != 0);
  }

  if (solaris_p2_sensor_) {
    // Booster pump flag (solaris_p2)
    solaris_p2_sensor_->publish_state(int_values[SOLARIS_P2] != 0);
  }

  // ========================================================================
  // PUBLISH NUMERIC SENSORS - Circulation pump speed (already a number)
  // ========================================================================
  if (solaris_p1_sensor_) {
    // Circulation pump speed (P1): 0-100 %
    solaris_p1_sensor_->publish_state(int_values[SOLARIS_P1]);
  }

  // ========================================================================
  // PUBLISH TEMPERATURE SENSORS - All in °C
  // ========================================================================
  if (solaris_tk_sensor_) {
    // Collector temperature (TK): -55 to +250 °C
    solaris_tk_sensor_->publish_state(int_values[SOLARIS_TK]);
  }

  if (solaris_tr_sensor_) {
    // Return temperature (TR): 0-100 °C
    solaris_tr_sensor_->publish_state(int_values[SOLARIS_TR]);
  }

  if (solaris_ts_sensor_) {
    // Storage temperature (TS): 0-100 °C
    solaris_ts_sensor_->publish_state(int_values[SOLARIS_TS]);
  }

  if (solaris_tv_sensor_) {
    // Flow temperature (TV): 0-100 °C
    solaris_tv_sensor_->publish_state(int_values[SOLARIS_TV]);
  }

  // ========================================================================
  // PUBLISH FLOW RATE SENSOR - Already in correct units (l/min)
  // ========================================================================
  if (solaris_df_sensor_) {
    // Flow rate (DF): 0.0-20.0 l/min
    solaris_df_sensor_->publish_state(solaris_df);
  }

  // ========================================================================
  // PUBLISH POWER SENSOR - Convert from Watts to kW with rounding
  // ========================================================================
  if (solaris_pwr_sensor_) {
    // Power (PWR): stored in Watts, convert to kW and round to 2 decimal places
    float solaris_pwr_kw = int_values[SOLARIS_PWR] / 1000.0f;
    solaris_pwr_kw = roundf(solaris_pwr_kw * 100.0f) / 100.0f;  // Round to 2 decimals: multiply, round, divide
    solaris_pwr_sensor_->publish_state(solaris_pwr_kw);
  }

  // ========================================================================
  // PUBLISH ERROR STATUS SENSOR - Lookup error description if applicable
  // ========================================================================
  if (solaris_err_sensor_) {
    const char *error_text = get_error_text_(error_code);
    solaris_err_sensor_->publish_state(error_text);
  }
}

} // namespace daikin_rotex_solaris
} // namespace esphome