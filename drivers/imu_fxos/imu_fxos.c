/**
 * @brief Implementation of I2C IMU driver
 * @file imu_fxos.c
 * @author Steven O'Rourke
 * @date Oct, 2017
 */

#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include "imu_fxos.h"
#include "drv_debug.h"
#include "delay.h"
#include "gpiointerrupt.h"
#include "led.h"
#include "debug-printing.h"
#include "FreeRTOS.h"
#include "queue.h"

void  FXOS8700CQ_Initialize(imu_FXOS8700CQ_t * obj, i2cdrv_t * i2c_device, pio_t enable, pio_t int_1, pio_t int_2)
{
    uint16_t i = 0;
    // sanity check for pointers
    DRV_ASSERT(obj);
    DRV_ASSERT(i2c_device);

    // do not reinitialize the eeprom driver
    if (obj->initialized)
    {
        return;
    }

    // assign i2c object
    obj->i2c_device = i2c_device;

    // store enable pins
    obj->enable = enable;

    // assigng interupt pins
    obj->int_1 = int_1;
    obj->int_2 = int_2;

    //intalise que
    obj->imu_event_queue = xQueueCreate(2, sizeof(imu_ev_t));

    // configure load switch pins
    GPIO_PinModeSet(PIO_PORT(obj->enable), PIO_PIN(obj->enable), gpioModePushPull, 1);

    obj->initialized = true;

    FXOS8700CQ_WriteByte(obj, CTRL_REG2, RST_MASK);                    //Reset sensor, and wait for reboot to complete
    delay_ms(2);                                        //Wait at least 1ms after issuing a reset before attempting communications
    FXOS8700CQ_StandbyMode(obj);

    while (FXOS8700CQ_ReadByte(obj, CTRL_REG2) & RST_MASK);

}

char FXOS8700CQ_ReadStatusReg(imu_FXOS8700CQ_t * obj)
{
    return FXOS8700CQ_ReadByte(obj, STATUS);
}

void FXOS8700CQ_ActiveMode (imu_FXOS8700CQ_t * obj)
{
    char reg1 = FXOS8700CQ_ReadByte(obj, CTRL_REG1);
    FXOS8700CQ_WriteByte(obj, CTRL_REG1, (reg1 | ACTIVE_MASK));   //Set the Active bit in System Control 1 Register.
}

char FXOS8700CQ_StandbyMode (imu_FXOS8700CQ_t * obj)
{
    char n = FXOS8700CQ_ReadByte(obj, CTRL_REG1);
    FXOS8700CQ_WriteByte(obj, CTRL_REG1, n & (~ACTIVE_MASK));
    return (n & ~ACTIVE_MASK);
}

void FXOS8700CQ_HybridMode(imu_FXOS8700CQ_t * obj)
{
    FXOS8700CQ_StandbyMode(obj);
    FXOS8700CQ_WriteByte(obj, M_CTRL_REG1, (HYBRID_ACTIVE|M_OSR2_MASK|M_OSR1_MASK|M_OSR0_MASK) );      // OSR=max, hybrid mode (TO, Aug 2012)
    FXOS8700CQ_WriteByte(obj, M_CTRL_REG2, M_HYB_AUTOINC_MASK);       // enable hybrid autoinc
    FXOS8700CQ_WriteByte(obj, CTRL_REG4, INT_EN_DRDY_MASK );           // Enable interrupts for DRDY (TO, Aug 2012)
    FXOS8700CQ_WriteByte(obj, XYZ_DATA_CFG, FULL_SCALE_2G);             // Set FSR of accel to +/-2g
    FXOS8700CQ_WriteByte(obj, CTRL_REG1, (HYB_ASLP_RATE_25HZ|HYB_DATA_RATE_50HZ)  );     // Set ODRs
    FXOS8700CQ_ActiveMode(obj);
}

char FXOS8700CQ_GetChipMode(imu_FXOS8700CQ_t * obj)
{
    char mode = FXOS8700CQ_ReadByte(obj, SYSMOD);
    return mode;
}

char FXOS8700CQ_ID (imu_FXOS8700CQ_t * obj)
{
     char id  = FXOS8700CQ_ReadByte(obj, WHO_AM_I);
     return id;
}

