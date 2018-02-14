# EFR32 Radio Configuration
EFR32 Radio based on RAIL library requires radio configurator (built-in with Simplicity Studio) to generate some magic arrays to properly 
configure the EFR32 on board radio. 

Radio Configurator Settings
--------

Operational Frequency
- Base Channel Frequency: 866 MHz
- Channel Spacing: 500 kHz

Crystal
- Frequency: 38.4 MHz
- Tx Crystal Accuracy: 0 ppm
- Rx Crystal Accuracy: 0 ppm

Modem
- Modulation Type: FSK2
- Bitrate: 1.2 kbps
- Deviation: 5 kHz
- Baudrate Tolerance: 0 ppm
- Shaping Filter: Gaussian
- Shaping Filter Parameter: 0.5
- FSK symbol map: MAP0

Packet
- Frame General
    - Header Enable
    - Frame Length Algorithm: VARIABLE_LENGTH
- Frame Variable Length
    - Maximum Length: 4094 bytes
    - Variable Length Bit Size: 12 bits
- CRC
    - CRC Polynomial: CCITT_16
    - CRC Seed: `1D 0F`
    - CRC Input Padding

Packet Format
- Preamble
    - Preamble Base Pattern: 2
    - Preamble Length Total: 64 bits
    - Preamble Pattern Length: 2 bits
- Syncword
    - Sync Word Length: 16 bits
    - Sync Word 0: `34`
    - Sync Word 1: `12`
- Frame Header
    - CRC Header
    - Header Size: 1 bytes
- Frame Payload
    - Insert/Check CRC after payload

