/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech
 ___ _____ _   ___ _  _____ ___  ___  ___ ___
/ __|_   _/_\ / __| |/ / __/ _ \| _ \/ __| __|
\__ \ | |/ _ \ (__| ' <| _| (_) |   / (__| _|
|___/ |_/_/ \_\___|_|\_\_| \___/|_|_\\___|___|
embedded.connectivity.solutions===============

Description: LoRa MAC layer implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis ( Semtech ), Gregory Cristian ( Semtech ) and Daniel Jaeckle ( STACKFORCE )
*/

/**
 * @brief Encryption engine for LoRaWAN implementation
 * @date 20, Oct, 2017
 * @file lorawan_mac_crypto.h
 * @author Semtech
 */
#include <string.h>
#include "lorawan_mac_crypto.h"
#include "aes.h"
#include "cmac.h"

/*!
 * CMAC/AES Message Integrity Code (MIC) Block B0 size
 */
#define LORAMAC_MIC_BLOCK_B0_SIZE                   16

/*!
 * MIC field computation initial data
 */
static uint8_t MicBlockB0[] = { 0x49, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/*!
 * Contains the computed MIC field.
 *
 * \remark Only the 4 first bytes are used
 */
static uint8_t Mic[16];

/*!
 * Encryption aBlock and sBlock
 */
static uint8_t aBlock[] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static uint8_t sBlock[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/*!
 * AES computation context variable
 */
static aes_context AesContext;

/*!
 * CMAC computation context variable
 */
static AES_CMAC_CTX AesCmacCtx[1];


void lorawan_mac_compute_mic(const uint8_t * buffer, uint16_t size, const uint8_t * key, uint32_t addr,
                             lorawan_mac_comm_direction_t direction, uint32_t counter, uint32_t * mic)
{
	MicBlockB0[5] = (uint8_t) direction;

	memcpy(&MicBlockB0[6], &addr, 4);
	memcpy(&MicBlockB0[10], &counter, 4);

	MicBlockB0[15] = (uint8_t) (size & 0xFF);

	AES_CMAC_Init( AesCmacCtx );

	AES_CMAC_SetKey( AesCmacCtx, key );

	AES_CMAC_Update( AesCmacCtx, MicBlockB0, LORAMAC_MIC_BLOCK_B0_SIZE );

	AES_CMAC_Update( AesCmacCtx, buffer, (uint8_t) (size & 0xFF) );

	AES_CMAC_Final( Mic, AesCmacCtx );

	*mic = ( uint32_t )( ( uint32_t )Mic[3] << 24 | ( uint32_t )Mic[2] << 16 | ( uint32_t )Mic[1] << 8 | ( uint32_t )Mic[0] );
}

void lorawan_mac_join_decrypt(const uint8_t * in_buffer, uint16_t size, const uint8_t * key, uint8_t * out_buffer)
{
	memset( AesContext.ksch, '\0', 240 );
	aes_set_key( key, 16, &AesContext );
	aes_encrypt( in_buffer, out_buffer, &AesContext );
	// Check if optional CFList is included
	if( size >= 16 )
	{
		aes_encrypt( in_buffer + 16, out_buffer + 16, &AesContext );
	}
}

void lorawan_mac_join_compute_mic(const uint8_t * in_buffer, uint16_t size, const uint8_t * key, uint32_t * mic)
{
	AES_CMAC_Init( AesCmacCtx );

	AES_CMAC_SetKey( AesCmacCtx, key );

	AES_CMAC_Update( AesCmacCtx, in_buffer, (uint8_t) (size & 0xFF) );

	AES_CMAC_Final( Mic, AesCmacCtx );

	*mic = ( uint32_t )( ( uint32_t )Mic[3] << 24 | ( uint32_t )Mic[2] << 16 | ( uint32_t )Mic[1] << 8 | ( uint32_t )Mic[0] );
}

void lorawan_mac_join_compute_session_key(const uint8_t * aes_key, const uint8_t * app_nonce, uint16_t dev_nonce,
                                          uint8_t * network_session_key, uint8_t * application_session_key)
{
	uint8_t nonce[16];
	uint8_t *pDevNonce = ( uint8_t * )&dev_nonce;

	memset( AesContext.ksch, '\0', 240 );
	aes_set_key( aes_key, 16, &AesContext );

	memset( nonce, 0, sizeof( nonce ) );
	nonce[0] = 0x01;
	memcpy( nonce + 1, app_nonce, 6 );
	memcpy( nonce + 7, pDevNonce, 2 );
	aes_encrypt( nonce, network_session_key, &AesContext );

	memset( nonce, 0, sizeof( nonce ) );
	nonce[0] = 0x02;
	memcpy( nonce + 1, app_nonce, 6 );
	memcpy( nonce + 7, pDevNonce, 2 );
	aes_encrypt( nonce, application_session_key, &AesContext );
}

void lorawan_mac_payload_decrypt(const uint8_t * in_buffer, uint16_t size, const uint8_t * key, uint32_t address,
                                 lorawan_mac_comm_direction_t direction, uint32_t counter, uint8_t * out_buffer)
{
	lorawan_mac_payload_encrypt(in_buffer, size, key, address, direction, counter, out_buffer);
}

void lorawan_mac_payload_encrypt(const uint8_t * in_buffer, uint16_t size, const uint8_t * key, uint32_t address,
                                 lorawan_mac_comm_direction_t direction, uint32_t counter, uint8_t * out_buffer)
{
	uint16_t i;
	uint8_t bufferIndex = 0;
	uint16_t ctr = 1;

	memset( AesContext.ksch, '\0', 240 );
	aes_set_key( key, 16, &AesContext );

	aBlock[5] = (uint8_t) direction;

	memcpy(&aBlock[6], &address, 4);
	memcpy(&aBlock[10], &counter, 4);

	while( size >= 16 )
	{
		aBlock[15] = (uint8_t) ( ( ctr ) & 0xFF );
		ctr++;
		aes_encrypt( aBlock, sBlock, &AesContext );
		for( i = 0; i < 16; i++ )
		{
			out_buffer[bufferIndex + i] = in_buffer[bufferIndex + i] ^ sBlock[i];
		}
		size -= 16;
		bufferIndex += 16;
	}

	if( size > 0 )
	{
		aBlock[15] = (uint8_t) ( ( ctr ) & 0xFF );
		aes_encrypt( aBlock, sBlock, &AesContext );
		for( i = 0; i < size; i++ )
		{
			out_buffer[bufferIndex + i] = in_buffer[bufferIndex + i] ^ sBlock[i];
		}
	}
}
