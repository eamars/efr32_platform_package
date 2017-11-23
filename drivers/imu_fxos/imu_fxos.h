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

#define FXOS8700CQ_ADDRESS    0x1F
#define M_THRESHOLD           20
#define M_VECTOR_DBNCE        0
#define POLL_THRESH           10

/**************************STATUS Register********************************/
#define ZYXOW_MASK            0x80
#define ZOW_MASK              0x40
#define YOW_MASK              0x20
#define XOW_MASK              0x10
#define ZYXDR_MASK            0x08
#define ZDR_MASK              0x04
#define YDR_MASK              0x02
#define XDR_MASK              0x01

/**************************STATUS Register********************************/
#define F_OVF_MASK            0x80
#define F_WMRK_FLAG_MASK      0x40
#define F_CNT5_MASK           0x20
#define F_CNT4_MASK           0x10
#define F_CNT3_MASK           0x08
#define F_CNT2_MASK           0x04
#define F_CNT1_MASK           0x02
#define F_CNT0_MASK           0x01
#define F_CNT_MASK            0x3F

/**************************STATUS Register********************************/
#define OUT_X_MSB_REG         0x01
#define OUT_X_LSB_REG         0x02
#define OUT_Y_MSB_REG         0x03
#define OUT_Y_LSB_REG         0x04
#define OUT_Z_MSB_REG         0x05
#define OUT_Z_LSB_REG         0x06

/**************************FIFO Register********************************/
#define F_MODE1_MASK          0x80
#define F_MODE0_MASK          0x40
#define F_WMRK5_MASK          0x20
#define F_WMRK4_MASK          0x10
#define F_WMRK3_MASK          0x08
#define F_WMRK2_MASK          0x04
#define F_WMRK1_MASK          0x02
#define F_WMRK0_MASK          0x01
#define F_MODE_MASK           0xC0
#define F_WMRK_MASK           0x3F

#define F_MODE_DISABLED       0x00
#define F_MODE_CIRCULAR       (F_MODE0_MASK)
#define F_MODE_FILL           (F_MODE1_MASK)
#define F_MODE_TRIGGER        (F_MODE1_MASK+F_MODE0_MASK)

/**************************TRIG_CFG Register********************************/
#define TRIG_TRANS_MASK       0x20
#define TRIG_LNDPRT_MASK      0x10
#define TRIG_PULSE_MASK       0x08
#define TRIG_FF_MT_MASK       0x04

/**************************SYSMOD Register********************************/
#define FGERR_MASK            0x80
#define FGT_4_MASK            0x40
#define FGT_3_MASK            0x20
#define FGT_2_MASK            0x10
#define FGT_1_MASK            0x08
#define FGT_0_MASK            0x04
#define FGT_MASK              0x7C
#define SYSMOD1_MASK          0x02
#define SYSMOD0_MASK          0x01
#define SYSMOD_MASK           0x03

#define SYSMOD_STANDBY        0x00
#define SYSMOD_WAKE           (SYSMOD0_MASK)
#define SYSMOD_SLEEP          (SYSMOD1_MASK)

/**************************INT_SOURCE Register********************************/
#define SRC_ASLP_MASK         0x80
#define SRC_FIFO_MASK         0x40
#define SRC_TRANS_MASK        0x20
#define SRC_LNDPRT_MASK       0x10
#define SRC_PULSE_MASK        0x08
#define SRC_FF_MT_MASK        0x04
#define SRC_DRDY_MASK         0x01

/***********************WHO_AM_I Device ID Register****************************/
#define FXOS8700CQ            0xC7
#define MXOS8700CQ            0xC4

/****************XYZ_DATA_CFG Sensor Data Configuration Register***************/
#define HPF_OUT_MASK          0x10    // MMA8451 and MMA8452 only
#define FS1_MASK              0x02
#define FS0_MASK              0x01
#define FS_MASK               0x03

#define FULL_SCALE_2G         0x00
#define FULL_SCALE_4G         (FS0_MASK)
#define FULL_SCALE_8G         (FS1_MASK)


