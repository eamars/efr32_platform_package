/**
 * @brief RFM9x LoRa/FSK module driver
 */

#ifndef RADIO_RFM9X_H_
#define RADIO_RFM9X_H_

#include "spidrv.h"
#include "pio_defs.h"
#include "radio_rfm9x_regs.h"

#include "FreeRTOS.h"
#include "semphr.h"

// The crystal oscillator frequency of the module
#define RH_RF95_FXOSC 32000000.0

// The Frequency Synthesizer step = RH_RF95_FXOSC / 2^^19
#define RH_RF95_FSTEP  (RH_RF95_FXOSC / 524288)

#define RH_RSSI_OFFSET (-137)

// Maximum a packet can be copied from chip to local
#define RADIO_RFM9X_RW_BUFFER_SIZE (0xFFUL)

#define RADIO_RFM9X_LOCAL_BUFFER_SIZE (0xFFUL * 4)

typedef enum
{
	RADIO_RFM9X_MODEM_FSK = 0,
	RADIO_RFM9X_MODEM_LORA = 1
} radio_rfm9x_modem_t;

typedef enum
{
	RADIO_RFM9X_SLEEP = 0,
	RADIO_RFM9X_IDLE,
	RADIO_RFM9X_TX,
	RADIO_RFM9X_RX,
	RADIO_RFM9X_CAD
} radio_rfm9x_state_t;

typedef enum
{
	RADIO_RFM9X_BW_7K8 = 0x0,
	RADIO_RFM9X_BW_10K4 = 0x1,
	RADIO_RFM9X_BW_15K6 = 0x2,
	RADIO_RFM9X_BW_20K8 = 0x3,
	RADIO_RFM9X_BW_32K25 = 0x4,
	RADIO_RFM9X_BW_41K7 = 0x5,
	RADIO_RFM9X_BW_62K5 = 0x6,
	RADIO_RFM9X_BW_125K = 0x7,
	RADIO_RFM9X_BW_250K = 0x8,
	RADIO_RFM9X_BW_500K = 0x9
} radio_rfm9x_bw_t;

typedef enum
{
	RADIO_RFM9X_CR_4_5 = 1,
	RADIO_RFM9X_CR_4_6 = 2,
	RADIO_RFM9X_CR_4_7 = 3,
	RADIO_RFM9X_CR_4_8 = 4
} radio_rfm9x_cr_t;

typedef enum
{
	// expressed in the power of 2
	RADIO_RFM9X_SF_64 = 6,
	RADIO_RFM9X_SF_128 = 7,
	RADIO_RFM9X_SF_256 = 8,
	RADIO_RFM9X_SF_512 = 9,
	RADIO_RFM9X_SF_1024 = 10,
	RADIO_RFM9X_SF_2048 = 11,
	RADIO_RFM9X_SF_4096 = 12
} radio_rfm9x_sf_t;

typedef enum
{
	RADIO_RFM9X_OP_SLEEP = RH_RF95_MODE_SLEEP,
	RADIO_RFM9X_OP_STDBY = RH_RF95_MODE_STDBY,
	RADIO_RFM9X_OP_FSTX = RH_RF95_MODE_FSTX,
	RADIO_RFM9X_OP_TX = RH_RF95_MODE_TX,
	RADIO_RFM9X_OP_FSRX = RH_RF95_MODE_FSRX,
	RADIO_RFM9X_OP_RX = RH_RF95_MODE_RXCONTINUOUS,
	RADIO_RFM9X_OP_RXSINGLE = RH_RF95_MODE_RXSINGLE,
	RADIO_RFM9X_OP_CAD = RH_RF95_MODE_CAD
} radio_rfm9x_op_t;


typedef void (*on_rx_done_handler)(uint8_t * payload, int16_t size, int16_t rssi, int8_t snr) ;
typedef void (*on_tx_done_handler)(void);

typedef struct
{
	// hardware pins
	pio_t rst;
	pio_t miso;
	pio_t mosi;
	pio_t clk;
	pio_t cs;
	pio_t dio0;

	// SPI driver
	SPIDRV_HandleData_t spi_handle_data;
	SemaphoreHandle_t spi_access_mutex;

	// radio status
	radio_rfm9x_state_t radio_state;
	int16_t last_packet_rssi;
	int8_t last_packet_snr;

	// packet buffer
	uint32_t local_buffer_read_ptr;
	uint32_t local_buffer_write_ptr;
	SemaphoreHandle_t local_buffer_mutex;
	uint8_t local_buffer[RADIO_RFM9X_LOCAL_BUFFER_SIZE];

	// handlers
	on_rx_done_handler on_rx_done;
	on_tx_done_handler on_tx_done;

} radio_rfm9x_t;


#ifdef __cplusplus
extern "C" {
#endif


void radio_rfm9x_init(radio_rfm9x_t * obj,
                      pio_t rst, pio_t miso, pio_t mosi, pio_t clk, pio_t cs,
                      pio_t dio0, pio_t dio1, pio_t dio2, pio_t dio3, pio_t dio4, pio_t dio5
);

void radio_rfm9x_hard_reset(radio_rfm9x_t * obj);

void radio_rfm9x_set_channel(radio_rfm9x_t * obj, uint32_t freq);

void radio_rfm9x_set_tx_power_use_rfo(radio_rfm9x_t * obj, int8_t power_dbm, bool use_rfo);
#define radio_rfm9x_set_tx_power(obj, power_dbm) \
		radio_rfm9x_set_tx_power_use_rfo((obj), (power_dbm), false)

void radio_rfm9x_set_modem(radio_rfm9x_t * obj, radio_rfm9x_modem_t modem);

void radio_rfm9x_set_preamble_length(radio_rfm9x_t * obj, uint16_t bytes);

/**
 * @brief Configure radio bandwidth. Note, in the lower band (169MHz), the signal bandwidth 250KHz and 500KHz are not available
 * @param obj radio object
 * @param bandwidth desired bandwidth
 */
void radio_rfm9x_set_bandwidth(radio_rfm9x_t * obj, radio_rfm9x_bw_t bandwidth);

void radio_rfm9x_set_coding_rate(radio_rfm9x_t * obj, radio_rfm9x_cr_t coding_rate);

void radio_rfm9x_set_implicit_header_mode_on(radio_rfm9x_t * obj, bool is_implicit_header);

void radio_rfm9x_set_spreading_factor(radio_rfm9x_t * obj, radio_rfm9x_sf_t spreading_factor);

void radio_rfm9x_set_opmode_idle(radio_rfm9x_t * obj);
void radio_rfm9x_set_opmode_tx(radio_rfm9x_t * obj);
void radio_rfm9x_set_opmode_rx(radio_rfm9x_t * obj);

void radio_rfm9x_send(radio_rfm9x_t * obj, void * buffer, uint8_t bytes);
uint32_t radio_rfm9x_recv(radio_rfm9x_t * obj, void * buffer, uint32_t bytes);


#ifdef __cplusplus
}
#endif

#endif // RADIO_RFM9X_H_
