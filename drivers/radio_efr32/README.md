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
    - Minimum Length: 1 byte
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

