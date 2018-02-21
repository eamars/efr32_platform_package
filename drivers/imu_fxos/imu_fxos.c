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
#include "board.h"

SemaphoreHandle_t xSemaphoreOriginReset = NULL;

void FXOS8700CQ_vTaskResetOrigin(imu_FXOS8700CQ_t * obj);
void FXOS8700CQ_vTaskCheckTemp(imu_FXOS8700CQ_t * obj);

void FXOS8700CQ_Reset_Origin(imu_FXOS8700CQ_t * obj);
void FXOS8700CQ_Save_Origin(imu_FXOS8700CQ_t * obj);


/**
 * Sets the open and closed threshold values in IMU object, may have some temperature dependence, needs testing
 * @param obj imu object
 */
void FXOS8700CQ_SetThresholds(imu_FXOS8700CQ_t * obj, int open_thresh, int closed_thresh)
{
    obj->origin.vector_threshold_open = open_thresh; // * fabs((obj->temp - obj->origin.calibration_temp) + 1) * 0.5;
    obj->origin.vector_threshold_closed = closed_thresh; // * fabs((obj->temp - obj->origin.calibration_temp) + 1) * 0.5;
}

/**
 * Intalise the imu and imu i2c instance
 * @param  obj        imu object holds all the necicary infomation about the IMU
 * @param  i2c_device i2c instace for the imu
 * @param  enable     enable pin for the load switch on the imu
 * @param  int_1      immu interupt pin 1
 * @param  int_2      imu interupt pin 2
 * @param  address    I2C slave address, depending on hardware configuration
 * @return            if already intalised will not reinitialise the
 */
void  FXOS8700CQ_Initialize(imu_FXOS8700CQ_t * obj, i2cdrv_t * i2c_device, pio_t enable, pio_t int_1, pio_t int_2, uint8_t address,
                            imu_backup_t * backup_pointer, imu_tmp_coef_t * tmp_coef_pointer)
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

    obj->origin.x_tmp_coef = 2.0;
    obj->origin.y_tmp_coef = 2.0;
    obj->origin.z_tmp_coef = 10.0;

    // assign hardware slave address
    obj->i2c_slave_addr = address;

    // Not currently used
    obj->current_compass = 0;
    obj->current_heading = 0;

	// initialize door state and calibrate state
	obj->door_state = IMU_EVENT_DOOR_CLOSE;

    // Intialise queue
    obj->imu_event_queue = xQueueCreate(2, sizeof(imu_event_t));

    // Configure load switch pins
    GPIO_PinModeSet(PIO_PORT(obj->enable), PIO_PIN(obj->enable), gpioModePushPull, 1);

    // Ensures driver isn't accidentally reinitialised
    obj->initialized = true;

    FXOS8700CQ_WriteByte(obj, CTRL_REG2, RST_MASK); // Reset sensor, and wait for reboot to complete
    delay_ms(20);
    FXOS8700CQ_StandbyMode(obj);

    // Ensure the reboot is complete
    while (FXOS8700CQ_ReadByte(obj, CTRL_REG2) & RST_MASK);

    // detect the existence of IMU
    DRV_ASSERT(FXOS8700CQ_ID(obj) == FXOS8700CQ_WHOAMI_VAL);

    // Configure the registers in accelerometer and magnetometer
    FXOS8700CQ_ConfigureAccelerometer(obj);
    FXOS8700CQ_ConfigureMagnetometer(obj);

    while (FXOS8700CQ_ReadByte(obj, CTRL_REG2) & RST_MASK);

    FXOS8700CQ_ActiveMode(obj);

    if(backup_pointer == NULL)
    {
        // If there is no saved data, calibrate - door must be shut
        delay_ms(2000); // Wait for movement to stop due to button press
        FXOS8700CQ_Calibrate(obj);
    } else
    {
        // Load from the eeprom
        obj->origin.x_origin = backup_pointer->x_origin;
        obj->origin.y_origin = backup_pointer->y_origin;
        obj->origin.z_origin = backup_pointer->z_origin;
    }

    if(tmp_coef_pointer == NULL)
    {
        // Write default values - can be calibrated manually with remote command
        obj->origin.x_tmp_coef = 2.0;
        obj->origin.x_tmp_coef = 2.0;
        obj->origin.x_tmp_coef = 10.0;
    } else
    {
        // Load from the eeprom
        obj->origin.x_tmp_coef = tmp_coef_pointer->x_tmp_coef;
        obj->origin.x_tmp_coef = tmp_coef_pointer->y_tmp_coef;
        obj->origin.x_tmp_coef = tmp_coef_pointer->z_tmp_coef;
    }
    FXOS8700CQ_Backup_Coefficient(obj);
    FXOS8700CQ_Calibrate(obj);


    xTaskCreate((void *) FXOS8700CQ_vTaskResetOrigin, "Reset_Origin", 300, obj, 2, NULL);
    xTaskCreate((void *) FXOS8700CQ_vTaskCheckTemp, "Check_Temp", 300, obj, 2, NULL);

    FXOS8700CQ_Init_Interrupt(obj);
}

