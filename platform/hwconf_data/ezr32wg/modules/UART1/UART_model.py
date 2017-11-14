from . import halconfig_types as types
from . import halconfig_dependency as dep

name = "UART"
description = "UART"
compatibility = dep.Dependency()  # all
peripheral = 'UART'
enable = {
    "define": "HAL_UART_ENABLE",
    "description": "Enable UART",
}
options = {
    "HAL_UART_BAUD_RATE": {
        "type": "uint32_t",
        "description": "UART baud rate (must be less than refFreq/oversampling)",
        "min": "1",
        "max": "4294967295",
        "defaultValue": "115200",
    },
    "HAL_UART_FLOW_CONTROL": {
        "type": "enum",
        "description": "UART flow control mode",
        "values": [types.EnumValue('HAL_UART_FLOW_CONTROL_NONE', 'No FC'),
                   types.EnumValue('HAL_UART_FLOW_CONTROL_SW', 'Xon-Xoff'),
                   types.EnumValue('HAL_UART_FLOW_CONTROL_HW', 'GPIO-based CTS/RTS'),
                   types.EnumValue('HAL_UART_FLOW_CONTROL_HWUART', 'USART-based CTS/RTS',
                                   dependency=dep.Dependency(platform=dep.Platform.SERIES1))],
    },
    "HAL_UART_RX_QUEUE_SIZE": {
        "type": "uint16_t",
        "description": "UART RX buffer size (must be a multiple of HAL_UART_RXSTOP)",
        "min": "0",
        "max": "65535",
        "advanced": True,
    },
    "HAL_UART_RXSTART": {
        "type": "uint16_t",
        "description": "UART release flow control threshold (must be a multiple of HAL_UART_RXSTOP)",
        "min": "0",
        "max": "65535",
        "advanced": True,
    },
    "HAL_UART_RXSTOP": {
        "type": "uint16_t",
        "description": "UART assert flow control threshold",
        "min": "0",
        "max": "65535",
        "advanced": True,
    },
    "HAL_UART_TX_QUEUE_SIZE": {
        "type": "uint16_t",
        "description": "UART TX buffer size",
        "min": "0",
        "max": "65535",
        "advanced": True,
    },
    "BSP_UART_RX": {
        "type": types.Pin(signal="RX"),
        "description": "UART RX",
    },
    "BSP_UART_TX": {
        "type": types.Pin(signal="TX"),
        "description": "UART TX",
    },
}
