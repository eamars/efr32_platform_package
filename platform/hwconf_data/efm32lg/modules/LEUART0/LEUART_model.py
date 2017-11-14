from . import halconfig_types as types
from . import halconfig_dependency as dep

name = "LEUART"
description = "Low-Energy UART"
compatibility = dep.Dependency()  # all
peripheral = 'LEUART'
enable = {
    "define": "HAL_LEUART_ENABLE",
    "description": "Enable LEUART",
}
options = {
    "HAL_LEUART_BAUD_RATE": {
        "type": "uint32_t",
        "description": "LEUART baud rate (must be less than refFreq/oversampling)",
        "min": "1",
        "max": "10000000",
        "defaultValue": "9600",
    },
    "HAL_LEUART_FLOW_CONTROL": {
        "type": "enum",
        "description": "LEUART flow control mode (LEUART)",
        "values": [types.EnumValue('HAL_LEUART_FLOW_CONTROL_NONE', 'No FC'),
                   types.EnumValue('HAL_LEUART_FLOW_CONTROL_SW', 'Xon-Xoff'),
                   types.EnumValue('HAL_LEUART_FLOW_CONTROL_HW', 'GPIO-based CTS/RTS')],
    },
    "HAL_LEUART_RX_QUEUE_SIZE": {
        "type": "uint16_t",
        "description": "LEUART RX buffer size (must be a multiple of HAL_LEUART_RXSTOP)",
        "min": "0",
        "max": "65535",
        "advanced": True,
    },
    "HAL_LEUART_RXSTART": {
        "type": "uint16_t",
        "description": "LEUART release flow control threshold (must be a multiple of HAL_LEUART_RXSTOP)",
        "min": "0",
        "max": "65535",
        "advanced": True,
    },
    "HAL_LEUART_RXSTOP": {
        "type": "uint16_t",
        "description": "LEUART assert flow control threshold",
        "min": "0",
        "max": "65535",
        "advanced": True,
    },
    "HAL_LEUART_TX_QUEUE_SIZE": {
        "type": "uint16_t",
        "description": "LEUART TX buffer size",
        "min": "0",
        "max": "65535",
        "advanced": True,
    },
    "BSP_LEUART_RX": {
        "type": types.Pin(signal="RX"),
        "description": "LEUART RX",
    },
    "BSP_LEUART_TX": {
        "type": types.Pin(signal="TX"),
        "description": "LEUART TX",
    },
}