void FXOS8700CQ_ConfigureAccelerometer(imu_FXOS8700CQ_t * obj)
{
    FXOS8700CQ_StandbyMode (obj);
    //FXOS8700CQ_WriteByte(obj, CTRL_REG4, INT_EN_DRDY_MASK );                   // Enable interrupts for DRDY (TO, Aug 2012)
    FXOS8700CQ_WriteByte(obj, XYZ_DATA_CFG, FULL_SCALE_2G);                    // Set FSR of accel to +/-2g
    FXOS8700CQ_WriteByte(obj, CTRL_REG1, (HYB_ASLP_RATE_25HZ|HYB_DATA_RATE_50HZ));     // Set ODRs
    FXOS8700CQ_WriteByte(obj, CTRL_REG2, (SMOD_LOW_POWER|SLPE_MASK));
    FXOS8700CQ_WriteByte(obj, CTRL_REG3, (IPOL_MASK));
    FXOS8700CQ_ActiveMode (obj);
}

void FXOS8700CQ_PollAccelerometer (imu_FXOS8700CQ_t * obj, rawdata_t *accel_data)
{
    char raw[6] = {0};
    FXOS8700CQ_ReadByteArray(obj, OUT_X_MSB, raw, 6);
    accel_data->x = (raw[0] << 8) | raw[1];     // Pull out 16-bit, 2's complement magnetometer data
    accel_data->y = (raw[2] << 8) | raw[3];
    accel_data->z = (raw[4] << 8) | raw[5];
}

void FXOS8700CQ_HighPassFilter(imu_FXOS8700CQ_t * obj, char status)
{
    FXOS8700CQ_WriteByte(obj, XYZ_DATA_CFG,status);
}

void FXOS8700CQ_FullScaleRange(imu_FXOS8700CQ_t * obj, range_t range)
{
    FXOS8700CQ_WriteByte(obj, XYZ_DATA_CFG,range);
}

void FXOS8700CQ_SetAccelerometerDynamicRange(imu_FXOS8700CQ_t * obj, range_t range)
{
    FXOS8700CQ_StandbyMode (obj);
    switch(range)
    {
        case SCALE2G:
            FXOS8700CQ_WriteByte(obj, XYZ_DATA_CFG, (FXOS8700CQ_ReadByte(obj, XYZ_DATA_CFG) & ~FS_MASK));     //Write the 2g dynamic range value into register 0x0E
        break;
        case SCALE4G:
            FXOS8700CQ_WriteByte(obj, XYZ_DATA_CFG, (FXOS8700CQ_ReadByte(obj, XYZ_DATA_CFG) & ~FS_MASK));     //Write the 4g dynamic range value into register 0x0E
            FXOS8700CQ_WriteByte(obj, XYZ_DATA_CFG, (FXOS8700CQ_ReadByte(obj, XYZ_DATA_CFG) | FULL_SCALE_4G));
        break;
        case SCALE8G:
            FXOS8700CQ_WriteByte(obj, XYZ_DATA_CFG, (FXOS8700CQ_ReadByte(obj, XYZ_DATA_CFG) & ~FS_MASK));     //Write the 8g dynamic range value into register 0x0E
            FXOS8700CQ_WriteByte(obj, XYZ_DATA_CFG, (FXOS8700CQ_ReadByte(obj, XYZ_DATA_CFG) | FULL_SCALE_8G));
        break;

        default:

        break;
    }
    FXOS8700CQ_ActiveMode (obj);
}

void FXOS8700CQ_ConfigureMagnetometer(imu_FXOS8700CQ_t * obj)
{
    FXOS8700CQ_StandbyMode (obj);
    FXOS8700CQ_WriteByte(obj, M_CTRL_REG1, (M_ACAL_MASK | MAG_ACTIVE|M_OSR1_MASK) );      // OSR=2, hybrid mode (TO, Aug 2012) auto calibrate mode on
    FXOS8700CQ_WriteByte(obj, M_CTRL_REG2, M_HYB_AUTOINC_MASK);       // enable hybrid autoinc
    FXOS8700CQ_WriteByte(obj, M_CTRL_REG3, M_ASLP_OS_1_MASK);       // OSR =2 in auto sleep mode
    FXOS8700CQ_WriteByte(obj, M_VECM_CFG, (M_VECM_EN_MASK| M_VECM_WAKE_EN_MASK| M_VECM_CFG));
    FXOS8700CQ_ActiveMode (obj);
}