/**
 * Finds the temperature coefficients for a device. Gets the xyz values for a given temperature and waits for the
 * device to be heated up by 20 degrees until it takes another reading. the coefficients can then be worked out for
 * each axis and saved to eeprom.
 * @param obj imu object
 */
void FXOS8700CQ_AutoTemperatureCoefficientFinder(imu_FXOS8700CQ_t * obj)
{
    int16_t x_init;
    int16_t y_init;
    int16_t z_init;
    float temp_init;


    int16_t x_final;
    int16_t y_final;
    int16_t z_final;
    float temp_final;

    while(!FXOS8700CQ_PollMagnetometer(obj));

    x_init = obj->rawmagdata.x;
    y_init = obj->rawmagdata.y;
    z_init = obj->rawmagdata.z;

    temp_init = FXOS8700CQ_GetTemperature(obj);

    bool led_state = true;

    uint16_t counter = 0;

    while(1)
    {
        if(temp_init + 20 < FXOS8700CQ_GetTemperature(obj))
        {
            break;
        }
        led_toggle(2, led_state);
        led_state = !led_state;
        counter = counter + 1;
        if(counter >= 6000)
        {
            break;
        }
        delay_ms(100);
    }

    led_toggle(2, false); // Make sure LED is turned off

    delay_ms(10000); // Wait 10 seconds for heat to evenly distribute through device

    temp_final = FXOS8700CQ_GetTemperature(obj);



    while(!FXOS8700CQ_PollMagnetometer(obj));

    x_final = obj->rawmagdata.x;
    y_final = obj->rawmagdata.y;
    z_final = obj->rawmagdata.z;


    obj->origin.x_tmp_coef = fabs((x_final - x_init) / (temp_final - temp_init));
    obj->origin.y_tmp_coef = fabs((y_final - y_init) / (temp_final - temp_init));
    obj->origin.z_tmp_coef = fabs((z_final - z_init) / (temp_final - temp_init));

    FXOS8700CQ_Calibrate(obj); // The heat often makes the door think its open, recalibrate and as the device cools the
                               // origin will periodically reset.
};

/**
 * Sets origin and thresholds for the IMU interrupts.
 * @param obj imu object
 */
void FXOS8700CQ_Calibrate(imu_FXOS8700CQ_t * obj)
{
    uint8_t i = 0;

    while(!FXOS8700CQ_PollMagnetometer(obj)); // Poll the magnetometer until non zero values returned

    for (i =0;i <= 2; i++) // First temperature reads trend to be zero, so read a couple of times to ensure a correct value
    {
        FXOS8700CQ_GetTemperature(obj);
        delay_ms(5);
    }

    FXOS8700CQ_Set_Origin(obj); // Sets the current magnetometer values as the origin, used as reference for vector mag interrupts
    FXOS8700CQ_Calculate_Vector(obj);
    obj->origin.vector = sqrt((obj->origin.x_origin*obj->origin.x_origin) + (obj->origin.y_origin*obj->origin.y_origin)
                                                   + (obj->origin.z_origin*obj->origin.z_origin)); // used to set thresholds

    obj->last_temp = obj->origin.calibration_temp; // last temp used to determine temp change

    FXOS8700CQ_SetThresholds(obj, DEFAULT_VECTOR_THRESHOLD_OPEN, DEFAULT_VECTOR_THRESHOLD_CLOSE); // Set the thresholds for the VECM interrupts
    FXOS8700CQ_Magnetic_Vector(obj); // Writes the thresholds and origin to the registers in the IMU
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
    FXOS8700CQ_WriteByte(obj, CTRL_REG1, (ASLP_RATE_1_56HZ | DATA_RATE_640MS));  // Set output data rate (ODR) - lower is better for battery
    FXOS8700CQ_WriteByte(obj, CTRL_REG3, (IPOL_MASK));                           // Interrupts are active high - seems to go high when open and return low when closed, should ideally send a pulse
    FXOS8700CQ_ActiveMode (obj);
}

