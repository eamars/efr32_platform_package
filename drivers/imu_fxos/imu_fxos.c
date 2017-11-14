/**
 * @brief Implementation of I2C IMU driver
 * @file imu_fxos.c
 * @author Steven O'Rourke
 * @date Oct, 2017
 */

#if USE_FREERTOS == 1

#include <math.h>
#include <stdlib.h>
#include "imu_fxos.h"
#include "drv_debug.h"
#include "delay.h"
#include "gpiointerrupt.h"
#include "bits.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"


/**
 * Intalise the imu and imu i2c instance
 * @param  obj        imu object holds all the necicary infomation about the IMU
 * @param  i2c_device i2c instace for the imu
 * @param  enable     enable pin for the load switch on the imu
 * @param  int_1      immu interupt pin 1
 * @param  int_2      imu interupt pin 2
 * @return            if already intalised will not redinalise the
 */
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

    obj->current_compass = 0;
    obj->current_heading = 0;

	// initialize door state and calibrate state
	obj->calibrated = false;
	obj->door_state = IMU_EVENT_DOOR_CLOSE;

    //intalise que
    obj->imu_event_queue = xQueueCreate(2, sizeof(imu_event_t));

    // configure load switch pins
    GPIO_PinModeSet(PIO_PORT(obj->enable), PIO_PIN(obj->enable), gpioModePushPull, 1);

    obj->initialized = true;

    FXOS8700CQ_WriteByte(obj, CTRL_REG2, RST_MASK);                    //Reset sensor, and wait for reboot to complete
    delay_ms(2);                                        //Wait at least 1ms after issuing a reset before attempting communications
    FXOS8700CQ_StandbyMode(obj);

    while (FXOS8700CQ_ReadByte(obj, CTRL_REG2) & RST_MASK);

    // detect the existence of IMU
    DRV_ASSERT(FXOS8700CQ_ID(obj) == FXOS8700CQ_WHOAMI_VAL);



    FXOS8700CQ_ConfigureAccelerometer(obj);
    FXOS8700CQ_ConfigureMagnetometer(obj);
    //sets inital thresholds (quite leaneant thesholds)
    obj->vector_threshold_open = 55;
    obj->vector_threshold_closed = 40;
    while (FXOS8700CQ_ReadByte(obj, CTRL_REG2) & RST_MASK);
    FXOS8700CQ_Set_Origin(obj);
    FXOS8700CQ_Magnetic_Vector(obj);

    FXOS8700CQ_Init_Interupt(obj);


}

char FXOS8700CQ_ReadStatusReg(imu_FXOS8700CQ_t * obj)
{
    return FXOS8700CQ_ReadByte(obj, IMU_STATUS);
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
    //FXOS8700CQ_WriteByte(obj, XYZ_DATA_CFG, FULL_SCALE_2G);                    // Set FSR of accel to +/-2g
    FXOS8700CQ_WriteByte(obj, CTRL_REG1, (ASLP_RATE_1_56HZ|DATA_RATE_640MS));     // Set ODRs
    //FXOS8700CQ_WriteByte(obj, CTRL_REG2, (SMOD_LOW_POWER|SLPE_MASK));
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
    FXOS8700CQ_WriteByte(obj, M_CTRL_REG1, (MAG_ACTIVE | M_OSR_400_HZ));      // OSR=400, mag only mode (TO, Aug 2012) auto calibrate mode off
    FXOS8700CQ_WriteByte(obj, M_CTRL_REG2, M_HYB_AUTOINC_MASK);       // enable hybrid autoinc
    FXOS8700CQ_WriteByte(obj, M_CTRL_REG3, M_ASLP_OSR_100_HZ);       // OSR =2 in auto sleep mode
    //FXOS8700CQ_WriteByte(obj, M_VECM_CFG, (M_VECM_EN_MASK| M_VECM_WAKE_EN_MASK| M_VECM_CFG));
    FXOS8700CQ_ActiveMode (obj);
}