void FXOS8700CQ_PollMagnetometer (imu_FXOS8700CQ_t * obj, rawdata_t *mag_data)
{
    char raw[6] = {0};
    FXOS8700CQ_ReadByteArray(obj, M_OUT_X_MSB, raw, 6);
    mag_data->x = (raw[0] << 8) | raw[1];           // Return 16-bit, 2's complement magnetometer data
    mag_data->y = (raw[2] << 8) | raw[3];
    mag_data->z = (raw[4] << 8) | raw[5];
}

char FXOS8700CQ_MagnetometerStatus(imu_FXOS8700CQ_t * obj)
{
    char stat  = FXOS8700CQ_ReadByte(obj, M_DR_STATUS);
    return stat;
}

void FXOS8700CQ_GetData(imu_FXOS8700CQ_t * obj, rawdata_t *accel_data, rawdata_t *magn_data)
{
    char raw[12] = {0};
    FXOS8700CQ_ReadByteArray(obj, OUT_X_MSB, raw, FXOS8700CQ_READ_LEN);

    magn_data->x = (raw[0] << 8) | raw[1];      // Pull out 16-bit, 2's complement magnetometer data
    magn_data->y = (raw[2] << 8) | raw[3];
    magn_data->z = (raw[4] << 8) | raw[5];

    accel_data->x = (raw[6] << 8) | raw[7];     // Pull out 14-bit, 2's complement, right-justified accelerometer data
    accel_data->y = (raw[8] << 8) | raw[9];
    accel_data->z = (raw[10] << 8) | raw[11];

    // Have to apply corrections to make the int16_t correct
    if(accel_data->x > UINT14_MAX/2)
    {
        accel_data->x -= UINT14_MAX;
    }
    if(accel_data->y > UINT14_MAX/2)
    {
        accel_data->y -= UINT14_MAX;
    }
    if(accel_data->z > UINT14_MAX/2)
    {
        accel_data->z -= UINT14_MAX;
    }
}

void FXOS8700CQ_FIFOMode(imu_FXOS8700CQ_t * obj, FX_mode_t mode)
{
    FXOS8700CQ_WriteByte(obj, F_SETUP,6<<mode);
}

char FXOS8700CQ_GetODR(imu_FXOS8700CQ_t * obj)
{
    unsigned char odr = 0;

    return odr;
}

void FXOS8700CQ_SetODR (imu_FXOS8700CQ_t * obj, char DataRateValue)
{
    DataRateValue <<= 3;        //Adjust the desired Output Data Rate value as needed.
    FXOS8700CQ_StandbyMode (obj);
    FXOS8700CQ_WriteByte(obj, CTRL_REG1,FXOS8700CQ_ReadByte(obj, CTRL_REG1) & ~DR_MASK);      //Write in the Data Rate value into Ctrl Reg 1
    FXOS8700CQ_WriteByte(obj, CTRL_REG1, FXOS8700CQ_ReadByte(obj, CTRL_REG1)| DataRateValue);
    FXOS8700CQ_ActiveMode (obj);
}

char FXOS8700CQ_GetTemperature(imu_FXOS8700CQ_t * obj)
{
    char temp = FXOS8700CQ_ReadByte(obj, TEMP);

    return temp;
}

char FXOS8700CQ_GetOrientation(imu_FXOS8700CQ_t * obj)
{
    unsigned char orientation = FXOS8700CQ_ReadByte(obj, PL_STATUS);
    return orientation;
}

