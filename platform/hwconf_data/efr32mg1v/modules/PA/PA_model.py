from . import halconfig_types as types
from . import halconfig_dependency as dep

name = "PA"
description = "Power Amplifier"
compatibility = dep.Dependency(platform=dep.Platform.SERIES1, mcu_type=dep.McuType.RADIO)  # EFR32
enable = {
    "define": "HAL_PA_ENABLE",
    "description": "Enable PA",
}
options = {
    "HAL_PA_CURVE_HEADER": {
        "type": "string",
        "description": "Custom PA curve header file",
        "defaultValue": '"pa_curves_efr32.h"',
    },
    "HAL_PA_POWER": {
        "type": "uint8_t",
        "description": "Raw power units",
        "min": "0",
        "max": "252",
        "defaultValue": "252",
    },
    "HAL_PA_RAMP": {
        "type": "uint16_t",
        "description": "Desired ramp time (us)",
        "min": "1",
        "max": "65535",
        "defaultValue": "10",
    },
    "HAL_PA_VOLTAGE": {
        "type": "uint16_t",
        "description": "PA voltage (mV)",
        "min": "0",
        "max": "3800",
        "defaultValue": "3300",
    },
    "HAL_PA_2P4_LOWPOWER": {
        "type": "boolean",
        "description": "Use low power 2.4GHz PA",
        "defaultValue": "False",
    },
}
