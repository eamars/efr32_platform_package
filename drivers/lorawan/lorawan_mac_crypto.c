/**
 * @brief Encryption engine for LoRaWAN implementation
 * @date 20, Oct, 2017
 * @file lorawan_mac_crypto.h
 * @author Semtech
 */

#include "lorawan_mac_crypto.h"

void lorawan_mac_compute_mic(const uint8_t * buffer, uint16_t size, const uint8_t * key, uint32_t addr,
                             lorawan_mac_comm_direction_t direction, uint32_t counter, uint32_t * mic)
{
	*mic = 0xffffffff;
}

void lorawan_mac_join_decrypt(const uint8_t * in_buffer, uint16_t size, const uint8_t * key, uint8_t * out_buffer)
{

}

void lorawan_mac_join_compute_mic(const uint8_t * in_buffer, uint16_t size, const uint8_t * key, uint32_t * mic)
{
	*mic = 0xffffffff;
}

void lorawan_mac_join_compute_session_key(const uint8_t * aes_key, const uint8_t * app_nonce, uint16_t dev_nonce,
                                          uint8_t * network_session_key, uint8_t * application_session_key)
{

}

void lorawan_mac_payload_decrypt(const uint8_t * in_buffer, uint16_t size, const uint8_t * key, uint32_t address,
                                 lorawan_mac_comm_direction_t direction, uint32_t counter, uint8_t * out_buffer)
{

}
