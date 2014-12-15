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
/*
 * used HW-resources:
 *  1. uart0 -> client-interface to master (raspi/USB)
 *  2. uart1 -> interface to ht-bus
 */


#include "pitiny_uart.h"
#include "ht_io.h"
#include <util/atomic.h>


static volatile if_msg_buffer_t rxd_msg;


// private used functions
int8_t  __tiny_uart_save_rxd_msgbyte(const uint8_t);
void    __tiny_uart_flush_rxd_buffer(void);


/**
 * uart initialisation
 */
void tiny_uart_init( const uint8_t uart_nr, const unsigned int baud )
{
	uint16_t ubrr_value = 0;
	if (uart_nr==0) {
		/* Set baud rate */
		ubrr_value = (uint16_t)(((F_CPU + baud/2) / baud) / (uint16_t)16);
		UBRR0H = (unsigned char)(ubrr_value>>8);
		UBRR0L = (unsigned char)ubrr_value;
		/* set uart mode to: 8N1 -> 8bits, no parity, asynchronous uart */
		/* -> UMSEL is set to 0 */
		UCSR0C=_BV(UCSZ00) | _BV(UCSZ01);
		/* Enable receiver and transmitter */
		UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	} 
	if (uart_nr==1) {
		/* Set baud rate */
		ubrr_value = (uint16_t)(((F_CPU) + 8UL * (baud)) / (16UL * (baud)) -1UL);
		UBRR1H = (unsigned char)(ubrr_value>>8);
		UBRR1L = (unsigned char)ubrr_value;
		/* set uart mode to: 8N1 -> 8bits, no parity, asynchronous uart */
		UCSR1C = _BV(UCSZ10) | _BV(UCSZ11);
		/* Enable receiver and transmitter */
		UCSR1B = (1<<RXEN1)|(1<<TXEN1);
	}
	rxd_msg.tail    = 0;
	rxd_msg.leading = 0;
}


/**
 * get byte
 */
uint8_t tiny_uart_getchar(  const uint8_t uart_nr )
{
	uint8_t rxd_tmp=0;
	if (uart_nr==0) {
		// wait until char received
		while ( ! tiny_uart_char_received() );
		rxd_tmp=rxd_msg.data[rxd_msg.tail];
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			++rxd_msg.tail;
			rxd_msg.tail %= IF_BUFFER_LEN;
		}
	}
	if (uart_nr==1) {
		// wait until char received
		while ( !(UCSR1A & (1<<RXC1)) ) ;
		rxd_tmp=UDR1;
	}
	return rxd_tmp;
}

/**
 * uart putchar function to send one char
 *
 */
void tiny_uart_putchar( const uint8_t val,  const uint8_t uart_nr  )			// send byte
{
	if (uart_nr==0) {
		/* Wait for empty transmit buffer */
		while ( !( UCSR0A & _BV(UDRE0)) )	;
		/* Put data into buffer, sends the data */
		UDR0 = val;
	}
	if (uart_nr==1) {
		/* Wait for empty transmit buffer */
		while ( !( UCSR1A & _BV(UDRE1)) )	;
		/* Put data into buffer, sends the data */
		UDR1 = val;
	}
}


/**
 * uart write function to send one string
 *
 */
void tiny_uart_puts( const char * p_txt, const uint8_t uart_nr  )			// send string
{
  while( * p_txt )
    tiny_uart_putchar( * p_txt++, uart_nr );
}

/**
 * returns true, if one ore more bytes received
 *   (this ringbuffer is only available for uart0)
 *
 */
uint8_t tiny_uart_char_received(void)
{
	return (rxd_msg.tail != rxd_msg.leading);
}

/**
 * returns 0, if rx-byte can be saved to ringbuffer, else returns -1
 *   (this ringbuffer is only available for uart0)
 *
 */
int8_t __tiny_uart_save_rxd_msgbyte(const uint8_t rx_byte)
{
	int8_t ertn=0;
	if((rxd_msg.tail - 1) == rxd_msg.leading) {
		// buffer full, no more data saveeable
		ertn = -1;
	} else {
		// save byte to buffer
		rxd_msg.data[rxd_msg.leading]=rx_byte;
		++rxd_msg.leading;
		rxd_msg.leading %= IF_BUFFER_LEN;
	}
	return ertn;
}

/**
 * flushes the rx-ringbuffer
 *   (this ringbuffer is only available for uart0)
 *
 */
void __tiny_uart_flush_rxd_buffer(void) 
{
	rxd_msg.tail=rxd_msg.leading;
}

/* USART0, Start */
ISR (USART0_START_vect)
{
	
}

/* USART0, Rx Complete */
ISR (USART0_RX_vect)
{
	uint8_t rx_status, rx_data;
	while ((rx_status = UCSR0A) & _BV(RXC0))
	{
		/* read rx-data */
		rx_data = UDR0;
		if (rx_status & (_BV(FE0) | _BV(DOR0) | _BV(UPE0)) ) {
			//  set byte to 0, if Frame-error, Data-overrun or parity-error
			rx_data = 0;
		}
				
		// save byte to ringbuffer
		if (__tiny_uart_save_rxd_msgbyte(rx_data) == -1) {
			__tiny_uart_flush_rxd_buffer();
			__tiny_uart_save_rxd_msgbyte(rx_data);
		}
	}
}

/* USART0 Data Register Empty */
ISR (USART0_UDRE_vect)
{
	
}

/* USART0, Tx Complete */
ISR (USART0_TX_vect)
{
	
}
