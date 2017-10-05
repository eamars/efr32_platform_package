/**
 * @brief RFM9x LoRa/FSK module driver
 */

#ifndef RADIO_RFM9X_H_
#define RADIO_RFM9X_H_

#include "spidrv.h"
#include "pio_defs.h"

// The crystal oscillator frequency of the module
#define RH_RF95_FXOSC 32000000.0

// The Frequency Synthesizer step = RH_RF95_FXOSC / 2^^19
#define RH_RF95_FSTEP  (RH_RF95_FXOSC / 524288)


typedef struct
{
	pio_t rst;
	pio_t miso;
	pio_t mosi;
	pio_t clk;
	pio_t cs;
	pio_t dio0;

	SPIDRV_HandleData_t spi_handle_data;
} radio_rfm9x_t;

typedef enum
{
	RADIO_RFM9X_MODEM_FSK = 0,
	RADIO_RFM9X_MODEM_LORA = 1
} radio_rfm9x_modem_t;


#ifdef __cplusplus
extern "C" {
#endif


void radio_rfm9x_init(radio_rfm9x_t * obj,
                      pio_t rst, pio_t miso, pio_t mosi, pio_t clk, pio_t cs,
                      pio_t dio0, pio_t dio1, pio_t dio2, pio_t dio3, pio_t dio4, pio_t dio5
);

void radio_rfm9x_hard_reset(radio_rfm9x_t * obj);

void radio_rfm9x_set_channel(radio_rfm9x_t * obj, uint32_t freq);

void radio_rfm9x_set_tx_power(radio_rfm9x_t * obj, int8_t power_dbm);

void radio_rfm9x_set_modem(radio_rfm9x_t * obj, radio_rfm9x_modem_t modem);

void radio_rfm9x_set_opmode(radio_rfm9x_t * obj, uint8_t opmode);

void radio_rfm9x_rx_chain_calibration(radio_rfm9x_t * obj);


#ifdef __cplusplus
}
#endif

#endif // RADIO_RFM9X_H_
