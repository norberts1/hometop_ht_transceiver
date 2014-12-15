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
/*
 * used HW-resources:
 *  1. timer2/OCR2A  for break-signal generation
 *  2. usart         used as uart with rx- and tx-pns
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "ht_process.h"
#include "ht_io.h"

/*
 * HT theory:
 *
 * Polling: master sends (address)
 * Answer from client:
 * 1. nothing to send: <addr | 0x80> <break>
 * 2. Broadcast: <addr | 0x80> 0x0 <data> ... <break>
 * 3. Send without request: <addr | 0x80> <dest | 0x80> <data> ... <crc> <break>
 * 4. Send with request: <addr | 0x80> <dest> <data> ... <crc> <break>
 *
 * -> send <addr | 0x80> <txdata> <break>
 *
 * after tx byte compare with rx
 * if mismatch -> abort with <break>
 */

#define STATE_RX                 0
#define STATE_TX_ADDR            1
#define STATE_TX_ADDR_WAIT_ECHO  2
#define STATE_TX_DATA            3
#define STATE_TX_DATA_WAIT_ECHO  4
#define STATE_TX_BREAK           5

#define MODE_RX	0
#define MODE_TX 1


static volatile uint8_t protocoll_state = STATE_RX;
static volatile uint8_t tx_packet_start= 0;
static volatile uint8_t last_sent_byte = 0;
static volatile uint8_t last_send_dest = 0;
static volatile uint8_t _uart_sendbreak_active=0;

static volatile enum {
    NOT_WAITING,
    WAIT_FOR_ACK,
    WAIT_FOR_RESPONSE
} response_wait_mode = NOT_WAITING;

void uart_got_response(void)
{
	if (response_wait_mode == WAIT_FOR_RESPONSE) {
    /* pretend being polled to trigger sending terminating address */
		polling_address = g_ht_cfg.our_address;
		response_wait_mode = NOT_WAITING;
	}
}


static inline uint8_t uart_is_polled(void)
{
	uint8_t polled = (polling_address == g_ht_cfg.our_address);
	if (response_wait_mode == WAIT_FOR_ACK) {
		if (polling_address == MSG_RESPONSE_OK) {
			UPDATE_STATS(onebyte_ack_packets, 1);
		}
		if (polling_address == MSG_RESPONSE_FAIL) {
			UPDATE_STATS(onebyte_nack_packets, 1);
		}
		if (polling_address == MSG_RESPONSE_OK || polling_address == MSG_RESPONSE_FAIL) {
			polled = 1;
		}
	} //end if (response_wait_mode == WAIT_FOR_ACK)
  if (polled) {
    response_wait_mode = NOT_WAITING;
  }
  polling_address = 0;
  return polled;
} //end uart_is_polled()

static void uart_switch_mode(uint8_t tx)
{
#ifndef PITINY
	volatile uint8_t reg = UCSR0B;
	if (tx == MODE_TX) {
		reg &= (uint8_t)~(_BV(RXEN0) | _BV(RXCIE0));
		reg |= _BV(UDRIE0);
	} else {
		reg &= (uint8_t)~_BV(UDRIE0);
		reg |= _BV(RXEN0) | _BV(RXCIE0);
	}
	UCSR0B = reg;
#else
	volatile uint8_t reg = UCSR1B;
	if (tx == MODE_TX) {
		reg &= (uint8_t)~(_BV(RXEN1) | _BV(RXCIE1));
		reg |= _BV(UDRIE1);
	} else {
		reg &= (uint8_t)~_BV(UDRIE1);
		reg |= _BV(RXEN1) | _BV(RXCIE1);
	}
	UCSR1B = reg;
#endif	
}

static void uart_go_to_rx(void)
{
  /* dummy read input buffer until empty */
#ifndef PITINY
	while (UCSR0A & _BV(RXC0)) {
		uint8_t data = UDR0;
		(void) data;
	}
#else
	while (UCSR1A & _BV(RXC1)) {
		uint8_t data = UDR1;
		(void) data;
	}
#endif	
	protocoll_state = STATE_RX;
	uart_switch_mode(MODE_RX);
}