/************HP_FILTER_CUTOFF High Pass Filter Register **********************/
#define PULSE_HPF_BYP_MASK    0x20
#define PULSE_LPF_EN_MASK     0x10
#define SEL1_MASK             0x02
#define SEL0_MASK             0x01
#define SEL_MASK              0x03

/*************PL_STATUS Portrait/Landscape Status Register *******************/
#define NEWLP_MASK            0x80
#define LO_MASK               0x40
#define LAPO1_MASK            0x04
#define LAPO0_MASK            0x02
#define BAFRO_MASK            0x01
#define LAPO_MASK             0x06


/************** PL_CFG Portrait/Landscape Configuration Register***************/
#define DBCNTM_MASK           0x80
#define PL_EN_MASK            0x40

/*****PL_BF_ZCOMP Back/Front and Z Compensation Register***********************/

#define BKFR1_MASK            0x80
#define BKFR0_MASK            0x40
#define ZLOCK2_MASK           0x04
#define ZLOCK1_MASK           0x02
#define ZLOCK0_MASK           0x01
#define BKFR_MASK             0xC0
#define ZLOCK_MASK            0x07

/************PL_P_L_THS Portrait to Landscape Threshold Register***************/
#define PL_P_L_THS_REG        0x14

#define P_L_THS4_MASK         0x80
#define P_L_THS3_MASK         0x40
#define P_L_THS2_MASK         0x20
#define P_L_THS1_MASK         0x10
#define P_L_THS0_MASK         0x08
#define HYS2_MASK             0x04
#define HYS1_MASK             0x02
#define HYS0_MASK             0x01
#define P_L_THS_MASK          0xF8
#define HYS_MASK              0x07

/********************FF_MT_CFG Freefall and Motion Configuration Register******/
#define FF_MT_CFG_REG         0x15

#define ELE_MASK              0x80
#define OAE_MASK              0x40
#define ZEFE_MASK             0x20
#define YEFE_MASK             0x10
#define XEFE_MASK             0x08

/**********FF_MT_SRC Freefall and Motion Source Registers**********************/
#define FF_MT_SRC_REG         0x16

#define EA_MASK               0x80
#define ZHE_MASK              0x20
#define ZHP_MASK              0x10
#define YHE_MASK              0x08
#define YHP_MASK              0x04
#define XHE_MASK              0x02
#define XHP_MASK              0x01

/*****FF_MT_THS Freefall and Motion Threshold Registers*********************/
#define FT_MT_THS_REG         0x17
#define TRANSIENT_THS_REG     0x1F

#define DBCNTM_MASK           0x80
#define THS6_MASK             0x40
#define THS5_MASK             0x20
#define THS4_MASK             0x10
#define THS3_MASK             0x08
#define THS2_MASK             0x04
#define TXS1_MASK             0x02
#define THS0_MASK             0x01
#define THS_MASK              0x7F


/********FF_MT_COUNT Freefall Motion Count Registers************************/
#define FF_MT_COUNT_REG       0x18

/****************TRANSIENT_CFG Transient Configuration Register****************/

#define TELE_MASK             0x10
#define ZTEFE_MASK            0x08
#define YTEFE_MASK            0x04
#define XTEFE_MASK            0x02
#define HPF_BYP_MASK          0x01

/***********TRANSIENT_SRC Transient Source Register****************************/

#define TEA_MASK              0x40
#define ZTRANSE_MASK          0x20
#define Z_TRANS_POL_MASK      0x10
#define YTRANSE_MASK          0x08
#define Y_TRANS_POL_MASK      0x04
#define XTRANSE_MASK          0x02
#define X_TRANS_POL_MASK      0x01


/**************************PULSE_CFG Register****************************/

#define DPA_MASK              0x80
#define PELE_MASK             0x40
#define ZDPEFE_MASK           0x20
#define ZSPEFE_MASK           0x10
#define YDPEFE_MASK           0x08
#define YSPEFE_MASK           0x04
#define XDPEFE_MASK           0x02
#define XSPEFE_MASK           0x01


