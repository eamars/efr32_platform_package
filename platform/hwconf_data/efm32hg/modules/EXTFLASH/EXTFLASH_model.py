from . import halconfig_types as types
from . import halconfig_dependency as dep

name = "EXTFLASH"
displayname = "SPI Flash"
compatibility = dep.Dependency()  # = all
options = {
    "BSP_EXTFLASH_INTERNAL": {
        "type": "enum",
        "description": "SPI Flash type",
        "values": [types.EnumValue('0', 'External SPI flash'),
                   types.EnumValue('1', 'Internal SPI flash')],
    },
    "BSP_EXTFLASH_HOLD": {
        "type": types.Pin(),
        "description": "SPI Flash hold",
    },
    "BSP_EXTFLASH_USART": {
        "type": types.Peripheral(filter=["USART"],
                                 inherit_options=True,
                                 define_value_prefix='HAL_SPI_PORT_',
                                 mode="spi"),
        "description": "USART connected to SPI Flash",
    },
    "BSP_EXTFLASH_CS": {
        "type": types.Pin(disabled_label="Inherited from USART"),
        "description": "CS pin",
    },
    "BSP_EXTFLASH_WP": {
        "type": types.Pin(),
        "description": "SPI Flash write protect",
    },
}