int8_t uart_check_timeout(void)
{
	uint8_t ertn=0;
	if(io_get_countdown_timer_100msec() == 0) {
		// timer stopped
		ertn=0;
	} else	if(io_get_countdown_timer_100msec() <= 1) {
		io_set_countdown_timer_100msec(0);
		uart_go_to_rx();
		ertn = -1;
	}
	return ertn;
}


void uart_putchar(uint8_t txbyte) {
#ifndef PITINY
	// Wait for empty transmit buffer
	while ( !(UCSR0A & _BV(UDRE0)) );
	ON_TX_ERROR_LED;
	// clear TX complete-flag by writing a one to its bit location
	UCSR0A = _BV(TXC0);
	// transmit byte	
	UDR0 = txbyte;
	// Wait until byte is transmitted
	while ( !(UCSR0A & _BV(TXC0)) );
	OFF_TX_ERROR_LED;
#else
	// Wait for empty transmit buffer
	while ( !(UCSR1A & _BV(UDRE1)) );
	ON_TX_ERROR_LED;
	// clear TX complete-flag by writing a one to its bit location
	UCSR1A = _BV(TXC1);
	// transmit byte
	UDR1 = txbyte;
	// Wait until byte is transmitted
	while ( !(UCSR1A & _BV(TXC1)) );
	OFF_TX_ERROR_LED;
#endif	
}

/**
 * get byte
 */
uint8_t uart_getchar(void)
{
	// wait until byte received
#ifndef PITINY
	while(!(UCSR0A & _BV(RXC0)));				
	uint8_t rx_byte=UDR0;
	return rx_byte;
#else	
	while(!(UCSR1A & _BV(RXC1)));
	uint8_t rx_byte=UDR1;
	return rx_byte;
#endif	
}



void uart_send_break(void)
{
#ifndef PITINY
	// Wait for empty transmit buffer
	while ( !(UCSR0A & _BV(UDRE0)) );
	
	// wait for send_break signal end
	while (_uart_sendbreak_active);
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// set flag do block further send_breaks
		_uart_sendbreak_active=1;
	}
	
	/* Disable 'Empty Interrupt' and Uart-tx mode  */
	/*  TX Port-pin is set to: output and Low-state to 
	 *  generate 'Break-status' for 11 bits
	*/
	UCSR0B &= (uint8_t)~(_BV(UDRIE0) | _BV(TXEN0) | _BV(TXCIE0));
	// set TX-port to low-state and Output
	PORTD &= (uint8_t)~_BV(PORTD1);
	DDRD |= _BV(DDD1);

	/* set handling-state */	
	protocoll_state = STATE_TX_BREAK;
	/* set Output compare register A to: 
	 *  current counter-value + BIT_TIME_HT * 10 bittimes
	 */
	OCR2A = TCNT2 + BIT_TIME_HT * 10;
	/* clear timer2 compareA interrupt-flag and
	 * set Timer2 Interrupt Mask register for
	 *  enable compareA Interrupt
	 */
	TIFR2 = _BV(OCF2A);
	TIMSK2 |= _BV(OCIE2A);
#else
	// Wait for empty transmit buffer
	while ( !(UCSR1A & _BV(UDRE1)) );
	
	// wait for send_break signal end
	while (_uart_sendbreak_active);
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// set flag do block further send_breaks
		_uart_sendbreak_active=1;
	}
	
	/* Disable 'Empty Interrupt' and Uart-tx mode  */
	/*  TX Port-pin is set to: output and Low-state to 
	 *  generate 'Break-status' for 11 bits
	*/
	UCSR1B &= (uint8_t)~(_BV(UDRIE1) | _BV(TXEN1) | _BV(TXCIE1));
	// set TX-port to low-state and Output
	PORTA &= (uint8_t)~_BV(PORTA5);
	DDRA |= _BV(DDRA5);

	/* set handling-state */	
	protocoll_state = STATE_TX_BREAK;
	/* set Output compare register A to: 
	 *  current counter-value + BIT_TIME_HT * 10 bittimes
	 */
	OCR2A = TCNT2 + BIT_TIME_HT * 10;
	/* clear timer2 compareA interrupt-flag and
	 * set Timer2 Interrupt Mask register for
	 *  enable compareA Interrupt
	 */
	TIFR2 = _BV(OCF2A);
	TIMSK2 |= _BV(OCIE2A);