/**************************PULSE_SRC Register********************************/

#define PEA_MASK              0x80
#define AXZ_MASK              0x40
#define AXY_MASK              0x20
#define AXX_MASK              0x10
#define DPE_MASK              0x08
#define POLZ_MASK             0x04
#define POLY_MASK             0x02
#define POLX_MASK             0x01

#define PTHS_MASK             0x7F

/***************CTRL_REG1 System Control 1 Register****************************/
#define ASLP_RATE1_MASK       0x80
#define ASLP_RATE0_MASK       0x40
#define DR2_MASK              0x20
#define DR1_MASK              0x10
#define DR0_MASK              0x08
#define LNOISE_MASK           0x04
#define FREAD_MASK            0x02
#define ACTIVE_MASK           0x01
#define ASLP_RATE_MASK        0xC0
#define DR_MASK               0x38

#define ASLP_RATE_20MS        0x00
#define ASLP_RATE_80MS        (ASLP_RATE0_MASK)
#define ASLP_RATE_160MS       (ASLP_RATE1_MASK)
#define ASLP_RATE_640MS       (ASLP_RATE1_MASK+ASLP_RATE0_MASK)

#define ASLP_RATE_50HZ        (ASLP_RATE_20MS)
#define ASLP_RATE_12_5HZ      (ASLP_RATE_80MS)
#define ASLP_RATE_6_25HZ      (ASLP_RATE_160MS)
#define ASLP_RATE_1_56HZ      (ASLP_RATE_640MS)

#define HYB_ASLP_RATE_25HZ        (ASLP_RATE_20MS)
#define HYB_ASLP_RATE_6_25HZ      (ASLP_RATE_80MS)
#define HYB_ASLP_RATE_1_56HZ      (ASLP_RATE_160MS)
#define HYB_ASLP_RATE_0_8HZ       (ASLP_RATE_640MS)

#define DATA_RATE_1250US      0x00
#define DATA_RATE_2500US      (DR0_MASK)
#define DATA_RATE_5MS         (DR1_MASK)
#define DATA_RATE_10MS        (DR1_MASK+DR0_MASK)
#define DATA_RATE_20MS        (DR2_MASK)
#define DATA_RATE_80MS        (DR2_MASK+DR0_MASK)
#define DATA_RATE_160MS       (DR2_MASK+DR1_MASK)
#define DATA_RATE_640MS       (DR2_MASK+DR1_MASK+DR0_MASK)

#define DATA_RATE_800HZ       (DATA_RATE_1250US)
#define DATA_RATE_400HZ       (DATA_RATE_2500US)
#define DATA_RATE_200HZ       (DATA_RATE_5MS)
#define DATA_RATE_100HZ       (DATA_RATE_10MS)
#define DATA_RATE_50HZ        (DATA_RATE_20MS)
#define DATA_RATE_12_5HZ      (DATA_RATE_80MS)
#define DATA_RATE_6_25HZ      (DATA_RATE_160MS)
#define DATA_RATE_1_56HZ      (DATA_RATE_640MS)

#define HYB_DATA_RATE_400HZ   (DATA_RATE_1250US)
#define HYB_DATA_RATE_200HZ   (DATA_RATE_2500US)
#define HYB_DATA_RATE_100HZ   (DATA_RATE_5MS)
#define HYB_DATA_RATE_50HZ    (DATA_RATE_10MS)
#define HYB_DATA_RATE_25HZ    (DATA_RATE_20MS)
#define HYB_DATA_RATE_6_25HZ  (DATA_RATE_80MS)
#define HYB_DATA_RATE_3_15HZ  (DATA_RATE_160MS)
#define HYB_DATA_RATE_0_8HZ   (DATA_RATE_640MS)

#define ACTIVE                (ACTIVE_MASK)
#define STANDBY               0x00

