/**
 * @brief Implementation of I2C IMU driver
 * @file imu_fxos.c
 * @author Steven O'Rourke
 * @date Oct, 2017
 */

/**
 * Thresholds are set on the IMU for opening and closing of the gate. When these thresholds are passed, an interrupt
 * is triggered on IMU line 2.
 *
 * an advancement on this would be to use the accelerometer or some other sort of signal analysis of the vector value/individual
 * values to decided if the door is open
 *
 * The device will calibrate on the first start if the device has not yet been calibrated or when a calibrate call is made. (What would make this call?)
 * In the future values such as the temperature coefficient will be remembered by the micro controller eeprom
 * even when the device is reset this way it can remember components characteristics and compensate accordingly/
 *
 * The temperature task happens every 20s and will change stuff if the temperature has changed. read more on this at the
 * temperature adjustment task.
 */

#if USE_FREERTOS == 1

#include <math.h>
#include <stdlib.h>

#include "drv_debug.h"
#include "imu_fxos.h"
#include "imu_fxos_regs.h"

#include "drv_debug.h"
#include "delay.h"
#include "gpiointerrupt.h"
#include "bits.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

// PROTOTYPES
void FXOS8700CQ_LoadBackup(imu_FXOS8700CQ_t * obj, imu_backup_t * backup_pointer);
/////////////////////////////////////////////////////////////////////////////////
/**
 * Initialise the imu and imu i2c instance
 * @param  obj        imu object holds all the necessary information about the IMU
 * @param  i2c_device i2c instance for the IMU
 * @param  enable     enable pin for the load switch on the imu
 * @param  int_1      imu interrupt pin 1
 * @param  int_2      imu interrupt pin 2
 * @param  address    I2C slave address, depending on hardware configuration
 * @return            if already initialised will not reinitialise the IMU
 */
void  FXOS8700CQ_Initialize(imu_FXOS8700CQ_t * obj, i2cdrv_t * i2c_device, pio_t enable, pio_t int_1, pio_t int_2, uint8_t address,
                            imu_backup_t * backup_pointer)
{
    // sanity check for pointers
    DRV_ASSERT(obj);
    DRV_ASSERT(i2c_device);

    obj->interrupt_flag = 1;

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

    // Object has not yet been calibrated
    obj->calibrated = false;

    //initialise queue
    obj->imu_event_queue = xQueueCreate(2, sizeof(imu_event_t));

    // configure load switch pins
    GPIO_PinModeSet(PIO_PORT(obj->enable), PIO_PIN(obj->enable), gpioModePushPull, 1);

    obj->initialized = true;


    FXOS8700CQ_WriteByte(obj, CTRL_REG2, RST_MASK);                    // Reset sensor, and wait for reboot to complete
    delay_ms(2);                                        //Wait at least 1ms after issuing a reset before attempting communications
    FXOS8700CQ_StandbyMode(obj);

    while (FXOS8700CQ_ReadByte(obj, CTRL_REG2) & RST_MASK);

    // detect the existence of IMU
    DRV_ASSERT(FXOS8700CQ_ID(obj) == FXOS8700CQ_WHOAMI_VAL);

    // Acc-cell config
    FXOS8700CQ_WriteByte(obj, XYZ_DATA_CFG, 0x02);    // +/- 4g accel range

    //M-cell config
    FXOS8700CQ_WriteByte(obj, M_CTRL_REG1, 0x17);     // changed from 0x1F - reduce sample rate to save power g
    FXOS8700CQ_WriteByte(obj, M_CTRL_REG2, 0x00);     //
    FXOS8700CQ_WriteByte(obj, M_CTRL_REG3, 0x80);

    // General config
    FXOS8700CQ_WriteByte(obj, TRIG_CFG, 0x00);        // No fifo used
    FXOS8700CQ_WriteByte(obj, CTRL_REG1, 0xF8);       // Super low power
    FXOS8700CQ_WriteByte(obj, CTRL_REG2, 0x10);       // Lowest power but higher noise
    FXOS8700CQ_WriteByte(obj, CTRL_REG3, 0x04);       // Will wake on transient detect
    FXOS8700CQ_WriteByte(obj, CTRL_REG4, 0x00);       // disables interrupts
    FXOS8700CQ_WriteByte(obj, CTRL_REG5, 0x00);       // Routed to INT1 pin

    FXOS8700CQ_WriteByte(obj, ASLP_COUNT, 0x06);


    FXOS8700CQ_WriteByte(obj, A_VECM_CFG, 0x48);
    FXOS8700CQ_WriteByte(obj, A_VECM_THS_MSB, 0x00);
    FXOS8700CQ_WriteByte(obj, A_VECM_THS_LSB, 0x06);

    FXOS8700CQ_WriteByte(obj, HP_FILTER_CUTOFF, 0x00);

    FXOS8700CQ_ActiveMode(obj);


    obj->interrupt_mode = IMU_ACCEL_INTERRUPT;

    // detect the existence of IMU

    while (FXOS8700CQ_ReadByte(obj, CTRL_REG2) & RST_MASK);

    FXOS8700CQ_Init_Interrupt(obj);
    //xTaskCreate((void *) ImuTempAdjustment, "temp_mon", 200, obj, 2, &obj->ImuTempHandler); // Was in init interrupt



    obj->current_compass = 0;
    obj->current_heading = 0;

    if (backup_pointer == NULL)
    {
        //Initialise from scratch - DOOR MUST BE CLOSED
        obj->origin.tmp_coef = 3.0; //initial guess for scaling factor of the imu z direction with temperature
        obj->door_state = IMU_EVENT_DOOR_CLOSE;
        FXOS8700CQ_Calibrate(obj);
    }
    else
    {
        // Data had been saved from before, so don't reset the tmp coef (as this gets more accurate over time) and don't
        // recalibrate the device (load the backup data straight in)
        //FXOS8700CQ_LoadBackup(obj, backup_pointer);
        // When do the threshold get written?
    }

    //Now get the current vector - check if open or closed
    FXOS8700CQ_Calculate_Vector(obj);
}