#endif	
} //end uart_send_break()

/* Timer/Counter2 Compare Match A Interrupt Handler */
ISR(TIMER2_COMPA_vect)
{
#ifndef PITINY
	if (_uart_sendbreak_active)
	{
		/* Break-Status end, so clear Timer2 compareA Interrupt Mask register
		 * reenable UART-TX output
		 */
		TIMSK2 &= (uint8_t)~_BV(OCIE2A);
		// set TX-port to Output and High-state
		DDRD  |= _BV(DDD1);
		PORTD |= _BV(PORTD1);
		/* reenable USART transmitter, 
		 * disable normal port-operation on UART TX-Pin
		 */
		UCSR0B |= _BV(TXEN0);
		// reset timeout
		io_set_countdown_timer_100msec(0);
		OFF_TX_ERROR_LED;
		// reset break_active-flag
		_uart_sendbreak_active=0;
		uart_go_to_rx();
	}
#else
	if (_uart_sendbreak_active)
	{
		/* Break-Status end, so clear Timer2 compareA Interrupt Mask register
		 * reenable UART-TX output
		 */
		TIMSK2 &= (uint8_t)~_BV(OCIE2A);
		// set TX-port to Output and High-state
		DDRA  |= _BV(DDRA5);
		PORTA |= _BV(PORTA5);
		/* reenable USART transmitter, 
		 * disable normal port-operation on UART TX-Pin
		 */
		UCSR1B |= _BV(TXEN1);
		// reset timeout
		io_set_countdown_timer_100msec(0);
		OFF_TX_ERROR_LED;
		// reset break_active-flag
		_uart_sendbreak_active=0;
		uart_go_to_rx();
	}
#endif
	//Clear compare match flag 2A
	TIFR2 = _BV(OCF2A);
} //end ISR(TIMER2_COMPA_vect)

/* USART Tx Complete Interrupt Handler */
#ifndef PITINY
	ISR(USART_TX_vect)
#else
	ISR(USART1_TX_vect)
#endif	
{
	/* TX finished, now send break */
	uart_send_break();
}

/* USART, Data Register Empty Interrupt Handler */
#ifndef PITINY
	ISR(USART_UDRE_vect)
#else
	ISR(USART1_UDRE_vect)
#endif	
{
	switch (protocoll_state) 
	{
		case STATE_TX_ADDR:
			protocoll_state = STATE_TX_ADDR_WAIT_ECHO;
			// set timeout to 1 sec
			io_set_countdown_timer_100msec(TX_TIMEOUT);
			tx_packet_start = ht_send_buffer.sent;
			last_sent_byte = (g_ht_cfg.our_address | CLIENT_ADDRESS_MASK);
			ON_TX_ERROR_LED;
#ifndef PITINY
			UDR0 = (g_ht_cfg.our_address | CLIENT_ADDRESS_MASK);
#else
			UDR1 = (g_ht_cfg.our_address | CLIENT_ADDRESS_MASK);
#endif			
			uart_switch_mode(MODE_RX);
		break;
		case STATE_TX_DATA:
			if (ht_send_buffer.sent < ht_send_buffer.len) {
				uint8_t byte = ht_send_buffer.data[ht_send_buffer.sent];
				if (ht_send_buffer.sent == tx_packet_start) {
					/* byte is the destination address */
					if (byte & CLIENT_ADDRESS_MASK) {
						response_wait_mode = WAIT_FOR_RESPONSE;
					} else {
						response_wait_mode = WAIT_FOR_ACK;
					}
					last_send_dest = byte & ~CLIENT_ADDRESS_MASK;
				}
				ht_send_buffer.sent++;
				last_sent_byte = byte;
				ON_TX_ERROR_LED;
#ifndef PITINY
				UDR0 = byte;
#else
				UDR1 = byte;
#endif				
				protocoll_state = STATE_TX_DATA_WAIT_ECHO;
				// set timeout to 1 sec
				io_set_countdown_timer_100msec(TX_TIMEOUT);
				uart_switch_mode(MODE_RX);
			} else {
				/* wait for TX to finish */
#ifndef PITINY
				UCSR0B &= (uint8_t)~_BV(UDRIE0);
				UCSR0B |= _BV(TXCIE0);
#else
				UCSR1B &= (uint8_t)~_BV(UDRIE1);
				UCSR1B |= _BV(TXCIE1);
#endif				
			}
		break;
		case STATE_TX_BREAK:
		default:
			/* Disable this interrupt */
#ifndef PITINY
			UCSR0B &= (uint8_t)~_BV(UDRIE0);
#else
			UCSR1B &= (uint8_t)~_BV(UDRIE1);
#endif
			uart_go_to_rx();
		break;
	}
} //end ISR(USART_UDRE_vect)