/***************CTRL_REG2 System Control 2 Register****************************/
#define CTRL_REG2             0x2B

#define ST_MASK               0x80
#define RST_MASK              0x40
#define SMODS1_MASK           0x10
#define SMODS0_MASK           0x08
#define SLPE_MASK             0x04
#define MODS1_MASK            0x02
#define MODS0_MASK            0x01
#define SMODS_MASK            0x18
#define MODS_MASK             0x03

#define SMOD_NORMAL           0x00
#define SMOD_LOW_NOISE        (SMODS0_MASK)
#define SMOD_HIGH_RES         (SMODS1_MASK)
#define SMOD_LOW_POWER        (SMODS1_MASK+SMODS0_MASK)

#define MOD_NORMAL            0x00
#define MOD_LOW_NOISE         (MODS0_MASK)
#define MOD_HIGH_RES          (MODS1_MASK)
#define MOD_LOW_POWER         (MODS1_MASK+MODS0_MASK)


/***************CTRL_REG3 System Control 3 Register****************************/
#define FIFO_GATE_MASK        0x80
#define WAKE_TRANS_MASK       0x40
#define WAKE_LNDPRT_MASK      0x20
#define WAKE_PULSE_MASK       0x10
#define WAKE_FF_MT_MASK       0x08
#define IPOL_MASK             0x02
#define PP_OD_MASK            0x01

/***************CTRL_REG4 System Control 4 Register****************************/
#define INT_EN_ASLP_MASK      0x80
#define INT_EN_FIFO_MASK      0x40
#define INT_EN_TRANS_MASK     0x20
#define INT_EN_LNDPRT_MASK    0x10
#define INT_EN_PULSE_MASK     0x08
#define INT_EN_FF_MT_MASK     0x04
#define INT_EN_DRDY_MASK      0x01

/***************CTRL_REG5 System Control 5 Register****************************/
#define INT_CFG_ASLP_MASK     0x80
#define INT_CFG_FIFO_MASK     0x40
#define INT_CFG_TRANS_MASK    0x20
#define INT_CFG_LNDPRT_MASK   0x10
#define INT_CFG_PULSE_MASK    0x08
#define INT_CFG_FF_MT_MASK    0x04
#define INT_CFG_DRDY_MASK     0x01

/*
**  XYZ Offset Correction Registers
*/
#define OFF_X_REG             0x2F
#define OFF_Y_REG             0x30
#define OFF_Z_REG             0x31

/*
** M_THS_CFG Magnetic threshold
 */
#define M_THS_ELE             0x80
#define M_THS_OAE             0x40
#define M_THS_ZEFE            0x20
#define M_THS_YEFE            0x10
#define M_THS_XEFE            0x08
#define M_THS_WAKE_EN         0x04
#define M_THS_INT_EN          0x02
#define M_THS_INT_CFG         0x01


/*
**  MAG CTRL_REG1 System Control 1 Register
*/
#define M_ACAL_MASK           0x80
#define M_RST_MASK            0x40
#define M_OST_MASK            0x20
#define M_OSR2_MASK           0x10
#define M_OSR1_MASK           0x08
#define M_OSR0_MASK           0x04
#define M_HMS1_MASK           0x02
#define M_HMS0_MASK           0x01
#define M_OSR_MASK            0x1C
#define M_HMS_MASK            0x03

//OSR Selections
#define M_OSR_1_56_HZ         0x00
#define M_OSR_6_25_HZ         M_OSR0_MASK
#define M_OSR_12_5_HZ         M_OSR1_MASK
#define M_OSR_50_HZ           M_OSR1_MASK | M_OSR0_MASK
#define M_OSR_100_HZ          M_OSR2_MASK
#define M_OSR_200_HZ          M_OSR2_MASK | M_OSR0_MASK
#define M_OSR_400_HZ          M_OSR2_MASK | M_OSR1_MASK
#define M_OSR_800_HZ          M_OSR2_MASK | M_OSR1_MASK | M_OSR0_MASK

