from . import halconfig_types as types
from . import halconfig_dependency as dep

name = "EZRADIOPRO"
displayname = "EZRadio Pro"
description = "EZRadio Pro external transceiver configuration"
compatibility = dep.Dependency(mcu_type=dep.McuType.RADIO)  # = all
category = " Radio"
studio_module = {
    "basename": "SDK.HAL.EZRADIOPRO",
    "modules": [types.StudioFrameworkModule("BASE", [types.Framework.ZNET, types.Framework.THREAD, types.Framework.CONNECT])],
    }
enable = {
    "define": "HAL_EZRADIOPRO_ENABLE",
    "description": "Enable EZRadio Pro",
}
options = {
    "BSP_EZRADIOPRO_USART": {
        "type": types.Peripheral(filter=["USART"],
                                 inherit_options=True,
                                 mode="spi",
                                 define_value_prefix="HAL_SPI_PORT_"),
        "description": "USART connected to the EZRadio Pro",
    },
    "BSP_EZRADIOPRO_CS": {
        "type": types.Pin(disabled_label="Inherited from USART"),
        "description": "nSEL/CS pin",
    },
    "BSP_EZRADIOPRO_INT": {
        "type": types.Pin(),
        "description": "EZRadio Pro Interrupt pin",
    },
    "BSP_EZRADIOPRO_SDN": {
        "type": types.Pin(),
        "description": "EZRadio Pro Shutdown pin",
    },
    "HAL_EZRADIOPRO_SHUTDOWN_SLEEP": {
        "type": "boolean",
        "description": "Shutdown EZRadio Pro when sleeping",
    },
}