void FXOS8700CQ_ConfigureOrientation(imu_FXOS8700CQ_t * obj)
{
    unsigned char CTRL_REG1_Data = FXOS8700CQ_ReadByte(obj, 0x2A);           //read contents of register
    CTRL_REG1_Data &= 0xFE;                                             //Set last bit to 0.
    FXOS8700CQ_WriteByte(obj, 0x2A, CTRL_REG1_Data);                         //Write the updated value in CTRL_REG1  Put the part into Standby Mode

    //Set the data rate to 50 Hz (for example, but can choose any sample rate).

    CTRL_REG1_Data = FXOS8700CQ_ReadByte(obj, 0x2A);             //Note: Can combine this step with above
    CTRL_REG1_Data &= 0xC7;                                 //Clear the sample rate bits
    CTRL_REG1_Data |= 0x20;                                 //Set the sample rate bits to 50 Hz
    FXOS8700CQ_WriteByte(obj, 0x2A, CTRL_REG1_Data);             //Write updated value into the register.

    //Set the PL_EN bit in Register 0x11 PL_CFG. This will enable the orientation detection.
    unsigned char PLCFG_Data = FXOS8700CQ_ReadByte (obj, 0x11);
    PLCFG_Data |= 0x40;
    FXOS8700CQ_WriteByte(obj, 0x11, PLCFG_Data);

    // Set the Back/Front Angle trip points in register 0x13 following the table in the data sheet.

    unsigned char PL_BF_ZCOMP_Data = FXOS8700CQ_ReadByte(obj, 0x13);
    PL_BF_ZCOMP_Data &= 0x3F;                           //Clear bit 7 and 6
    //Select one of the following to set the B/F angle value:
    PL_BF_ZCOMP_Data |= 0x00;                           // This does nothing additional and keeps bits [7:6] = 00
    PL_BF_ZCOMP_Data |= 0x40;                           // Sets bits[7:6] = 01
    PL_BF_ZCOMP_Data |= 0x80;                           // Sets bits[7:6] = 02
    PL_BF_ZCOMP_Data |= 0xC0;                           // Sets bits[7:6] = 03
    FXOS8700CQ_WriteByte(obj, 0x13, PL_BF_ZCOMP_Data); //Write in the updated Back/Front Angle
    //Set the Z-Lockout angle trip point in register 0x13 following the table in datasheet
    PL_BF_ZCOMP_Data = FXOS8700CQ_ReadByte(obj, 0x1C);   //Read out contents of the register (can be read by all
    PL_BF_ZCOMP_Data &= 0xF8;                       //Clear the last three bits of the register
    PL_BF_ZCOMP_Data |= 0x00;                       //This does nothing additional but the Z-lockout selection will remain at 14°
    //PL_BF_ZCOMP_Data | = 0x01; //Set the Z-lockout angle to 18°
    //PL_BF_ZCOMP_Data | = 0x02; //Set the Z-lockout angle to 21°
    //PL_BF_ZCOMP_Data | = 0x03; //Set the Z-lockout angle to 25°
    //PL_BF_ZCOMP_Data | = 0x04; //Set the Z-lockout angle to 29°
    //PL_BF_ZCOMP_Data | = 0x05; //Set the Z-lockout angle to 33°
    //PL_BF_ZCOMP_Data | = 0x06; //Set the Z-lockout angle to 37°
    //PL_BF_ZCOMP_Data | = 0x07; //Set the Z-lockout angle to 42°
    FXOS8700CQ_WriteByte(obj, 0x13, PL_BF_ZCOMP_Data); //Write in the updated Z-lockout angle

    //Set the Trip Threshold Angle
    //NOTE: This register is readable in all versions of MMA845xQ but it is only modifiable in the MMA8451Q.

    unsigned char P_L_THS_Data = FXOS8700CQ_ReadByte(obj, 0x14);                     //(can be read by all versions of MMA845xQ)  //The remaining parts of this step only apply to MMA8451Q
    P_L_THS_Data &= 0x07; //Clear the Threshold values

    //Choose one of the following options
    P_L_THS_Data |= (0x07)<<3; //Set Threshold to 15°
    //P_L_THS_Data | = (0x09)<<3; //Set Threshold to 20°
    //P_L_THS_Data | = (0x0C)<<3; //Set Threshold to 30°
    //P_L_THS_Data | = (0x0D)<<3; //Set Threshold to 35°
    //P_L_THS_Data | = (0x0F)<<3; //Set Threshold to 40°
    //P_L_THS_Data | = (0x10)<<3; //Set Threshold to 45°
    //P_L_THS_Data | = (0x13)<<3; //Set Threshold to 55°
    //P_L_THS_Data | = (0x14)<<3; //Set Threshold to 60°
    //P_L_THS_Data | = (0x17)<<3; //Set Threshold to 70°
    //P_L_THS_Data | = (0x19)<<3; //Set Threshold to 75°
    FXOS8700CQ_WriteByte(obj, 0x14,P_L_THS_Data);

    // Set the Hysteresis Angle
    P_L_THS_Data = FXOS8700CQ_ReadByte(obj, 0x14);
    //NOTE: The remaining parts of this step only apply to the MMA8451Q.
    P_L_THS_Data &= 0xF8;//Clear the Hysteresis values
    //P_L_THS_Data | = 0x01; //Set Hysteresis to ±4°
    P_L_THS_Data |= 0x02;//Set Threshold to ±7°
    //P_L_THS_Data | = 0x03;//Set Threshold to ±11°
    //P_L_THS_Data | = 0x04;//Set Threshold to ±14°
    //P_L_THS_Data | = 0x05;//Set Threshold to ±17°
    //P_L_THS_Data | = 0x06; //Set Threshold to ±21°
    //P_L_THS_Data | = 0x07; //Set Threshold to ±24°
    FXOS8700CQ_WriteByte(obj, 0x14,P_L_THS_Data);

    //Register 0x2D, Control Register 4 configures all embedded features for interrupt detection.
    /*
    To set this device up to run an interrupt service routine:
    Program the Orientation Detection bit in Control Register 4.
    Set bit 4 to enable the orientation detection “INT_EN_LNDPRT”.
    */
    unsigned char CTRL_REG4_Data = FXOS8700CQ_ReadByte(obj, 0x2D);   //Read out the contents of the register
    CTRL_REG4_Data |= 0x10;                     //Set bit 4
    FXOS8700CQ_WriteByte(obj, 0x2D, CTRL_REG4_Data);     //Set the bit and write into CTRL_REG4


    // Register 0x2E is Control Register 5 which gives the option of routing the interrupt to either INT1 or INT2
    /*
    Depending on which interrupt pin is enabled and configured to the processor:
    Set bit 4 “INT_CFG_LNDPRT” to configure INT1, or,
    Leave the bit clear to configure INT2.
    */

    unsigned char CTRL_REG5_Data = FXOS8700CQ_ReadByte(obj, 0x2E);           //In the next two lines choose to clear bit 4 to route to INT2 or set bit 4 to route to INT1
    CTRL_REG5_Data &= 0xEF;                         //Clear bit 4 to choose the interrupt to route to INT2
    CTRL_REG5_Data |= 0x10;                     //Set bit 4 to choose the interrupt to route to INT1
    FXOS8700CQ_WriteByte(obj, 0x2E, CTRL_REG5_Data);             //Write in the interrupt routing selection

    // Set the debounce counter in register 0x12
    /*
    This value will scale depending on the application-specific required ODR.
    If the device is set to go to sleep, reset the debounce counter before the device goes to sleep. This setting
    helps avoid long delays since the debounce will always scale with the current sample rate. The debounce
    can be set between 50 ms - 100 ms to avoid long delays.
    */
    FXOS8700CQ_WriteByte(obj, 0x12, 0x05);//This sets the debounce counter to 100 ms at 50 Hz

    //Put the device in Active Mode
    CTRL_REG1_Data = FXOS8700CQ_ReadByte(obj, 0x2A);//Read out the contents of the register
    CTRL_REG1_Data |= 0x01;//Change the value in the register to Active Mode.
    FXOS8700CQ_WriteByte(obj, 0x2A, CTRL_REG1_Data);//Write in the updated value to put the device in Active Mode

}

