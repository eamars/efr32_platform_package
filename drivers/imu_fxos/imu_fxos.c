/**
 * @brief Implementation of I2C IMU driver
 * @file imu_fxos.c
 * @author Steven O'Rourke
 * @date Oct, 2017
 */

/**
 * Code will set thresholds ont he imu to use interupt line 2 on the imu. the mmicro controller uses purly weather this line is high to decice if the door is open
 * an advancement on this would be to use the acceleromiter or some other sort of signal alnasis of the vecto value/ indivicual values to decied if the door is open
 *
 * The device will calibrate on the first start if the device has not yet been calibrated or when a calibrate call is made. In the future values such as the temperature coeffcient will be
 * remembered by the micro controller eeprom even when the device is reset this way it can get an idea of its compoents.
 *
 * THe tempertarue task happens every 20s and will change stuff if the temperature has changed. read more on this at the tempeartuer adjustment task.
 */


#if USE_FREERTOS == 1

#include <math.h>
#include <stdlib.h>

#include "imu_fxos.h"
#include "imu_fxos_regs.h"

#include "drv_debug.h"
#include "delay.h"
#include "gpiointerrupt.h"
#include "bits.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"

SemaphoreHandle_t xSemaphoreOriginReset = NULL;

void FXOS8700CQ_vTaskResetOrigin(imu_FXOS8700CQ_t * obj);
void FXOS8700CQ_vTaskCheckTemp(imu_FXOS8700CQ_t * obj);

void FXOS8700CQ_Reset_Origin(imu_FXOS8700CQ_t * obj);

void FXOS8700CQ_SetThresholds(imu_FXOS8700CQ_t * obj)
{
    obj->origin.vector_threshold_open = (obj->vector * 1.5) + 60;
    obj->origin.vector_threshold_closed = (obj->vector * 1.2) + 40;
}

/**
 * Intalise the imu and imu i2c instance
 * @param  obj        imu object holds all the necicary infomation about the IMU
 * @param  i2c_device i2c instace for the imu
 * @param  enable     enable pin for the load switch on the imu
 * @param  int_1      immu interupt pin 1
 * @param  int_2      imu interupt pin 2
 * @param  address    I2C slave address, depending on hardware configuration
 * @return            if already intalised will not redinalise the
 */
void  FXOS8700CQ_Initialize(imu_FXOS8700CQ_t * obj, i2cdrv_t * i2c_device, pio_t enable, pio_t int_1, pio_t int_2, uint8_t address,
                            imu_backup_t * backup_pointer)
{
    uint16_t i = 0;
    // sanity check for pointers
    DRV_ASSERT(obj);
    DRV_ASSERT(i2c_device);

    // do not reinitialize the imu driver
    if (obj->initialized)
    {
        return;
    }

    // assign i2c object
    obj->i2c_device = i2c_device;

    // store enable pins
    obj->enable = enable;

    // assign interrupt pins
    obj->int_1 = int_1;
    obj->int_2 = int_2;

    // assign hardware slave address
    obj->i2c_slave_addr = address;

    obj->current_compass = 0;
    obj->current_heading = 0;
    obj->origin.x_tmp_coef = 1.405; //initial guess for scaling factor of the imu z direction with temperature
    obj->origin.y_tmp_coef = 1.858;
    obj->origin.z_tmp_coef = 10.631;

	// initialize door state and calibrate state
	obj->door_state = IMU_EVENT_DOOR_CLOSE;

    //intalise queue
    obj->imu_event_queue = xQueueCreate(2, sizeof(imu_event_t));

    // configure load switch pins
    GPIO_PinModeSet(PIO_PORT(obj->enable), PIO_PIN(obj->enable), gpioModePushPull, 1);

    obj->initialized = true;

    FXOS8700CQ_WriteByte(obj, CTRL_REG2, RST_MASK);                    //Reset sensor, and wait for reboot to complete
    delay_ms(20);                                        //Wait at least 1ms after issuing a reset before attempting communications
    FXOS8700CQ_StandbyMode(obj);

    while (FXOS8700CQ_ReadByte(obj, CTRL_REG2) & RST_MASK);

    // detect the existence of IMU
    DRV_ASSERT(FXOS8700CQ_ID(obj) == FXOS8700CQ_WHOAMI_VAL);

    FXOS8700CQ_ConfigureAccelerometer(obj);
    FXOS8700CQ_ConfigureMagnetometer(obj);
    //sets inital thresholds (quite leaneant thesholds)

    while (FXOS8700CQ_ReadByte(obj, CTRL_REG2) & RST_MASK);

    // creates the temperature adjusting que(this is here instead of the init as the temp code needs the int to initalised first)
    //xTaskCreate((void *) ImuTempAdjustment, "temp_mon", 500, obj, 2, &obj->ImuTempHandler);
    FXOS8700CQ_ActiveMode(obj);

    FXOS8700CQ_Calibrate(obj);


    xTaskCreate((void *) FXOS8700CQ_vTaskResetOrigin, "Reset_Origin", 300, obj, 1, NULL);
    xTaskCreate((void *) FXOS8700CQ_vTaskCheckTemp, "Check_Temp", 300, obj, 1, NULL);

    FXOS8700CQ_Init_Interrupt(obj);
}