uint8_t FXOS8700CQ_PollMagnetometer (imu_FXOS8700CQ_t * obj, rawdata_t *mag_data)
{
    char raw[6] = {0};
    FXOS8700CQ_ReadByteArray(obj, M_OUT_X_MSB, raw, 6);
    mag_data->x = (raw[0] << 8) | raw[1];           // Return 16-bit, 2's complement magnetometer data
    mag_data->y = (raw[2] << 8) | raw[3];
    mag_data->z = (raw[4] << 8) | raw[5];
    return (mag_data->x + mag_data->y + mag_data->z);

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
    while (!(FXOS8700CQ_PollMagnetometer(obj ,&mag_raw)));
    FXOS8700CQ_StandbyMode (obj);
    FXOS8700CQ_WriteByte(obj, M_CTRL_REG1, (MAG_ACTIVE | M_OSR_400_HZ) ); // turns auto calibrate mode off
    obj->x_origin = mag_raw.x;
    obj->y_origin = mag_raw.y;
    obj->z_origin = mag_raw.z;
    obj->start_position = 180.0 * atan2(mag_raw.x, mag_raw.z) / (float)M_PI;
	obj->door_state = IMU_EVENT_DOOR_CLOSE;
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
    rawdata_t mag_raw;

    ref[1] = obj->x_origin;
    ref[0] = obj->x_origin >> 8;
    ref[3] = obj->y_origin;
    ref[2] = obj->y_origin >> 8;
    ref[5] = obj->z_origin;
    ref[4] = obj->z_origin >> 8;
    Vector_Threshold[0] = obj->vector_threshold_open >> 8 | 0x80;
    Vector_Threshold[1] = (uint8_t)obj->vector_threshold_open ;


    //FXOS8700CQ_WriteByte(obj, M_VECM_CFG,0X00); //  reset values in the vec_cfg register
    obj->temp = FXOS8700CQ_GetTemperature(obj);
    FXOS8700CQ_WriteByteArray(obj, M_VECM_INITX_MSB, ref, 6);
    FXOS8700CQ_WriteByte(obj, M_VECM_CFG, (M_VECM_UPDM_MASK | M_VECM_EN_MASK| M_VECM_WAKE_EN_MASK| M_VECM_INIT_EN_MASK | M_VECM_INITM_MASK));//values do not update, vector ena bleld,procs wake up,interupt on pin 2 enlabeled
    FXOS8700CQ_WriteByteArray(obj, M_VECM_THS_MSB, Vector_Threshold, 2);
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

/**
 * Handles teh the intterupts associate with the imu. check if the int pin is high or low and sends a door open/ closed to the que
 * @param pin in that is used for detection of the interupt (IMU INT pn2)
 * @param obj IMU object
 */
static void FXOS8700CQ_Imu_Int_Handler(uint8_t pin, imu_FXOS8700CQ_t * obj)
{
    bool interupt_1 = 0;
    uint8_t Vector_Threshold[2] = {0};
    interupt_1 = (bool) GPIO_PinInGet(PIO_PORT(obj->int_2), PIO_PIN(obj->int_2));
    if (abs(xTaskGetTickCountFromISR() - obj->last_call) >= 500 )
    {
        if (interupt_1 == true)
        {
            obj->door_state = IMU_EVENT_DOOR_OPEN;
            //halSetLed(BOARDLED1);
            Vector_Threshold[0] = obj->vector_threshold_closed >> 8 | 0x80;
            Vector_Threshold[1] = (uint8_t)obj->vector_threshold_closed ;


        }
        else
        {
            obj->door_state = IMU_EVENT_DOOR_CLOSE;
            //halClearLed(BOARDLED1);
            Vector_Threshold[0] = obj->vector_threshold_open >> 8 | 0x80;
            Vector_Threshold[1] = (uint8_t)obj->vector_threshold_open ;



        }
        if (obj->door_state != obj->last_event)
        {
            FXOS8700CQ_WriteByteArray(obj, M_VECM_THS_MSB, Vector_Threshold, 2);
            FXOS8700CQ_Door_State_Poll(obj);
            xQueueSendFromISR(obj->imu_event_queue, &obj->door_state,NULL);
            obj->last_event = obj->door_state;
        }
        obj->last_call = xTaskGetTickCountFromISR();
    }
}


/**
 * Polls the magnetomiter and checks for a door open/ closed condition and if a change has occured sends it to the queue
 * @param obj IMU device object
 */
void FXOS8700CQ_Door_State_Poll(imu_FXOS8700CQ_t * obj)
{
    int16_t angle = 0;
    int16_t change = 0;

    angle = FXOS8700CQ_Get_Heading(obj);
    change = abs(angle - obj->start_position);
    change = (change < 180 ? change : 180 - (change % 180));

    // Store the current values in the object.
    obj->current_compass = angle; // The compass angle
    obj->current_heading = change; // The heading away from calibrated angle


//removed so a heading can be gained from the interupt
    // if (!obj->calibrated)
    // {
    //
    //     obj->door_state = IMU_EVENT_CALIBRATING;
    // }
    // else if (change >= POLL_THRESH) // angle that is needed for door to trigger
    // {
    //
	//     obj->door_state = IMU_EVENT_DOOR_OPEN;
    // }
    // else
    // {
	//     obj->door_state = IMU_EVENT_DOOR_CLOSE;
    // }
    // if (obj->door_state != obj->last_event)
    // {
    //     xQueueSend(obj->imu_event_queue, &obj->door_state, portMAX_DELAY);
    //     obj->last_event = obj->door_state;
    // }

}

/**
 * [FXOS8700CQ_Init_Interupt  Sets up the interupt for the imu interupt_1
 * @param obj IMU object holing information about the imu and door open / closed state.
 */
void FXOS8700CQ_Init_Interupt (imu_FXOS8700CQ_t * obj)
{
     // creates the temperature adjusting que(this is here instead of the init as the temp code needs the int to initalised first)
    xTaskCreate((void *) ImuTempAdjustment, "temp_mon", 200, obj, 2, &obj->ImuTempHandler);

    GPIO_PinModeSet(PIO_PORT(obj->int_1), PIO_PIN(obj->int_1), gpioModeInput, 0);
    GPIO_PinModeSet(PIO_PORT(obj->int_2), PIO_PIN(obj->int_2), gpioModeInput, 0);

    // configure port interrupt
    GPIOINT_Init();
    GPIOINT_CallbackRegisterWithArgs(PIO_PIN(obj->int_2), (GPIOINT_IrqCallbackPtrWithArgs_t) FXOS8700CQ_Imu_Int_Handler, (void *) obj);
    GPIO_ExtIntConfig(PIO_PORT(obj->int_2), PIO_PIN(obj->int_2), PIO_PIN(obj->int_2),
                      true /* raising edge */, true /* falling edge */, true /* enable now */);

}

/**
 * Takes a measurement of the temperatue and check to see if it has changed more than 2 degrees. if the
 * device has changed and the door is closed the device reclaibrates
 * obj->temp is the last temp that was measure_period_mstemperature_imu is the current temperature
 * @param obj imu object
 */
static void ImuTempAdjustment(imu_FXOS8700CQ_t * obj)
{
	// initialize the task tick handler
	portTickType xLastWakeTime;
    int16_t temperature_imu;
    int16_t temp_change;
    bool interrupt_check;
	// get last execution time
	xLastWakeTime = xTaskGetTickCount();
	while (1)
	{
        temperature_imu = FXOS8700CQ_GetTemperature(obj);
        temp_change = temperature_imu - obj->temp;
        if (abs(temp_change) > 3)
        {
            //FXOS8700CQ_Set_Origin(obj);
            obj->x_origin = obj->x_origin - temp_change;
            obj->y_origin = obj->y_origin - temp_change;
            obj->z_origin = obj->z_origin + (temp_change*2);
            interrupt_check = (bool) GPIO_PinInGet(PIO_PORT(obj->int_2), PIO_PIN(obj->int_2));
            GPIOINT_CallbackUnRegister(PIO_PIN(obj->int_2));
            FXOS8700CQ_Magnetic_Vector(obj);
            delay_ms(1000);
            GPIOINT_CallbackRegisterWithArgs(PIO_PIN(obj->int_2), (GPIOINT_IrqCallbackPtrWithArgs_t) FXOS8700CQ_Imu_Int_Handler, (void *) obj);
            if (interrupt_check !=(bool) GPIO_PinInGet(PIO_PORT(obj->int_2), PIO_PIN(obj->int_2)))
            {
                FXOS8700CQ_Imu_Int_Handler(PIO_PIN(obj->int_2), obj);
            }
        }
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(30000));
	}
}