void FXOS8700CQ_ConfigureGenericTapMode(imu_FXOS8700CQ_t * obj)
{
    FXOS8700CQ_WriteByte(obj, PULSE_CFG, 0x55);  //Example X, Y and Z configured for Single Tap with Latch enabled
    //FXOS8700CQ_WriteByte(obj, PULSE_CFG, 0x6A);    //Example X, Y and Z configured for Double Tap with Latch enabled
    //FXOS8700CQ_WriteByte(obj, PULSE_CFG, 0x7F);    //Example X, Y and Z configured for Single Tap and Double Tap with Latch enabled

    /**************Set Threesholds*************************/
    FXOS8700CQ_WriteByte(obj, PULSE_THSX, 0x20);     //Set X Threshold to 32 counts or 2g
    FXOS8700CQ_WriteByte(obj, PULSE_THSY, 0x20);     //Set Y Threshold to 32 counts or 2g
    FXOS8700CQ_WriteByte(obj, PULSE_THSZ, 0x0C);     //Set Z Threshold to 48 counts or 3g

    FXOS8700CQ_WriteByte(obj, PULSE_TMLT, 0x06); //Set the Pulse Time Limit for 30 ms at 200 Hz ODR in Normal Mode with the LPF Enabled A. 30 ms/5 ms = 6 counts
    FXOS8700CQ_WriteByte(obj, PULSE_LTCY, 0x28); //Set the Pulse Latency Timer to 200 ms, 200 Hz ODR Low Power Mode, LPF Not Enabled. 200 ms/5.0 ms = 40 counts
    FXOS8700CQ_WriteByte(obj, PULSE_WIND, 0x0F); //Set the Pulse window to 300 ms, 100 Hz ODR Low Power Mode, LPF Enabled 300 ms/20 ms = 15 counts

    FXOS8700CQ_WriteByte(obj, CTRL_REG4, 0x08);  //Enable Tap Interrupt in Register 0x2D.
    FXOS8700CQ_WriteByte(obj, CTRL_REG5,0x08);   //Set Tap to INT1
}

