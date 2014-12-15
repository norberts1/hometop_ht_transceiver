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

#include <avr/io.h>
#include "ht_process.h"
#include "ht_io.h"
#include "ht_cfg.h"
#include "ht_if.h"
#include "ht_error.h"
#include <stdio.h>
#include "avr/wdt.h"

/* Watchdog disable routine at startup */
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init1")));

void wdt_init(void)
{
	MCUSR = 0;
	wdt_disable();
	return;
}


int main(void)
{
	// disable global interrupts
	cli();
	
	cht_trx_if  piduino_if;
	uint16_t led_counter=0;
	uart_rx_buffer_t rx_buffer;
	uint8_t bytecountdown=IF_BUFFER_LEN-1;
		
	ht_process_init();
	piduino_if.init_if();
	swuart_init();
	// enable global interrupts
	sei();

	//dummy-read swuart rxd_buffer 
	if (swuart_char_received()) {
		swuart_getchar();
	}
	
	char tx_buffer[25];
	sprintf(tx_buffer,"Piduino 0.0.5\r");
	swuart_puts(tx_buffer);
	sprintf(tx_buffer,"junky-zs@gmx.de\r");
	swuart_puts(tx_buffer);
	
	while(1)
	{
		if (ht_process_rxdata(&rx_buffer)) {
			if (piduino_if.cht_trx_cfg::IsModeNative() || piduino_if.cht_trx_cfg::IsModeTx()) {
				for(uint8_t index=0; index < rx_buffer.count; index++ ) {
					swuart_putchar(rx_buffer.data[index]);
				}
			} else {
				if(piduino_if.cht_trx_cfg::IsModeRx_Header() || piduino_if.cht_trx_cfg::IsModeRx_Tx()) {
					piduino_if.send_ht_msg(&rx_buffer.data[0], rx_buffer.count);
				}
			}
			rx_buffer.count=0;
		}
		if (! piduino_if.cht_trx_cfg::IsModeNative()) {
			bytecountdown=IF_BUFFER_LEN-1;
			while (swuart_char_received() && bytecountdown) {
				bytecountdown=piduino_if.parse_and_dispatch_msg(swuart_getchar());
				if((int8_t)bytecountdown < 0) {
					//error occured, terminate loop
					bytecountdown=0;
					break;
				}
			} //end while()
		} //end if (! IsModeNative())

		uart_check_timeout();
			
		led_counter++;
		if (led_counter > 30000) {
			TOGGLE_STATUS_LED;
			led_counter=0;
		}
	}
}