/* USART Rx Complete Interrupt Handler */
#ifndef PITINY
	ISR(USART_RX_vect)
#else
	ISR(USART1_RX_vect)
#endif
{
	uint8_t rx_status, rx_data;

	switch (protocoll_state) 
	{
		case STATE_TX_ADDR_WAIT_ECHO:
		case STATE_TX_DATA_WAIT_ECHO:
#ifndef PITINY
			rx_status = UCSR0A;
			if (rx_status & _BV(RXC0))
			{
				// reset timeout
				io_set_countdown_timer_100msec(0);
				/* read rx-data */
				rx_data = UDR0;
#else
			rx_status = UCSR1A;
			if (rx_status & _BV(RXC1))
			{
				// reset timeout
				io_set_countdown_timer_100msec(0);
				/* read rx-data */
				rx_data = UDR1;
#endif				
				if (last_sent_byte != rx_data) {
					/* mismatch -> abort */
					HT_ERRORDEBUG("Last sent byte %02x, echo %02x -> MISMATCH\n", last_sent_byte, rx_data);
					ht_send_buffer.sent = tx_packet_start;
					uart_send_break();
				} else {
					protocoll_state = STATE_TX_DATA;
				}
				uart_switch_mode(MODE_TX);
			}
		break;
		default:
#ifndef PITINY
			while ((rx_status = UCSR0A) & _BV(RXC0)) 
			{
				uint8_t real_status = 0;
				/* read rx-data */
				rx_data = UDR0;
				if (rx_status & _BV(FE0)) {
					real_status |= FRAMEEND;
					OFF_STATUS_LED;
				} else {
					ON_STATUS_LED;
				}
				if (rx_status & (_BV(DOR0) | _BV(UPE0)) ) {
					real_status |= ERROR;
					OFF_STATUS_LED;
				}
				ht_process_received_byte(rx_data, real_status);
			}
#else
			while ((rx_status = UCSR1A) & _BV(RXC1))
			{
				uint8_t real_status = 0;
				/* read rx-data */
				rx_data = UDR1;
				if (rx_status & _BV(FE1)) {
					real_status |= FRAMEEND;
					OFF_STATUS_LED;
					} else {
					ON_STATUS_LED;
				}
				if (rx_status & (_BV(DOR1) | _BV(UPE1)) ) {
					real_status |= ERROR;
					OFF_STATUS_LED;
				}
				ht_process_received_byte(rx_data, real_status);
			}
#endif
			if (uart_is_polled()) 
			{
				UPDATE_STATS(onebyte_own_packets, 1);
				protocoll_state = STATE_TX_ADDR;
				uart_switch_mode(MODE_TX);
#ifdef HT_DEBUG
				if (ht_send_buffer.sent != ht_send_buffer.len) {
					HT_IODEBUG("Sending %d bytes\n", ht_send_buffer.len - ht_send_buffer.sent);
				}
#endif
			}
		break;
	}
} //end ISR(USART_RX_vect)
