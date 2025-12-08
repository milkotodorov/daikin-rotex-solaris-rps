"""
Multilingual sensor name and error message support for DAIKIN/ROTEX Solaris 
component

This module provides translations for sensor names and error codes in multiple
languages. Translations are used by __init__.py to generate the 
solaris_error_codes.h header file only with the selected language and to set
sensor names translations upon initialization.
"""

from .de import SENSOR_NAMES_DE, ERROR_CODES_DE
from .en import SENSOR_NAMES_EN, ERROR_CODES_EN
from .fr import SENSOR_NAMES_FR, ERROR_CODES_FR
from .it import SENSOR_NAMES_IT, ERROR_CODES_IT
from .es import SENSOR_NAMES_ES, ERROR_CODES_ES

DEFAULT_LANGUAGE = "de" # Default language and fallback

# ============================================================================
# SENSOR NAMES TRANSLATIONS - Used to label sensors in the UI
# ============================================================================
SENSORS_TRANSLATIONS = {
    "de": SENSOR_NAMES_DE,
    "en": SENSOR_NAMES_EN,
    "fr": SENSOR_NAMES_FR,
    "it": SENSOR_NAMES_IT,
    "es": SENSOR_NAMES_ES,
}

# ============================================================================
# ERROR CODES TRANSLATIONS - Error codes to description mapping
# ============================================================================
# Note: "unknown" entry is used as fallback for any error code not listed - should not happen
ERROR_CODES_TRANSLATIONS = {
    "de": ERROR_CODES_DE,
    "en": ERROR_CODES_EN,
    "fr": ERROR_CODES_FR,
    "it": ERROR_CODES_IT,
    "es": ERROR_CODES_ES,
}

def get_sensor_name(key, lang=DEFAULT_LANGUAGE):
    """Get sensor name for a given key and language.
    
    Args:
        key: Sensor configuration key (e.g., "solaris_p1")
        lang: Language code (e.g., en, de, fr, it, es). Defaults to DEFAULT_LANGUAGE.
    
    Returns:
        Sensor name for given language or fallback to DEFAULT_LANGUAGE 
        if language not supported
    """

    if lang in SENSORS_TRANSLATIONS and key in SENSORS_TRANSLATIONS[lang]:
        return SENSORS_TRANSLATIONS[lang][key]
    
    return SENSORS_TRANSLATIONS.get(DEFAULT_LANGUAGE, {}).get(key, key)


def get_codes_description(lang=DEFAULT_LANGUAGE):
    """Get error codes descriptions for a specific lang.
    
    Args:
        lang: Language code (e.g., en, de, fr, it, es). Defaults to DEFAULT_LANGUAGE.
    
    Returns:
        Dictionary mapping error code to description for the selected language.
    """
    
    if lang not in ERROR_CODES_TRANSLATIONS:
        lang = DEFAULT_LANGUAGE
    
    return ERROR_CODES_TRANSLATIONS[lang]


def translation_exists(lang):
    """Check if a lang exists in the translation dictionaries.
    
    Args:
        lang: Language code (e.g., en, de, fr, it, es).
    
    Returns:
        bool: True if language exists in both SENSORS_TRANSLATIONS **AND**
              ERROR_CODES_TRANSLATIONS, False otherwise
    """
    return lang in SENSORS_TRANSLATIONS and lang in ERROR_CODES_TRANSLATIONS