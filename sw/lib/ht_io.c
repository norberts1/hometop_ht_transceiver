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

#include "ht_io.h"

#include <util/setbaud.h>
#include <util/atomic.h>
#ifdef PITINY
	#include "pitiny_uart.h"
#endif


static volatile uint16_t io_systimer_msec=0;
static volatile uint8_t  io_countdown_timer_100msec=0;

void io_set_led(portnr_t portnr, uint8_t port_bit, uint8_t on)
{
	io_set_portpin(portnr, port_bit, on);
}

void io_set_portpin(portnr_t portnr, uint8_t port_bit, uint8_t on)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		switch (portnr){
#ifndef PITINY
			case PORTB_NR:
				DDRB  |= _BV(port_bit);
				(on) ? (PORTB |= _BV(port_bit)):(PORTB &= (uint8_t)~_BV(port_bit));
			break;
			case PORTC_NR:
				DDRC  |= _BV(port_bit);
				(on) ? (PORTC |= _BV(port_bit)):(PORTC &= (uint8_t)~_BV(port_bit));
			break;
			case PORTD_NR:
				DDRD  |= _BV(port_bit);
				(on) ? (PORTD |= _BV(port_bit)):(PORTD &= (uint8_t)~_BV(port_bit));
			break;
#else
			case PORTA_NR:
				DDRA  |= _BV(port_bit);
				(on) ? (PORTA |= _BV(port_bit)):(PORTA &= (uint8_t)~_BV(port_bit));
			break;
			case PORTB_NR:
				DDRB  |= _BV(port_bit);
				(on) ? (PORTB |= _BV(port_bit)):(PORTB &= (uint8_t)~_BV(port_bit));
			break;
#endif			
			default:
			break;
		}
	}
}

void io_toggle_portpin(portnr_t portnr, uint8_t port_bit)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		switch (portnr){
#ifndef PITINY
			case PORTB_NR:
				PINB = _BV(port_bit);
			break;
			case PORTC_NR:
				PINC = _BV(port_bit);
			break;
			case PORTD_NR:
				PIND = _BV(port_bit);
			break;
#else			
			case PORTA_NR:
				PINA = _BV(port_bit);
			break;
			case PORTB_NR:
				PINB = _BV(port_bit);
			break;
#endif			
			default:
			break;
		}
	}
}

uint8_t io_getstatus_portpin   (portnr_t portnr, uint8_t port_bit)
{
	uint8_t rtnvalue=0;
	volatile uint8_t portvalue=0;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		switch (portnr){
#ifndef PITINY
			case PORTB_NR:
				portvalue=PINB;
			break;
			case PORTC_NR:
				portvalue=PINC;
			break;
			case PORTD_NR:
				portvalue=PIND;
			break;
#else			
			case PORTA_NR:
				portvalue=PINA;
			break;
			case PORTB_NR:
				portvalue=PINB;
			break;
#endif			
			default:
			break;
		}
	}
	rtnvalue = ((portvalue & _BV(port_bit))>0) ? 1:0;
	return rtnvalue;
}

void io_port_init (void)
{
	//set Testpoints (TP-IN and TP-OUT)
#ifndef PITINY
	// TP-Out as output and low-state (low sink)
	DDRB  |= _BV(DDB1);
	PORTB &= (uint8_t)~(_BV(PORTB1));

	// TP-In as input and with pullup
	DDRB  &= (uint8_t)~(_BV(DDB0));
	PORTB |= _BV(PORTB0);
	MCUCR &= (uint8_t)~(_BV(PUD));
#else
	// TP-Out as output and low-state (low sink)
	DDRA |= _BV(DDRA7);
	PORTA &= (uint8_t)~(_BV(PORTA7));
	PUEA  &= (uint8_t)~(_BV(PORTA7));

	// TP-In as input and with pullup
	DDRB &= (uint8_t)~(_BV(DDRB2));
	PUEB |= _BV(PORTB2);
#endif	
}


