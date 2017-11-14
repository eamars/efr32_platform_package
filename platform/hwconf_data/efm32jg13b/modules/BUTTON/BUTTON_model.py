from . import halconfig_types as types
from . import halconfig_dependency as dep

name = "BUTTON"
displayname = "Button"
compatibility = dep.Dependency()  # = all
enable = {
    "define": "BSP_BUTTON_PRESENT",
    "description": "Buttons present on board",
}
options = {
    "BSP_BUTTON": {
        "type": types.PinArray(
            "BSP_BUTTON",
            min=0,
            max=8,
            default=2,
            item_description="Button %n"
        ),
        "description": "Number of buttons available on board",
        "allowedconflicts": ["BSP_LED", "BSP_BTL_BUTTON"]
    },
    "BSP_BUTTON_GPIO_DOUT": {
        "type": "enum",
        "description": "DOUT value of button pins. High/low for pullup/pulldown, high for filter on input only mode.",
        "values": [
            types.EnumValue("HAL_GPIO_DOUT_LOW", "Low"),
            types.EnumValue("HAL_GPIO_DOUT_HIGH", "High")
        ],
    },
    "BSP_BUTTON_GPIO_MODE": {
        "type": "enum",
        "description": "GPIO mode of button pins",
        "values": [
            types.EnumValue("HAL_GPIO_MODE_INPUT", "Input"),
            types.EnumValue("HAL_GPIO_MODE_INPUT_PULL", "Input with pullup/down"),
            types.EnumValue("HAL_GPIO_MODE_INPUT_PULL_FILTER", "Input with pullup/down and filter")
        ],
    },
    "HAL_BUTTON_COUNT": {
        "type": "uint8_t",
        "description": "Number of buttons to enable",
        "min": "0",
        "max": "255",
        "advanced": True,
    },
    "HAL_BUTTON_ENABLE": {
        "type": "array",
        "description": "Array of button indices to enable",
        "defaultValue": "0, 1",
        "advanced": True,
    },
}