//Hybrid Mode Selection
#define ACCEL_ACTIVE          0x00
#define MAG_ACTIVE            M_HMS0_MASK
#define HYBRID_ACTIVE         (M_HMS1_MASK | M_HMS0_MASK)

/*
**  MAG CTRL_REG2 System Control 2 Register
*/

#define M_HYB_AUTOINC_MASK    0x20
#define M_MAXMIN_DIS_MASK     0x10
#define M_MAXMIN_DIS_THS_MASK 0x08
#define M_MAXMIN_RST_MASK     0x04
#define M_RST_CNT1_MASK       0x02
#define M_RST_CNT0_MASK       0x01

//Mag Auto-Reset De-Gauss Frequency
#define RST_ODR_CYCLE         0x00
#define RST_16_ODR_CYCLE      M_RST_CNT0_MASK
#define RST_512_ODR_CYCLE     M_RST_CNT1_MASK
#define RST_DISABLED          M_RST_CNT1_MASK+M_RST_CNT0_MASK

/*
**  MAG CTRL_REG3 System Control 3 Register
*/

#define M_RAW_MASK            0x80
#define M_ASLP_OS_2_MASK      0x40
#define M_ASLP_OS_1_MASK      0x20
#define M_ASLP_OS_0_MASK      0x10
#define M_THS_XYZ_MASK        0x08
#define M_ST_Z_MASK           0x04
#define M_ST_XY1_MASK         0x02
#define M_ST_XY0_MASK         0x01
#define M_ASLP_OSR_MASK       0x70
#define M_ST_XY_MASK          0x03

//OSR Selections
#define M_ASLP_OSR_1_56_HZ    0x00
#define M_ASLP_OSR_6_25_HZ    M_ASLP_OS_0_MASK
#define M_ASLP_OSR_12_5_HZ    M_ASLP_OS_1_MASK
#define M_ASLP_OSR_50_HZ      M_ASLP_OS_1_MASK+M_ASLP_OS_0_MASK
#define M_ASLP_OSR_100_HZ     M_ASLP_OS_2_MASK
#define M_ASLP_OSR_200_HZ     M_ASLP_OS_2_MASK+M_ASLP_OS_0_MASK
#define M_ASLP_OSR_400_HZ     M_ASLP_OS_2_MASK+M_ASLP_OS_1_MASK
#define M_ASLP_OSR_800_HZ     M_ASLP_OS_2_MASK+M_ASLP_OS_1_MASK+M_ASLP_OS_0_MASK

/*
**  MAG INT SOURCE Register
*/
#define M_INT_SOURCE          0x5E

#define SRC_M_DRDY_MASK       0x04
#define SRC_M_VECM_MASK       0x02
#define SRC_M_THS_MASK        0x01

/*
**  ACCEL VECTOR CONFIG Register
*/

#define A_VECM_INIT_CFG_MASK  0x40
#define A_VECM_INIT_EN_MASK   0x20
#define A_VECM_WAKE_EN_MASK   0x10
#define A_VECM_EN_MASK        0x08
#define A_VECM_UPDM_MASK      0x04
#define A_VECM_INITM_MASK     0x02
#define A_VECM_ELE_MASK       0x01

/*
**  ACCEL VECTOR THS MSB AND LSB Register
*/

#define A_VECM_DBCNTM_MASK    0x80

/*
**  MAG VECTOR CONFIG Register
*/



#define M_VECM_ELE_MASK       0x40
#define M_VECM_INITM_MASK     0x20
#define M_VECM_UPDM_MASK      0x10
#define M_VECM_EN_MASK        0x08
#define M_VECM_WAKE_EN_MASK   0x04
#define M_VECM_INIT_EN_MASK   0x02
#define M_VECM_INIT_CFG_MASK  0x01
/*
**  MAG VECTOR THS MSB AND LSB Register
*/

#define M_VECM_DBCNTM_MASK    0x80

/*
**  ACCEL FFMT THS X MSB AND LSB Register
*/

