from . import halconfig_types as types
from . import halconfig_dependency as dep

name = "VUART"
displayname = "Virtual UART"
compatibility = dep.Dependency(mcu_type=dep.McuType.RADIO)  # all
category = " Serial"
enable = {
    "define": "HAL_SERIAL_VUART_ENABLE",
    "description": "Enable VUART/Semihosting",
}
options = {
    "HAL_VUART_TYPE": {
        "type": "enum",
        "values": [types.EnumValue('HAL_VUART_TYPE_NONE', 'No VUART'),
                   types.EnumValue('HAL_VUART_TYPE_SWO', 'VUART via SWO'),
                   types.EnumValue('HAL_VUART_TYPE_RTT', 'VUART via RTT')],
        "description": "VUART type",
    },
}
