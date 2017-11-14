from . import halconfig_types as types
from . import halconfig_dependency as dep

name = "IOEXP"
displayname = "I2C I/O Expander"
compatibility = dep.Dependency()  # all
studio_module = {
    "basename": "SDK.HAL.IOEXP",
    "modules": [types.StudioFrameworkModule("COMMON", [types.Framework.ZNET, types.Framework.THREAD, types.Framework.CONNECT]),
                types.StudioFrameworkModule("BLE", types.Framework.BLE)],
    }
enable = {
    "define": "HAL_IOEXP_ENABLE",
    "description": "Enable IO expander",
}
options = {
    "BSP_IOEXP_WAKE": {
        "type": types.Pin(),
        "description": "IOEXP wake pin",
    },
    "HAL_IOEXP_VCOM_ENABLE": {
        "type": "boolean",
        "description": "Enable VCOM via IO expander",
    },
    "BSP_IOEXP_PERIPHERAL": {
        "type": types.Peripheral(filter=["I2C"], inherit_options=True, define_value_prefix='HAL_I2C_PORT_'),
        "description": "IO expander I2C",
    },
}