void FXOS8700CQ_ConfigureMagnetometer(imu_FXOS8700CQ_t * obj)
{
    FXOS8700CQ_StandbyMode (obj);
    FXOS8700CQ_WriteByte(obj, M_CTRL_REG1, (MAG_ACTIVE | M_OSR_800_HZ));         // OSR = 1.56HZ, mag only mode (TO, Aug 2012) auto calibrate mode off
    FXOS8700CQ_WriteByte(obj, M_CTRL_REG2, M_HYB_AUTOINC_MASK);                  // enable hybrid autoinc - flicks through registers automatically
    FXOS8700CQ_WriteByte(obj, M_CTRL_REG3, M_ASLP_OSR_100_HZ);                   // OSR = 2 in auto sleep mode
    FXOS8700CQ_ActiveMode (obj);
}

/**
 * Polls the magnetometer, copies the x y and z readings to the IMU object
 * @param obj imu object
 */
uint8_t FXOS8700CQ_PollMagnetometer (imu_FXOS8700CQ_t * obj)
{
    char raw[6] = {0};

    obj->temp = FXOS8700CQ_GetTemperature(obj); // Need to know the temp value corresponding to the magnetometer readings for temp compensation

    FXOS8700CQ_ReadByteArray(obj, M_OUT_X_MSB, raw, 6);

    // Copy the values from the registers to IMU object
    obj->rawmagdata.x = (raw[0] << 8) | raw[1];
    obj->rawmagdata.y = (raw[2] << 8) | raw[3];
    obj->rawmagdata.z = (raw[4] << 8) | raw[5];

    if ((obj->rawmagdata.x !=0) && (obj->rawmagdata.y !=0) && (obj->rawmagdata.z != 0))
    {
        return (obj->rawmagdata.x + obj->rawmagdata.y + obj->rawmagdata.z);
    } else
    {
        return 0;
    }
}

char FXOS8700CQ_GetTemperature(imu_FXOS8700CQ_t * obj)
{
    char temp = FXOS8700CQ_ReadByte(obj, TEMP); // Should actually multiply by 0.96 - as one LSB = 0.96 degrees C
    return temp;
}


void FXOS8700CQ_SetTmpCoeff(imu_FXOS8700CQ_t * obj, float x, float y, float z)
{
    obj->origin.x_tmp_coef = x;
    obj->origin.y_tmp_coef = y;
    obj->origin.z_tmp_coef = z;
}

/**
 * Sets the origin - needed for the magnetometer vector magnitude interrupt
 * @param obj imu object
 */
void FXOS8700CQ_Set_Origin(imu_FXOS8700CQ_t * obj)
{
    while (!(FXOS8700CQ_PollMagnetometer(obj)));
    //FXOS8700CQ_StandbyMode (obj);

    // FXOS8700CQ_WriteByte(obj, M_CTRL_REG1, (MAG_ACTIVE | M_OSR_800_HZ) ); // turns auto calibrate mode off
    obj->origin.x_origin = obj->rawmagdata.x;
    obj->origin.y_origin = obj->rawmagdata.y;
    obj->origin.z_origin = obj->rawmagdata.z;

    // These values are used to compensate the origin and thresholds for temp changes - must be initialised first.
    obj->origin.x_origin_compensated =  obj->rawmagdata.x;
    obj->origin.y_origin_compensated =  obj->rawmagdata.y;
    obj->origin.z_origin_compensated =  obj->rawmagdata.z;

    // Not currently being used
    obj->start_position = 180.0 * atan2(obj->origin.x_origin, obj->origin.z_origin) / (float)M_PI;

    // Door must be closed for calibration
	obj->door_state = IMU_EVENT_DOOR_CLOSE;

    //FXOS8700CQ_ActiveMode (obj);
    obj->origin.calibration_temp = FXOS8700CQ_GetTemperature(obj);

    FXOS8700CQ_Backup_Origin(obj);
}

/**
 * Saves the origin but doesn't write anything to the IMU
 * @param obj imu object
 */
void FXOS8700CQ_Save_Origin(imu_FXOS8700CQ_t * obj)
{
    while (!(FXOS8700CQ_PollMagnetometer(obj)));
    obj->temporary_origin.x_origin = (obj->rawmagdata.x + obj->origin.x_origin) / 2; // Stops any crazy changes taking place
    obj->temporary_origin.y_origin = (obj->rawmagdata.y + obj->origin.y_origin) / 2;
    obj->temporary_origin.z_origin = (obj->rawmagdata.z + obj->origin.z_origin) / 2;

    obj->temporary_origin.x_origin_compensated =  obj->origin.x_origin;
    obj->temporary_origin.y_origin_compensated =  obj->origin.y_origin;
    obj->temporary_origin.z_origin_compensated =  obj->origin.z_origin;

    obj->temporary_origin.calibration_temp = FXOS8700CQ_GetTemperature(obj); //Seems to be 0 on first read

    obj->saved_origin = true;
}

