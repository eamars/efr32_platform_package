from . import halconfig_types as types
from . import halconfig_dependency as dep

name = "DCDC"
description = "DC-to-DC Converter"
compatibility = dep.Dependency(platform=dep.Platform.SERIES1)  # 90nm
enable = {
    "define": "BSP_DCDC_PRESENT",
    "description": "Power circuit configured for DCDC",
}
options = {
    "BSP_DCDC_INIT": {
        "type": "string",
        "description": "DCDC initialization options struct",
        "defaultValue": "EMU_DCDCINIT_DEFAULT",
    },
    "HAL_DCDC_BYPASS": {
        "type": "boolean",
        "description": "Override DCDC mode to bypass mode (when board is configured for DCDC)",
    },
}