static void io_usart_init(void)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		/* set direction and port for HW - uart - RX/TX	*/
#ifndef PITINY
		/*  RX Input  -> DDRnx 0; PORTxn 0
		 *  TX Output -> DDRnx 1; PORTxn 1
		*/
		DDRD  &= (uint8_t)~_BV( DDD0 );
		PORTD |= _BV( PORTD0 );
		/* set TX-pin for output and to low-active used for BREAK-signal */
		DDRD  |= _BV( PORTD1 );
		PORTD &= (uint8_t)~_BV( DDD1 );

#else		
		/** UART 0 ***/
		/*    used as client-port connected to ht-masterprocess */
		/*  RX Input  -> DDRA; PORTxn 2
		 *  TX Output -> DDRA; PORTxn 1
		*/
		DDRA  &= (uint8_t)~_BV(DDRA2);
		PORTA |= _BV(PORTA2);
		/* set TX-pin for output */
		DDRA  |= _BV(DDRA1);
		
		/** UART 1 ***/
		/*    used as bus-port connected to heater-bus */
		/*  RX Input  -> DDRA; PORTxn 4
		 *  TX Output -> DDRA; PORTxn 5
		*/
		DDRA  &= (uint8_t)~_BV(DDRA4);
		PORTA |= _BV(PORTA4);
		/* set TX-pin for output and to low-active used for BREAK-signal */
		DDRA  |= _BV(DDRA5);
		PORTA &= (uint8_t)~_BV(DDRA5);
#endif
	}
}

static void io_uart_enable(void)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		/* enable transmitter, receiver and interrupts */
		UCSR0B |= ( _BV(TXEN0) | _BV(RXEN0) | _BV(RXCIE0) );
#ifdef PITINY
		/* additional enable uart1 */
		/* enable uart 1 transmitter, receiver and interrupts */
		UCSR1B |= ( _BV(TXEN1) | _BV(RXEN1) | _BV(RXCIE1) );
#endif		
	}
}

void io_uart_init(void)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
#ifndef PITINY
		/* disable transmitter, receiver and uart-interrupts */
		UCSR0B &= (uint8_t)~( _BV(TXEN0) | _BV(RXEN0) | _BV(RXCIE0) );
		/* Reset power-reduction mode for uart */
		PRR  &= (uint8_t)~_BV(PRUSART0);
		/* clear TX complete-flag writing a one to it and 
		   rest all error-flags, double-speed and multi-processor mode
		    ~(FE0 | DOR0 | UPE0 | U2X0 | MPCM0)
		*/
		UCSR0A = _BV(TXC0);
		
		/* set uart baudrate */
		UBRR0H=(uint8_t)UBRRH_VALUE;
		UBRR0L=(uint8_t)UBRRL_VALUE;
		/* set uart mode to: 8N1 -> 8bits, no parity, asynchronous uart */
		/* -> UMSEL is set to 0 */
		UCSR0C=_BV(UCSZ00) | _BV(UCSZ01);
#else
		/** UART 0 ***/
		/* disable transmitter, receiver and uart-interrupts for uart0*/
		UCSR0B &= (uint8_t)~( _BV(TXEN0) | _BV(RXEN0) | _BV(RXCIE0) );
		/* Reset power-reduction mode for uart0 */
		PRR  &= (uint8_t)~_BV(PRUSART0);
		/* clear TX complete-flag writing a one to it and 
		   rest all error-flags, double-speed and multi-processor mode
		    ~(FE0 | DOR0 | UPE0 | U2X0 | MPCM0)
		*/
		UCSR0A = _BV(TXC0);
		tiny_uart_init(UART0, BAUD_UART0);
		
		/** UART 1 ***/
		/* disable transmitter, receiver and uart-interrupts for uart1 */
		UCSR1B &= (uint8_t)~( _BV(TXEN1) | _BV(RXEN1) | _BV(RXCIE1) );
		/* Reset power-reduction mode for uart1 */
		PRR  &= (uint8_t)~_BV(PRUSART1);
		/* clear TX complete-flag writing a one to it and 
		   rest all error-flags, double-speed and multi-processor mode
		    ~(FE0 | DOR0 | UPE0 | U2X0 | MPCM0)
		*/
		UCSR1A = _BV(TXC1);
		tiny_uart_init(UART1, BAUD_HT);
#endif		
	} //end ATOMIC_BLOCK
	io_usart_init();
	io_uart_enable();
}

