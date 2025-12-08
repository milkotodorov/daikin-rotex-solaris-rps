"""
Configuration schema and setup for Solaris sensors component.

Handles configuration, validation, and instantiation of sensors entities
like temperature, flow rate, power output sensors, etc.
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, binary_sensor, text_sensor
from esphome.const import CONF_NAME

from .translations.translations import DEFAULT_LANGUAGE, translation_exists
from .sensors_config import SENSORS_CONFIG

def _generate_sensors_schema():
    """Generate sensors schema from configuration array in sensors_config.py"""
    schema_dict = {}

    for sensor_cfg in SENSORS_CONFIG:
        sensor_type = sensor_cfg['type']

        # Build schema based on sensor type
        match sensor_type:
            case 'numeric':
                schema_kwargs = {
                    'unit_of_measurement': sensor_cfg['unit'],
                    'icon': sensor_cfg['icon'],
                    'accuracy_decimals': sensor_cfg['accuracy'],
                }
                if sensor_cfg.get('device_class'):
                    schema_kwargs['device_class'] = sensor_cfg['device_class']
                if sensor_cfg.get('state_class'):
                    schema_kwargs['state_class'] = sensor_cfg['state_class']
                sensor_schema = sensor.sensor_schema(**schema_kwargs)

            case 'binary':
                schema_kwargs = {'icon': sensor_cfg['icon']}
                if sensor_cfg.get('device_class'):
                    schema_kwargs['device_class'] = sensor_cfg['device_class']
                sensor_schema = binary_sensor.binary_sensor_schema(**schema_kwargs)

            case 'text':
                sensor_schema = text_sensor.text_sensor_schema(icon=sensor_cfg['icon'])

        # Add name override support initially with DEFAULT_LANGUAGE
        display_name = sensor_cfg['display_name'](DEFAULT_LANGUAGE)
        schema_dict[cv.Optional(sensor_cfg['key'])] = sensor_schema.extend({
            cv.Optional(CONF_NAME, default=display_name): cv.string,
        })

    return cv.Schema(schema_dict)

# Generated sensors schema from sensors configuration array for CONFIG_SCHEMA 
# initialization in __init__.py - we still do not have access to the selected
# language at this point, so default language names are used here
SENSORS_SCHEMA = _generate_sensors_schema()

async def setup_sensors(parent, config):
    """Setup sensor entities from configuration.
    
    This function creates sensor instances for each sensor and registers it with 
    the component. Sensor names are translated based on the configured language.
    
    Args:
        parent: The DaikinRotexSolarisComponent instance to register sensors with
        config: The parsed YAML configuration dictionary
    """
    
    # Mapping of sensor types to their ESPHome creation functions
    SENSOR_TYPE_HANDLERS = {
        'numeric': sensor.new_sensor,
        'binary': binary_sensor.new_binary_sensor,
        'text': text_sensor.new_text_sensor,
    }

    # Get selected language from configuration
    lang = config.get('language', DEFAULT_LANGUAGE)
    # Fallback to DEFAULT_LANGUAGE if translation missing
    if not translation_exists(lang):
        lang = DEFAULT_LANGUAGE
    
    # Iterate over all sensor configurations
    for sensor_cfg in SENSORS_CONFIG:
        if sensor_cfg['key'] in config:
            # Get sensor config from YAML
            sensor_config = config[sensor_cfg['key']]
            
            # Update sensor name with translated display name
            sensor_config[CONF_NAME] = sensor_cfg['display_name'](lang)

            # Get the appropriate sensor creation function based on type
            sensor_creator = SENSOR_TYPE_HANDLERS[sensor_cfg['type']]
            
            # Create sensor instance using the appropriate ESPHome function
            sens = await sensor_creator(sensor_config)
            
            # Register sensor with the component using the appropriate setter
            cg.add(getattr(parent, sensor_cfg['setter'])(sens))