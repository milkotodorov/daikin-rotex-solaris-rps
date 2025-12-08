"""
ESPHome integration for DAIKIN/ROTEX Solaris RPS3/RPS4 solar controller.

This module:
1. Configures sensors schema for numeric, binary and text sensors
2. Generates solaris_error_codes.h header with translations for the specified
language in the YAML configuration
3. Registers the component and sets up all sensors entities
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID
from pathlib import Path

from .sensors import SENSORS_SCHEMA, setup_sensors
from .translations.translations import DEFAULT_LANGUAGE, get_codes_description

# Configuration key in YAML for language selection
CONF_LANGUAGE = "language"

# ============================================================================
# COMPONENT METADATA
# ============================================================================
# Dependencies on other ESPHome components
DEPENDENCIES = ["uart"]

# Components that will be auto-loaded by ESPHome when this component is used
AUTO_LOAD = ["sensor", "text_sensor", "binary_sensor"]

# ============================================================================
# C++ NAMESPACE AND CLASS REGISTRATION
# ============================================================================
# Create C++ namespace for this component
daikin_rotex_solaris_ns = cg.esphome_ns.namespace("daikin_rotex_solaris")

# Register C++ class DaikinRotexSolarisComponent with ESPHome code generator
DaikinRotexSolarisComponent = daikin_rotex_solaris_ns.class_(
    "DaikinRotexSolarisComponent", cg.Component, uart.UARTDevice
)

# ============================================================================
# CONFIGURATION SCHEMA - defines valid YAML configuration structure
# ============================================================================
CONFIG_SCHEMA = (
    cv.Schema({
        # Component ID for internal reference
        cv.GenerateID(): cv.declare_id(DaikinRotexSolarisComponent),
        # Language selection for sensor names and error messages (default: DEFAULT_LANGUAGE)
        cv.Optional(CONF_LANGUAGE, default=DEFAULT_LANGUAGE): cv.string,
    })
    # Include sensors schema
    .extend(SENSORS_SCHEMA)
    # Required ESPHome component configuration
    .extend(cv.COMPONENT_SCHEMA)
    # Required for UART communication
    .extend(uart.UART_DEVICE_SCHEMA)
)


def _cpp_escape(s):
    r"""Escape a Python string for safe inclusion in a C++ string literal.
    
    This function escapes special characters so the string can be safely
    embedded in C++ code as a string literal:
        - Backslashes are doubled (\ becomes \\)
        - Quotes are escaped (" becomes \")
    """
    # Escape backslashes first, then quotes (order matters!)
    return s.replace("\\", "\\\\").replace('"', '\\"')


def generate_error_codes_header(lang):
    """Generate solaris_error_codes.h header from translations.py
    
    This function creates a C++ header file containing error codes definitions
    with descriptions in the selected language. The header is generated at
    build time.
    
    Args:
        language: Language code (e.g., de, en, fr, it, es). DEFAULT_LANGUAGE 
        will be used if the specified language is not available.
    
    Returns:
        String containing complete C++ header file content
    """
    # Get translations for selected language
    translations = get_codes_description(lang)

    # ========================================================================
    # BUILD C++ HEADER CONTENT
    # ========================================================================
    header_lines = [
        "// AUTO-GENERATED FILE: Do not edit manually!",
        "// Generated from translations.py by __init__.py during build",
        "// Language: " + lang.upper(),
        "",
        "namespace esphome {",
        "namespace daikin_rotex_solaris {",
        "",
        "// Struct to hold error code and description pairs",
        "struct SolarisErrorCodes {",
        "  char code;               // Single character error code (K, R, S, V, D, G, F, W, or \\0)",
        "  const char *description; // Pointer to error description string in the selected language",
        "};",
        "",
        "// Error codes only for the selected language",
        "static const SolarisErrorCodes ERROR_CODES[] = {",
    ]

    # ========================================================================
    # ADD ERROR CODE ENTRIES TO ARRAY
    # ========================================================================
    for key, description in translations.items():
        # Format the error code character for C++
        if key == "unknown":
            # "unknown" is a special fallback entry (no character match)
            code_char = "'\\x00'"
        elif key == "":
            # Empty string maps to null character (no error condition)
            code_char = "'\\0'"
        else:
            # Single-character codes: K, R, S, D, V, G, F, W
            code_char = "'" + key + "'"

        # Escape the description for safe inclusion in C++ string literal
        escaped_desc = _cpp_escape(str(description))
        
        # Add struct initializer to array
        line = "  {{{}, \"{}\"}},".format(code_char, escaped_desc)
        header_lines.append(line)

    # ========================================================================
    # CLOSE ARRAY AND ADD UTILITY CONSTANTS
    # ========================================================================
    header_lines.extend([
        "};",
        "",
        "// Calculate array size at compile time",
        "static constexpr size_t ERROR_CODES_COUNT = sizeof(ERROR_CODES) / sizeof(ERROR_CODES[0]);",
        "// Index of the \"unknown\" fallback entry (always the last one)",
        "static constexpr size_t UNKNOWN_ERROR_INDEX = ERROR_CODES_COUNT - 1;",
        "",
        "} // namespace daikin_rotex_solaris",
        "} // namespace esphome"
    ])

    return "\n".join(header_lines)


async def to_code(config):
    """Main async function called by ESPHome to generate C++ code for this component.
    
    This function:
    1. Creates the C++ component instance
    2. Registers it with ESPHome's component registry
    3. Generates the error codes header for the selected language
    4. Sets up all sensor entities (numeric, binary, text)
    """

    # Create new C++ component instance
    var = cg.new_Pvariable(config[CONF_ID])

    # Register component with ESPHome's component system
    await cg.register_component(var, config)

    # Register as UART device (connects to uart_id and sets up communication)
    await uart.register_uart_device(var, config)    

    # Generate C++ header content with error codes in the selected language
    language = config[CONF_LANGUAGE]
    header_text = generate_error_codes_header(language)

    # Write Generated header file
    component_dir = Path(__file__).parent
    header_path = component_dir / "solaris_error_codes.h"
    with open(header_path, "w", encoding="utf-8") as f:
        f.write(header_text)

    # Initialize all sensors
    await setup_sensors(var, config)