#define A_FFMT_THS_XYZ_EN_MASK 0x80
#define A_FFMT_THS_X_LSB_MASK  0xFC

/*
**  ACCEL FFMT THS Y MSB AND LSB Register
*/

#define A_FFMT_THS_Y_EN_MASK 0x80
#define A_FFMT_THS_Y_LSB_MASK  0xFC

/*
**  ACCEL FFMT THS Z MSB AND LSB Register
*/

#define A_FFMT_THS_Z_EN_MASK        0x80
#define A_FFMT_THS_Z_LSB_MASK       0xFC

#define FXOS8700CQ_WHOAMI_VAL   0xC7        // FXOS8700CQ WHOAMI production register value
#define FXOS8700CQ_READ_LEN     12          // 6 channels of two bytes = 12 bytes
#define UINT14_MAX              16383       // For processing the accelerometer data to right-justified 2's complement

/*****************************************************************************/
// register addresses
#define  IMU_STATUS       0x00
#define  DR_STATUS        0x00
#define  F_STATUS         0x00
#define  OUT_X_MSB        0x01
#define  OUT_X_LSB        0x02
#define  OUT_Y_MSB        0x03
#define  OUT_Y_LSB        0x04
#define  OUT_Z_MSB        0x05
#define  OUT_Z_LSB        0x06
#define  F_SETUP          0x09
#define  TRIG_CFG         0x0A
#define  SYSMOD           0x0B
#define  INT_SOURCE       0x0C
#define  WHO_AM_I         0x0D
#define  XYZ_DATA_CFG     0x0E
#define  HP_FILTER_CUTOFF 0x0F
#define  PL_STATUS        0x10
#define  PL_CFG           0x11
#define  PL_COUNT         0x12
#define  PL_BF_ZCOMP      0x13
#define  P_L_THS_REG      0x14
#define  A_FFMT_CFG       0x15
#define  A_FFMT_SRC       0x16
#define  A_FFMT_THS       0x17
#define  A_FFMT_COUNT     0x18
#define  TRANSIENT_CFG    0x1D
#define  TRANSIENT_SRC    0x1E
#define  TRANSIENT_THS    0x1F
#define  TRANSIENT_COUNT  0x20
#define  PULSE_CFG        0x21
#define  PULSE_SRC        0x22
#define  PULSE_THSX       0x23
#define  PULSE_THSY       0x24
#define  PULSE_THSZ       0x25
#define  PULSE_TMLT       0x26
#define  PULSE_LTCY       0x27
#define  PULSE_WIND       0x28
#define  ASLP_COUNT       0x29
#define  CTRL_REG1        0x2A
#define  CTRL_REG2        0x2B
#define  CTRL_REG3        0x2C
#define  CTRL_REG4        0x2D
#define  CTRL_REG5        0x2E
#define  OFF_X            0x2F
#define  OFF_Y            0x30
#define  OFF_Z            0x31
#define  M_DR_STATUS      0x32
#define  M_OUT_X_MSB      0x33
#define  M_OUT_X_LSB      0x34
#define  M_OUT_Y_MSB      0x35
#define  M_OUT_Y_LSB      0x36
#define  M_OUT_Z_MSB      0x37
#define  M_OUT_Z_LSB      0x38
#define  CMP_OUT_X_MSB    0x39
#define  CMP_OUT_X_LSB    0x3A
#define  CMP_OUT_Y_MSB    0x3B
#define  CMP_OUT_Y_LSB    0x3C
#define  CMP_OUT_Z_MSB    0x3D
#define  CMP_OUT_Z_LSB    0x3E
#define  M_OFF_X_MSB      0x3F
#define  M_OFF_X_LSB      0x40
#define  M_OFF_Y_MSB      0x41
#define  M_OFF_Y_LSB      0x42
#define  M_OFF_Z_MSB      0x43
#define  M_OFF_Z_LSB      0x44
#define  MAX_X_MSB        0x45
#define  MAX_X_LSB        0x46
#define  MAX_Y_MSB        0x47
#define  MAX_Y_LSB        0x48
#define  MAX_Z_MSB        0x49
#define  MAX_Z_LSB        0x4A
#define  MIN_X_MSB        0x4B
#define  MIN_X_LSB        0x4C
#define  MIN_Y_MSB        0x4D
#define  MIN_Y_LSB        0x4E
#define  MIN_Z_MSB        0x4F
#define  MIN_Z_LSB        0x50
#define  TEMP             0x51
#define  M_THS_CFG        0x52
#define  M_THS_SRC        0x53
#define  M_THS_X_MSB      0x54
#define  M_THS_X_LSB      0x55
#define  M_THS_Y_MSB      0x56
#define  M_THS_Y_LSB      0x57
#define  M_THS_Z_MSB      0x58
#define  M_THS_Z_LSB      0x59
#define  M_THS_COUNT      0x5A
#define  M_CTRL_REG1      0x5B
#define  M_CTRL_REG2      0x5C
#define  M_CTRL_REG3      0x5D
#define  M_INT_SRC        0x5E
#define  A_VECM_CFG       0x5F
#define  A_VECM_THS_MSB   0x60
#define  A_VECM_THS_LSB   0x61
#define  A_VECM_CNT       0x62
#define  A_VECM_INITX_MSB 0x63
#define  A_VECM_INITX_LSB 0x64
#define  A_VECM_INITY_MSB 0x65
#define  A_VECM_INITY_LSB 0x66
#define  A_VECM_INITZ_MSB 0x67
#define  A_VECM_INITZ_LSB 0x68
#define  M_VECM_CFG       0x69
#define  M_VECM_THS_MSB   0x6A
#define  M_VECM_THS_LSB   0x6B
#define  M_VECM_CNT       0x6C
#define  M_VECM_INITX_MSB 0x6D
#define  M_VECM_INITX_LSB 0x6E
#define  M_VECM_INITY_MSB 0x6F
#define  M_VECM_INITY_LSB 0x70
#define  M_VECM_INITZ_MSB 0x71
#define  M_VECM_INITZ_LSB 0x72
#define  A_FFMT_THS_X_MSB 0x73
#define  A_FFMT_THS_X_LSB 0x74
#define  A_FFMT_THS_Y_MSB 0x75
#define  A_FFMT_THS_Y_LSB 0x76
#define  A_FFMT_THS_Z_MSB 0x77
#define  A_FFMT_THS_Z_LSB 0x78

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


