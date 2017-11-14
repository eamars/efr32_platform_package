from . import halconfig_types as types
from . import halconfig_dependency as dep

name = "I2C"
description = "Inter-Integrated Circuit"
compatibility = dep.Dependency()  # all
peripheral = 'I2C'
modes = {
    "define": "hal_i2c_mode",
    "description": "mode",
    "values": [types.EnumValue('master', 'Master'),
                types.EnumValue('slave', 'Slave')]
}
enable = {
    "define": "HAL_I2C_ENABLE",
    "description": "Enable I2C peripheral",
}
options = {
    "HAL_I2C_INIT_ENABLE": {
        "description": "Enable I2C after init",
        "type": "boolean"
    },
    "HAL_I2C_MAX_BUS_FREQ": {
        "description": "I2C max bus frequency",
        "type": "uint32_t",
        "min": "1",
        "max": "2000000",
        "mode": "master",
        "advanced": True,
    },
    "HAL_I2C_LOW_HIGH_RATIO": {
        "description": "Clock Low/High ratio",
        "type": "enum",
        "values": [types.EnumValue('_I2C_CTRL_CLHR_STANDARD', 'Standard'),
                   types.EnumValue('_I2C_CTRL_CLHR_ASYMMETRIC', 'Asymmetric'),
                   types.EnumValue('_I2C_CTRL_CLHR_FAST', 'Fast')],
        "advanced": True,
    },
    "HAL_I2C_MASTER": {
        "description": "Master/Slave",
        "type": "boolean",
        "visible": False,
        "advanced": True,
    },
    "HAL_I2C_SLAVE_ADDRESS": {
        "description": "Slave address",
        "type": "uint8_t",
        "min": "0",
        "max": "255",
        "mode": "slave",
        "advanced": True,
    },
    "BSP_I2C_SCL": {
        "type": types.Pin(signal="SCL"),
        "description": "I2C SCL",
    },
    "BSP_I2C_SDA": {
        "type": types.Pin(signal="SDA"),
        "description": "I2C SDA",
    },
}