void io_t2_init(void)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		/* first disable timer2-interrupts*/
		TIMSK2 &= (uint8_t)~(_BV(OCIE2A)|_BV(OCIE2B));

#ifndef PITINY
		/* set prescaler clk(T2S)/128 */
		TCCR2B &= (uint8_t)~(_BV(CS21));
		TCCR2B |= (_BV(CS22)|_BV(CS20));

		/* set mode:0 -> 'Normal mode' */
		TCCR2A &= (uint8_t)~( _BV(WGM20) | _BV(WGM21) );
		TCCR2B &= (uint8_t)~(_BV(WGM22));

		/* set Output compare register A to: BIT_TIME_HT * 10 bittimes */
		OCR2A = (uint8_t)(BIT_TIME_HT * 10);
		/* set Output compare register B to: create 1msec timestamps */
		OCR2B = (uint8_t)125;
#else
		/* set prescaler clk(T2S)/8 */
		TCCR2B &= (uint8_t)~(_BV(CS22)|_BV(CS20));
		TCCR2B |= (_BV(CS21));

		/* set mode:0 -> 'Normal mode' */
		TCCR2A &= (uint8_t)~( _BV(WGM20) | _BV(WGM21) );
		TCCR2B &= (uint8_t)~(_BV(WGM22));

		/* set Output compare register A to: BIT_TIME_HT * 10 bittimes */
		OCR2A = (uint16_t)(BIT_TIME_HT * 10);
		/* set Output compare register B to: create 1msec timestamps */
		OCR2B = (uint16_t)125;
#endif		
		/* reset Timer/Counter Register */
		TCNT2 = 0;
		
		/* clear any old interrupts */
		TIFR2 = (_BV(OCF2A)|_BV(OCF2B));
		/* enable only timer2-compareB interrupt*/
		TIMSK2 |= _BV(OCIE2B);
	}
}


void io_set_countdown_timer_100msec(uint8_t msec_slices)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		io_countdown_timer_100msec=msec_slices;
	}
}


uint8_t io_get_countdown_timer_100msec(void)
{
	volatile uint8_t timer_tmp=0;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		timer_tmp = io_countdown_timer_100msec;
	}
	return timer_tmp;
}

/**
 * Fkt.: io_wait_100usec wait n * 100usec
 *
 * rtn.: -1 if values are >= 10 slices else 0
 *  function should not be used in interrupt-handler
 *
 */
uint8_t io_wait_100usec(uint8_t usec_slices)
{
#ifndef PITINY
	volatile uint8_t Stopvalue;
#else
	volatile uint16_t Stopvalue;
#endif	
	uint8_t rtn_value = -1;
	
	if(usec_slices <= 10) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			Stopvalue=TCNT2+13*usec_slices;
		}
		while(TCNT2 != Stopvalue);
		rtn_value=0;
	}
	return rtn_value;
}


/**
 * Fkt.: io_wait_msec wait n * 1msec
 *
 * rtn.: -1 if values are > 10 slices else 0
 *
 */
uint8_t io_wait_msec(uint8_t msec_slices)
{
#ifndef PITINY
volatile uint8_t Stopvalue;
#else
volatile uint16_t Stopvalue;
#endif
	uint8_t rtn_value = -1;
	
	if(msec_slices < 3) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			Stopvalue=TCNT2+125*msec_slices;
			while(TCNT2 != Stopvalue);
		}
		rtn_value=0;
	} else  if (msec_slices <= 10) {
		while(msec_slices) 
		{
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				Stopvalue=TCNT2+125;
				while(TCNT2 != Stopvalue);
			}
			msec_slices--;
		}
		rtn_value=0;
	} else {
		rtn_value=-1;
	}
	return rtn_value;
}

/* Timer/Counter2 Compare Match B Interrupt Handler */
ISR(TIMER2_COMPB_vect)
{
	io_systimer_msec++;
	if(io_systimer_msec>65000) {
		io_systimer_msec=0;
	}
	if (io_countdown_timer_100msec > 0) {
		if (!(io_systimer_msec % (uint16_t)100))   {
			io_countdown_timer_100msec--;
		}
	}
	OCR2B=TCNT2+125;
} //end ISR(TIMER2_COMPB_vect)

