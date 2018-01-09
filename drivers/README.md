# EFR32 drivers

For more information please visit specific driver linked to this folder.

#.RESETINFO

The .RESETINFO region sits between `0x20000000` and `0x20000100` in SRAM. There 
are multiple overlays for this region. Currently EFR32 driver implements two:
- `reset_info_map_t`: contains persist data before resetting the device due to fatal
error or hardware failure. 
- `bootloader_info_map_t`: contains partition information for switching firmware purpose.

Different memory overlay is told by first 16bits in `.RESETINFO` region. Please refer to `reset_info.h`
for more information.
