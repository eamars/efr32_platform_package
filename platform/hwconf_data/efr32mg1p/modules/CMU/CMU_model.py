from . import halconfig_types as types
from . import halconfig_dependency as dep

name = "CMU"
description = "Clock Management Unit. Configure clock sources for the device."
compatibility = dep.Dependency()  # all
peripheral = 'CMU'
options = {
    "HAL_CLK_HFCLK_SOURCE": {
        "type": "enum",
        "description": "HF clock source",
        "values": [types.EnumValue('HAL_CLK_HFCLK_SOURCE_HFRCO', 'HFRCO (High frequency RC oscillator)', compatibility),
                   types.EnumValue('HAL_CLK_HFCLK_SOURCE_HFXO', 'HFXO (High frequency crystal oscillator)', compatibility)],
        "subcategory": "Clock Sources",
    },
    "HAL_CLK_LFACLK_SOURCE": {
        "type": "enum",
        "description": "LFA clock source",
        "values": [types.EnumValue('HAL_CLK_LFCLK_SOURCE_DISABLED', 'Disabled', compatibility),
                   types.EnumValue('HAL_CLK_LFCLK_SOURCE_LFRCO', 'LFRCO (Low frequency RC oscillator)', compatibility),
                   types.EnumValue('HAL_CLK_LFCLK_SOURCE_LFXO', 'LFXO (Low frequency crystal oscillator)', compatibility),
                   types.EnumValue('HAL_CLK_LFCLK_SOURCE_ULFRCO', 'ULFRCO (Ultra low frequency crystal oscillator)', compatibility),
                   types.EnumValue('HAL_CLK_LFCLK_SOURCE_HFLE', 'HFLE (Low energy high frequency clock)', dep.Dependency(platform=dep.Platform.SERIES0))],
        "subcategory": "Clock Sources",
    },
    "HAL_CLK_LFBCLK_SOURCE": {
        "type": "enum",
        "description": "LFB clock source",
        "values": [types.EnumValue('HAL_CLK_LFCLK_SOURCE_DISABLED', 'Disabled', compatibility),
                   types.EnumValue('HAL_CLK_LFCLK_SOURCE_LFRCO', 'LFRCO (Low frequency RC oscillator)', compatibility),
                   types.EnumValue('HAL_CLK_LFCLK_SOURCE_LFXO', 'LFXO (Low frequency crystal oscillator)', compatibility),
                   types.EnumValue('HAL_CLK_LFCLK_SOURCE_ULFRCO', 'ULFRCO (Ultra low frequency crystal oscillator)', compatibility),
                   types.EnumValue('HAL_CLK_LFCLK_SOURCE_HFLE', 'HFLE (Low energy high frequency clock)', compatibility)],
        "subcategory": "Clock Sources",
    },
    "HAL_CLK_LFCCLK_SOURCE": {
        "type": "enum",
        "description": "LFC clock source",
        "values": [types.EnumValue('HAL_CLK_LFCLK_SOURCE_DISABLED', 'Disabled', compatibility),
                   types.EnumValue('HAL_CLK_LFCLK_SOURCE_LFRCO', 'LFRCO (Low frequency RC oscillator)', compatibility),
                   types.EnumValue('HAL_CLK_LFCLK_SOURCE_LFXO', 'LFXO (Low frequency crystal oscillator)', compatibility)],
        "subcategory": "Clock Sources",
        "dependency": dep.Dependency(sdid=[77,100]),
    },
    "HAL_CLK_LFECLK_SOURCE": {
        "type": "enum",
        "description": "LFE clock source",
        "values": [types.EnumValue('HAL_CLK_LFCLK_SOURCE_DISABLED', 'Disabled', compatibility),
                   types.EnumValue('HAL_CLK_LFCLK_SOURCE_LFRCO', 'LFRCO (Low frequency RC oscillator)', compatibility),
                   types.EnumValue('HAL_CLK_LFCLK_SOURCE_LFXO', 'LFXO (Low frequency crystal oscillator)', compatibility),
                   types.EnumValue('HAL_CLK_LFCLK_SOURCE_ULFRCO', 'ULFRCO (Ultra low frequency crystal oscillator)', compatibility)],
        "subcategory": "Clock Sources",
        "dependency": dep.Dependency(platform=dep.Platform.SERIES1),
    },
    "BSP_CLK_HFXO_PRESENT": {
        "type": "boolean",
        "description": "HFXO present on board",
        "subcategory": "HFXO",
    },
    "BSP_CLK_HFXO_FREQ": [
        {
            "type": "uint32_t",
            "description": "HFXO frequency",
            "min": "0",
            "max": "40000000",
            "defaultValue": "38400000",
            "subcategory": "HFXO",
            "dependency": dep.Dependency(sdid=[80,84,89,95])
        },
        {
            "type": "uint32_t",
            "description": "HFXO frequency",
            "min": "0",
            "max": "50000000",
            "defaultValue": "50000000",
            "subcategory": "HFXO",
            "dependency": dep.Dependency(sdid=[100,106])
        },
        {
            "type": "uint32_t",
            "description": "HFXO frequency",
            "min": "0",
            "max": "32000000",
            "defaultValue": "32000000",
            "subcategory": "HFXO",
            "dependency": dep.Dependency(sdid=[71,73])
        },
        {
            "type": "uint32_t",
            "description": "HFXO frequency",
            "min": "0",
            "max": "48000000",
            "defaultValue": "48000000",
            "subcategory": "HFXO",
            "dependency": dep.Dependency(sdid=[72,74,103])
        },
        {
            "type": "uint32_t",
            "description": "HFXO frequency",
            "min": "0",
            "max": "24000000",
            "defaultValue": "24000000",
            "subcategory": "HFXO",
            "dependency": dep.Dependency(sdid=[76,77])
        },
        # Documentation option
        {
            "type": "uint32_t",
            "description": "HFXO frequency (See datasheet for maximum)",
            "subcategory": "HFXO",
            "documentation": "True",
        },
    ],
    "BSP_CLK_HFXO_INIT": {
        "type": "string",
        "description": "HFXO initialization settings struct",
        "defaultValue": "CMU_HFXOINIT_DEFAULT",
        "subcategory": "HFXO",
    },
    "HAL_CLK_HFXO_AUTOSTART": {
        "type": "enum",
        "description": "Start HFXO automatically on EM0/1 entry",
        "values": [
            types.EnumValue('HAL_CLK_HFXO_AUTOSTART_NONE', 'Do not start automatically'),
            types.EnumValue('HAL_CLK_HFXO_AUTOSTART_START', 'Start automatically'),
            types.EnumValue('HAL_CLK_HFXO_AUTOSTART_SELECT', 'Start automatically and select as HFCLK'),
        ],
        "dependency": dep.Dependency(platform=dep.Platform.SERIES1),
        "subcategory": "HFXO",
    },
    "BSP_CLK_HFXO_CTUNE": {
        "type": "uint16_t",
        "description": "HFXO CTUNE value",
        "min": "-1",
        "max": "511",
        "defaultValue": "-1",
        "dependency": dep.Dependency(platform=dep.Platform.SERIES1),
        "subcategory": "HFXO",
    },
    "BSP_CLK_HFXO_CTUNE_TOKEN": {
        "type": "boolean",
        "description": "Calibrate HFXO CTUNE from mfg token",
        "dependency": dep.Dependency(platform=dep.Platform.SERIES1, mcu_type=dep.McuType.RADIO),
        "subcategory": "HFXO",
    },
    "BSP_CLK_HFXO_BOOST": {
        "type": "enum",
        "description": "HFXO Boost",
        "values": [
            types.EnumValue('_CMU_CTRL_HFXOBOOST_50PCENT', '50%'),
            types.EnumValue('_CMU_CTRL_HFXOBOOST_70PCENT', '70%'),
            types.EnumValue('_CMU_CTRL_HFXOBOOST_80PCENT', '80%'),
            types.EnumValue('_CMU_CTRL_HFXOBOOST_100PCENT', '100%'),
        ],
        "defaultValue": "100%",
        "dependency": dep.Dependency(platform=dep.Platform.SERIES0),
        "subcategory": "HFXO",
    },

    "BSP_CLK_LFXO_PRESENT": {
        "type": "boolean",
        "description": "LFXO present on board",
        "subcategory": "LFXO",
    },
    "BSP_CLK_LFXO_INIT": {
        "type": "string",
        "description": "LFXO initialization settings struct",
        "defaultValue": "CMU_LFXOINIT_DEFAULT",
        "subcategory": "LFXO",
    },
    "BSP_CLK_LFXO_FREQ": {
        "type": "uint32_t",
        "description": "LFXO frequency",
        "min": "0",
        "max": "32768",
        "defaultValue": "32768",
        "subcategory": "LFXO",
    },
    "BSP_CLK_LFXO_CTUNE": {
        "type": "uint16_t",
        "description": "LFXO CTUNE value",
        "min": "0",
        "max": "511",
        "defaultValue": "0",
        "subcategory": "LFXO",
        "dependency": dep.Dependency(platform=dep.Platform.SERIES1),
    },
    "BSP_CLK_LFXO_BOOST": {
        "type": "enum",
        "description": "LFXO Boost",
        "values": [
            types.EnumValue('0', '70%'),
            types.EnumValue('2', '100%'),
            types.EnumValue('1', '70% (reduced)', dependency=dep.Dependency(sdid=[72])),
            types.EnumValue('3', '100% (reduced)', dependency=dep.Dependency(sdid=[72])),
        ],
        "dependency": dep.Dependency(platform=dep.Platform.SERIES0),
        "subcategory": "LFXO",
    },
}