void  FXOS8700CQ_ConfigureSingleTapMode(imu_FXOS8700CQ_t * obj)
{
    //To set up any configuration make sure to be in Standby Mode.

    FXOS8700CQ_WriteByte(obj, CTRL_REG1, 0x08);                  //400 Hz, Standby Mode
    FXOS8700CQ_WriteByte(obj, PULSE_CFG, 0x15);                  //Enable X and Y and Z Single Pulse
    FXOS8700CQ_WriteByte(obj, PULSE_THSX, 0x19);                 //Set X Threshold to 1.575g
    FXOS8700CQ_WriteByte(obj, PULSE_THSY, 0x19);                 //Set Y Threshold to 1.575g
    FXOS8700CQ_WriteByte(obj, PULSE_THSZ, 0x2A);                 //Set Z Threshold to 2.65g
    FXOS8700CQ_WriteByte(obj, PULSE_TMLT,0x50);                  //Set Time Limit for Tap Detection to 50 ms, Normal Mode, No LPF
    FXOS8700CQ_WriteByte(obj, PULSE_LTCY,0xF0);                  // Set Latency Time to 300 ms
    FXOS8700CQ_WriteByte(obj, CTRL_REG4, 0x08);                  //Route INT1 to System
    FXOS8700CQ_WriteByte(obj, CTRL_REG5, 0x08);                  //Route Pulse Interrupt Block to INT1 hardware Pin
    unsigned char CTRL_REG1_Data = FXOS8700CQ_ReadByte(obj, CTRL_REG1);      //Read out the contents of the register
    CTRL_REG1_Data |= 0x01;                                 //Change the value in the register to Active Mode.

    FXOS8700CQ_WriteByte(obj, CTRL_REG1, CTRL_REG1_Data);        //Write in the updated value to put the device in
}

void FXOS8700CQ_ConfigureDoubleTapMode(imu_FXOS8700CQ_t * obj)
{
    FXOS8700CQ_WriteByte(obj, CTRL_REG1, 0x08);                          //400 Hz, Standby Mode
    FXOS8700CQ_WriteByte(obj, PULSE_CFG, 0x2A);                          //Enable X, Y and Z Double Pulse with DPA = 0 no double pulse abort
    FXOS8700CQ_WriteByte(obj, PULSE_THSX, 0x08);                         //Set X Threshold to 3g
    FXOS8700CQ_WriteByte(obj, PULSE_THSY, 0x08);                         //Set Y Threshold to 3g
    FXOS8700CQ_WriteByte(obj, PULSE_THSZ, 0x03);                         //Set Z Threshold to 5g
    FXOS8700CQ_WriteByte(obj, PULSE_TMLT,0x30);                          //60 ms Note: 400 Hz ODR, Time step is 1.25 ms per step
    FXOS8700CQ_WriteByte(obj, PULSE_LTCY,0x50);                          //200 ms Set Latency Time to 200 ms
    FXOS8700CQ_WriteByte(obj, PULSE_WIND,0x78);                          //300 ms Set Time Window for second tap to 300 ms
    FXOS8700CQ_WriteByte(obj, CTRL_REG4, 0x08);                          //Enable Pulse Interrupt in System CTRL_REG4
    FXOS8700CQ_WriteByte(obj, CTRL_REG5, 0x08);                          //Route Pulse Interrupt to INT1 hardware Pin CTRL_REG5

    unsigned char CTRL_REG1_Data = FXOS8700CQ_ReadByte(obj, CTRL_REG1);  //Read out the contents of the register
    CTRL_REG1_Data |= 0x01;                                         //Change the value in the register to Active Mode.

    FXOS8700CQ_WriteByte(obj, CTRL_REG1, CTRL_REG1_Data);                //Write in the updated value to put the device in Active Mode.
}