/**
 * calibrates the thresholds dependant on the current vector of the imu. this can be used to pull the gate to the maximum allowed and claibrating. show
 * @param obj imu object
 */
void FXOS8700CQ_Calibrate(imu_FXOS8700CQ_t * obj)
{
    uint8_t i = 0;
    //Finds the current vector  and adds 5 then a  scaling factor to get the open and closed threshold constants.

    while(!FXOS8700CQ_PollMagnetometer(obj));

    delay_ms(500);
    for (i =0;i <= 2; i++)
    {
        FXOS8700CQ_GetTemperature(obj);
        delay_ms(5);

    }

    FXOS8700CQ_Set_Origin(obj);
    FXOS8700CQ_Calculate_Vector(obj);
    obj->origin.vector = obj->origin.vector = sqrt((obj->origin.x_origin*obj->origin.x_origin) + (obj->origin.y_origin*obj->origin.y_origin)
                                                   + (obj->origin.z_origin*obj->origin.z_origin));

    obj->last_temp = obj->origin.calibration_temp;

    FXOS8700CQ_SetThresholds(obj);
    FXOS8700CQ_Magnetic_Vector(obj);
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

char FXOS8700CQ_ID (imu_FXOS8700CQ_t * obj)
{
     char id  = FXOS8700CQ_ReadByte(obj, WHO_AM_I);
     return id;
}

void FXOS8700CQ_ConfigureAccelerometer(imu_FXOS8700CQ_t * obj)
{
    FXOS8700CQ_StandbyMode (obj);
    //FXOS8700CQ_WriteByte(obj, XYZ_DATA_CFG, FULL_SCALE_2G);                    // Set FSR of accel to +/-2g
    FXOS8700CQ_WriteByte(obj, CTRL_REG1, (ASLP_RATE_1_56HZ | DATA_RATE_640MS));  // Set ODRs
    //FXOS8700CQ_WriteByte(obj, CTRL_REG2, (SMOD_LOW_POWER|SLPE_MASK));
    FXOS8700CQ_WriteByte(obj, CTRL_REG3, (IPOL_MASK));                           // Interrupts are active high
    //FXOS8700CQ_WriteByte(obj, CTRL_REG4, INT_EN_DRDY_MASK );                   // Enable interrupts for DRDY (TO, Aug 2012)
    //FXOS8700CQ_WriteByte(obj, CTRL_REG5, );                                    // Don't write to this reg - all interrupts sent to line 2 by default
    FXOS8700CQ_ActiveMode (obj);
}

void FXOS8700CQ_ConfigureMagnetometer(imu_FXOS8700CQ_t * obj)
{
    FXOS8700CQ_StandbyMode (obj);
    FXOS8700CQ_WriteByte(obj, M_CTRL_REG1, (MAG_ACTIVE | M_OSR_800_HZ));         // OSR=1.56HZ, mag only mode (TO, Aug 2012) auto calibrate mode off
    FXOS8700CQ_WriteByte(obj, M_CTRL_REG2, M_HYB_AUTOINC_MASK);                  // enable hybrid autoinc
    FXOS8700CQ_WriteByte(obj, M_CTRL_REG3, M_ASLP_OSR_100_HZ);                   // OSR =2 in auto sleep mode
    //FXOS8700CQ_WriteByte(obj, M_VECM_CFG, (M_VECM_EN_MASK| M_VECM_WAKE_EN_MASK| M_VECM_CFG));
    FXOS8700CQ_ActiveMode (obj);
}

uint8_t FXOS8700CQ_PollMagnetometer (imu_FXOS8700CQ_t * obj)
{
    float x;
    float y;
    float z;

    float x_raw;
    float y_raw;
    float z_raw;

    uint8_t i = 0;
    char raw[6] = {0};
    //loop through to insure that there are no zerod values from a mishap either efr or imu side or in the twi line.
    obj->temp = FXOS8700CQ_GetTemperature(obj);

    FXOS8700CQ_ReadByteArray(obj, M_OUT_X_MSB, raw, 6);

    x_raw = (raw[0] << 8) | raw[1];
    y_raw = (raw[2] << 8) | raw[3];
    z_raw = (raw[4] << 8) | raw[5];

    // Don't know why but needed to use x_raw etc as an intermediate variable for it to work
    obj->rawmagdata.x = x_raw;
    obj->rawmagdata.y = y_raw;
    obj->rawmagdata.z = z_raw;


    x = x_raw; // this is  - ((obj->temp - obj->origin.calibration_temp) * obj->origin.x_tmp_coef)
    y = y_raw;
    z = z_raw;

    obj->magdata.x = x;
    obj->magdata.y = y;
    obj->magdata.z = z;

    if ((obj->magdata.x !=0) && (obj->magdata.y !=0) && (obj->magdata.z != 0))
    {
        return (obj->magdata.x + obj->magdata.y + obj->magdata.z);
    } else
    {
        return 0;
    }
}

char FXOS8700CQ_GetTemperature(imu_FXOS8700CQ_t * obj)
{
    char temp = FXOS8700CQ_ReadByte(obj, TEMP); // Should actually multiply by 0.96
    return temp;
}

/**
 * Sets the origin - needed for the magnetometer vector magnitude interrupt
 * @param obj imu object
 */
void FXOS8700CQ_Set_Origin(imu_FXOS8700CQ_t * obj)
{
    while (!(FXOS8700CQ_PollMagnetometer(obj)));
    FXOS8700CQ_StandbyMode (obj);

    FXOS8700CQ_WriteByte(obj, M_CTRL_REG1, (MAG_ACTIVE | M_OSR_800_HZ) ); // turns auto calibrate mode off
    obj->origin.x_origin = obj->rawmagdata.x;
    obj->origin.y_origin = obj->rawmagdata.y;
    obj->origin.z_origin = obj->rawmagdata.z;

    obj->origin.x_origin_compensated =  obj->rawmagdata.x;
    obj->origin.y_origin_compensated =  obj->rawmagdata.y;
    obj->origin.z_origin_compensated =  obj->rawmagdata.z;


    obj->start_position = 180.0 * atan2(obj->origin.x_origin, obj->origin.z_origin) / (float)M_PI;
	obj->door_state = IMU_EVENT_DOOR_CLOSE;
    FXOS8700CQ_ActiveMode (obj);
    obj->origin.calibration_temp = FXOS8700CQ_GetTemperature(obj); //Seems to be 0 on first read
}

/**
 * Resets the origin - taking an average of the last value and the current value to avoid drastic changes
 * @param obj imu object
 */
void FXOS8700CQ_Reset_Origin(imu_FXOS8700CQ_t * obj)
{
    while (!(FXOS8700CQ_PollMagnetometer(obj)));
    FXOS8700CQ_StandbyMode (obj);

    FXOS8700CQ_WriteByte(obj, M_CTRL_REG1, (MAG_ACTIVE | M_OSR_800_HZ) ); // turns auto calibrate mode off
    obj->origin.x_origin = (obj->rawmagdata.x + obj->origin.x_origin) / 2; // Stops any crazy changes taking place
    obj->origin.y_origin = (obj->rawmagdata.y + obj->origin.y_origin) / 2;
    obj->origin.z_origin = (obj->rawmagdata.z + obj->origin.z_origin) / 2;

    obj->origin.x_origin_compensated =  obj->origin.x_origin;
    obj->origin.y_origin_compensated =  obj->origin.y_origin;
    obj->origin.z_origin_compensated =  obj->origin.z_origin;


    obj->start_position = 180.0 * atan2(obj->origin.x_origin, obj->origin.z_origin) / (float)M_PI;
    obj->door_state = IMU_EVENT_DOOR_CLOSE;
    FXOS8700CQ_ActiveMode (obj);
    obj->origin.calibration_temp = FXOS8700CQ_GetTemperature(obj); //Seems to be 0 on first read
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

    ref[1] = (obj->origin.x_origin);
    ref[0] = (obj->origin.x_origin) >> 8;
    ref[3] = (obj->origin.y_origin);
    ref[2] = (obj->origin.y_origin) >> 8;
    ref[5] = (obj->origin.z_origin);
    ref[4] = (obj->origin.z_origin) >> 8;
    Vector_Threshold[0] = obj->origin.vector_threshold_open >> 8 | 0x80;
    Vector_Threshold[1] = (uint8_t)obj->origin.vector_threshold_open;

    FXOS8700CQ_WriteByte(obj, M_VECM_CFG,0X00); //  reset values in the vec_cfg register

    FXOS8700CQ_WriteByteArray(obj, M_VECM_INITX_MSB, ref, 6);
    FXOS8700CQ_WriteByte(obj, M_VECM_THS_MSB, Vector_Threshold[0]);
    FXOS8700CQ_WriteByte(obj, M_VECM_THS_LSB, Vector_Threshold[1]);
    FXOS8700CQ_WriteByte(obj,M_VECM_CNT, M_VECTOR_DBNCE);
    FXOS8700CQ_WriteByte(obj, M_VECM_CFG, (M_VECM_INITM_MASK | M_VECM_UPDM_MASK | M_VECM_EN_MASK| M_VECM_WAKE_EN_MASK | M_VECM_INT_EN_MASK));//values do not update, vector ena bleld,procs wake up,interupt on pin 2 enlabeled

    FXOS8700CQ_ActiveMode (obj);
}

void FXOS8700CQ_Magnetic_Temperature_Compensation(imu_FXOS8700CQ_t * obj)
{
    FXOS8700CQ_StandbyMode (obj);
    uint8_t Vector_Threshold[2] = {0};
    uint8_t ref[6] = {0};

    // Calculates what the origin would be if the device was recalibrated at the current temp
    if (obj->origin.x_origin >= 0)
    {
        obj->origin.x_origin_compensated = obj->origin.x_origin + ((obj->temp - obj->origin.calibration_temp) * obj->origin.x_tmp_coef);
    } else
    {
        obj->origin.x_origin_compensated = obj->origin.x_origin - ((obj->temp - obj->origin.calibration_temp) * obj->origin.x_tmp_coef);
    }
    if (obj->origin.y_origin >= 0)
    {
        obj->origin.y_origin_compensated = obj->origin.y_origin + ((obj->temp - obj->origin.calibration_temp) * obj->origin.y_tmp_coef);
    } else
    {
        obj->origin.y_origin_compensated = obj->origin.y_origin - ((obj->temp - obj->origin.calibration_temp) * obj->origin.y_tmp_coef);
    }
    if (obj->origin.z_origin >= 0)
    {
        obj->origin.z_origin_compensated = obj->origin.z_origin + ((obj->temp - obj->origin.calibration_temp) * obj->origin.z_tmp_coef);
    } else
    {
        obj->origin.z_origin_compensated = obj->origin.z_origin - ((obj->temp - obj->origin.calibration_temp) * obj->origin.z_tmp_coef);
    }

    ref[1] = (obj->origin.x_origin_compensated);
    ref[0] = (obj->origin.x_origin_compensated) >> 8;
    ref[3] = (obj->origin.y_origin_compensated);
    ref[2] = (obj->origin.y_origin_compensated) >> 8;
    ref[5] = (obj->origin.z_origin_compensated);
    ref[4] = (obj->origin.z_origin_compensated) >> 8;

    obj->origin.vector = sqrt((obj->origin.x_origin_compensated*obj->origin.x_origin_compensated) + (obj->origin.y_origin_compensated*obj->origin.y_origin_compensated)
                              + (obj->origin.z_origin_compensated*obj->origin.z_origin_compensated));

    FXOS8700CQ_WriteByteArray(obj, M_VECM_INITX_MSB, ref, 6); // Sets the new origin

    FXOS8700CQ_ActiveMode (obj);
}

/**
 * Resets the origin after a door closed event - helps deal with temp changes
 * @param obj IMU object
 */
void FXOS8700CQ_vTaskResetOrigin(imu_FXOS8700CQ_t * obj)
{
    xSemaphoreOriginReset = xSemaphoreCreateBinary();
    for ( ;; ) {
        xSemaphoreTake(xSemaphoreOriginReset, 0xFFFF);
        // Will leave this off for now, if the origin resets at the wrong time it can seriously muck things up.
//        delay_ms(1000);
//        FXOS8700CQ_Reset_Origin(obj);
    }
}

/**
 * Checks the temperature periodically and recalibrates if necessary - this is needed incase the door is left in the open
 * state for a long time and the temperature changes, causing the vector to drift further from the origin and potentially
 * meaning the next close event won't be detected.
 * @param obj IMU object
 */
void FXOS8700CQ_vTaskCheckTemp(imu_FXOS8700CQ_t * obj)
{
    // initialize the task tick handler
    portTickType xLastWakeTime;

    xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        obj->temp = FXOS8700CQ_GetTemperature(obj);
        if(obj->temp != obj->last_temp)
        {
            //do temp compensation
            FXOS8700CQ_Magnetic_Temperature_Compensation(obj);
        }
        obj->last_temp = obj->temp;
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10000));
    }
}

