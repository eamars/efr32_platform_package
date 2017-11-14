from . import halconfig_types as types
from . import halconfig_dependency as dep

name = "VCOM"
displayname = "Virtual COM"
compatibility = dep.Dependency()  # all
category = " Serial"
enable = {
    "define": "HAL_VCOM_ENABLE",
    "description": "Enable VCOM",
}
options = {
    "BSP_VCOM_ENABLE": {
        "type": types.Pin(),
        "description": "VCOM enable",
    }
}