/**
 * takes a measurement of the magnetomoiter and sets the x,y and z inital positoins
 * @param object [description]
 */
void FXOS8700CQ_Set_Origin(imu_FXOS8700CQ_t * obj)
{
    rawdata_t mag_raw;
    delay_ms(30);  // wait after configuring before reading
    FXOS8700CQ_PollMagnetometer(obj ,&mag_raw);
    FXOS8700CQ_StandbyMode (obj);
    FXOS8700CQ_WriteByte(obj, M_CTRL_REG1, (MAG_ACTIVE|M_OSR1_MASK) ); // turns auto calibrate mode off
    obj->x_origin = abs(mag_raw.x);
    obj->y_origin = abs(mag_raw.y);
    obj->z_origin = abs(mag_raw.z);
    obj->start_position = 180.0 * atan2(mag_raw.x, mag_raw.z) / (float)M_PI;
    obj->calibrated = true;
    FXOS8700CQ_ActiveMode (obj);

}

/**
 * Sets teh threshold for the magnetomiter interup as the origin +the threshold value
 * set the imu to interupt on the x and z axis as if the device is upright on the door
 * with no debounce and no latching interupt is on pin 1
 * @param obj Imu object
 */
void FXOS8700CQ_Magnetic_Threshold_Setting(imu_FXOS8700CQ_t * obj)
{
    FXOS8700CQ_StandbyMode (obj);
    uint8_t origin[6] = {0};

    origin[1] = obj->x_origin;
    origin[0] = obj->x_origin >> 8;
    origin[3] = obj->y_origin;
    origin[2] = obj->y_origin >> 8;
    origin[5] = obj->z_origin;
    origin[4] = obj->z_origin >> 8;


    FXOS8700CQ_WriteByte(obj, M_THS_CFG, M_THS_ZEFE | M_THS_YEFE| M_THS_XEFE| M_THS_WAKE_EN | M_THS_INT_EN | M_THS_OAE);
    FXOS8700CQ_WriteByteArray(obj, M_THS_X_MSB, origin, 6);
    FXOS8700CQ_ActiveMode(obj);
}
 /**
  * set up the magnetic vector function of the imu and set up it as an interupt
  * @param obj imu object
  */
void FXOS8700CQ_Magnetic_Vector(imu_FXOS8700CQ_t * obj)
{
    FXOS8700CQ_StandbyMode (obj);
    uint8_t Vector_Threshold[2] = {0};
    uint8_t ref[6] = {0};

    ref[1] = obj->x_origin;
    ref[0] = obj->x_origin >> 8;
    ref[3] = obj->y_origin;
    ref[2] = obj->y_origin >> 8;
    ref[5] = obj->z_origin;
    ref[4] = obj->z_origin >> 8;
    Vector_Threshold[1] = (uint8_t)VECTOR_THRESH ;
    Vector_Threshold[0] = VECTOR_THRESH >> 8 | 0x80;

    //FXOS8700CQ_WriteByte(obj, M_VECM_CFG,0X00); //  reset values in the vec_cfg register
    FXOS8700CQ_WriteByte(obj, M_VECM_CFG, (M_VECM_UPDM_MASK | M_VECM_EN_MASK| M_VECM_WAKE_EN_MASK| M_VECM_INIT_EN_MASK));
    FXOS8700CQ_WriteByteArray(obj, M_VECM_THS_MSB, Vector_Threshold, 2);
    FXOS8700CQ_WriteByteArray(obj, M_VECM_INITX_MSB, ref, 6);
    FXOS8700CQ_WriteByte(obj,M_VECM_CNT, M_VECTOR_DBNCE);
    FXOS8700CQ_ActiveMode (obj);
}