/**
 * Handles the the intterupts associate with the imu. check if the int pin is high or low and sends a door open/ closed to the que
 * @param pin in that is used for detection of the interupt (IMU INT pn2)
 * @param obj IMU object
 */
static void FXOS8700CQ_Imu_Int_Handler(uint8_t pin, imu_FXOS8700CQ_t * obj)
{
    obj->last_event = obj->door_state;
    uint8_t Vector_Threshold[2] = {0};
    if (abs(xTaskGetTickCountFromISR() - obj->last_call) >= 500 ) // essentialy debouncing wont let the state change constantly
    {
        // Was this because of a temp change?
        FXOS8700CQ_Calculate_Vector(obj);
        obj->temp = FXOS8700CQ_GetTemperature(obj);
        if(obj->temp != obj->last_temp)
        {
            // do temp compensation - this basically changes the reference value (origin) programmed into the IMU
            FXOS8700CQ_Magnetic_Temperature_Compensation(obj);
        }

        FXOS8700CQ_Calculate_Vector(obj); // Calculate the vector with the new origin value

        // Now check whether or not the interrupt would have still occurred after the temp compensation - if so, then
        // a door event has taken place
        if ((obj->door_state == IMU_EVENT_DOOR_CLOSE) && (obj->vector > obj->origin.vector_threshold_open))
        {
            obj->door_state = IMU_EVENT_DOOR_OPEN;
            Vector_Threshold[0] = obj->origin.vector_threshold_closed >> 8 | 0x80;
            Vector_Threshold[1] = (uint8_t)obj->origin.vector_threshold_closed ;
        }

        else if ((obj->door_state == IMU_EVENT_DOOR_OPEN) && (obj->vector < obj->origin.vector_threshold_closed))
        {
            obj->door_state = IMU_EVENT_DOOR_CLOSE;
            Vector_Threshold[0] = obj->origin.vector_threshold_open >> 8 | 0x80;
            Vector_Threshold[1] = (uint8_t)obj->origin.vector_threshold_open ;
            //xSemaphoreGiveFromISR(xSemaphoreOriginReset, NULL); // Would unblock the ResetOrigin task - but still needs work
        }

        if (obj->door_state != obj->last_event)
        {
            FXOS8700CQ_WriteByteArray(obj, M_VECM_THS_MSB, Vector_Threshold, 2);

            xQueueSendFromISR(obj->imu_event_queue, &obj->door_state,NULL);
            obj->last_event = obj->door_state;
        }

        obj->last_temp = obj->temp;

        obj->last_call = xTaskGetTickCountFromISR();
    }
}

