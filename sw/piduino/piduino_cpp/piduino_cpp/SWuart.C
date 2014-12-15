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
 *  1. timer0/OCR0A  for swuart tx-signalsampling
 *  2. timer1/OCR1A  for swuart rx-signalsampling
 *  3. INT0 ext Sig  for rx-startbit detection 
 */


#include "ht_io.h"
#include <util/atomic.h>


static volatile byte_exchange_t tx_byte;
static volatile byte_exchange_t rx_byte;
volatile uint8_t rx_byte_mask;
static volatile if_msg_buffer_t rxd_msg;


// private used functions
int8_t  __swuart_save_rxd_msgbyte(const uint8_t);
void    __swuart_flush_rxd_buffer(void);


/**
 * swuart initialisation
 *  timer0 -> Software UART (TX); Output OC0A
 *  timer1 -> Software UART (RX); Input  INT0
 */
void swuart_init( void )
{
	tx_byte.bitcounter=0;
	tx_byte.bytedata  =0;
	tx_byte.done      =1;
	rx_byte.bitcounter=10;
	rx_byte.bytedata  =0;
	rx_byte_mask	=0;
	
	// disable powersaving mode for Timer0 and Timer1
	PRR &= (uint8_t)~(PRTIM0 | PRTIM1);
	// set OC0A high on compare match Timer0 Mode:2 CTC -> TOP:OCRA, CLK/8
	TCCR0A = (1<<COM0A1)|(1<<COM0A0)|(1<<WGM01);	
	TCCR0B = (1<<CS01);
	// input capture noise canceler
	// falling edge transition,	CLK/8
	// Timer1 Mode 4 CTC OCR1A <- TOP
	TCCR1A = 0;
	TCCR1B = (1<<ICNC1) |(1<<CS11)|(1<<WGM12);
	// set timer1 OCR1A to first compare-value
	OCR1A = (uint16_t)(BIT_TIME_SW_UART * 1.5);
									
	// clear pending interrupts for timer0 and timer1
	TIFR0  = (1<<OCF0B)|(1<<OCF0A)|(1<<TOV0);
	TIFR1  = (1<<OCF1B)|(1<<OCF1A)|(1<<TOV1)|(1<<ICF1);	
	// Set data direction for TX output on PortD PD6
	DDRD |= (1<<DDD6);
	// set INT0 on portD for RX-Startbit detection and 1->0 triggering
	//  use internal pullups for this bit
	DDRD &= (uint8_t)~(1<<DDD2);
	PORTD |= (1<<PORTD2);
	EICRA |= (1<<ISC01);
	
	// Set Interrupt Mask Register for:
	//  enable  Timer0 Compare A interrupt (TX) and
	//  disable Timer1 Compare A interrupt (RX)
	TIMSK0 = (1<<OCIE0A);
	TIMSK1 &= (uint8_t)~(1<<OCIE1A);
		
	// force compare to set OC0A-high-level (TX)
	TCCR0B |= (1<<FOC0A);

	// Enable Interrupt for INT0
	EIMSK |= (1<<INT0);
}


/**
 * get byte
 */
uint8_t swuart_getchar( void )
{
//ZS	
/*	
	// wait until byte received
	while( !rx_byte.done );				
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		rx_byte.done = 0;
	}
	// copy byte to rx_tmp at first
	uint8_t rx_tmp=rx_byte.bytedata;
	return rx_tmp;
*/	
	// wait until char received
	while ( ! swuart_char_received() );
	uint8_t rxd_tmp=rxd_msg.data[rxd_msg.tail];
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		++rxd_msg.tail;
		rxd_msg.tail %= IF_BUFFER_LEN;
	}
	return rxd_tmp;
}

/**
 * swuart putchar function to send one char
 *
 */
void swuart_putchar( uint8_t val )			// send byte
{
	// wait until all data is send
	while( !tx_byte.done);
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		tx_byte.done=0;
	}

	// disable any Timer0 Compare A interrupt
	TIMSK0 &= (uint8_t)~(1<<OCIE0A);
	// Set data direction for TX output on PortD PD6
	DDRD |= (1<<DDD6);
	// first prepare timer-values
	OCR0A = (uint8_t)BIT_TIME_SW_UART;
	TCNT0 = 0;
	
	// invert data for stop bit generation
	tx_byte.bytedata = (uint8_t)~val;
	// 10 bits: Start + data + Stop
	tx_byte.bitcounter = 10;

	// set low (startbit) on next compare and Timer0 Mode2
	TCCR0A = (1<<COM0A1)|(1<<WGM01);
	// force compare to set port-lowlevel (startbit)
	TCCR0B |= (1<<FOC0A);
	--tx_byte.bitcounter;
	
	// reset timer-counterA value
	TCNT0 = 0;
	// clear any pending Timer0 CompareA interrupt
	TIFR0  = (1<<OCF0A);
	// enable Timer0 Compare A interrupt
	TIMSK0 |= (1<<OCIE0A);
}


/**
 * swuart write function to send one string
 *
 */