Auto Generated Radio Configuration with Above Configurations
-----
```c
// Copyright 2018 Silicon Laboratories, Inc.
//
//

/***************************************************************************//**
 * @file rail_config.c
 * @brief RAIL Configuration
 * @copyright Copyright 2015 Silicon Laboratories, Inc. http://www.silabs.com
 ******************************************************************************/
// =============================================================================
//
//  WARNING: Auto-Generated Radio Config  -  DO NOT EDIT
//  Radio Configurator Version: 2.11.0
//  RAIL Adapter Version: 2.0.4
//  RAIL Compatibility: 2.x
//
// =============================================================================
#include "em_common.h"
#include "rail_config.h"

const uint8_t generated_irCalConfig[] = {
  24, 69, 3, 6, 4, 16, 0, 1, 1, 3, 0, 2, 2, 0, 0, 0, 0, 5, 0, 1, 1, 0, 0, 0, 0
};

const uint32_t generated_phyInfo[] = {
  1UL,
  0x0002B580, // 2.70899470899
  (uint32_t) NULL,
  (uint32_t) generated_irCalConfig,
#ifdef RADIO_CONFIG_ENABLE_TIMING
  (uint32_t) &generated_timing,
#else
  (uint32_t) NULL,
#endif
};

const uint32_t generated[] = {
  0x01031FF0UL, 0x003F003FUL,
     /* 1FF4 */ 0x00000000UL,
     /* 1FF8 */ (uint32_t) generated_phyInfo,
  0x00020004UL, 0x0000C003UL,
     /* 0008 */ 0x00000FFEUL,
  0x00020018UL, 0x00000000UL,
     /* 001C */ 0x00000000UL,
  0x00070028UL, 0x00000000UL,
     /* 002C */ 0x00000000UL,
     /* 0030 */ 0x00000000UL,
     /* 0034 */ 0x00000000UL,
     /* 0038 */ 0x00000000UL,
     /* 003C */ 0x00000000UL,
     /* 0040 */ 0x000007A0UL,
  0x00010048UL, 0x00000000UL,
  0x00020054UL, 0x00000000UL,
     /* 0058 */ 0x00000000UL,
  0x000400A0UL, 0x00004800UL,
     /* 00A4 */ 0x00004CFFUL,
     /* 00A8 */ 0x00004900UL,
     /* 00AC */ 0x00004DFFUL,
  0x00012000UL, 0x00001744UL,
  0x00012010UL, 0x0000F0B8UL,
  0x00012018UL, 0x00008408UL,
  0x00013008UL, 0x0100AC13UL,
  0x00023030UL, 0x0010231CUL,
     /* 3034 */ 0x00000003UL,
  0x00013040UL, 0x00000000UL,
  0x000140A0UL, 0x0F0027AAUL,
  0x000140B8UL, 0x0023C000UL,
  0x000140F4UL, 0x00001020UL,
  0x00024134UL, 0x00000880UL,
     /* 4138 */ 0x000087F6UL,
  0x00024140UL, 0x00880020UL,
     /* 4144 */ 0x4D52E6C1UL,
  0x00044160UL, 0x00000000UL,
     /* 4164 */ 0x00000000UL,
     /* 4168 */ 0x00000006UL,
     /* 416C */ 0x00000006UL,
  0x00086014UL, 0x00000010UL,
     /* 6018 */ 0x04000000UL,
     /* 601C */ 0x0002C20FUL,
     /* 6020 */ 0x000040C8UL,
     /* 6024 */ 0x000AD000UL,
     /* 6028 */ 0x03000000UL,
     /* 602C */ 0x00000000UL,
     /* 6030 */ 0x00000000UL,
  0x00066050UL, 0x00087D00UL,
     /* 6054 */ 0x00000C40UL,
     /* 6058 */ 0x0010029CUL,
     /* 605C */ 0x00200011UL,
     /* 6060 */ 0x00002C00UL,
     /* 6064 */ 0x00004800UL,
  0x000C6078UL, 0x0460281FUL,
     /* 607C */ 0x00000000UL,
     /* 6080 */ 0x00360334UL,
     /* 6084 */ 0x00000000UL,
     /* 6088 */ 0x00000000UL,
     /* 608C */ 0x22140A04UL,
     /* 6090 */ 0x4F4A4132UL,
     /* 6094 */ 0x00000000UL,
     /* 6098 */ 0x00000000UL,
     /* 609C */ 0x00000000UL,
     /* 60A0 */ 0x00000000UL,
     /* 60A4 */ 0x00000000UL,
  0x000760E4UL, 0xCB870080UL,
     /* 60E8 */ 0x00000000UL,
     /* 60EC */ 0x07830464UL,
     /* 60F0 */ 0x3AC81388UL,
     /* 60F4 */ 0x0006209CUL,
     /* 60F8 */ 0x00206100UL,
     /* 60FC */ 0x208556B7UL,
  0x00036104UL, 0x0010BB3FUL,
     /* 6108 */ 0x00003020UL,
     /* 610C */ 0x0000BB88UL,
  0x00016120UL, 0x00000000UL,
  0x00077014UL, 0x000270FEUL,
     /* 7018 */ 0x00001300UL,
     /* 701C */ 0x87EA0060UL,
     /* 7020 */ 0x00000000UL,
     /* 7024 */ 0x00000082UL,
     /* 7028 */ 0x00000000UL,
     /* 702C */ 0x000000D5UL,
  0x00027048UL, 0x0000383EUL,
     /* 704C */ 0x000025BCUL,
  0x00037070UL, 0x00120103UL,
     /* 7074 */ 0x0008302CUL,
     /* 7078 */ 0x006D8480UL,

  0xFFFFFFFFUL,
};

RAIL_ChannelConfigEntryAttr_t generated_entryAttr = {
  { 0xFFFFFFFF }
};

const RAIL_ChannelConfigEntry_t generated_channels[] = {
  {
    .phyConfigDeltaAdd = NULL,
    .baseFrequency = 866000000,
    .channelSpacing = 50000,
    .physicalChannelOffset = 0,
    .channelNumberStart = 0,
    .channelNumberEnd = 20,
    .maxPower = RAIL_TX_POWER_MAX,
    .attr = &generated_entryAttr
  },
};

const RAIL_ChannelConfig_t generated_channelConfig = {
  generated,
  NULL,
  generated_channels,
  1
};
const RAIL_ChannelConfig_t *channelConfigs[] = {
  &generated_channelConfig,
  NULL
};



//        _  _                          
//       | )/ )         Wireless        
//    \\ |//,' __       Application     
//    (")(_)-"()))=-    Software        
//       (\\            Platform        


```
