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

#ifndef __HT_IO_H__
#define __HT_IO_H__

/* Enable c-linkage */
#if defined (__cplusplus)
	extern "C" {
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <assert.h>


/* Define 'PITINY' is used for ATtiny and activated in project-configuration */
/* Deactivate for ATmega 328 */


/* LED defines */
#ifndef PITINY
	#define STATUS_LED	DDD7
	#define LED_TX			DDD5
#else
	#define STATUS_LED	DDRA0
	#define LED_TX			DDRA3
#endif	

#define LED_RX_OK		STATUS_LED
#define LED_TX_FAIL		LED_TX
#define NUM_LEDS		2

#define OUTPUT	1
#define INPUT		0
#define ON      OUTPUT
#define OFF			INPUT


#ifndef PITINY
	/* Testpoint defines */
	#define TP_IN_BIT		PINB0
	#define TP_OUT_BIT	PINB1
	#define TOGGLE_TP_OUT (io_toggle_portpin(PORTB_NR, TP_OUT_BIT))
	#define SET_TP_OUT(p) (io_set_portpin(PORTB_NR, TP_OUT_BIT, p))
	#define CLEAR_TP_OUT  (io_set_portpin(PORTB_NR, TP_OUT_BIT, OFF))
	#define GETSTATUS_TP_IN (io_getstatus_portpin(PORTB_NR, TP_IN_BIT))

	/* Status-/Error-LED defines */
	#define TOGGLE_STATUS_LED (io_toggle_portpin(PORTD_NR, STATUS_LED))
	#define ON_STATUS_LED     (io_set_led(PORTD_NR, STATUS_LED, ON))
	#define OFF_STATUS_LED    (io_set_led(PORTD_NR, STATUS_LED, OFF))
	#define ON_TX_ERROR_LED   (io_set_led(PORTD_NR, LED_TX_FAIL, ON))
	#define OFF_TX_ERROR_LED  (io_set_led(PORTD_NR, LED_TX_FAIL, OFF))
	
	/* HT-bus enable port-bit */
	#define ON_HT_BUS_ENABLE  (io_set_led(PORTB_NR, PINB2, ON))
	#define OFF_HT_BUS_ENABLE (io_set_led(PORTB_NR, PINB2, OFF))
#else
	/* Testpoint defines */
	#define TP_IN_BIT		PINB2
	#define TP_OUT_BIT	PINA7
	#define TOGGLE_TP_OUT (io_toggle_portpin(PORTA_NR, TP_OUT_BIT))
	#define SET_TP_OUT(p) (io_set_portpin(PORTA_NR, TP_OUT_BIT, p))
	#define CLEAR_TP_OUT  (io_set_portpin(PORTA_NR, TP_OUT_BIT, OFF))
	#define GETSTATUS_TP_IN (io_getstatus_portpin(PORTB_NR, TP_IN_BIT))

	/* Status-/Error-LED defines */
	#define TOGGLE_STATUS_LED (io_toggle_portpin(PORTA_NR, STATUS_LED))
	#define ON_STATUS_LED     (io_set_led(PORTA_NR, STATUS_LED, ON))
	#define OFF_STATUS_LED    (io_set_led(PORTA_NR, STATUS_LED, OFF))
	#define ON_TX_ERROR_LED   (io_set_led(PORTA_NR, LED_TX_FAIL, ON))
	#define OFF_TX_ERROR_LED  (io_set_led(PORTA_NR, LED_TX_FAIL, OFF))
	
	/* HT-bus enable port-bit */	
	#define ON_HT_BUS_ENABLE  (io_set_led(PORTA_NR, PINA7, ON))
	#define OFF_HT_BUS_ENABLE (io_set_led(PORTA_NR, PINA7, OFF))
#endif	


// UART used time- and speed-definitions
#ifndef BAUD
	#define BAUD 9600
#endif
#define BAUD_HT 9600

// time-out values
#define TX_TIMEOUT 10 /* x100ms = 1 second */

#ifndef PITINY
	#define PRESCALE      8
	#define PRESCALE_HT 128
	#define F_CPU 16000000UL
	#define BAUD_SW_UART 19200
	#define BIT_TIME_SW_UART (uint16_t)(((F_CPU + BAUD_SW_UART/2) / BAUD_SW_UART) / (uint16_t)PRESCALE)
	#define BIT_TIME_HT      (uint8_t)(((F_CPU + BAUD/2) / BAUD) / PRESCALE_HT)
#else
	#define PRESCALE      8
	#define PRESCALE_HT   8
	#define F_CPU  10000000UL
	#define BAUD_UART0 19200
	#define BAUD_UART1 BAUD_HT
	#define BIT_TIME_UART0   (uint16_t)(((F_CPU + BAUD_UART0/2) / BAUD_UART0) / (uint16_t)16)
	#define BIT_TIME_HT      (uint8_t)(((F_CPU + BAUD/2) / BAUD) / PRESCALE_HT)
#endif

typedef struct BYTE_EXCHANGE {
	volatile uint8_t bytedata;
	volatile uint8_t bitcounter;
	volatile uint8_t done;
} byte_exchange_t;

#define IF_BUFFER_LEN 64
typedef struct IF_MSG_BUFFER {
	int8_t leading;
	int8_t tail;
	uint8_t data[IF_BUFFER_LEN];
} if_msg_buffer_t;

typedef enum {
	PORTX_NR,
	PORTA_NR,
	PORTB_NR,
	PORTC_NR,
	PORTD_NR
} portnr_t;


void io_set_led       (portnr_t portnr, uint8_t LED, uint8_t enable);
void io_set_portpin   (portnr_t portnr, uint8_t port_bit, uint8_t on);
void io_toggle_portpin(portnr_t portnr, uint8_t port_bit);
void io_port_init    (void);

uint8_t io_getstatus_portpin   (portnr_t portnr, uint8_t port_bit);


void io_t2_init(void);
void io_uart_init(void);


//************* HW-Uart deklarations ********************
// UART Rx-function
uint8_t uart_getchar( void );
// UART Tx-function
void uart_putchar(uint8_t txbyte);

#ifndef PITINY
	//************* SW-Uart deklarations ********************
	void    swuart_init( void );
	void    swuart_putchar( uint8_t val );
	uint8_t swuart_getchar( void );
	void    swuart_puts( const char * p );
	uint8_t swuart_char_received(void);
#endif

// timer-fkt's
void    io_set_countdown_timer_100msec(uint8_t msec_slices);
uint8_t io_get_countdown_timer_100msec(void);
uint8_t io_wait_100usec(uint8_t usec_slices);
uint8_t io_wait_msec(uint8_t msec_slices);

void uart_send_break(void);


#if defined (__cplusplus)
} //extern "C"
#endif

#endif //end __HT_IO_H__