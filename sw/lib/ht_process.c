/*
 * Copyright (c) 2013 Norbert S. <junky-zs@gmx.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <avr/interrupt.h>
#include <util/atomic.h>
#include <string.h>
#include "ht_process.h"
#include "ht_error.h"
#include "ht_io.h"

static uart_rx_buffer_t  uart_rx_buffer;
static uart_rx_control_t rx_ctrl[UART_CONTROL_BUFSIZE];

msg_buffer_t ht_send_buffer;
uint8_t polling_address;
static uint8_t last_destination = 0;
static uint8_t dest_counter = 0;
static uint8_t uart_rx_buffer_index = 0;

void ht_process_init(void)
{
	/* init rx_buffer handling */
	for (uint8_t uindex=0; uindex < UART_CONTROL_BUFSIZE; uindex++) {
		rx_ctrl[uindex].rx_index = 0;
		rx_ctrl[uindex].rx_length= 0;
	}
	uart_rx_buffer.leading = 0;
	uart_rx_buffer.tail    = 0;
	
	ht_send_buffer.len = 0;
	ht_send_buffer.sent= 0;
	
	/* init hardware */
	io_port_init();
  io_uart_init();
  io_t2_init();
	OFF_STATUS_LED;
	OFF_TX_ERROR_LED;
}

uint8_t ht_process_calc_checksum(uint8_t * const buffer, uint8_t size)
{
	uint8_t crc = 0, d;

	for (uint8_t i = 0; i < size; i++) {
		d = 0;
		if (crc & 0x80) {
			crc ^= 0xc;
			d = 1;
		}
		crc <<= 1;
		crc &= 0xfe;
		crc |= d;
		crc = crc ^ buffer[i];
	}
	return crc;
}


int8_t ht_process_txdata(uint8_t *p_data, uint16_t len)
{
	int8_t rtn=0;
	if (len == 0) {	
		rtn = ERR_COMMON; 
	} else {
		uint16_t diff = ht_send_buffer.len - ht_send_buffer.sent;
		if (diff == 0 && len <= (BUFFER_LEN - 1)) 
		{
			/* Copy the data to the send buffer */
			ht_send_buffer.addr_byte = (g_ht_cfg.our_address | CLIENT_ADDRESS_MASK);
			memcpy(ht_send_buffer.data, p_data, len);
			ht_send_buffer.data[len] = ht_process_calc_checksum(&ht_send_buffer.addr_byte, len + 1);
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				ht_send_buffer.len = len + 1 /* data + checksum */;
				ht_send_buffer.sent = 0;
			}
			rtn=len;
		} else {
			rtn = ERR_BUFFER_FULL;
		}
	}
	if(rtn < 0){
		ON_TX_ERROR_LED;
	} else {
		OFF_TX_ERROR_LED;
	}
	return rtn;
}

void ht_process_received_byte(uint8_t data, uint8_t status)
{
	static uint8_t packet_bytes = 0;
	static uint8_t last_data=0;

	if (status & FRAMEEND) {
		/* end-of-frame */
		uart_rx_buffer.data[uart_rx_buffer_index] = 0;
		rx_ctrl[uart_rx_buffer.leading].rx_length = packet_bytes+1;
		uart_rx_buffer_index++;
		uart_rx_buffer_index %= UART_INPUT_BUFSIZE;
		polling_address = (packet_bytes == 1) ? last_data : 0;
		if (packet_bytes > 1 && last_destination == (g_ht_cfg.our_address | CLIENT_ADDRESS_MASK)) {
			uart_got_response();
		}
		/* setup buffer-control for next frame */
		uart_rx_buffer.leading++;
		uart_rx_buffer.leading %= UART_CONTROL_BUFSIZE;
		rx_ctrl[uart_rx_buffer.leading].rx_index  = uart_rx_buffer_index;
		rx_ctrl[uart_rx_buffer.leading].rx_length = 0;

		packet_bytes = 0;
		dest_counter = 0;
		last_destination = 0;
	} else if (status & ERROR) {
		/* error -> drop */
	} else {
		uart_rx_buffer.data[uart_rx_buffer_index] = data;
		uart_rx_buffer_index++;
		uart_rx_buffer_index %= UART_INPUT_BUFSIZE;
		last_data = data;
		packet_bytes++;
		dest_counter++;
		if (dest_counter == 2) {
			last_destination = data;
		}
	}
}

uint8_t ht_process_rxdata(uart_rx_buffer_t * const p_rxbuffer)
{
	uint8_t rtn=0;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (ht_process_frame_received())
		{
			if (rx_ctrl[uart_rx_buffer.tail].rx_length > 0)
			{
				p_rxbuffer->count = rx_ctrl[uart_rx_buffer.tail].rx_length;
				rx_ctrl[uart_rx_buffer.tail].rx_length=0;

				uint8_t max_index=rx_ctrl[uart_rx_buffer.tail].rx_index + p_rxbuffer->count;
				if(max_index < UART_INPUT_BUFSIZE) {
					memcpy(p_rxbuffer->data, &uart_rx_buffer.data[rx_ctrl[uart_rx_buffer.tail].rx_index], p_rxbuffer->count);
				} else {
					uint8_t rest_bytes =max_index - UART_INPUT_BUFSIZE;
					uint8_t first_bytes=UART_INPUT_BUFSIZE - rx_ctrl[uart_rx_buffer.tail].rx_index;
					memcpy(&p_rxbuffer->data[0], &uart_rx_buffer.data[rx_ctrl[uart_rx_buffer.tail].rx_index], first_bytes);
					memcpy(&p_rxbuffer->data[first_bytes], &uart_rx_buffer.data[0], rest_bytes);
				}
				rtn=1;
			}
			uart_rx_buffer.tail++;
			uart_rx_buffer.tail %= UART_CONTROL_BUFSIZE;
		}
	} // end ATOMIC Block
	return rtn;
}

uint8_t ht_process_frame_received(void)
{
	return ((uart_rx_buffer.leading != uart_rx_buffer.tail) ? 1 : 0);
}