void FXOS8700CQ_LoadBackup(imu_FXOS8700CQ_t * obj, imu_backup_t * backup_pointer)
{
    obj->origin.tmp_coef = backup_pointer->tmp_coef;
    obj->origin.vector_threshold_closed = backup_pointer->vector_threshold_closed;
    obj->origin.vector_threshold_open = backup_pointer->vector_threshold_open;
    obj->origin.x_origin = backup_pointer->x_origin;
    obj->origin.y_origin = backup_pointer->y_origin;
    obj->origin.z_origin = backup_pointer->y_origin;

    obj->calibrated = true;
}

// When should this be run?
void FXOS8700CQ_SaveBackup(imu_FXOS8700CQ_t * obj, imu_backup_t * backup_pointer)
{
    backup_pointer->tmp_coef = obj->origin.tmp_coef;
    backup_pointer->vector_threshold_closed = obj->origin.vector_threshold_closed;
    backup_pointer->vector_threshold_open = obj->origin.vector_threshold_open;
    backup_pointer->x_origin = obj->origin.x_origin;
    backup_pointer->y_origin = obj->origin.x_origin;
    backup_pointer->y_origin = obj->origin.z_origin;
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

uint8_t FXOS8700CQ_PollAccelerometer (imu_FXOS8700CQ_t * obj)
{
    rawdata_t raw_accel_data;

    char raw[6] = {0};
    FXOS8700CQ_ReadByteArray(obj, OUT_X_MSB, raw, 6);

    raw_accel_data.x = ((raw[0] << 8) | raw[1]) >> 2;     // Pull out 14-bit, 2's complement, right-justified accelerometer data
    raw_accel_data.y = ((raw[2] << 8) | raw[3]) >> 2;
    raw_accel_data.z = ((raw[4] << 8) | raw[5]) >> 2;

    obj->accel_data.x = raw_accel_data.x;
    obj->accel_data.y = raw_accel_data.y;
    obj->accel_data.z = raw_accel_data.z;

    return (obj->accel_data.x + obj->accel_data.y + obj->accel_data.z);
}

uint8_t FXOS8700CQ_PollMagnetometer (imu_FXOS8700CQ_t * obj)
{
    uint8_t i = 0;
    char raw[6] = {0};
    //loop through to insure that there are no zeroed values from a mishap either efr or imu side or in the twi line.
    for (i = 0; i <= 10; i++)
    {
        FXOS8700CQ_ReadByteArray(obj, M_OUT_X_MSB, raw, 6);
        obj->mag_data.x = (raw[0] << 8) | raw[1];           // Return 16-bit, 2's complement magnetometer data
        obj->mag_data.y = (raw[2] << 8) | raw[3];
        obj->mag_data.z = (raw[4] << 8) | raw[5];
        if ((obj->mag_data.x !=0) && (obj->mag_data.y !=0) && (obj->mag_data.z != 0))
        {
            return (obj->mag_data.x + obj->mag_data.y + obj->mag_data.z);
        }
        delay_ms(500); // delay for the imu to get new values.
    }
    return (obj->mag_data.x + obj->mag_data.y + obj->mag_data.z);
}

char FXOS8700CQ_GetTemperature(imu_FXOS8700CQ_t * obj)
{
    char temp = FXOS8700CQ_ReadByte(obj, TEMP);
    return temp;
}

/**
 * takes a measurement of the magnetometer and sets the x,y and z initial positions
 * @param object [description]
 */
void FXOS8700CQ_Set_Origin(imu_FXOS8700CQ_t * obj)
{
    while (!(FXOS8700CQ_PollMagnetometer(obj)));
    while (!(FXOS8700CQ_PollAccelerometer(obj)));

    obj->origin.x_origin = obj->mag_data.x;
    obj->origin.y_origin = obj->mag_data.y;
    obj->origin.z_origin = obj->mag_data.z;
    obj->start_position = 180.0 * atan2(obj->mag_data.x, obj->mag_data.x) / (float)M_PI;

    obj->accel_origin.x = obj->accel_data.x;
    obj->accel_origin.y = obj->accel_data.y;
    obj->accel_origin.z = obj->accel_data.z;

    obj->door_state = IMU_EVENT_DOOR_CLOSE;
}

/**
 * LEGACY - not currently being used
 *
 * Sets the threshold for the magnetometer interrupt as the origin + the threshold value
 * set the imu to interrupt on the x and z axis as if the device is upright on the door
 * with no debounce and no latching interrupt is on pin 1
 * @param obj Imu object
 */
void FXOS8700CQ_Magnetic_Threshold_Setting(imu_FXOS8700CQ_t * obj)
{
    FXOS8700CQ_StandbyMode (obj);
    uint8_t origin[6] = {0};

    origin[1] = obj->origin.x_origin;
    origin[0] = obj->origin.x_origin >> 8;
    origin[3] = obj->origin.y_origin;
    origin[2] = obj->origin.y_origin >> 8;
    origin[5] = obj->origin.z_origin;
    origin[4] = obj->origin.z_origin >> 8;

    FXOS8700CQ_WriteByte(obj, M_THS_CFG, M_THS_ZEFE | M_THS_YEFE| M_THS_XEFE| M_THS_WAKE_EN | M_THS_INT_EN | M_THS_OAE);
    FXOS8700CQ_WriteByteArray(obj, M_THS_X_MSB, origin, 6);
    FXOS8700CQ_ActiveMode(obj);
}

 /**
  * set up the magnetic vector function of the imu and set up it as an interrupt
  * @param obj imu object
  */
void FXOS8700CQ_Magnetic_Vector_Interrupt_Calibration(imu_FXOS8700CQ_t * obj)
{
    FXOS8700CQ_StandbyMode (obj);
    uint8_t Vector_Threshold[2] = {0};
    uint8_t ref[6] = {0};

    ref[1] = obj->origin.x_origin;
    ref[0] = obj->origin.x_origin >> 8;
    ref[3] = obj->origin.y_origin;
    ref[2] = obj->origin.y_origin >> 8;
    ref[5] = obj->origin.z_origin;
    ref[4] = obj->origin.z_origin >> 8;
    Vector_Threshold[0] = obj->origin.vector_threshold_open >> 8 | 0x80;
    Vector_Threshold[1] = (uint8_t)obj->origin.vector_threshold_open ;

    //FXOS8700CQ_WriteByte(obj, M_VECM_CFG,0X00); //  reset values in the vec_cfg register

    // Configures interrupt
    FXOS8700CQ_WriteByteArray(obj, M_VECM_INITX_MSB, ref, 6);
    FXOS8700CQ_WriteByte(obj, M_VECM_CFG, (M_VECM_UPDM_MASK | M_VECM_EN_MASK| M_VECM_WAKE_EN_MASK| M_VECM_INIT_EN_MASK | M_VECM_INITM_MASK));//values do not update, vector enabled, procs wake up, interrupt on pin 2 enabled
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
    FXOS8700CQ_PollMagnetometer(obj);

    angle = 180.0 * atan2(obj->mag_data.x, obj->mag_data.z) / (float)M_PI;

    return angle;
}

/**
 * Handles the interrupts associated with the IMU. check if the int pin is high or low and sends a door open/closed to the queue
 * @param pin in that is used for detection of the interrupt (IMU INT pn2)
 * @param obj IMU object
 */
static void FXOS8700CQ_Imu_Int_Handler(uint8_t pin, imu_FXOS8700CQ_t * obj)
{
    obj->interrupt_flag = 0;
    if (obj->interrupt_mode == IMU_MAG_INTERRUPT) {
        FXOS8700CQ_Imu_Magnetometer_Int_Handler(pin, obj);
    }
    if (obj->interrupt_mode == IMU_ACCEL_INTERRUPT){
        FXOS8700CQ_Imu_Accelerometer_Int_Handler(pin, obj);
    }
}

void FXOS8700CQ_Imu_Accelerometer_Int_Handler(uint8_t pin, imu_FXOS8700CQ_t * obj)
{
    if (abs(xTaskGetTickCountFromISR() - obj->last_call) >= 500 ) // essentially debouncing wont let the state change constantly
    {

        FXOS8700CQ_Calculate_Vector(obj);

        // decided to only check for the high of the imu lin as there could be problems in the i2c line which will mess with results.
        // this may also remove some false positives that are only there for a very short time
        if (obj->door_state == IMU_EVENT_DOOR_CLOSE)
        {
            // Check to see if the mag is above thresholds
            if(obj->vector > obj->origin.vector_threshold_open)
            {
                obj->door_state = IMU_EVENT_DOOR_OPEN;
            }
        }
        else
        {
         if(obj->door_state == IMU_EVENT_DOOR_OPEN)
            {
                obj->door_state = IMU_EVENT_DOOR_CLOSE;
            }
        }

        if (obj->door_state != obj->last_event) // Statement seems defunct, this would always be true
        {
            xQueueSendFromISR(obj->imu_event_queue, &obj->door_state,NULL);
            obj->last_event = obj->door_state;
        }
        obj->last_call = xTaskGetTickCountFromISR(); //could potentially be a really large number, will wrap every 50 days
    }
}

void FXOS8700CQ_Imu_Magnetometer_Int_Handler(uint8_t pin, imu_FXOS8700CQ_t * obj)
{
    bool interrupt_1 = 0;
    uint8_t Vector_Threshold[2] = {0};

    interrupt_1 = (bool) GPIO_PinInGet(PIO_PORT(obj->int_2), PIO_PIN(obj->int_2));
    if (abs(xTaskGetTickCountFromISR() - obj->last_call) >= 500 ) // essentially debouncing wont let the state change constantly
    {
        // decided to only check for the high of the imu lin as there could be problems in the i2c line which will mess with results.
        // this may also remove some false positives that are only there for a very short time
        if (interrupt_1 == true)
        {
            obj->door_state = IMU_EVENT_DOOR_OPEN;
            //halSetLed(BOARDLED1);
            Vector_Threshold[0] = obj->origin.vector_threshold_closed >> 8 | 0x80;
            Vector_Threshold[1] = (uint8_t)obj->origin.vector_threshold_closed;
        }
        else
        {
            obj->door_state = IMU_EVENT_DOOR_CLOSE;
            //halClearLed(BOARDLED1);
            Vector_Threshold[0] = obj->origin.vector_threshold_open >> 8 | 0x80;
            Vector_Threshold[1] = (uint8_t)obj->origin.vector_threshold_open ;
        }

        //printf("int %lu, %lu\r\n", (unsigned long)obj->vector, (unsigned long)obj->door_state);

        if (obj->door_state != obj->last_event) // Statement seems defunct, this would always be true
        {
            FXOS8700CQ_WriteByteArray(obj, M_VECM_THS_MSB, Vector_Threshold, 2);


            xQueueSendFromISR(obj->imu_event_queue, &obj->door_state,NULL);
            obj->last_event = obj->door_state;
        }
        obj->last_call = xTaskGetTickCountFromISR(); //could potentially be a really large number, will wrap every 50 days
    }
}

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
 * [FXOS8700CQ_Init_Interrupt  Sets up the interrupt for the imu interrupt_1
 * @param obj IMU object holing information about the imu and door open / closed state.
 */
void FXOS8700CQ_Init_Interrupt (imu_FXOS8700CQ_t * obj)
{
    // creates the temperature adjusting queue (this is here instead of the init as the temp code
    // needs the int to initialised first)
    //xTaskCreate((void *) ImuTempAdjustment, "temp_mon", 600, obj, 2, &obj->ImuTempHandler);

    GPIO_PinModeSet(PIO_PORT(obj->int_1), PIO_PIN(obj->int_1), gpioModeInput, 0);
    GPIO_PinModeSet(PIO_PORT(obj->int_2), PIO_PIN(obj->int_2), gpioModeInput, 0);

    // configure port interrupt
    GPIOINT_Init();
    GPIOINT_CallbackRegisterWithArgs(PIO_PIN(obj->int_2), (GPIOINT_IrqCallbackPtrWithArgs_t) FXOS8700CQ_Imu_Int_Handler, (void *) obj);
    GPIO_ExtIntConfig(PIO_PORT(obj->int_2), PIO_PIN(obj->int_2), PIO_PIN(obj->int_2),
                      false /* raising edge */, true /* falling edge */, true /* enable now */);

    NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
    NVIC_EnableIRQ(GPIO_ODD_IRQn);
}


/**
 * Takes a measurement of the temperature and check to see if it has changed more than 2 degrees. if the
 * device has changed and the door is closed the device recalibrates
 * obj->temp is the last temp that was measure_period_mstemperature_imu is the current temperature
 * @param obj imu object
 */
static void ImuTempAdjustment(imu_FXOS8700CQ_t * obj)
{
	// initialize the task tick handler
	portTickType xLastWakeTime;
    int8_t temperature_imu = 0;
    int8_t temp_change = 0;
    int8_t last_temp_change = 0;

    int16_t max_z = 0;

    bool interrupt_check;
	// get last execution time
	xLastWakeTime = xTaskGetTickCount();
	while (1)
	{
        temperature_imu = FXOS8700CQ_GetTemperature(obj);

        temp_change = temperature_imu - obj->temp;
        FXOS8700CQ_PollMagnetometer(obj);

        if ((max_z < obj->mag_data.z) && (obj->door_state == IMU_EVENT_DOOR_CLOSE))
        {
            max_z = obj->mag_data.z;
        }

        if ((abs(temp_change) >= 1) && (abs(temp_change) < 10)) // the less than 20 is because it should really never get here and i think it may on the odd ocaion of crossing 0 degrees it stuffs up some times
        {
            // checks the line status then unregisters the line as it will drop during the reset.
            interrupt_check = (bool) GPIO_PinInGet(PIO_PORT(obj->int_2), PIO_PIN(obj->int_2));
            GPIOINT_CallbackUnRegister(PIO_PIN(obj->int_2));
            //While the door is closed the device tries to work out a factor that the z axis will change with temperature by. this also
            //resets the orign.    This whole thing is very depenant on assuming tha the door is actually closed and not just almost closed/
            //when the door is open the device will either use the inital guess or the calcualted value of the temperautre coeffcient.
            if (obj->door_state == IMU_EVENT_DOOR_CLOSE)
            {
                if (abs(temp_change + last_temp_change) >= 2 )
                {
                    FXOS8700CQ_Cal_Scaling(obj,temp_change, max_z);
                }
                FXOS8700CQ_Set_Origin(obj);
            }
            else
            {
                obj->origin.z_origin = obj->origin.z_origin + (obj->origin.tmp_coef*temp_change) ; // when this is set and open the device sets the threshold to the open threshold as it is larger and will allow for some temperature leway to get closed.
            }

            FXOS8700CQ_WriteByte(obj, CTRL_REG2, RST_MASK);                    //Reset sensor, and wait for reboot to complete. // for some raeson resseting the deviec made it more predicatable in the x and y axis during a temp change
            delay_ms(2);                                        //Wait at least 1ms after issuing a reset before attempting communications
            FXOS8700CQ_StandbyMode(obj);

            while (FXOS8700CQ_ReadByte(obj, CTRL_REG2) & RST_MASK);

            FXOS8700CQ_ConfigureAccelerometer(obj);
            FXOS8700CQ_ConfigureMagnetometer(obj);
            while (FXOS8700CQ_ReadByte(obj, CTRL_REG2) & RST_MASK);
            // change the orgin of the device depenant on temperature
            FXOS8700CQ_Magnetic_Vector_Interrupt_Calibration(obj);
            FXOS8700CQ_ActiveMode(obj);
            delay_ms(1000);
            GPIOINT_CallbackRegisterWithArgs(PIO_PIN(obj->int_2), (GPIOINT_IrqCallbackPtrWithArgs_t) FXOS8700CQ_Imu_Int_Handler, (void *) obj);
            // checks if the interupt line has been changed and if it has go to the intterupt handler
            if (interrupt_check !=(bool) GPIO_PinInGet(PIO_PORT(obj->int_2), PIO_PIN(obj->int_2)))
            {
                FXOS8700CQ_Imu_Int_Handler(PIO_PIN(obj->int_2), obj);
            }
            // Sets the temp to old temp and resets the z max and remembers the last temp change and temperature.
            obj->temp = temperature_imu;
            last_temp_change = temp_change;
            max_z = 0;
        }
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(20000));
	}
}

/**
 * finds the temperature scaling factor and uses very simple smoothing to make sure no outlandish values are achieved. 4
 * starts off with a guess of 3
 * everything in this is made to be a float as it was having problems with it before.
 * @param obj         imu object
 * @param temp_change the change in temperatue since the device last updated the z value
 * @param max_z   largest value of z found in each section of the temperature graph
 */
void FXOS8700CQ_Cal_Scaling(imu_FXOS8700CQ_t *obj,int16_t temp_change, int16_t max_z)
{
    float new_scaler;

    new_scaler = (float)((max_z - obj->old_magdata.z) /(float)temp_change);
    obj->origin.tmp_coef = ((obj->origin.tmp_coef * 29.0 ) + new_scaler) /30.0;

    obj->old_magdata.z = max_z;
}

/**
 * calculates the angle between the curent vector and the origin
 * @param obj imu object
 */
void FXOS8700CQ_Vector_Angle(imu_FXOS8700CQ_t* obj)
{
    FXOS8700CQ_PollMagnetometer(obj);
    int16_t mag_x = obj->mag_data.x;
    int16_t mag_y = obj->mag_data.y;
    int16_t mag_z = obj->mag_data.z;

    int16_t origin_x = obj->origin.x_origin;
    int16_t origin_y = obj->origin.y_origin;
    int16_t origin_z = obj->origin.z_origin;

    uint8_t r2d2 = (180/(float)M_PI); //radins to degrees

    int32_t current_vector = sqrt((mag_x * mag_x) + (mag_y * mag_y) + (mag_z * mag_z));
    int32_t origin_vector  = sqrt((origin_x * origin_x) + (origin_y * origin_y) + (origin_z * origin_z));
    int32_t dot_product    = (mag_x * origin_x) + (mag_y * origin_y) + (mag_z * origin_z);
    float fraction         = dot_product/(float)(current_vector * origin_vector);
    float rad              = acosf(fraction);

    obj->vector_angle = (int16_t)(r2d2 * rad);
}


/**
 * poll the magnetometer and calculates the change in vector (the same way the the imu does and interrupt is driven by)
 * @param obj imu object
 */
void FXOS8700CQ_Calculate_Vector(imu_FXOS8700CQ_t * obj)
{
    FXOS8700CQ_PollMagnetometer(obj);
    int16_t mag_x = obj->mag_data.x;
    int16_t mag_y = obj->mag_data.y;
    int16_t mag_z = obj->mag_data.z;

    int16_t origin_x = obj->origin.x_origin;
    int16_t origin_y = obj->origin.y_origin;
    int16_t origin_z = obj->origin.z_origin;

    int32_t dx = mag_x - origin_x;
    int32_t dy = mag_y - origin_y;
    int32_t dz = mag_z - origin_z;

    obj->vector = sqrt((dx*dx) + (dy*dy) + (dz*dz));
}

void FXOS8700CQ_Calculate_Accel_Vector(imu_FXOS8700CQ_t * obj)
{
    FXOS8700CQ_PollAccelerometer(obj);
    int16_t x = obj->accel_data.x;
    int16_t y = obj->accel_data.y;
    int16_t z = obj->accel_data.z;

    int16_t origin_x = obj->accel_origin.x;
    int16_t origin_y = obj->accel_origin.y;
    int16_t origin_z = obj->accel_origin.z;

    int32_t dx = x - origin_x;
    int32_t dy = y - origin_y;
    int32_t dz = z - origin_z;

    obj->accel_vector = sqrt((dx*dx) + (dy*dy) + (dz*dz));
}

/**
 * calibrates the thresholds dependant on the current vector of the imu. this can be used to pull the gate to the maximum allowed and claibrating. show
 * @param obj imu object
 */
void FXOS8700CQ_Calibrate(imu_FXOS8700CQ_t * obj)
{
    uint32_t temp_vector = 0;
    uint8_t i = 0;
    //Finds the current vector  and adds 5 then a  scaling factor to get the open and closed threshold constants.
    FXOS8700CQ_Set_Origin(obj);
    for (i =0;i <= 2; i++)
    {
        FXOS8700CQ_Calculate_Vector(obj);
        if (temp_vector < obj->vector)
        {
            temp_vector = obj->vector;
        }
        delay_ms(800);
    }
    obj->origin.vector_threshold_open = (temp_vector * 2) + 40;
    obj->origin.vector_threshold_closed = (temp_vector * 1.2) + 25;
    obj->temp = FXOS8700CQ_GetTemperature(obj);
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
    ret = i2cdrv_master_write_iaddr(obj->i2c_device, obj->i2c_slave_addr, internal_addr, &value, 1);
    delay_ms(10);
}

void FXOS8700CQ_WriteByteArray(imu_FXOS8700CQ_t * obj, char internal_addr, uint8_t* buffer, char length)
{
    uint8_t ret = 0;
    //I2C_WriteByteArray(reg,buffer,length);          //Write values to register
    ret = i2cdrv_master_write_iaddr(obj->i2c_device, obj->i2c_slave_addr, internal_addr, buffer, length);

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

#endif // USE_FREERTOS == 1
