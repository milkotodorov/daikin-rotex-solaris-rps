"""
Sensors configuration definitions for DAIKIN/ROTEX Solaris RPS component.

This module contains the SENSORS_CONFIG array that defines all available
sensors, their types, units, icons, and other metadata. 'display_name' is
a lambda function that retrieves the localized name based on the provided
language code.
"""

from esphome.const import (
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_HEAT,
    DEVICE_CLASS_RUNNING,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_KILOWATT,
    UNIT_PERCENT,
)

from .translations.translations import get_sensor_name

# Flow rate unit (not in ESPHome standard units)
UNIT_LITERS_PER_MIN = "l/min"

# ============================================================================
# SENSORS CONFIGURATION ARRAY
# ============================================================================
SENSORS_CONFIG = [
    # ================
    # Numeric Sensors
    # ================
    # Circulation Pump Speed (0-100%)
    {
        'type': 'numeric',
        'key': 'solaris_p1',
        'display_name': lambda lang: get_sensor_name('solaris_p1', lang),
        'setter': 'set_solaris_p1_sensor',
        'unit': UNIT_PERCENT,
        'icon': 'mdi:pump',
        'device_class': None,
        'state_class': None,
        'accuracy': 0,
    },
    # Collector Temperature (-55 to +250 °C)
    {
        'type': 'numeric',
        'key': 'solaris_tk',
        'display_name': lambda lang: get_sensor_name('solaris_tk', lang),
        'setter': 'set_solaris_tk_sensor',
        'unit': UNIT_CELSIUS,
        'icon': 'mdi:sun-thermometer',
        'device_class': DEVICE_CLASS_TEMPERATURE,
        'state_class': STATE_CLASS_MEASUREMENT,
        'accuracy': 0,
    },
    # Return Temperature (0-100 °C)
    {
        'type': 'numeric',
        'key': 'solaris_tr',
        'display_name': lambda lang: get_sensor_name('solaris_tr', lang),
        'setter': 'set_solaris_tr_sensor',
        'unit': UNIT_CELSIUS,
        'icon': 'mdi:water-thermometer',
        'device_class': DEVICE_CLASS_TEMPERATURE,
        'state_class': STATE_CLASS_MEASUREMENT,
        'accuracy': 0,
    },
    # Storage Temperature (0-100 °C)
    {
        'type': 'numeric',
        'key': 'solaris_ts',
        'display_name': lambda lang: get_sensor_name('solaris_ts', lang),
        'setter': 'set_solaris_ts_sensor',
        'unit': UNIT_CELSIUS,
        'icon': 'mdi:water-thermometer',
        'device_class': DEVICE_CLASS_TEMPERATURE,
        'state_class': STATE_CLASS_MEASUREMENT,
        'accuracy': 0,
    },
    # Flow Temperature (0-100 °C)
    {
        'type': 'numeric',
        'key': 'solaris_tv',
        'display_name': lambda lang: get_sensor_name('solaris_tv', lang),
        'setter': 'set_solaris_tv_sensor',
        'unit': UNIT_CELSIUS,
        'icon': 'mdi:water-thermometer',
        'device_class': DEVICE_CLASS_TEMPERATURE,
        'state_class': STATE_CLASS_MEASUREMENT,
        'accuracy': 0,
    },
    # Flow Rate (0-20 l/min, displayed with 1 decimal place)
    {
        'type': 'numeric',
        'key': 'solaris_df',
        'display_name': lambda lang: get_sensor_name('solaris_df', lang),
        'setter': 'set_solaris_df_sensor',
        'unit': UNIT_LITERS_PER_MIN,
        'icon': 'mdi:waves-arrow-right',
        'device_class': None,
        'state_class': STATE_CLASS_MEASUREMENT,
        'accuracy': 1,
    },
    # Power Output (0-X kW, displayed with 2 decimal places)
    {
        'type': 'numeric',
        'key': 'solaris_pwr',
        'display_name': lambda lang: get_sensor_name('solaris_pwr', lang),
        'setter': 'set_solaris_pwr_sensor',
        'unit': UNIT_KILOWATT,
        'icon': 'mdi:solar-power',
        'device_class': DEVICE_CLASS_POWER,
        'state_class': STATE_CLASS_MEASUREMENT,
        'accuracy': 2,
    },
    # ===============
    # Binary Sensors
    # ===============
    # Manual operation mode
    {
        'type': 'binary',
        'key': 'solaris_ha',
        'display_name': lambda lang: get_sensor_name('solaris_ha', lang),
        'setter': 'set_solaris_ha_sensor',
        'icon': 'mdi:gesture-tap',
        'device_class': None,
    },
    # Burner contact
    {
        'type': 'binary',
        'key': 'solaris_bk',
        'display_name': lambda lang: get_sensor_name('solaris_bk', lang),
        'setter': 'set_solaris_bk_sensor',
        'icon': 'mdi:electric-switch',
        'device_class': DEVICE_CLASS_HEAT,
    },
    # Booster pump
    {
        'type': 'binary',
        'key': 'solaris_p2',
        'display_name': lambda lang: get_sensor_name('solaris_p2', lang),
        'setter': 'set_solaris_p2_sensor',
        'icon': 'mdi:pump',
        'device_class': DEVICE_CLASS_RUNNING,
    },
    # =============
    # Text Sensors
    # =============
    # Error status (single character code)
    {
        'type': 'text',
        'key': 'solaris_err',
        'display_name': lambda lang: get_sensor_name('solaris_err', lang),
        'setter': 'set_solaris_err_sensor',
        'icon': 'mdi:alert-decagram-outline',
    },
]