/**
 * [FXOS8700CQ_Init_Interupt  Sets up the interupt for the imu interupt_1
 * @param obj IMU object holing information about the imu and door open / closed state.
 */
void FXOS8700CQ_Init_Interrupt (imu_FXOS8700CQ_t * obj)
{
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
 *
 * @param obj imu object
 */
//static void ImuTempAdjustment(imu_FXOS8700CQ_t * obj)
//{
//	// initialize the task tick handler
//	portTickType xLastWakeTime;
//    int8_t temperature_imu = 0;
//    int8_t last_temp_change = 0;
//
//    int16_t max_z = 0;
//
//
//    bool interrupt_check;
//	// get last execution time
//	xLastWakeTime = xTaskGetTickCount();
//	while (1)
//	{
//        obj->temp = FXOS8700CQ_GetTemperature(obj);
//
//        if(obj->temp != obj->last_temp)
//        {
//            //change origin
//            //
//        }
//
//
//        temperature_imu = FXOS8700CQ_GetTemperature(obj);
//
//        obj->temp_change = temperature_imu - obj->temp;
//        FXOS8700CQ_PollMagnetometer(obj);
//
//        if ((max_z < obj->magdata.z) && (obj->door_state == IMU_EVENT_DOOR_CLOSE))
//        {
//            max_z = obj->magdata.z;
//        }
//
//
//        if ((abs(obj->temp_change) >= 1) && (abs(obj->temp_change) < 10)) // the less than 20 is because it should really never get here and i think it may on the odd ocaion of crossing 0 degrees it stuffs up some times
//        {
//            // checks the line status then unregisters the line as it will drop during the reset.
//            interrupt_check = (bool) GPIO_PinInGet(PIO_PORT(obj->int_2), PIO_PIN(obj->int_2));
//            GPIOINT_CallbackUnRegister(PIO_PIN(obj->int_2));
//            //While the door is closed the device tries to work out a factor that the z axis will change with temperature by. this also
//            //resets the orign.    This whole thing is very depenant on asuing tha the door is actually closed and not just almost closed/
//            //when the door is open the device will either use the inital guess or the calcualted value of the temperautre coeffcient.
//            if (obj->door_state == IMU_EVENT_DOOR_CLOSE)
//            {
//                if (abs(obj->temp_change + last_temp_change) >= 2 )
//                {
//                    FXOS8700CQ_Cal_Scaling(obj,obj->temp_change, max_z);
//                }
//                FXOS8700CQ_Set_Origin(obj);
//            }
//            else
//            {
//                obj->origin.z_origin = obj->origin.z_origin + (obj->origin.z_tmp_coef*obj->temp_change) ; // when this is set and open the device sets the threshold to the open threshold as it is larger and will allow for some temperature leway to get closed.
//            }
//
//
//            FXOS8700CQ_WriteByte(obj, CTRL_REG2, RST_MASK);                    //Reset sensor, and wait for reboot to complete. // for some raeson resseting the deviec made it more predicatable in the x and y axis during a temp change
//            delay_ms(2);                                        //Wait at least 1ms after issuing a reset before attempting communications
//            FXOS8700CQ_StandbyMode(obj);
//
//            while (FXOS8700CQ_ReadByte(obj, CTRL_REG2) & RST_MASK);
//
//            FXOS8700CQ_ConfigureAccelerometer(obj);
//            FXOS8700CQ_ConfigureMagnetometer(obj);
//            while (FXOS8700CQ_ReadByte(obj, CTRL_REG2) & RST_MASK);
//            // change the orgin of the device depenant on temperature
//            FXOS8700CQ_Magnetic_Vector(obj);
//            FXOS8700CQ_ActiveMode(obj);
//            delay_ms(1000);
//            GPIOINT_CallbackRegisterWithArgs(PIO_PIN(obj->int_2), (GPIOINT_IrqCallbackPtrWithArgs_t) FXOS8700CQ_Imu_Int_Handler, (void *) obj);
//            // checks if the interupt line has been changed and if it has go to the intterupt handler
//            if (interrupt_check !=(bool) GPIO_PinInGet(PIO_PORT(obj->int_2), PIO_PIN(obj->int_2)))
//            {
//                FXOS8700CQ_Imu_Int_Handler(PIO_PIN(obj->int_2), obj);
//            }
//            // Sets the temp to old temp and resets the z max and remembers the last emp change and temperature.
//            obj->temp = temperature_imu;
//            last_temp_change = obj->temp_change;
//            max_z = 0;
//        }
//		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2000));
//	}
//}

/**
 * finds the temperature scaling factor and used very simple smoothing to make sure to outlandish valeus are achived. starts off with a gues of 3
 * every thing in this is made to be a float as it was having problems with it befor.
 * @param obj         imu object
 * @param temp_change the change in temperatue since the device last updated the z value
 * @param max_z   largest value of z found in each section of the temperature graph
 */
void FXOS8700CQ_Cal_Scaling(imu_FXOS8700CQ_t *obj,int16_t temp_change, int16_t max_z)
{
    float new_scaler;

    new_scaler = (float)((max_z - obj->old_magdata.z) /(float)temp_change);
    obj->origin.z_tmp_coef = ((obj->origin.z_tmp_coef * 29.0 ) + new_scaler) /30.0;

    obj->old_magdata.z = max_z;
}

/**
 * poll the magnetomiter and calcutes the change in vector (the same way the the imu does and intterupt is driven by)
 * @param obj imu object
 */
void FXOS8700CQ_Calculate_Vector(imu_FXOS8700CQ_t * obj)
{
    FXOS8700CQ_PollMagnetometer(obj);
    int16_t mag_x = abs(obj->rawmagdata.x); // Absolutes used as only concerned with magnitudes
    int16_t mag_y = abs(obj->rawmagdata.y);
    int16_t mag_z = abs(obj->rawmagdata.z);

    int16_t origin_x = abs(obj->origin.x_origin_compensated);
    int16_t origin_y = abs(obj->origin.y_origin_compensated);
    int16_t origin_z = abs(obj->origin.z_origin_compensated);

    int32_t dx = mag_x - origin_x;
    int32_t dy = mag_y - origin_y;
    int32_t dz = mag_z - origin_z;


    obj->vector = sqrt((dx*dx) + (dy*dy) + (dz*dz));
}

void FXOS8700CQ_WriteByte(imu_FXOS8700CQ_t * obj, char internal_addr, char value)
{
    uint8_t ret = 0;
    //I2C_WriteByteRegister(reg,value);           //Write value to register
    ret = i2cdrv_master_write_iaddr(obj->i2c_device, obj->i2c_slave_addr, internal_addr, &value, 1);
}

void FXOS8700CQ_WriteByteArray(imu_FXOS8700CQ_t * obj, char internal_addr, uint8_t* buffer, char length)
{
    //I2C_WriteByteArray(reg,buffer,length);          //Write values to register
    i2cdrv_master_write_iaddr(obj->i2c_device, obj->i2c_slave_addr, internal_addr, buffer, length);
}

char FXOS8700CQ_ReadByte(imu_FXOS8700CQ_t * obj, char internal_addr)
{
    uint8_t ret = 0;
    char buffer = 0;
    //return I2C_ReadByteRegister(reg);       //Read register current value
    ret = i2cdrv_master_write_read(obj->i2c_device, obj->i2c_slave_addr, &internal_addr, 1, &buffer, 1);
    return buffer;
}

void FXOS8700CQ_ReadByteArray(imu_FXOS8700CQ_t * obj, char internal_addr, char *buffer, uint32_t length)
{
    uint8_t ret = 0;
    //I2C_ReadByteArray(reg,buffer,length);   //Read values starting from the reg address
    ret = i2cdrv_master_write_read(obj->i2c_device, obj->i2c_slave_addr, &internal_addr, 1, buffer, length);
}

//Used outside of this driver
/**
 * Polls the magnetometer and checks for a door change in angle
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
}

/**
 * find the current heading of the compass using the x and z axis
 * @param  obj iu object
 * @return     returns the compass angle
 */
int16_t FXOS8700CQ_Get_Heading(imu_FXOS8700CQ_t *obj)
{
    int16_t angle = 0;
    FXOS8700CQ_PollMagnetometer(obj);

    angle = 180.0 * atan2(obj->magdata.x, obj->magdata.z) / (float)M_PI;

    return angle;
}

/**
 * calculates the angle between the curent vector and the origin
 * @param obj imu object
 */
void FXOS8700CQ_Vector_Angle(imu_FXOS8700CQ_t* obj)
{
    FXOS8700CQ_PollMagnetometer(obj);
    int16_t mag_x = obj->magdata.x;
    int16_t mag_y = obj->magdata.y;
    int16_t mag_z = obj->magdata.z;

    int16_t origin_x = obj->origin.x_origin;
    int16_t origin_y = obj->origin.y_origin;
    int16_t origin_z = obj->origin.z_origin;

    uint8_t r2d2 = (180/(float)M_PI); //radins to degrees

    int32_t current_vector = sqrt((mag_x * mag_x) + (mag_y * mag_y) + (mag_z * mag_z));
    int32_t origin_vector  = sqrt((origin_x * origin_x) + (origin_y * origin_y) + (origin_z * origin_z));
    int32_t dot_product    = (mag_x * origin_x) + (mag_y * origin_y) + (mag_z * origin_z);
    float fraction         = dot_product/(float)(current_vector * origin_vector);
    float rad              = acos(fraction);

    obj->vector_angle = (int16_t)(r2d2 * rad);
}

#endif // USE_FREERTOS == 1