/**
 * find the current heading of the compass using the x and z axis
 * @param  obj iu object
 * @return     returns the compass angle
 */
int16_t FXOS8700CQ_Get_Heading(imu_FXOS8700CQ_t *obj)
{
    int16_t angle = 0;
    rawdata_t mag_raw;
    FXOS8700CQ_PollMagnetometer(obj, &mag_raw);

    angle = 180.0 * atan2(mag_raw.x, mag_raw.z) / (float)M_PI;


    return angle;

}


static void FXOS8700CQ_Imu_Int_Handler(uint8_t pin, imu_FXOS8700CQ_t * obj)
{
    bool interupt_1 = 0;
    interupt_1 = (bool) GPIO_PinInGet(PIO_PORT(obj->int_2), PIO_PIN(obj->int_2));
    if (interupt_1 == true)
    {
        xQueueSendFromISR(obj->imu_event_queue, &IMU_EVENT_DOOR_OPEN, NULL);
        halSetLed(BOARDLED1);
    }
    else
    {
        xQueueSendFromISR(obj->imu_event_queue, &IMU_EVENT_DOOR_CLOSE, NULL);
        obj->door_state = false;

    }
}


/**
 * [FXOS8700CQ_Init_Interupt  Sets up the interupt for the imu interupt_1
 * @param obj IMU object holing information about the imu and door open / closed state.
 */
void FXOS8700CQ_Init_Interupt (imu_FXOS8700CQ_t * obj)
{
    GPIO_PinModeSet(PIO_PORT(obj->int_1), PIO_PIN(obj->int_1), gpioModeInput, 0);
    GPIO_PinModeSet(PIO_PORT(obj->int_2), PIO_PIN(obj->int_2), gpioModeInput, 0);

    // configure port interrupt
    GPIOINT_Init();
    GPIOINT_CallbackRegisterWithArgs(PIO_PIN(obj->int_2), (GPIOINT_IrqCallbackPtrWithArgs_t) FXOS8700CQ_Imu_Int_Handler, (void *) obj);
    GPIO_ExtIntConfig(PIO_PORT(obj->int_2), PIO_PIN(obj->int_2), PIO_PIN(obj->int_2),
                      true /* raising edge */, true /* falling edge */, true /* enable now */);

}

void FXOS8700CQ_WriteByte(imu_FXOS8700CQ_t * obj, char internal_addr, char value)
{
    uint8_t ret = 0;
    //I2C_WriteByteRegister(reg,value);           //Write value to register
    ret = i2cdrv_master_write_iaddr(obj->i2c_device, FXOS8700CQ_ADDRESS, internal_addr, &value, 1);
}

void FXOS8700CQ_WriteByteArray(imu_FXOS8700CQ_t * obj, char internal_addr, uint8_t* buffer, char length)
{
    uint8_t ret = 0;
    //I2C_WriteByteArray(reg,buffer,length);          //Write values to register
    ret = i2cdrv_master_write_iaddr(obj->i2c_device, FXOS8700CQ_ADDRESS, internal_addr, buffer, length);

}

char FXOS8700CQ_ReadByte(imu_FXOS8700CQ_t * obj, char internal_addr)
{
    uint8_t ret = 0;
    char buffer = 0;
    //return I2C_ReadByteRegister(reg);       //Read register current value
    ret = i2cdrv_master_write_read(obj->i2c_device, FXOS8700CQ_ADDRESS, &internal_addr, 1, &buffer, 1);
    return buffer;
}

void FXOS8700CQ_ReadByteArray(imu_FXOS8700CQ_t * obj, char internal_addr, char *buffer, uint32_t length)
{
    uint8_t ret = 0;
    //I2C_ReadByteArray(reg,buffer,length);   //Read values starting from the reg address
    ret = i2cdrv_master_write_read(obj->i2c_device, FXOS8700CQ_ADDRESS, &internal_addr, 1, buffer, length);
}