typedef struct
{
    /// pins
    pio_t int_1;
    pio_t int_2;


    i2cdrv_t * i2c_device;
    pio_t enable;
    bool initialized;
	imu_event_t door_state;
    bool calibrated;
    int16_t x_origin;
    int16_t y_origin;
    int16_t z_origin;
    int16_t start_position;
    uint32_t vector;
    int16_t vector_angle;

    int16_t current_compass; // The current angle
    int16_t current_heading; // The heading away from calibrated angle

    QueueHandle_t imu_event_queue;
    imu_event_t last_event;
    uint32_t last_call;
    TaskHandle_t ImuTempHandler;
    int16_t temp;

    TaskHandle_t ImuCalHandler;
    uint16_t vector_threshold_closed;
    uint16_t vector_threshold_open;



} imu_FXOS8700CQ_t;


void       FXOS8700CQ_Initialize(imu_FXOS8700CQ_t * obj, i2cdrv_t * i2c_device, pio_t enable, pio_t int_1, pio_t int_2);
char       FXOS8700CQ_ReadStatusReg(imu_FXOS8700CQ_t * obj);
void       FXOS8700CQ_ActiveMode (imu_FXOS8700CQ_t * obj);
char       FXOS8700CQ_StandbyMode (imu_FXOS8700CQ_t * obj);
void       FXOS8700CQ_HybridMode(imu_FXOS8700CQ_t * obj);
char       FXOS8700CQ_GetChipMode(imu_FXOS8700CQ_t * obj);
char       FXOS8700CQ_ID (imu_FXOS8700CQ_t * obj);

