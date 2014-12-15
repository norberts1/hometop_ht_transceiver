/*
 * Copyright (c) 2011 by Danny Baumann <dannybaumann@web.de>
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

#ifndef __HT_PROCESS_H__
  #define __HT_PROCESS_H__

/* Enable c-linkage */
#if defined (__cplusplus)
	extern "C" {
#endif


#define OUR_ADDRESS				0x0d	// Modem
#define CLIENT_ADDRESS_MASK	0x80
#define MSG_TYPE_RESP			0xff
#define MSG_RESPONSE_OK		0x01
#define MSG_RESPONSE_FAIL	0x04

#define BUFFER_LEN           64
#define UART_INPUT_BUFSIZE   64
#define UART_CONTROL_BUFSIZE 10


typedef struct UART_RX_CONTROL {
	uint8_t	rx_index;
	uint8_t	rx_length;
} uart_rx_control_t;

typedef struct UART_RX_BUFFER {
	uint8_t	leading;
	uint8_t tail;
  uint8_t data[UART_INPUT_BUFSIZE];
  uint8_t count;
} uart_rx_buffer_t;

struct ht_status {
  uint32_t total_bytes;
  uint32_t good_bytes;
  uint32_t dropped_bytes;
  uint32_t onebyte_packets;
  uint32_t onebyte_own_packets;
  uint32_t onebyte_ack_packets;
  uint32_t onebyte_nack_packets;
  uint32_t good_packets;
  uint32_t bad_packets;
  uint32_t dropped_packets;
  uint32_t buffer_overflow;
  uint8_t max_fill;
};

typedef struct MSG_BUFFER {
  uint8_t len;       /* message-length to be send */
  uint8_t sent;
  uint8_t addr_byte; /* space for address byte when calculating TX checksum */
  uint8_t data[BUFFER_LEN];
} msg_buffer_t;

typedef struct HT_CFG {
	uint8_t our_address;
	uint8_t run_mode;
} ht_cfg_t;

extern volatile ht_cfg_t g_ht_cfg;

void    ht_process_init(void);
uint8_t ht_process_rxdata(uart_rx_buffer_t * const);
 int8_t ht_process_txdata(uint8_t *data, uint16_t len);
uint8_t ht_process_calc_checksum(uint8_t * const buffer, uint8_t size);
void    ht_process_received_byte(uint8_t data, uint8_t status);
uint8_t ht_process_frame_received(void);

void    uart_got_response(void);
int8_t  uart_check_timeout(void);


#define FRAMEEND _BV(0)
#define ERROR    _BV(1)


extern uint8_t polling_address;
extern msg_buffer_t ht_send_buffer;

//#define HT_DEBUG_STATS
#ifdef HT_DEBUG_STATS
	struct ht_status ht_status_buffer;
	#define UPDATE_STATS(value,count) (ht_status_buffer. value += count)
#else
	#define UPDATE_STATS(...)
#endif


#ifdef HT_PROTO_DEBUG
# define HT_PROTODEBUG(a...)  debug_printf("ht: " a)
#else
# define HT_PROTODEBUG(a...)
#endif

#ifdef HT_IO_DEBUG
# define HT_IODEBUG(a...)  debug_printf("ht: " a)
#else
# define HT_IODEBUG(a...)
#endif

#ifdef HT_ERROR_DEBUG
# define HT_ERRORDEBUG(a...)  debug_printf("ht: " a)
#else
# define HT_ERRORDEBUG(a...)
#endif

#if defined (__cplusplus)
	} //extern "C"
#endif

#endif // __HT_PROCESS_H__
