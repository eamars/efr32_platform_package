# Manufacture Hardware Diagnostic Tool
This module aim to provide a generic interface for driver module to test its functionality on Hatch platform. 
 
Why
===
Some drivers require specific hardware to perform unit test and blackbox test. This module provides a clean 
environment for those drivers to run & log data. 

How
===
A software reset is necessary to switch from application mode to MFG test mode. The presence of bootloader is 
necessary to perform the firmware switching. 

Reboot is required and will be handled by framework between each test entry. 