void       FXOS8700CQ_ConfigureAccelerometer(imu_FXOS8700CQ_t * obj);
void       FXOS8700CQ_PollAccelerometer (imu_FXOS8700CQ_t * obj, rawdata_t *accel_data);
void       FXOS8700CQ_HighPassFilter(imu_FXOS8700CQ_t * obj, char status);
void       FXOS8700CQ_FullScaleRange(imu_FXOS8700CQ_t * obj, range_t range);
void       FXOS8700CQ_SetAccelerometerDynamicRange(imu_FXOS8700CQ_t * obj, range_t range);

void       FXOS8700CQ_ConfigureMagnetometer(imu_FXOS8700CQ_t * obj);
uint8_t       FXOS8700CQ_PollMagnetometer (imu_FXOS8700CQ_t * obj, rawdata_t *mag_data);
char       FXOS8700CQ_MagnetometerStatus(imu_FXOS8700CQ_t * obj);

void       FXOS8700CQ_GetData(imu_FXOS8700CQ_t * obj, rawdata_t *accel_data, rawdata_t *magn_data);

void       FXOS8700CQ_FIFOMode(imu_FXOS8700CQ_t * obj, FX_mode_t mode);
char       FXOS8700CQ_GetODR(imu_FXOS8700CQ_t * obj);
void       FXOS8700CQ_SetODR (imu_FXOS8700CQ_t * obj, char DataRateValue);
char       FXOS8700CQ_GetTemperature(imu_FXOS8700CQ_t * obj);

char       FXOS8700CQ_GetOrientation(imu_FXOS8700CQ_t * obj);
void       FXOS8700CQ_ConfigureOrientation(imu_FXOS8700CQ_t * obj);
void       FXOS8700CQ_ConfigureGenericTapMode(imu_FXOS8700CQ_t * obj);
void       FXOS8700CQ_ConfigureSingleTapMode(imu_FXOS8700CQ_t * obj);
void       FXOS8700CQ_ConfigureDoubleTapMode(imu_FXOS8700CQ_t * obj);

void       FXOS8700CQ_Set_Origin(imu_FXOS8700CQ_t * object);
void       FXOS8700CQ_Magnetic_Threshold_Setting(imu_FXOS8700CQ_t * obj);
void       FXOS8700CQ_Magnetic_Vector(imu_FXOS8700CQ_t * obj);
int16_t    FXOS8700CQ_Get_Heading(imu_FXOS8700CQ_t *obj);
static void FXOS8700CQ_Imu_Int_Handler(uint8_t pin, imu_FXOS8700CQ_t * obj);
static void ImuTempAdjustment(imu_FXOS8700CQ_t * obj);
void       FXOS8700CQ_Door_State_Poll(imu_FXOS8700CQ_t * obj);
void       FXOS8700CQ_Init_Interupt (imu_FXOS8700CQ_t * obj);
void       FXOS8700CQ_Vector_Angle(imu_FXOS8700CQ_t * obj);
void       FXOS8700CQ_Caclculate_Vector(imu_FXOS8700CQ_t * obj);
void       FXOS8700CQ_Calibrate(imu_FXOS8700CQ_t * obj);

void       FXOS8700CQ_ModifyBytes(imu_FXOS8700CQ_t * obj, char internal_addr, char value, char mask);
void       FXOS8700CQ_WriteByte(imu_FXOS8700CQ_t * obj, char internal_addr, char value);
void       FXOS8700CQ_WriteByteArray(imu_FXOS8700CQ_t * obj, char internal_addr, uint8_t* buffer, char length);
char       FXOS8700CQ_ReadByte(imu_FXOS8700CQ_t * obj, char internal_addr);
void       FXOS8700CQ_ReadByteArray(imu_FXOS8700CQ_t * obj, char internal_addr, char *buffer, uint32_t length);

#endif // USE_FREERTOS == 1

#endif
