from . import halconfig_types as types
from . import halconfig_dependency as dep

name = "COEX"
displayname = "Coexistence"
description = "Coexistence between multiple radios. Radio hold-off, request, grant."
compatibility = dep.Dependency(mcu_type=dep.McuType.RADIO)  # = all
category = " Radio"
studio_module = {
    "basename" : "SDK.HAL.COEX",
    "modules" : [types.StudioFrameworkModule("BASE", [types.Framework.ZNET, types.Framework.THREAD, types.Framework.CONNECT]),
                 types.StudioFrameworkModule("BLE", types.Framework.BLE)],
    }
enable = {
    "define": "HAL_COEX_ENABLE",
    "description": "Enable radio coexistence",
}
options = {
    "BSP_COEX_REQ": {
        "type": types.Pin(),
        "description": "Request",
        "subcategory": "Request",
    },
    # depends on BSP_COEX_REQ
    "BSP_COEX_REQ_ASSERT_LEVEL": {
        "type": "enum",
        "description": "Request assert signal level",
        "values": [types.EnumValue('1', 'High'),
                   types.EnumValue('0', 'Low')],
        "subcategory": "Request",
    },
    # depends on BSP_COEX_REQ
    "HAL_COEX_REQ_SHARED": {
        "type": "boolean",
        "description": "Configure REQ signal for shared mode",
        "defaultValue": "False",
        "subcategory": "Request",
    },
    # depends on HAL_COEX_REQ_SHARED
    "HAL_COEX_REQ_BACKOFF":{
        "type": "uint8_t",
        "description": "Max backoff after REQ deassert",
        "min": "0",
        "max": "255",
        "defaultValue": "15",
        "subcategory": "Request",
    },
    # depends on BSP_COEX_REQ
    "HAL_COEX_REQ_WINDOW":{
        "type": "uint16_t",
        "description": "Microseconds between asserting REQUEST and starting RX/TX (BLE only)",
        "min": "0",
        "max": "5000",
        "defaultValue": "500",
        "subcategory": "Request",
    },
    # depends on BSP_COEX_REQ
    "HAL_COEX_RETRYRX_ENABLE": {
        "type": "boolean",
        "description": "Receive retry REQ",
        "defaultValue": "False",
        "subcategory": "Request",
    },
    # depends on HAL_COEX_RETRYRX_ENABLE
    "HAL_COEX_RETRYRX_TIMEOUT": {
        "type": "uint8_t",
        "description": "Receive retry REQ timeout (ms)",
        "min": "0",
        "max": "255",
        "defaultValue": "16",
        "subcategory": "Request",
    },
    # depends on BSP_COEX_PRI and HAL_COEX_RETRYRX_ENABLE
    "HAL_COEX_RETRYRX_HIPRI": {
        "type": "boolean",
        "description": "Receive retry REQ high PRI",
        "defaultValue": "True",
        "subcategory": "Request",
    },
    "BSP_COEX_GNT": {
        "type": types.Pin(),
        "description": "Grant",
        "subcategory": "Grant",
    },
    "BSP_COEX_GNT_ASSERT_LEVEL": {
        "type": "enum",
        "description": "Grant assert signal level",
        "values": [types.EnumValue('1', 'High'),
                   types.EnumValue('0', 'Low')],
        "subcategory": "Grant",
    },
    "HAL_COEX_TX_ABORT": {
        "type": "boolean",
        "description": "Abort transmission mid-packet if GNT is lost",
        "defaultValue": "False",
        "subcategory": "Grant",
    },
    "HAL_COEX_ACKHOLDOFF": {
        "type": "boolean",
        "description": "Disable ACKing when GNT deasserted, RHO asserted, or REQ not secured (shared REQ only)",
        "defaultValue": "True",
        "subcategory": "Grant",
    },
    "BSP_COEX_PRI": {
        "type": types.Pin(),
        "description": "Priority",
        "subcategory": "Priority",
    },
    "BSP_COEX_PRI_ASSERT_LEVEL": {
        "type": "enum",
        "description": "Priority assert signal level",
        "values": [types.EnumValue('1', 'High'),
                   types.EnumValue('0', 'Low')],
        "subcategory": "Priority",
    },
    # depends on BSP_COEX_PRI
    "HAL_COEX_TX_HIPRI": {
        "type": "boolean",
        "description": "Assert high PRI when transmitting a packet",
        "defaultValue": "True",
        "subcategory": "Priority",
    },
    # depends on BSP_COEX_PRI
    "HAL_COEX_RX_HIPRI": {
        "type": "boolean",
        "description": "Assert high PRI when receiving a packet",
        "defaultValue": "True",
        "subcategory": "Priority",
    },
    "BSP_COEX_RHO": {
        "type": types.Pin(),
        "description": "Radio hold off",
        "subcategory": "Radio Hold Off",
    },
    "BSP_COEX_RHO_ASSERT_LEVEL": {
        "type": "enum",
        "description": "Radio hold off assert signal level",
        "values": [types.EnumValue('1', 'High'),
                   types.EnumValue('0', 'Low')],
        "subcategory": "Radio Hold Off",
    },
}