/**
 * Writes the temporary values from SaveOrigin to the IMU
 * @param obj imu object
 */
void FXOS8700CQ_Reset_Origin(imu_FXOS8700CQ_t * obj)
{
    obj->origin.x_origin = obj->temporary_origin.x_origin;
    obj->origin.y_origin = obj->temporary_origin.y_origin;
    obj->origin.z_origin = obj->temporary_origin.z_origin;

    obj->origin.x_origin_compensated = obj->temporary_origin.x_origin_compensated;
    obj->origin.y_origin_compensated = obj->temporary_origin.y_origin_compensated;
    obj->origin.z_origin_compensated = obj->temporary_origin.z_origin_compensated;

    obj->origin.calibration_temp =  obj->temporary_origin.calibration_temp ;//Seems to be 0 on first read

    FXOS8700CQ_Magnetic_Vector(obj);

    obj->saved_origin = false;
}

 /**
  * set up the magnetic vector function of the imu and set up it as an interrupt
  * @param obj imu object
  */
void FXOS8700CQ_Magnetic_Vector(imu_FXOS8700CQ_t * obj)
{
    FXOS8700CQ_StandbyMode (obj); // Allow register writes

    uint8_t Vector_Threshold[2] = {0};
    uint8_t ref[6] = {0};

    // fill ref with the x,y,z reference values
    ref[1] = (obj->origin.x_origin);
    ref[0] = (obj->origin.x_origin) >> 8;
    ref[3] = (obj->origin.y_origin);
    ref[2] = (obj->origin.y_origin) >> 8;
    ref[5] = (obj->origin.z_origin);
    ref[4] = (obj->origin.z_origin) >> 8;
    Vector_Threshold[0] = obj->origin.vector_threshold_open >> 8 | 0x80; // 15 bit value - first bit dictates debouncing behaviour
    Vector_Threshold[1] = (uint8_t)obj->origin.vector_threshold_open;

    FXOS8700CQ_WriteByte(obj, M_VECM_CFG, 0X00); //  reset values in the vec_cfg register

    FXOS8700CQ_WriteByteArray(obj, M_VECM_INITX_MSB, ref, 6);
    FXOS8700CQ_WriteByte(obj, M_VECM_THS_MSB, Vector_Threshold[0]);
    FXOS8700CQ_WriteByte(obj, M_VECM_THS_LSB, Vector_Threshold[1]);
    FXOS8700CQ_WriteByte(obj,M_VECM_CNT, M_VECTOR_DBNCE);
    FXOS8700CQ_WriteByte(obj, M_VECM_CFG, (M_VECM_INITM_MASK | M_VECM_UPDM_MASK | M_VECM_EN_MASK| M_VECM_WAKE_EN_MASK | M_VECM_INT_EN_MASK));//values do not update, vector ena bleld,procs wake up,interupt on pin 2 enlabeled

    FXOS8700CQ_ActiveMode (obj);
}

/**
 * Compensates for changes in temperature by increasing the magnitude of each of the origin coordinates linearly wrt temperature
 * The coefficients for the linear relationship are set in the init function. The thresholds don't seem to require changing, just
 * the reference values. Over large temperature ranges, the linear approximation begins to fall apart, so ideally the origin would
 * be reset periodically when it is guaranteed to be in a closed position.
 * @param obj imu object
 */
void FXOS8700CQ_Magnetic_Temperature_Compensation(imu_FXOS8700CQ_t * obj)
{
    FXOS8700CQ_StandbyMode (obj);
    uint8_t Vector_Threshold[2] = {0};
    uint8_t ref[6] = {0};

    // Calculates what the origin would be if the device was recalibrated at the current temp - need to increase
    // magnitude while maintaining direction - hence the if statements. Still not sure on what should happen to a zero origin value
    if (obj->origin.x_origin >= 0) // If positive, add
    {
        obj->origin.x_origin_compensated = obj->origin.x_origin + ((obj->temp - obj->origin.calibration_temp) * obj->origin.x_tmp_coef);
    } else // If negative, subtract
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
        FXOS8700CQ_Backup_Origin(obj);
    }
}

/**
 * Checks the temperature periodically and recalibrates if necessary - this is needed incase the door is left in the open
 * state for a long time and the temperature changes, causing the vector to drift further from the origin and potentially
 * meaning the next close event won't be detected. Also, if the door has been closed for a while, the origin is saved.
 * If the door remains closed, the origin is reset - this prevents the origin being reset during an event.
 * @param obj IMU object
 */