void swuart_puts( const char  * p_txt )			// send string
{
  while( * p_txt )
    swuart_putchar( * p_txt++ );
}

uint8_t swuart_char_received(void)
{
	return (rxd_msg.tail != rxd_msg.leading);
}

int8_t __swuart_save_rxd_msgbyte(const uint8_t rx_byte)
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

void __swuart_flush_rxd_buffer(void) 
{
	rxd_msg.tail=rxd_msg.leading;
}

/**
 * Interrupt Service for Timer0 Compare A (TX)
 *
 */
ISR(TIMER0_COMPA_vect)
{
	if( tx_byte.bitcounter )
	{
		if( tx_byte.bitcounter <= 1 )	{
			// set port to high-level for stop-bit and Timer0 Mode2
			TCCR0A = (1<<COM0A1)|(1<<COM0A0)|(1<<WGM01);
			// force compare to set port-level now
			TCCR0B |= (1<<FOC0A);
		} else {
			// check bit-value in TX-data
			if( !(tx_byte.bytedata & 1) ) {
				// set high on next compare and Timer0 Mode2
				TCCR0A = (1<<COM0A1)|(1<<COM0A0)|(1<<WGM01);
			} else {
				// set low on next compare and Timer0 Mode2
				TCCR0A = (1<<COM0A1)|(1<<WGM01);
			}
			// force compare to set port-level now
			TCCR0B |= (1<<FOC0A);
			// shift TX-Byte to right, zero in from left
			tx_byte.bytedata >>= 1;
		}
		--tx_byte.bitcounter;
		} else {
		// disable further Timer0 Compare A interrupts
		TIMSK0 &= (uint8_t)~(1<<OCIE0A);
		tx_byte.done=1;
	}
	// Interrupt-Flag 'OCF0A' is automatically cleared after
	//  leaving this handler	TIFR0 = (1<<OCF0A);
}

/**
 * Interrupt Service for Timer0 Compare B
 *
 */
ISR(TIMER0_COMPB_vect)
{
}


/**
 * Interrupt Service for Timer1 Compare A (RX)
 *
 */
ISR(TIMER1_COMPA_vect)
{
	if (rx_byte.bitcounter)
	{
		// set timer1 compareA register for next sampling
		OCR1A = (uint16_t)(BIT_TIME_SW_UART);
		rx_byte.bitcounter--;
		// check only port-level for 8 databits without start/stop levels
		//  bitvalues are at at rx_byte.bitcounter:=9 to 2
		//  bitcounter:=10 -> start-bit
		//  bitcounter==1  -> stop-bit
		if (rx_byte.bitcounter>1) {
			if( rx_byte_mask ) {
				if( PIND & (1<<PIND2)) {
					rx_byte.bytedata |= rx_byte_mask;
				}
				rx_byte_mask <<= 1;
			}
		} else {
			// disable this timer1A interrupt
			TIMSK1 &= (uint8_t)~(1<<OCIE1A);
			// save byte to ringbuffer
			if (__swuart_save_rxd_msgbyte(rx_byte.bytedata) == -1) {
				__swuart_flush_rxd_buffer();
				__swuart_save_rxd_msgbyte(rx_byte.bytedata);
			}
			rx_byte.bitcounter=0;
			// clear any external INT0 request
			EIFR = (1<<INTF0);
			// reenable INT0 Interrupt
			EIMSK |= (1<<INT0);
		}
	}
	// Interrupt-Flag 'OCF1A' is automatically cleared after
	//  leaving this handler
}

/**
 * Interrupt Service for Timer1 Compare B
 *
 */
ISR(TIMER1_COMPB_vect)
{
	/* unused */
}

/**
 * Interrupt Service for INT0 (RX)
 *   rx start, 1->0 transition seen on portD 'INT0'
 *
 */
ISR(INT0_vect)
{
	// if port is set to low, disable further interrupts
	//  and set timer1A compare register for port sampling
	if( !(PIND & (1<<PIND2))) {
		// disable further interrupts for INT0
		EIMSK &= (uint8_t)~(1<<INT0);
		// disable timer1A interrupt
		TIMSK1 &= (uint8_t)~(1<<OCIE1A);
		// preset all data
		rx_byte.bitcounter=10;
		rx_byte.bytedata=0;
		// Set rx bit mask
		rx_byte_mask = 1;
		
		// set timer1 compareA registers
		OCR1A = (uint16_t)(BIT_TIME_SW_UART * 1.5);
		TCNT1 = (uint16_t)0;

		// clear any pending interrupt timer1A
		TIFR1 = (1<<OCF1A);
		// enable timer1A interrupt
		TIMSK1 |= (1<<OCIE1A);
	}
	// Interrupt-Flag 'INTF0' is automatically cleared after
	//  leaving this handler
}

ISR(INT1_vect)
{
	/* unused */
}

/**
 * Interrupt Service for PCINT0
 *
 */
ISR(PCINT0_vect)
{
	/* unused */
}

ISR(TIMER0_OVF_vect)
{
	/* unused */
}
