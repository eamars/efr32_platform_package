
WirelessGuard Data Protocol v1
==============================

This protocol defines the communications between a Hatch Outdoor and network coprocessor. The data packets are encapsulated by lower level protocols, and this is effectively the "Application Layer" protocol for the Hatch Outdoor. 


Basic Report
------------
*Hatch -> NCP*
A simple packet for periodic reporting of data. This is the default periodic packet sent from the havtch, but can also be requested by a (Data Request)[wg_data_v1.md#DataRequest].


Extended Report
---------------
*Hatch -> NCP*
This is for reporting much datas. Since the basic report doesn't report a lot, this provides a deeper look into the device sensors for debugging, or simply for checkups on the device. This can be requested by a (Data Request)[wg_data_v1.md#DataRequest].


Data Request
------------
*NCP -> Hatch*
Used to request data from the device. This is not just for basic and extended reports, but can request the calibration, settings, and debug data as well. It has a single field to specify which report is requested.


Calibrate Command
-----------------
*NCP -> Hatch*
Used to calibrate the Hatch. This has fields to specify what to calibrate. A (Calibrate Report)[wg_data_v1.md#CalibrateReport] will be sent after calibration completes.


Calibrate Report
----------------
*Hatch -> NCP*
Reports the calibration data from the hatch. This is sent after calibration completes, and is mostly for debugging. This can also be requested by a (Data Request)[wg_data_v1.md#DataRequest].


Reset Command
-------------
*NCP -> Hatch*
Resets the hatch. An argument is provided to determine the type of reset.


Reset Acknowledge
-----------------
*Hatch -> NCP*
Sent after a reset command, this acknowledges that the hatch will reset itself.


Debug Report
----------
*Hatch -> NCP*
This provides detailed debugging information about the hatch. This can be requested by a (Data Request)[wg_data_v1.md#DataRequest].