void FXOS8700CQ_vTaskCheckTemp(imu_FXOS8700CQ_t * obj)
{
    // initialize the task tick handler
    portTickType xLastWakeTime;

    xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        obj->counter = obj->counter + 1; // This is reset to 0 each time the door state changes
        obj->temp = FXOS8700CQ_GetTemperature(obj);
        if(obj->temp != obj->last_temp)
        {
            //do temp compensation
            FXOS8700CQ_Magnetic_Temperature_Compensation(obj);
        }
        obj->last_temp = obj->temp;

        if(obj->counter > 1 && obj->door_state == IMU_EVENT_DOOR_CLOSE) // If it's been more than 5 seconds since last door event
        {
            if(obj->saved_origin == true) // If its been more than 5 seconds since saving the last origin
            {
                FXOS8700CQ_Reset_Origin(obj); // Resets the vector interrupt
            }
            FXOS8700CQ_Save_Origin(obj); // Saves the origin but doesn't change the values in the IMU
        } else
        {
            obj->saved_origin = false; // If there has jut been a door event, then the saved origin data could be invalid
        }

        FXOS8700CQ_Backup_Origin(obj); // A change to the origin may have taken place, so save that data.

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(5000));
    }
}

/**
 * Handles the the interrupts from the IMU. First checks to see how much the vector may have changed due to
 * temperature since the last interrupt or call of vTaskCheckTemp, then determines if the interrupt was caused due to the
 * temperature change or a genuine door event. Resets the origin to compensate for the temperature change if needed.
 * @param pin in that is used for detection of the interrupt (IMU INT pn2)
 * @param obj IMU object
 */
static void FXOS8700CQ_Imu_Int_Handler(uint8_t pin, imu_FXOS8700CQ_t * obj)
{
    obj->last_event = obj->door_state;
    uint8_t Vector_Threshold[2] = {0};
    if (abs(xTaskGetTickCountFromISR() - obj->last_call) >= 100 ) // essentialy debouncing, wont let the state change constantly
    {
        // Was this because of a temp change?
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
            obj->counter = 0; // Tells the vTaskCheckTemp that a door event has just occurred
        }

        obj->last_temp = obj->temp;

        obj->last_call = xTaskGetTickCountFromISR();
    }
}

/**
 * [FXOS8700CQ_Init_Interrupt  Sets up the interupt for the imu interupt_1
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

    angle = 180.0 * atan2(obj->rawmagdata.x, obj->rawmagdata.z) / (float)M_PI;

    return angle;
}

/**
 * calculates the angle between the curent vector and the origin
 * @param obj imu object
 */
void FXOS8700CQ_Vector_Angle(imu_FXOS8700CQ_t* obj)
{
    FXOS8700CQ_PollMagnetometer(obj);
    int16_t mag_x = obj->rawmagdata.x;
    int16_t mag_y = obj->rawmagdata.y;
    int16_t mag_z = obj->rawmagdata.z;

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


void FXOS8700CQ_Backup_Origin(imu_FXOS8700CQ_t * obj)
{
    DRV_ASSERT(obj);

    // do anything only if backup handler is configured
    if (obj->callbacks.on_backup_origin_requested)
    {
        // create a backup object on stack
        imu_backup_t backup;
        memset(&backup, 0x0, sizeof(imu_backup_t));

        // fill data
        backup.x_origin = obj->origin.x_origin;
        backup.y_origin = obj->origin.y_origin;
        backup.z_origin = obj->origin.z_origin;
        backup.calibration_temp = obj->origin.calibration_temp;

        // call backup handler
        obj->callbacks.on_backup_origin_requested(obj, &backup);
    }
}


void FXOS8700CQ_Backup_Coefficient(imu_FXOS8700CQ_t * obj)
{
    DRV_ASSERT(obj);

    // do anything only if backup handler is configured
    if (obj->callbacks.on_backup_coefficient_requested)
    {
        // create a backuo object on stack
        imu_tmp_coef_t backup;
        memset(&backup, 0x0, sizeof(imu_tmp_coef_t));

        // fill data
        backup.x_tmp_coef = obj->origin.x_tmp_coef;
        backup.y_tmp_coef = obj->origin.y_tmp_coef;
        backup.z_tmp_coef = obj->origin.z_tmp_coef;

        // call backup handler
        obj->callbacks.on_backup_coefficient_requested(obj, &backup);
    }
}

#endif // USE_FREERTOS == 1
