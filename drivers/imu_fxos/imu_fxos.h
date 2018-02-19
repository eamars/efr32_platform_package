/**
 * @brief Implementation of I2C IMU driver header file
 * @file imu_fxos.h
 * @author Steven O'Rourke
 * @date Oct, 2017
 */

#ifndef __FXOS8700CQ_H__
#define __FXOS8700CQ_H__

#if USE_FREERTOS == 1

#include <stdint.h>
#include "i2cdrv.h"
#include "pio_defs.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"


#define M_THRESHOLD           20
#define M_VECTOR_DBNCE        0
#define POLL_THRESH           10

 /***************************TYPES*********************************/

typedef enum
{
    DISABLED=0x00,
    BUFFER=0x01,
//    OVERFLOW=0x10,
    TRIGGERED=0x11
} FX_mode_t;

typedef enum
{
    SCALE2G=0x00,
    SCALE4G=0x01,
    SCALE8G=0x10
} range_t;


typedef enum
{
    PORT_UP_FRONT = 0,
    PORT_UP_BACK,
    PORT_DOWN_FRONT,
    PORT_DOWN_BACK,
    LAND_LEFT_FRONT,
    LAND_LEFT_BACK,
    LAND_RIGHT_FRONT,
    LAND_RIGHT_BACK
} ACC_orientation_t;

typedef struct rawdata {
    int16_t x;
    int16_t y;
    int16_t z;
} rawdata_t;

typedef enum
{
   IMU_EVENT_DOOR_OPEN = 0,
   IMU_EVENT_DOOR_CLOSE = 1,
} imu_event_t;

typedef struct __attribute__((packed))
{
    uint16_t crc16;
    int16_t x_origin;
    int16_t y_origin;
    int16_t z_origin;
    uint16_t vector_threshold_closed;
    uint16_t vector_threshold_open;
    float tmp_coef;
} imu_backup_t;

// prototype for the callback
typedef void (*imu_fxos_on_backup_requested)(void * obj, imu_backup_t * backup);

typedef struct
{
    /// pins
    pio_t int_1;
    pio_t int_2;

    i2cdrv_t * i2c_device;
    pio_t enable;
    uint8_t i2c_slave_addr;

    bool initialized;

	imu_event_t door_state;
    int16_t start_position;
    uint32_t vector;
    uint32_t raw_vector;
    int16_t vector_angle;

    int16_t current_compass; // The current angle
    int16_t current_heading; // The heading away from calibrated angle

    QueueHandle_t imu_event_queue;
    imu_event_t last_event;
    uint32_t last_call;
    TaskHandle_t ImuTempHandler;
    int8_t temp;
    int8_t last_temp;

    TaskHandle_t ImuCalHandler;

    rawdata_t old_magdata;
    rawdata_t magdata;
    rawdata_t rawmagdata;

    struct
    {
        int16_t x_origin;
        int16_t y_origin;
        int16_t z_origin;

        int16_t x_origin_compensated;
        int16_t y_origin_compensated;
        int16_t z_origin_compensated;

        uint32_t vector;
        uint16_t vector_threshold_closed;
        uint16_t vector_threshold_open;
        int8_t calibration_temp;
        float x_tmp_coef;
        float y_tmp_coef;
        float z_tmp_coef;
    } origin;

    // callbacks
    struct
    {
        imu_fxos_on_backup_requested on_backup_requested;
    } callbacks;

} imu_FXOS8700CQ_t;


void  FXOS8700CQ_Initialize(imu_FXOS8700CQ_t * obj, i2cdrv_t * i2c_device, pio_t enable, pio_t int_1, pio_t int_2, uint8_t address,
                            imu_backup_t * backup_pointer);
char       FXOS8700CQ_ReadStatusReg(imu_FXOS8700CQ_t * obj);
void       FXOS8700CQ_ActiveMode (imu_FXOS8700CQ_t * obj);
char       FXOS8700CQ_StandbyMode (imu_FXOS8700CQ_t * obj);
char       FXOS8700CQ_ID (imu_FXOS8700CQ_t * obj);

void       FXOS8700CQ_ConfigureAccelerometer(imu_FXOS8700CQ_t * obj);

void       FXOS8700CQ_ConfigureMagnetometer(imu_FXOS8700CQ_t * obj);
uint8_t    FXOS8700CQ_PollMagnetometer (imu_FXOS8700CQ_t * obj);

char       FXOS8700CQ_GetTemperature(imu_FXOS8700CQ_t * obj);

void       FXOS8700CQ_Set_Origin(imu_FXOS8700CQ_t * object);

void       FXOS8700CQ_Magnetic_Vector(imu_FXOS8700CQ_t * obj);
int16_t    FXOS8700CQ_Get_Heading(imu_FXOS8700CQ_t *obj);
static void FXOS8700CQ_Imu_Int_Handler(uint8_t pin, imu_FXOS8700CQ_t * obj);
static void ImuTempAdjustment(imu_FXOS8700CQ_t * obj);
void       FXOS8700CQ_Cal_Scaling(imu_FXOS8700CQ_t *obj,int16_t temperature, int16_t z_average);
void       FXOS8700CQ_Door_State_Poll(imu_FXOS8700CQ_t * obj);
void       FXOS8700CQ_Init_Interrupt (imu_FXOS8700CQ_t * obj);
void       FXOS8700CQ_Vector_Angle(imu_FXOS8700CQ_t * obj);
void       FXOS8700CQ_Calculate_Vector(imu_FXOS8700CQ_t * obj);
void       FXOS8700CQ_Calibrate(imu_FXOS8700CQ_t * obj);

void       FXOS8700CQ_WriteByte(imu_FXOS8700CQ_t * obj, char internal_addr, char value);
void       FXOS8700CQ_WriteByteArray(imu_FXOS8700CQ_t * obj, char internal_addr, uint8_t* buffer, char length);
char       FXOS8700CQ_ReadByte(imu_FXOS8700CQ_t * obj, char internal_addr);
void       FXOS8700CQ_ReadByteArray(imu_FXOS8700CQ_t * obj, char internal_addr, char *buffer, uint32_t length);

#endif // USE_FREERTOS == 1

#endif
