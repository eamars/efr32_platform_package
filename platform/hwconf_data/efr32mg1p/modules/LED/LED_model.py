from . import halconfig_types as types
from . import halconfig_dependency as dep

name = "LED"
compatibility = dep.Dependency()  # = all
studio_module = {
    "basename" : "SDK.HAL.LED",
    "modules" : [types.StudioFrameworkModule("BLE", types.Framework.BLE)],
    }
enable = {
    "define": "BSP_LED_PRESENT",
    "description": "LEDs present on board",
}
options = {
    "BSP_LED": {
        "type": types.PinArray(
            "BSP_LED",
            min=0,
            max=8,
            default=2,
            item_description="LED %n"
        ),
        "description": "Number of LEDs available on board",
        "allowedconflicts": ["BSP_BUTTON", "BSP_BTL_BUTTON"]
    },
    "HAL_LED_COUNT": {
        "type": "uint8_t",
        "description": "Number of LEDs to initialize",
        "min": "0",
        "max": "8",
        "advanced": True,
    },
    "HAL_LED_ENABLE": {
        "type": "array",
        "description": "Array of LED indices to initialize",
        "defaultValue": "0, 1",
        "advanced": True,
    },
}