void FXOS8700CQ_Caclculate_Vector(imu_FXOS8700CQ_t * obj)
{
    rawdata_t mag_raw;
    FXOS8700CQ_PollMagnetometer(obj,&mag_raw);
    uint16_t mag_x = mag_raw.x;
    uint16_t mag_y = mag_raw.y;
    uint16_t mag_z = mag_raw.z;

    uint16_t origin_x = obj->x_origin;
    uint16_t origin_y = obj->y_origin;
    uint16_t origin_z = obj->z_origin;

    int32_t dx = mag_x - origin_x;
    int32_t dy = mag_y - origin_y;
    int32_t dz = mag_z - origin_z;


    obj->vector = sqrt((dx*dx) + (dy*dy) + (dz*dz));

}

void FXOS8700CQ_Calibrate(imu_FXOS8700CQ_t * obj)
{
    //Finds the current vector  and adds 5 then a  scaling factor to get the open and closed threshold constants.
    FXOS8700CQ_Caclculate_Vector(obj);
    obj->vector_threshold_open = (obj->vector +5)*1.5;
    obj->vector_threshold_closed = (obj->vector + 5)*1.3;
    FXOS8700CQ_Magnetic_Vector(obj);
    obj->calibrated = true;
}


void FXOS8700CQ_ModifyBytes(imu_FXOS8700CQ_t * obj, char internal_addr, char value, char mask)
{
    char reg = 0;

    // perform a write on modify operation
    reg = FXOS8700CQ_ReadByte(obj, internal_addr);

    BITS_MODIFY(reg, value, mask);

    // write back
    FXOS8700CQ_WriteByte(obj, internal_addr, reg);
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

#endif // USE_FREERTOS == 1
