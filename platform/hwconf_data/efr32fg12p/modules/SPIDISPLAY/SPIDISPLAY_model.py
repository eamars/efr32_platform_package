from . import halconfig_types as types
from . import halconfig_dependency as dep

name = "SPIDISPLAY"
displayname = "SPI Display"
compatibility = dep.Dependency(not_sdid=[71])  # = all
studio_module = {
    "basename": "SDK.HAL.SPIDISPLAY",
    "modules": [types.StudioFrameworkModule("COMMON", [types.Framework.ZNET, types.Framework.THREAD, types.Framework.CONNECT]),
                types.StudioFrameworkModule("BLE", types.Framework.BLE)],
    }
enable = {
    "define": "HAL_SPIDISPLAY_ENABLE",
    "description": "Enable SPI display",
}
options = {
    "BSP_SPIDISPLAY_DISPLAY": {
        "type": "enum",
        "description": "Use display",
        "values": [
            types.EnumValue('HAL_DISPLAY_SHARP_LS013B7DH03', 'Sharp LS013B7DH03'),
            types.EnumValue('HAL_DISPLAY_SHARP_LS013B7DH06', 'Sharp LS013B7DH06'),
        ]
    },
    "BSP_SPIDISPLAY_USART": {
        "type": types.Peripheral(filter=["USART"],
                                 inherit_options=True,
                                 mode="spi",
                                 define_value_prefix='HAL_SPI_PORT_'),
        "description": "USART to use for display",
    },
    "BSP_SPIDISPLAY_CS": {
        "type": types.Pin(disabled_label="Inherited from USART"),
        "description": "CS pin",
    },
    "BSP_SPIDISPLAY_ENABLE": {
        "type": types.Pin(),
        "description": "Display enable pin",
    },
    "BSP_SPIDISPLAY_EXTMODE": {
        "type": types.Pin(),
        "description": "EXTMODE pin",
    },
    "HAL_SPIDISPLAY_EXTMODE_EXTCOMIN": {
        "type": "boolean",
        "description": "Use EXTCOMIN pin for polarity inversion",
        "defaultValue": "True",
    },
    "HAL_SPIDISPLAY_EXTMODE_SPI": {
        "type": "boolean",
        "description": "Use SPI command for polarity inversion",
    },
    "HAL_SPIDISPLAY_EXTCOMIN_USE_CALLBACK": {
        "type": "boolean",
        "description": "Toggle EXTCOMIN from software callback"
    },
    "HAL_SPIDISPLAY_EXTCOMIN_CALLBACK": {
        "type": "string",
        "description": "EXTCOMIN callback register function"
    },
    "HAL_SPIDISPLAY_EXTCOMIN_USE_PRS": {
        "type": "boolean",
        "description": "Autonomously toggle EXTCOMIN using PRS",
    },
    "BSP_SPIDISPLAY_EXTCOMIN": {
        "type": types.Pin(),
        "description": "EXTCOMIN pin",
        "allowedconflicts": ["PRS_CH"]
    },
    "BSP_SPIDISPLAY_EXTCOMIN_PRS": {
        "type": types.PRSChannelLocation("BSP_SPIDISPLAY_EXTCOMIN", custom_name="SPIDISPLAY_EXTCOMIN"),
        "description": "EXTCOMIN PRS channel"
    },
}
